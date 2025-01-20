#pragma once
#include "optional"
#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>


namespace mlts
{

template<typename T>
struct lock_free_queue_node
{
    T m_value;
    std::atomic<lock_free_queue_node*> m_next{};
};

// apply for signal consumer, multiple producers
template<typename T, typename Alloc = std::allocator<lock_free_queue_node<std::remove_cvref_t<T>>>>
class lock_free_queue
{

public:
    using node = lock_free_queue_node<T>;
    using value_type = std::remove_cvref_t<T>;

    lock_free_queue()
    {
        static_assert(std::atomic<node*>::is_always_lock_free, "not support lock free");
        static_assert(std::atomic<size_t>::is_always_lock_free, "not support lock free");
        node* n = m_alloc.allocate(1);
        std::construct_at<node>(n);
        m_head.store(n, std::memory_order_relaxed);
        m_tail.store(n, std::memory_order_relaxed);
    }

    ~lock_free_queue()
    {
        while (node* n = m_head.load(std::memory_order_relaxed))
        {
            m_head.store(n->m_next, std::memory_order_relaxed);
            std::destroy_at(n);
            m_alloc.deallocate(n, 1);
        }
    }

    template<typename TValue>
    void push_back(TValue val)
    {
        node* n = m_alloc.allocate(1);
        std::construct_at<node>(n);
        n->m_value = std::forward<TValue>(val);
        node* o = m_tail.load(std::memory_order_acquire);

        while (not m_tail.compare_exchange_weak(o, n, std::memory_order_release, std::memory_order_acquire))
        {
        }
        o->m_next.store(n, std::memory_order_release);
    }

    bool pop_front(value_type& val)
    {
        node* const o = m_head.load(std::memory_order_relaxed);
        node* const n = o->m_next.load(std::memory_order_acquire);
        if (n == nullptr) [[unlikely]]
        {
            return false;
        }
        val = std::move(n->m_value);
        m_head.store(n, std::memory_order_relaxed);
        std::destroy_at(o);
        m_alloc.deallocate(o, 1);
        return true;
    }

private:
    std::atomic<node*> m_head{nullptr};
    std::atomic<node*> m_tail{nullptr};
    Alloc m_alloc{};
};

} // namespace mlts