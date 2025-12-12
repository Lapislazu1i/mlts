#pragma once
#include <cstring>
#include <functional>
#include <exception>
#include <type_traits>


namespace mlts
{

struct no_exp
{
};
constexpr inline no_exp no_exp_t{};

template<typename Sig, size_t SSize = 24>
class function;

template<typename R, typename... Args, size_t SSize>
class function<R(Args...), SSize>
{
    constexpr static inline size_t storage_size = SSize;
    constexpr static inline size_t storage_align = alignof(std::max_align_t);
    struct vtable
    {
        void (*destructor)(void*);
        void (*copy)(void*, const void*);
        void (*move)(void*, void*);
        R (*invoke)(void*, Args...);
        size_t size;
        bool is_small;
        bool is_trivial;
    };
    alignas(storage_align) unsigned char m_storage[storage_size];
    const vtable* m_vtable{nullptr};

    template<typename F>
    constexpr static F* data(void* obj) noexcept
    {
        if constexpr (is_small<F>())
        {
            return reinterpret_cast<F*>(obj);
        }
        else
        {
            return *reinterpret_cast<F**>(obj);
        }
    }

    template<typename F>
    constexpr static void destroy(void* obj) noexcept
    {
        if constexpr (is_small<F>())
        {
            auto f = reinterpret_cast<F*>(obj);
            f->~F();
        }
        else
        {
            auto f = *reinterpret_cast<F**>(obj);
            delete f;
        }
    }


    template<typename F>
    constexpr static void copy_construct(void* dst, const void* src) noexcept(std::is_nothrow_copy_constructible_v<F>)
    {
        if constexpr (std::is_copy_constructible_v<F>)
        {
            auto ptr = data<F>(dst);
            auto src_ptr = data<F>(const_cast<void*>(src));
            new (ptr) F(*reinterpret_cast<const F*>(src_ptr));
        }
        else
        {
            assert(false && "function is move only type");
            throw std::runtime_error("function is move only type");
        }
    }

    template<typename F>
    constexpr static void move_construct(void* dst, void* src) noexcept
    {
        auto ptr = data<F>(dst);
        auto src_ptr = data<F>(src);
        new (ptr) F(std::move(*reinterpret_cast<F*>(src_ptr)));
    }


    template<typename F>
    constexpr static bool is_small() noexcept
    {
        return sizeof(F) <= storage_size && alignof(F) <= storage_align;
    }

    template<typename F>
    constexpr static bool is_trivial() noexcept
    {
        return std::is_trivially_destructible_v<F> && std::is_trivially_copyable_v<F>;
    }

    template<typename F>
    static const vtable& make_vtable() noexcept
    {
        static vtable table = {+[](void* obj) {
            destroy<F>(obj);
        }, +[](void* dst, const void* src) {
            copy_construct<F>(dst, src);
        }, +[](void* dst, void* src) {
            move_construct<F>(dst, src);
            destroy<F>(src);
        }, +[](void* obj, Args... args) noexcept(std::is_nothrow_invocable_v<F, Args...>) -> R {
            if constexpr (std::is_same_v<void, R>)
            {
                std::invoke(*data<F>(obj), std::forward<Args>(args)...);
            }
            else
            {
                return std::invoke(*data<F>(obj), std::forward<Args>(args)...);
            }
        }, sizeof(F), is_small<F>(), is_trivial<F>()};
        return table;
    }

    template<typename F>
    static const vtable& make_trivial_vtable() noexcept
    {
        static vtable table = {+[](void* obj) {
            if constexpr (!is_small<F>())
            {
                auto ptr = *reinterpret_cast<F**>(obj);
                std::free(ptr);
            }
        }, +[](void* dst, const void* src) {
            std::memcpy(data<F>(dst), data<F>(const_cast<void*>(src)), sizeof(F));
        }, +[](void* dst, void* src) {
            std::memcpy(data<F>(dst), data<F>(const_cast<void*>(src)), sizeof(F));
        }, +[](void* obj, Args... args) noexcept(std::is_nothrow_invocable_v<F, Args...>) -> R {
            if constexpr (std::is_same_v<void, R>)
            {
                std::invoke(*data<F>(obj), std::forward<Args>(args)...);
            }
            else
            {
                return std::invoke(*data<F>(obj), std::forward<Args>(args)...);
            }
        }, sizeof(F), is_small<F>(), is_trivial<F>()};
        return table;
    }

