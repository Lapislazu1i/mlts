#pragma once
#include "define_type.hpp"
#include "detail/config.hpp"
#include "detail/queue_circular_buffer.hpp"
#include <assert.h>
#include <atomic>
#include <bit>
#include <mutex>
#include <stdexcept>


namespace mlts
{

template<typename T>
struct lock_free_circular_queue_node
{
    T m_value{};
    bool m_has_value{false};
};

template<typename T, std::uint32_t Capacity = 1024>
struct lock_free_circular_queue
{
public:
    using node = lock_free_circular_queue_node<T>;
    using value_type = T;
    using difference_type = std::ptrdiff_t;

    struct control
    {
        uint32_t m_head;
        uint32_t m_tail;
    };

    lock_free_circular_queue() : m_buffer()
    {
        static_assert(std::atomic<node*>::is_always_lock_free, "not support lock free");
        static_assert(std::atomic<size_t>::is_always_lock_free, "not support lock free");
        static_assert(std::atomic<control>::is_always_lock_free, "not support lock free");
        // m_ctl.store(control{0, 1}, std::memory_order_relaxed);
    }

    ~lock_free_circular_queue() = default;

    lock_free_circular_queue(const lock_free_circular_queue&) = delete;
    lock_free_circular_queue& operator=(const lock_free_circular_queue&) = delete;
    lock_free_circular_queue(lock_free_circular_queue&& other) noexcept = delete;
    lock_free_circular_queue& operator=(lock_free_circular_queue&& other) noexcept = delete;


    template<typename TValue>
    void push(TValue val)
    {
        std::lock_guard lk(m_mu);
        std::uint32_t tail, head, new_tail;
        while(m_size.load(std::memory_order_acquire) == Capacity)
        {
        }

        m_buffer[tail] = std::forward<TValue>(val);
    }

    bool pop(value_type& val)
    {

        auto tail = m_tail.load();
        auto head = m_head.load();
        if(tail == head)
        {
            return false;
        }
        auto new_head = (head + 1) & m_buffer.mask();
        val = std::move(m_buffer[new_head]);
        m_head= new_head;
        return true;
    }

    constexpr auto capacity() const
    {
        return m_buffer.size();
    }

    // private:
    std::mutex m_mu;
    detail::queue_circular_buffer<T, Capacity> m_buffer{};
    // std::atomic<control> m_ctl{};
    std::atomic<std::uint32_t> m_head{};
    std::atomic<std::uint32_t> m_tail{};
    std::atomic<std::uint32_t> m_size{};
};


} // namespace mlts