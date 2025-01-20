#include "lock_free_queue.hpp"
#include "thread_pool.hpp"
#include "timer.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <mutex>
#include <queue>
#include <vector>


TEST(thread_pool, push_func)
{
    mlts::thread_pool<> tp(1);
    int ret{};
    tp.push_func([&ret]() { ret = 4; });
    tp.wait_done();
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(ret, 4);
}

TEST(thread_pool, identify_true_mul_thread)
{
    mlts::thread_pool<> tp(2);
    std::thread::id s1{};
    std::thread::id s2{};
    tp.push_func([&s1]() { s1 = std::this_thread::get_id(); });
    tp.push_func([&s2]() { s2 = std::this_thread::get_id(); });
    tp.wait_done();
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(s1 == s2, false);
}

TEST(thread_pool, mul_thread_push_func)
{
    std::int32_t thread_size{16};
    std::int32_t add_op_size{100000};
    mlts::thread_pool<> tp(thread_size, std::chrono::milliseconds(1), 10000000);
    std::vector<std::thread> threads;
    std::mutex mu{};
    std::int64_t real_val{};

    for (std::int32_t i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([real_val_ptr = &real_val, &tp, &mu, i, add_op_size]() {
            for (std::int32_t j = 0; j < add_op_size; ++j)
            {
                // std::scoped_lock lk(mu);
                tp.push_func([j, &mu, real_val_ptr]() {
                    std::scoped_lock lk(mu);
                    auto old = *real_val_ptr;
                    *real_val_ptr = j + old;
                });
            }
        });
    }
    std::int64_t right_val{};
    for (std::int32_t j = 0; j < add_op_size; ++j)
    {
        right_val += j;
    }
    right_val *= thread_size;
    for (auto& th : threads)
    {
        th.join();
    }
    tp.wait_done(std::chrono::milliseconds(1));
    EXPECT_EQ(real_val, right_val);
}

