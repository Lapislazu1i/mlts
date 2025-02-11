#include "mlts/allocator.hpp"
#include "mlts/lock_free_circular_queue.hpp"
#include "mlts/lock_free_queue.hpp"
#include "mlts/timer.hpp"
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <vector>


TEST(lock_free_circular_queue, pop_front)
{
    mlts::lock_free_circular_queue<int> queue{};
    constexpr auto opacity = queue.capacity();
    constexpr auto ma = queue.m_buffer.mask();
    std::atomic<std::int64_t> res{};
    std::int64_t real_res{};
    constexpr auto push_th_size = 1;
    constexpr auto pop_th_size = 1;
    constexpr auto push_loop_size = 400;
    constexpr auto pop_loop_size = push_loop_size * (push_th_size / pop_th_size);
    {
        std::vector<std::jthread> push_ths;
        std::vector<std::jthread> pop_ths;
        for (size_t i = 0; i < push_th_size; ++i)
        {
            push_ths.emplace_back([&queue]() {
                for (int j = 0; j < push_loop_size; ++j)
                {
                    queue.push(j);
                }
            });
        }

        for (size_t i = 0; i < pop_th_size; ++i)
        {
            pop_ths.emplace_back([&queue, &res]() {
                for (int j = 0; j < pop_loop_size;)
                {
                    int tmp{};
                    if (queue.pop(tmp))
                    {
                        res.fetch_add(tmp);
                        ++j;
                    }
                }
                int c = 4;
            });
        }
    }

    for (size_t i = 0; i < push_th_size; ++i)
    {
        for (int j = 0; j < push_loop_size; ++j)
        {
            real_res += j;
        }
    }
    EXPECT_EQ(res, real_res);
}
