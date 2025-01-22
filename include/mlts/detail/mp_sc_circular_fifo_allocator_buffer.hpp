#include "../define_type.hpp"
#include <vector>
#include <array>

namespace mlts
{
namespace detail
{
template<typename T, size_t Size>
struct mp_sc_circular_fifo_allocator_buffer : public std::array<T, Size + 1>
{
    constexpr void resize()
    {
        static_assert(Size != mlts::DynamicSize);
    }

};

template<typename T>
struct mp_sc_circular_fifo_allocator_buffer<T, mlts::DynamicSize> : public std::vector<T>
{
};
} // namespace detail
} // namespace mlts