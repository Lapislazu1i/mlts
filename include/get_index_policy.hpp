#pragma once
#include <stddef.h>

struct get_index_policy
{
    get_index_policy(size_t max_size) noexcept : m_index(0), m_max_size(max_size)
    {
    }

    size_t get_index() noexcept
    {
        if (m_index < m_max_size)
        {
            auto index = m_index;
            ++m_index;
            return index;
        }
        else
        {
            m_index = 0;
        }
        return m_index;
    }

    size_t m_index;
    size_t m_max_size;
};