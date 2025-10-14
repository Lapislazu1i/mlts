#pragma once
#include <mutex>
#include <vector>
#include <optional>
#include <span>
#include <cassert>

namespace mlts
{
template<typename T>
struct ring_buffer
{
public:

    ring_buffer(size_t capacity)
        : m_buf(reinterpret_cast<T*>(std::malloc(capacity* esize)))
        , m_mask(capacity - 1)
    {
        assert(capacity % 2 == 0);
    }

    ~ring_buffer()
    {
        auto in = m_in & m_mask;
        auto out = m_out & m_mask;
        for (; out != in; out = (out + 1) & m_mask)
        {
            std::destroy_at(&m_buf[out]);
        }
        std::free(reinterpret_cast<void*>(m_buf));
    }

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
        std::lock_guard lk(m_mu);
        len = std::min(len, capacity() - m_in + m_out);

        size_t l = std::min(len, capacity() - (m_in & m_mask));
        for (size_t i = 0; i < l; ++i)
        {
            m_buf[i + (m_in & m_mask)] = std::move(values[i]);
        }

        size_t rl = len - l;
        for (size_t i = 0; i < rl; ++i)
        {
            m_buf[i] = std::move(values[i + l]);
        }

        m_in += len;

        return len;
    }

    size_t get(T* values, size_t len)
    {
        std::lock_guard lk(m_mu);
        len = std::min(len, m_in - m_out);
        size_t l = std::min(len, capacity() - (m_out & m_mask));
        for (size_t i = 0; i < l; ++i)
        {
            values[i] = std::move(m_buf[i + (m_out & m_mask)]);
        }

        size_t rl = len - l;
        for (size_t i = 0; i < rl; ++i)
        {
            values[l + i] = std::move(m_buf[i]);
        }
        m_out += len;
        return len;
    }

    size_t exist_len() const
    {
        std::lock_guard lk(m_mu);
        return m_in - m_out;
    }

    size_t empty_len() const
    {
        std::lock_guard lk(m_mu);
        return m_mask + 1 - m_in + m_out;
    }

    size_t capacity() const
    {
        return m_mask + 1;
    }

    bool empty() const
    {
        return !empty_len();
    }

private:
    size_t m_in{};
    size_t m_out{};
    const size_t m_mask;
    T* m_buf{};
    mutable std::mutex m_mu{};
    static constexpr inline size_t esize = sizeof(T);
};

}