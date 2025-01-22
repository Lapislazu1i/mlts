#pragma once
#include "define_type.hpp"
#include "detail/mp_sc_circular_fifo_allocator_buffer.hpp"
#include <assert.h>
#include <atomic>
#include <stdexcept>

namespace mlts
{

template<typename T, size_t Size = DynamicSize>
struct mp_sc_circular_fifo_allocator
{
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;

    mp_sc_circular_fifo_allocator(size_type capacity = 0) : m_buffer(), m_head(0), m_tail(1)
    {

        if constexpr (Size == DynamicSize)
        {

            if (capacity > 1)
            {
                std::logic_error("mp_sc_circular_fifo_allocator capacity need greater then one");
            }
            m_buffer.resize((capacity + 1));
        }
        else
        {
        }
    }

    ~mp_sc_circular_fifo_allocator() = default;
    mp_sc_circular_fifo_allocator(const mp_sc_circular_fifo_allocator&) = delete;
    mp_sc_circular_fifo_allocator& operator=(const mp_sc_circular_fifo_allocator&) = delete;

    mp_sc_circular_fifo_allocator(mp_sc_circular_fifo_allocator&& other) noexcept
    {
        *this = std::move(other);
    }

    mp_sc_circular_fifo_allocator& operator=(mp_sc_circular_fifo_allocator&& other) noexcept
    {
        if (std::addressof(other) == this) [[unlikely]]
        {
            return *this;
        }
        m_buffer = std::move(other.m_buffer);
        m_head.store(other.m_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
        m_tail.store(other.m_tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }

    T* allocate(size_type n)
    {
        if (n != 1) [[unlikely]]
        {
            std::logic_error("allocation only one on a time");
        }

        difference_type tail = m_tail.load(std::memory_order_relaxed);

        do
        {
            difference_type head = m_head.load(std::memory_order_relaxed);
            difference_type diff = tail - head;

            if (diff == 0) [[unlikely]]
            {
                throw std::runtime_error("mp_sc_circular_queue_allocator capacity is not enough");
            }

        } while (not m_tail.compare_exchange_strong(tail, (tail + 1) % capacity(), std::memory_order_release,
                                                    std::memory_order_relaxed));

        return reinterpret_cast<T*>(m_buffer.data() + tail);
    }

    void deallocate(const T* const p, size_type n)
    {

        if (n != 1) [[unlikely]]
        {
            std::logic_error("deallocation only one on a time");
        }

        difference_type head = m_head.load(std::memory_order_relaxed);
        while (not m_head.compare_exchange_strong(head, (head + 1) % capacity(), std::memory_order_release,
                                                  std::memory_order_relaxed))
        {
        }
    }

    constexpr const auto& capacity() const
    {
        return m_buffer.size();
    }

private:
    detail::mp_sc_circular_fifo_allocator_buffer<T, Size> m_buffer{};
    std::atomic<difference_type> m_head{0};
    std::atomic<difference_type> m_tail{1};
};


} // namespace mlts