#pragma once
#include <functional>
#include <tuple>


namespace mlts
{

template<typename T>
struct function_trait;

template<typename R, typename... Args>
struct function_trait<R(Args...)>
{
    using return_type = R;

    using args_tuple_type = std::tuple<Args...>;

    static constexpr size_t arity = sizeof...(Args);

    template<size_t N>
    struct arg
    {
        using type = typename std::tuple_element_t<N, args_tuple_type>;
    };

    template<size_t N>
    using arg_t = arg<N>::type;
};

template<typename R, typename... Args>
struct function_trait<std::function<R(Args...)>> : public function_trait<R(Args...)>
{
};

template<typename T>
struct function_trait : public function_trait<decltype(&T::operator())>
{
};

template<typename R, typename... Args>
struct function_trait<R (*)(Args...)> : public function_trait<R(Args...)>
{
};

template<typename C, typename R, typename... Args>
struct function_trait<R (C::*)(Args...)> : public function_trait<R(Args...)>
{
};

template<typename C, typename R, typename... Args>
struct function_trait<R (C::*)(Args...) const> : public function_trait<R(Args...)>
{
};

template<typename C, typename R, typename... Args>
struct function_trait<R (C::*)(Args...) const&> : public function_trait<R(Args...)>
{
};

template<typename C, typename R, typename... Args>
struct function_trait<R (C::*)(Args...) &&> : public function_trait<R(Args...)>
{
};

} // namespace hfl