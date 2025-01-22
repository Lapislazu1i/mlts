#pragma once
#include <atomic>
#include <memory>
#include <stddef.h>


struct get_index_policy
{
    get_index_policy(size_t max_size) noexcept
        : m_index(std::make_unique<decltype(m_index)::element_type>()), m_max_size(max_size)
    {
    }
    
    ~get_index_policy() = default;
    get_index_policy(const get_index_policy&) = delete;
    get_index_policy& operator=(const get_index_policy& other) = delete;
    get_index_policy(get_index_policy&&) noexcept = default;
    get_index_policy& operator=(get_index_policy&&) noexcept = default;

    size_t get_index() noexcept
    {
        auto index = m_index->load(std::memory_order_relaxed);
        if (index < m_max_size)
        {
            m_index->fetch_add(1, std::memory_order_relaxed);
            return index;
        }
        else
        {
            m_index->store(0, std::memory_order_relaxed);
            return 0;
        }
    }

    std::unique_ptr<std::atomic<size_t>> m_index;
    size_t m_max_size;
};