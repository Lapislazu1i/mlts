#pragma once
#include "detail/lambda_wrap.hpp"
#include "overloads.hpp"
#include "rc_type_container.hpp"
#include <tuple>


namespace mlts
{
template<typename FunSig, size_t SSize = 32, bool IsStatic = true>
class lambda_wrap
{
};

template<typename R, typename... Args, size_t SSize>
class lambda_wrap<R(Args...), SSize, false>
{
public:
    using return_type = R;

    constexpr lambda_wrap() = default;
    template<typename Func>
    // requires std::is_nothrow_invocable_r_v<R, Func>
    lambda_wrap(Func&& f) noexcept
        : m_wrap_base(
              std::make_unique<detail::lambda_wrap_impl<std::remove_cvref_t<Func>, R, Args...>>(std::forward<Func>(f)))
    {
    }

    lambda_wrap(const lambda_wrap&) = delete;
    lambda_wrap& operator=(const lambda_wrap&) = delete;
    lambda_wrap(lambda_wrap&& val) noexcept = default;
    lambda_wrap& operator=(lambda_wrap&& val) noexcept = default;

    template<typename... RunArgs>
    R operator()(RunArgs... args) noexcept
    {
        return m_wrap_base->run(std::forward<RunArgs>(args)...);
    }

    [[no_unique_address]] std::unique_ptr<detail::lambda_wrap_base<R, Args...>> m_wrap_base{};
};

template<typename R, typename... Args, size_t SSize>
class lambda_wrap<R(Args...), SSize, true>
{
public:
    using return_type = R;
    using base_t = detail::lambda_wrap_base<R, Args...>;

    constexpr lambda_wrap() = default;

    template<typename Func>
    // requires std::is_nothrow_invocable_r_v<R, Func>
    lambda_wrap(Func&& f) noexcept : m_is_init(true)
    {
        using func_t = std::remove_cvref_t<Func>;
        using impl_t = detail::lambda_wrap_impl<func_t, R, Args...>;
        static_assert(sizeof(impl_t) <= SSize, "function size greater then buffer size");
        std::construct_at<impl_t>(reinterpret_cast<impl_t*>(m_buffer), std::forward<Func>(f));
    }

    ~lambda_wrap()
    {
        if (m_is_init)
        {
            reinterpret_cast<base_t*>(m_buffer)->destroy_at(reinterpret_cast<void*>(m_buffer));
        }
    }

    lambda_wrap(const lambda_wrap&) = delete;
    lambda_wrap& operator=(const lambda_wrap&) = delete;
    lambda_wrap(lambda_wrap&& val) noexcept = default;
    lambda_wrap& operator=(lambda_wrap&& val) noexcept = default;

    template<typename... RunArgs>
    R operator()(RunArgs... args) noexcept
    {
        return reinterpret_cast<base_t*>(m_buffer)->run(std::forward<RunArgs>(args)...);
    }
    char m_buffer[SSize];
    bool m_is_init{false};
};

template<typename FunSig, size_t SSize = 16>
class lambda_box
{
};

template<typename R, typename... Args, size_t SSize>
class lambda_box<R(Args...), SSize>
{
public:
    lambda_box() = default;
    using func_t = R(Args...);

    template<typename Func>
    // requires std::is_nothrow_invocable_r_v<R, Func>
    explicit lambda_box(Func&& f) noexcept
        : m_box(lambda_wrap<func_t, SSize, sizeof(std::remove_cvref_t<Func>) <= SSize>(std::forward<Func>(f)))
    {
        // auto tmp =  lambda_wrap<func_t, SSize, sizeof(std::remove_cvref_t<Func>) <= SSize>(std::forward<Func>(f));
        // m_box = std::move(tmp);
    }

    ~lambda_box() = default;
    lambda_box(const lambda_box&) = delete;
    lambda_box& operator=(const lambda_box&) = delete;
    lambda_box(lambda_box&& val) noexcept = default;
    lambda_box& operator=(lambda_box&& val) noexcept = default;

    template<typename... RunArgs>
    R operator()(RunArgs... args) noexcept
    {
        auto args_tuple = std::forward_as_tuple(args...);
        auto visitor = overloads{
            [&args_tuple](lambda_wrap<func_t, SSize, false>& wrap) -> R { return std::apply(wrap, args_tuple); },
            [&args_tuple](lambda_wrap<func_t, SSize, true>& wrap) -> R { return std::apply(wrap, args_tuple); }};
        return m_box.visit(visitor);
    }

private:
    rc_type_container<lambda_wrap<func_t, SSize, false>, lambda_wrap<func_t, SSize, true>> m_box{};
};

} // namespace mlts