    template<typename F>
    constexpr void assign(F&& f) noexcept(std::is_nothrow_constructible_v<std::decay_t<F>, F&&>)
    {
        using Functor = std::decay_t<F>;

        if constexpr (is_small<Functor>())
        {
            new (m_storage) Functor(std::forward<F>(f));
            if constexpr (is_trivial<Functor>())
            {
                m_vtable = &make_trivial_vtable<Functor>();
            }
            else
            {
                m_vtable = &make_vtable<Functor>();
            }
        }
        else
        {
            auto ptr = new Functor(std::forward<F>(f));
            new (m_storage) Functor*(ptr);
            if constexpr (is_trivial<Functor>())
            {
                m_vtable = &make_trivial_vtable<Functor>();
            }
            else
            {
                m_vtable = &make_vtable<Functor>();
            }
        }
    }

    void move(function&& other) noexcept
    {
        if (other.m_vtable)
        {
            if (m_vtable)
            {
                m_vtable->destructor(m_storage);
            }

            m_vtable = other.m_vtable;

            if (other.m_vtable->is_small)
            {
                other.m_vtable->move(m_storage, other.m_storage);
            }
            else
            {
                std::memcpy(m_storage, other.m_storage, sizeof(void*));
                std::memset(other.m_storage, 0, sizeof(void*));
            }
            other.m_vtable = nullptr;
        }
    }

public:
    constexpr function() noexcept : m_vtable(nullptr)
    {
    }

    template<typename F>
    constexpr function(F&& f) 
        noexcept(
            (!std::is_same_v<const function&, const std::decay_t<F>&> 
            && std::is_nothrow_constructible_v<std::decay_t<F>, F&&>)
            || function::is_trivial<std::decay_t<F>>()
        )
    {
        using Functor = std::decay_t<F>;
        if constexpr (std::is_same_v<const function&, const Functor&>)
        {
            copy(std::forward<F>(f));
        }
        else
        {
            assign(std::forward<F>(f));
        }
    }

    constexpr function(const function& other) : m_vtable(other.m_vtable)
    {
        copy(other);
    }

    constexpr function(function&& other) noexcept
    {
        move(std::move(other));
    }

    constexpr function& operator=(const function& other)
    {
        if (this != &other)
        {
            this->~function();
            new (this) function(other);
        }
        return *this;
    }

    constexpr function& operator=(function&& other) noexcept
    {
        if (this != &other)
        {
            this->~function();
            new (this) function(std::move(other));
        }
        return *this;
    }

    constexpr function& operator=(std::nullptr_t) noexcept
    {
        this->~function();
        m_vtable = nullptr;
        return *this;
    }

    template<typename F>
    constexpr function& operator=(F&& f) noexcept
    {
        this->~function();
        assign(std::forward<F>(f));
        return *this;
    }

    constexpr ~function() noexcept
    {
        if (m_vtable)
        {
            m_vtable->destructor(m_storage);
        }
    }

    struct result
    {
        R value;
        std::exception_ptr e{};
        constexpr operator bool() const noexcept
        {
            if (e)
            {
                return true;
            }
            return false;
        }
    };

    constexpr result operator()(no_exp, Args... args) noexcept
    {
        result res{};
        try
        {
            if (!m_vtable)
            {
                throw std::bad_function_call();
            }
            res.value = m_vtable->invoke(m_storage, std::forward<Args>(args)...);
        }
        catch (...)
        {
            res.e = std::current_exception();
        }
        return res;
    }

    constexpr R operator()(Args... args)
    {
        if (!m_vtable)
        {
            throw std::bad_function_call();
        }
        return m_vtable->invoke(m_storage, std::forward<Args>(args)...);
    }

    constexpr explicit operator bool() const noexcept
    {
        return m_vtable != nullptr;
    }

    constexpr void copy(const function& other)
    {
        if (m_vtable)
        {
            this->~function();
            m_vtable = nullptr;
        }

        if (other.m_vtable)
        {

            if (!other.m_vtable->is_small)
            {
                auto tmp = std::malloc(other.m_vtable->size);
                new (m_storage) void*(tmp);
            }
            other.m_vtable->copy(m_storage, other.m_storage);
            m_vtable = other.m_vtable;
        }
    }

    constexpr void swap(function& other) noexcept
    {
        function tmp = std::move(other);
        other = std::move(*this);
        (*this) = std::move(tmp);
    }
};


} // namespace mlts