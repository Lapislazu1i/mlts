#include "../define_type.hpp"
#include <array>
#include <bit>
#include <memory>


namespace mlts
{
namespace detail
{
template<typename T, std::uint32_t Size>
struct queue_circular_buffer : public std::array<T, Size>
{
    constexpr void resize(std::uint32_t)
    {
        static_assert(Size != mlts::DynamicSize);
    }

    constexpr std::uint32_t mask() const noexcept
    {
        return Size - 1;
    }
};

template<typename T>
struct queue_circular_buffer<T, mlts::DynamicSize32>
{

    constexpr queue_circular_buffer(std::uint32_t capacity = 1024)
        : m_buffer(std::make_unique_for_overwrite<T[]>(capacity)), m_capacity(capacity), m_mask(capacity - 1)
    {
        assert(std::has_single_bit(capacity) && "not support");
        static_assert(false, "not support");
    }

    constexpr void resize(size_t size)
    {
        std::uint32_t capacity = m_capacity * 2;
    }
    constexpr std::uint32_t mask() const noexcept
    {
        return m_mask;
    }
    std::unique_ptr<T[]> m_buffer;
    std::uint32_t m_capacity;
    std::uint32_t m_mask;
};

} // namespace detail
} // namespace mlts