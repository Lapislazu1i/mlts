#pragma once
#include <chrono>
namespace mlts
{
struct timer
{
public:
    timer()
    {
        m_start = std::chrono::high_resolution_clock::now();
    }

    void start()
    {
        m_start = std::chrono::high_resolution_clock::now();
    }

    void end()
    {
        m_end = std::chrono::high_resolution_clock::now();
    }

    template<typename T = std::chrono::milliseconds>
    auto elapsed_time() const
    {
        auto duration = m_end - m_start;
        return std::chrono::duration_cast<T>(duration);
    }


private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_end;
};

constexpr bool in_duration(std::int64_t l_count, std::int64_t r_count, std::int64_t delim = 100)
{
    auto diff = l_count - r_count;
    diff = diff * diff;
    const auto delim2 = delim * delim;
    if (diff < delim2)
    {
        return true;
    }
    return false;
}
} // namespace mlts