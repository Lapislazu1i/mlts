#pragma once
#include "define_type.hpp"
#include "get_index_policy.hpp"
#include "lock_free_queue.hpp"
#include <algorithm>
#include <atomic>
#include <memory>
#include <thread>


namespace mlts
{

template<typename TFunc = std::function<void()>, typename TQueue = lock_free_queue<TFunc>>
class thread_pool
{
    enum class thread_state : int
    {
        normal,
        idle,
        yield,
        wait,
    };

    using function = TFunc;

    struct thread
    {
        explicit thread(thread_state state, size_t idle_count_max)
            : m_state(std::make_unique<std::atomic<thread_state>>()),
              m_queue(std::make_unique<lock_free_queue<function>>()), m_idle_count_max(idle_count_max), m_idle_count(0),
              m_yield_count(0), m_wait_count(0), m_is_close(std::make_unique<std::atomic<bool>>(false)),
              m_is_wait(std::make_unique<std::atomic<bool>>(false)),
              m_ins(std::make_unique<std::thread>([this]() { work(); }))
        {
        }

        ~thread()
        {
            m_is_close->store(true);
            m_is_wait->store(false, std::memory_order_release);
            m_is_wait->notify_all();
            if (m_ins)
            {
                if (m_ins->joinable())
                {
                    m_ins->join();
                }
            }
        }

        thread(const thread&) = delete;
        thread& operator=(const thread& other) = delete;
        thread(thread&&) noexcept = default;
        thread& operator=(thread&&) noexcept = default;

        bool run_one()
        {
            function f;
            if (not m_queue) [[unlikely]]
            {
                return false;
            }
            bool ret = m_queue->pop(f);
            if (ret) [[likely]]
            {
                f();
            }
            return ret;
        }

        void work()
        {
            while (1)
            {
                if (m_is_close->load(std::memory_order_relaxed)) [[unlikely]]
                {
                    return;
                }

                switch (m_state->load(std::memory_order_relaxed))
                {
                case thread_state::normal: {

                    if (run_one()) [[likely]]
                    {
                        continue;
                    }
                    if (m_idle_count >= m_idle_count_max)
                    {
                        m_idle_count = 0;
                        m_state->store(thread_state::idle, std::memory_order_relaxed);
                    }
                    ++m_idle_count;
                    continue;
                    break;
                }
                case thread_state::idle: {
                    if (m_yield_count >= m_idle_count_max)
                    {
                        m_yield_count = 0;
                        m_state->store(thread_state::yield, std::memory_order_relaxed);
                    }
                    if (run_one())
                    {
                        m_yield_count = 0;
                        m_state->store(thread_state::normal, std::memory_order_relaxed);
                    }
                    else
                    {
                        ++m_yield_count;
                    }
                    continue;
                    break;
                }

                case thread_state::yield: {
                    if (m_wait_count >= m_idle_count_max)
                    {
                        m_wait_count = 0;
                        m_is_wait->store(true, std::memory_order_release);
                        m_is_wait->notify_all();
                        m_state->store(thread_state::wait, std::memory_order_relaxed);
                    }
                    if (run_one())
                    {
                        m_wait_count = 0;
                        m_state->store(thread_state::normal, std::memory_order_relaxed);
                    }
                    else
                    {
                        ++m_wait_count;
                        std::this_thread::yield();
                    }
                    continue;
                    break;
                }

                case thread_state::wait: {
                    m_is_wait->wait(true, std::memory_order_acquire);
                    if (run_one())
                    {
                        m_is_wait->store(false, std::memory_order_release);
                        m_state->store(thread_state::normal, std::memory_order_relaxed);
                    }
                    continue;
                    break;
                }
                }
            }
        }

        template<typename AddFunc>
        void add_task(AddFunc&& f)
        {
            m_queue->push(std::forward<AddFunc>(f));
            m_is_wait->store(false, std::memory_order_release);
            m_is_wait->notify_all();
        }

        std::unique_ptr<std::atomic<thread_state>> m_state;
        std::unique_ptr<TQueue> m_queue;
        size_t m_idle_count_max;
        size_t m_idle_count;
        size_t m_yield_count;
        size_t m_wait_count;
        std::unique_ptr<std::atomic<bool>> m_is_close;
        std::unique_ptr<std::atomic<bool>> m_is_wait;
        std::unique_ptr<std::thread> m_ins;
    };

public:
    thread_pool(size_t thread_size = 4, size_t idle_count_max = 1000)
        : m_index_policy(thread_size), m_idle_count_max(idle_count_max)
    {
        for (size_t i = 0; i < thread_size; ++i)
        {
            auto th = std::make_unique<thread>(thread_state::normal, idle_count_max);
            m_threads.emplace_back(std::move(th));
        }
    }

    ~thread_pool() = default;
    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool& other) = delete;
    thread_pool(thread_pool&&) noexcept = default;
    thread_pool& operator=(thread_pool&&) noexcept = default;

    template<typename Func>
    void push_func(Func&& f)
    {
        // for (auto& thp : m_threads)
        // {
        //     auto& th = *thp;
        //     if (th.m_state->load(std::memory_order_relaxed) == thread_state::idle)
        //     {
        //         th.add_task(std::forward<Func>(f));
        //         return;
        //     }
        // }
        auto& thp = m_threads.at(m_index_policy.get_index());
        auto& th = *thp;
        th.add_task(std::forward<Func>(f));
    }

    template<typename Func>
    void push_func(size_t index, Func&& f)
    {
        auto& thp = m_threads.at(index);
        auto& th = *thp;
        th.add_task(std::forward<Func>(f));
    }

    void wait_done() const
    {
        bool is_wait;
        for (auto& thp : m_threads)
        {
            auto& th = *thp;
            is_wait = th.m_is_wait->load(std::memory_order_acquire);
            if(not is_wait)
            {
                th.m_is_wait->wait(false, std::memory_order_acquire);
            }
        }
    }

    void reset(size_t count)
    {
        if (count == 0)
        {
            count = 1;
        }
        get_index_policy new_policy(count);
        m_index_policy = std::move(new_policy);
        m_threads.clear();
        for (size_t i = 0; i < count; ++i)
        {
            auto th = std::make_unique<thread>(thread_state::normal, m_idle_count_max);
            m_threads.emplace_back(std::move(th));
        }
    }

private:
    std::vector<std::unique_ptr<thread>> m_threads;
    get_index_policy m_index_policy;
    size_t m_idle_count_max;
};


} // namespace mlts
