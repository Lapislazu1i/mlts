#pragma once
#include "function_trait.hpp"
#include <tuple>
#include <unordered_map>
#include <variant>


namespace mlts
{

template<typename T, typename Tuple>
struct tuple_index;

template<typename T, typename... Ts>
struct tuple_index<T, std::tuple<Ts...>>
{
    constexpr static std::size_t count = (std::is_same_v<T, Ts> + ...);

    static_assert(count > 0, "type not found in tuple");
    static_assert(count == 1, "type appears more than once in tuple");

    template<std::size_t... Is>
    constexpr static std::size_t find_index(std::index_sequence<Is...>)
    {
        std::size_t index = 0;
        ((std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<Ts>> ? (index = Is + 1) : 0), ...);
        return index - 1; // 转换为0-based
    }

    constexpr static std::size_t value = find_index(std::index_sequence_for<Ts...>{});
};
template<typename T, typename Tuple>
constexpr auto tuple_index_v = tuple_index<T, Tuple>::value;


template<typename Tuple>
struct tuple_to_variant;

template<typename... Ts>
struct tuple_to_variant<std::tuple<Ts...>>
{
    using type = std::variant<std::monostate, Ts...>;
};


template<typename Tuple>
using tuple_to_variant_t = typename tuple_to_variant<Tuple>::type;


template<typename... Ts>
struct cache_object_trait
{
    using return_types = std::tuple<typename function_trait<Ts>::return_type...>;
    using function_types = std::tuple<Ts...>;
};

struct no_lock_for_co
{
    bool write_lock() const noexcept
    {
        return true;
    };
    bool read_lock() const noexcept
    {
        return true;
    }
};

template<typename T, typename Trait, typename Lock = no_lock_for_co>
class cache_object : private T
{
public:
    using value_t = tuple_to_variant_t<typename Trait::return_types>;
    using const_functions_t = Trait::function_types;
    using T::T;

    struct map_item_t
    {
        value_t m_value{std::monostate{}};
        bool m_dirty{true};
    };

    template<typename Func, typename... FArgs>
    decltype(auto) set(Func&& f, FArgs&&... args)
    {
        auto lock = m_lock.write_lock();
        for (auto& [k, v] : m_map)
        {
            v.m_dirty = true;
        }
        return std::invoke(std::forward<Func>(f), this->derived(), std::forward<FArgs>(args)...);
    }

    template<typename Func, typename... FArgs>
    decltype(auto) get(Func&& f, FArgs&&... args) const
    {
        auto lock = m_lock.read_lock();
        auto& v = m_map[tuple_index_v<Func, const_functions_t>];
        if (v.m_dirty)
        {
            v.m_value = std::move(std::invoke(std::forward<Func>(f), this->derived(), std::forward<FArgs>(args)...));
            v.m_dirty = false;
        }
        return std::get<tuple_index_v<Func, const_functions_t> + 1>(v.m_value);
    }

private:
    T& derived() noexcept
    {
        return *this;
    }

    const T& derived() const noexcept
    {
        return *this;
    }

private:
    mutable std::unordered_map<size_t, map_item_t> m_map{};
    mutable Lock m_lock;
};

} // namespace mlts
