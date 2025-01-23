#pragma once
#include <functional>
#include <numeric>
namespace mlts
{
constexpr inline size_t DynamicSize = std::numeric_limits<size_t>::max();
constexpr inline std::int32_t AllocatorWaitCount = 100000;
}