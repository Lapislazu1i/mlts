#pragma once
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <tuple>


namespace mlts
{

template<typename... Ts>
struct max_tuple_element_size_impl
{
};

template<typename T>
struct max_tuple_element_size_impl<T>
{
    static constexpr size_t value = sizeof(T);
};

template<typename T, typename... Ts>
struct max_tuple_element_size_impl<T, Ts...>
{
    static constexpr size_t value = std::max(sizeof(T), max_tuple_element_size_impl<Ts...>::value);
};

template<typename Tuple>
struct max_tuple_element_size
{
};
template<typename... Ts>
struct max_tuple_element_size<std::tuple<Ts...>> : public max_tuple_element_size_impl<Ts...>
{
};


template<typename TRuntime, typename TCompile>
struct rc_type_container
{
    constexpr static size_t capacity = max_tuple_element_size<std::tuple<TRuntime, TCompile>>::value;

    enum class type : unsigned char
    {
        no,
        runtime,
        compile,
    };

    constexpr rc_type_container() : m_index(type::no)
    {
    }

    constexpr rc_type_container(TRuntime&& val) : m_index(type::runtime)
    {
        TRuntime* ptr = reinterpret_cast<TRuntime*>(m_buffer);
        std::construct_at<TRuntime>(ptr);
        (*ptr) = std::move(val);
    }

    rc_type_container(const TRuntime& val) : m_index(type::runtime)
    {
        TRuntime* ptr = reinterpret_cast<TRuntime*>(m_buffer);
        std::construct_at<TRuntime>(ptr);
        (*ptr) = val;
    }

    rc_type_container(TCompile&& val) : m_index(type::compile)
    {
        TCompile* ptr = reinterpret_cast<TCompile*>(m_buffer);
        std::construct_at<TCompile>(ptr);
        (*ptr) = std::move(val);
    }

    rc_type_container(const TCompile& val) : m_index(type::compile)
    {
        TCompile* ptr = reinterpret_cast<TCompile*>(m_buffer);
        std::construct_at<TCompile>(ptr);
        (*ptr) = val;
    }

    ~rc_type_container() noexcept
    {
        destroy();
    }


    rc_type_container(const rc_type_container&) = delete;
    rc_type_container& operator=(const rc_type_container&) = delete;

    rc_type_container(rc_type_container&& val) noexcept
    {
        (*this) = std::move(val);
    }

    rc_type_container& operator=(rc_type_container&& val) noexcept
    {
        if (this == &val)
        {
            return *this;
        }
        destroy();
        m_index = val.m_index;
        construct();
        switch (val.m_index)
        {
        case type::no: {
            break;
        }
        case type::runtime: {
            auto* ptr = reinterpret_cast<TRuntime*>(m_buffer);
            auto* val_ptr = reinterpret_cast<TRuntime*>(val.m_buffer);
            (*ptr) = std::move(*val_ptr);
            break;
        }
        case type::compile: {
            auto* ptr = reinterpret_cast<TCompile*>(m_buffer);
            auto* val_ptr = reinterpret_cast<TCompile*>(val.m_buffer);
            (*ptr) = std::move(*val_ptr);
            break;
        }
        }
        return *this;
    }

    template<typename Visitor>
    constexpr decltype(auto) visit(Visitor&& visitor)
    {
        switch (m_index)
        {
        case type::no: {
            throw std::logic_error("not has value");
            break;
        }
        case type::runtime: {
            auto* ptr = reinterpret_cast<TRuntime*>(m_buffer);
            return visitor(*ptr);
            break;
        }
        case type::compile: {
            auto* ptr = reinterpret_cast<TCompile*>(m_buffer);
            return visitor(*ptr);
            break;
        }
        }
        throw std::logic_error("not call visitor");
    }

private:
    void construct() noexcept
    {
        switch (m_index)
        {
        case type::no: {
            break;
        }
        case type::runtime: {
            std::construct_at<TRuntime>(reinterpret_cast<TRuntime*>(m_buffer));
            break;
        }
        case type::compile: {
            std::construct_at<TCompile>(reinterpret_cast<TCompile*>(m_buffer));
            break;
        }
        }
    }
    void destroy() noexcept
    {
        switch (m_index)
        {
        case type::no: {
            break;
        }
        case type::runtime: {
            std::destroy_at<TRuntime>(reinterpret_cast<TRuntime*>(m_buffer));
            break;
        }
        case type::compile: {
            std::destroy_at<TCompile>(reinterpret_cast<TCompile*>(m_buffer));
            break;
        }
        }
    }
    char m_buffer[capacity];
    type m_index{type::no};
};


} // namespace mlts
