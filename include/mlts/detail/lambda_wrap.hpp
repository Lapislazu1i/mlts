#pragma once
#include <functional>
#include <memory>
#include <type_traits>


namespace mlts
{
namespace detail
{

template<typename R, typename... Args>
struct lambda_wrap_base
{
    virtual ~lambda_wrap_base()
    {
    }

    virtual R run(Args... args) noexcept = 0;

    virtual void destroy_at(void* buffer) noexcept = 0;
};

template<typename Func, typename R, typename... Args>
// requires std::is_nothrow_invocable_r_v<R, Func>
struct lambda_wrap_impl : public lambda_wrap_base<R, Args...>
{
    lambda_wrap_impl() = default;

    template<typename CFunc>
    explicit lambda_wrap_impl(CFunc&& f) noexcept : m_func(std::forward<CFunc>(f))
    {
    }

    ~lambda_wrap_impl() = default;
    lambda_wrap_impl(const lambda_wrap_impl&) = delete;
    lambda_wrap_impl& operator=(const lambda_wrap_impl&) = delete;
    lambda_wrap_impl(lambda_wrap_impl&& val) noexcept = default;
    lambda_wrap_impl& operator=(lambda_wrap_impl&& val) noexcept = default;

    R run(Args... args) noexcept override
    {
        return std::invoke(m_func, std::forward<Args>(args)...);
    }

    void destroy_at(void* buffer) noexcept override
    {
        std::destroy_at<lambda_wrap_impl>(reinterpret_cast<lambda_wrap_impl*>(buffer));
    }

    [[no_unique_address]] Func m_func;
};

} // namespace detail
} // namespace mlts