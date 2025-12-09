#pragma once
#include <atomic>
#include <vector>
#include <optional>
#include <span>
#include <cassert>

namespace mlts
{

// only for SPSC
template<typename T>
struct atomic_ring_buffer
{
public:

    atomic_ring_buffer(size_t capacity)
        : m_buf(reinterpret_cast<T*>(std::malloc(capacity * esize)))
        , m_mask(capacity - 1)
    {
        assert(capacity % 2 == 0);
    }

    ~atomic_ring_buffer()
    {
        auto in = m_in & m_mask;
        auto out = m_out & m_mask;
        for (; out != in; out = (out + 1) & m_mask)
        {
            std::destroy_at(&m_buf[out]);
        }
        std::free(reinterpret_cast<void*>(m_buf));
    }

    atomic_ring_buffer(const atomic_ring_buffer&) = delete;
    atomic_ring_buffer& operator=(const atomic_ring_buffer&) = delete;
    atomic_ring_buffer(atomic_ring_buffer&&) noexcept = delete;
    atomic_ring_buffer* operator=(atomic_ring_buffer&&) noexcept = delete;

    size_t put(std::span<T> values)
    {
        return put(values.data(), values.size());
    }

    size_t put(std::initializer_list<T> values)
    {
        std::vector<T> vec{ values };
        return put(vec);
    }

    bool put_one(T value)
    {
        return put(&value, 1);
    }

    std::optional<T> get_one()
    {
        T v;
        auto ret = get(&v, 1);
        if (ret == 0)
        {
            return {};
        }
        else
        {
            return v;
        }
    }

    size_t put(T* values, size_t len)
    {
        auto in = m_in.load(std::memory_order_acquire);
        auto out = m_out.load(std::memory_order_acquire);
        len = std::min(len, capacity() - in + out);
        m_in.store(len + in, std::memory_order_release);

        size_t l = std::min(len, capacity() - (in & m_mask));
        for (size_t i = 0; i < l; ++i)
        {
            std::construct_at<T>(&m_buf[i + (in & m_mask)], std::move(values[i]));
        }

        size_t rl = len - l;
        for (size_t i = 0; i < rl; ++i)
        {
            m_buf[i] = std::move(values[i + l]);
        }


        return len;
    }

    size_t get(T* values, size_t len)
    {
        auto in = m_in.load(std::memory_order_acquire);
        auto out = m_out.load(std::memory_order_acquire);

        len = std::min(len, in - out);
        m_out.store(len + out, std::memory_order_release);

        size_t l = std::min(len, capacity() - (out & m_mask));
        for (size_t i = 0; i < l; ++i)
        {
            values[i] = std::move(m_buf[i + (out & m_mask)]);
        }

        size_t rl = len - l;
        for (size_t i = 0; i < rl; ++i)
        {
            values[l + i] = std::move(m_buf[i]);
        }
        return len;
    }

    size_t exist_len() const
    {
        return m_in.load(std::memory_order_relaxed) - m_out.load(std::memory_order_relaxed);
    }

    size_t empty_len() const
    {
        return m_mask + 1 - m_in.load(std::memory_order_relaxed) + m_out.load(std::memory_order_relaxed);
    }

    size_t capacity() const noexcept
    {
        return m_mask + 1;
    }

    bool empty() const
    {
        return !empty_len();
    }

private:
    std::atomic<size_t> m_in{};
    std::atomic<size_t> m_out{};
    const size_t m_mask;
    T* m_buf{};
    static constexpr inline size_t esize = sizeof(T);
};

}