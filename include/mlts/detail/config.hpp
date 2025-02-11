#pragma once
#include <new>

namespace mlts
{
namespace detail
{
    constexpr auto k_machine_cache_line = std::hardware_constructive_interference_size;
}
} // namespace mlts