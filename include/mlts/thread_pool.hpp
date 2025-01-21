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

template<typename TFunc = std::function<void()>>
class thread_pool
{
    enum class thread_state : int
    {
        normal,
        idle,
    };

    using function = TFunc;

    struct thread
    {
        explicit thread(thread_state state, std::chrono::milliseconds idle_period, size_t idle_count_max)
            : m_state(std::make_unique<std::atomic<thread_state>>()),
              m_queue(std::make_unique<lock_free_queue<function>>()), m_idle_period(idle_period),
              m_idle_count_max(idle_count_max), m_idle_count(0), m_is_close(std::make_unique<std::atomic<bool>>()),
              m_ins(std::make_unique<std::thread>([this]() { work(); }))
        {
        }

        ~thread()
        {
            m_is_close->store(true);
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
            bool ret = m_queue->pop_front(f);
            if (ret) [[likely]]
            {
                f();
            }
            else
            {
                if (m_idle_count > m_idle_count_max)
                {
                    m_state->store(thread_state::idle, std::memory_order_relaxed);
                }
                ++m_idle_count;
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
                    ++m_idle_count;
                    if (m_idle_count > m_idle_count_max)
                    {
                        m_idle_count = 0;
                        m_state->store(thread_state::idle, std::memory_order_relaxed);
                    }
                    continue;
                    break;
                }
                case thread_state::idle: {
                    std::this_thread::sleep_for(m_idle_period);
                    if (run_one())
                    {
                        m_state->store(thread_state::normal, std::memory_order_relaxed);
                    }

                    continue;
                    break;
                }
                }
            }
        }

        std::unique_ptr<std::atomic<thread_state>> m_state;
        std::unique_ptr<lock_free_queue<function>> m_queue;
        std::chrono::milliseconds m_idle_period;
        size_t m_idle_count_max;
        size_t m_idle_count;
        std::unique_ptr<std::atomic<bool>> m_is_close;
        std::unique_ptr<std::thread> m_ins;
    };

public:
    thread_pool(size_t thread_size = 4, std::chrono::milliseconds idle_period = std::chrono::milliseconds(200),
                size_t idle_count_max = 10000)
        : m_index_policy(thread_size), m_idle_period(idle_period), m_idle_count_max(idle_count_max)
    {
        for (size_t i = 0; i < thread_size; ++i)
        {
            auto th = std::make_unique<thread>(thread_state::normal, idle_period, idle_count_max);
            m_threads.emplace_back(std::move(th));
        }
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool& other) = delete;
    thread_pool(thread_pool&&) noexcept = default;
    thread_pool& operator=(thread_pool&&) noexcept = default;

    template<typename Func>
    void push_func(Func&& f)
    {
        for (auto& thp : m_threads)
        {
            auto& th = *thp;
            if (th.m_state->load(std::memory_order_relaxed) == thread_state::idle)
            {
                th.m_queue->push_back(std::forward<Func>(f));
                return;
            }
        }
        auto& thp = m_threads.at(m_index_policy.get_index());
        auto& th = *thp;
        th.m_queue->push_back(std::forward<Func>(f));
    }

    void wait_done(std::chrono::milliseconds wait_period = std::chrono::milliseconds(100)) const
    {
        std::this_thread::sleep_for(m_idle_period * 2);

        while (1)
        {
            std::vector<char> res{};
            res.reserve(m_threads.size());
            for (auto& thp : m_threads)
            {
                auto& th = *thp;
                res.emplace_back(
                    static_cast<char>(th.m_state->load(std::memory_order_relaxed) != thread_state::normal));
            }
            if (std::all_of(res.begin(), res.end(), [](char v) { return v == 1; }))
            {
                return;
            }
            std::this_thread::sleep_for(wait_period);
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
            auto th = std::make_unique<thread>(thread_state::normal, m_idle_period, m_idle_count_max);
            m_threads.emplace_back(std::move(th));
        }
    }

private:
    std::vector<std::unique_ptr<thread>> m_threads;
    get_index_policy m_index_policy;
    std::chrono::milliseconds m_idle_period;
    size_t m_idle_count_max;
};


} // namespace mlts
