#pragma once
#include <cstdint>
#include <functional>
#include <numeric>
namespace mlts
{
constexpr inline size_t DynamicSize = std::numeric_limits<size_t>::max();
constexpr inline std::uint32_t DynamicSize32 = std::numeric_limits<std::uint32_t>::max();
constexpr inline std::int32_t AllocatorWaitCount = 100000;
}