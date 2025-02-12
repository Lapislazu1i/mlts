#pragma once

namespace mlts
{
template<class... Ts>
struct overloads : Ts...
{
    using Ts::operator()...;
};
} // namespace mlts