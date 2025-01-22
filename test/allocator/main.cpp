#include "mlts/allocator.hpp"
#include "mlts/timer.hpp"
#include <gtest/gtest.h>

TEST(mp_sc_circular_fifo_allocator, move)
{
    mlts::mp_sc_circular_fifo_allocator<int> alloc{2};
    auto type_size = sizeof(int);
    auto* p1 = alloc.allocate(1);
    auto* p2 = alloc.allocate(1);
    auto diff = reinterpret_cast<std::ptrdiff_t>(p2) - reinterpret_cast<std::ptrdiff_t>(p1);
    EXPECT_EQ(diff, type_size);
    mlts::mp_sc_circular_fifo_allocator<int> alloc2(std::move(alloc));
    alloc2.deallocate(p1, 1);
    alloc2.deallocate(p2, 1);
    auto* p3 = alloc2.allocate(1);
    auto* p4 = alloc2.allocate(1);
    EXPECT_EQ(p1, p4);
}

TEST(mp_sc_circular_fifo_allocator, address_test)
{
    mlts::mp_sc_circular_fifo_allocator<int> alloc{2};
    auto type_size = sizeof(int);
    auto* p1 = alloc.allocate(1);
    auto* p2 = alloc.allocate(1);
    auto diff = reinterpret_cast<std::ptrdiff_t>(p2) - reinterpret_cast<std::ptrdiff_t>(p1);
    EXPECT_EQ(diff, type_size);
    alloc.deallocate(p1, 1);
    alloc.deallocate(p2, 1);
    auto* p3 = alloc.allocate(1);
    auto* p4 = alloc.allocate(1);
    EXPECT_EQ(p1, p4);
}

TEST(mp_sc_circular_fifo_allocator, static_address_test)
{
    mlts::mp_sc_circular_fifo_allocator<int, 2> alloc(2);
    auto type_size = sizeof(int);
    auto* p1 = alloc.allocate(1);
    auto* p2 = alloc.allocate(1);
    auto diff = reinterpret_cast<std::ptrdiff_t>(p2) - reinterpret_cast<std::ptrdiff_t>(p1);
    EXPECT_EQ(diff, type_size);
    alloc.deallocate(p1, 1);
    alloc.deallocate(p2, 1);
    auto* p3 = alloc.allocate(1);
    auto* p4 = alloc.allocate(1);
    EXPECT_EQ(p1, p4);
}

TEST(mp_sc_circular_fifo_allocator, address_test_over)
{
    try
    {
        mlts::mp_sc_circular_fifo_allocator<int> alloc{2};
        auto type_size = sizeof(int);
        auto* p1 = alloc.allocate(1);
        auto* p2 = alloc.allocate(1);
        auto* p3 = alloc.allocate(1);
        EXPECT_EQ(1, 2);

    }
    catch (std::exception&)
    {
        EXPECT_EQ(1, 1);
    }
}

TEST(mp_sc_circular_fifo_allocator, static_address_test_over)
{
    try
    {
        mlts::mp_sc_circular_fifo_allocator<int, 2> alloc{};
        auto type_size = sizeof(int);
        auto* p1 = alloc.allocate(1);
        auto* p2 = alloc.allocate(1);
        auto* p3 = alloc.allocate(1);
        EXPECT_EQ(1, 2);

    }
    catch (std::exception&)
    {
        EXPECT_EQ(1, 1);
    }
}

TEST(mp_sc_circular_fifo_allocator, mul_thread_test)
{
    int el_size = 10;
    int th_size=  2;
    // mlts::mp_sc_circular_fifo_allocator<int> alloc(el_size * th_size);
    mlts::mp_sc_circular_fifo_allocator<int, 10> alloc(el_size * th_size);
    auto* p1= alloc.allocate(1);
    alloc.deallocate(0, 1);
    alloc.deallocate(0, 1);
    alloc.deallocate(0, 1);
    auto* p2= alloc.allocate(1);


}