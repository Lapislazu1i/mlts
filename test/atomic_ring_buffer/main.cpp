#include "mlts/lambda_box.hpp"
#include "mlts/ring_buffer.hpp"
#include "mlts/atomic_ring_buffer.hpp"
#include "mlts/thread_pool.hpp"
#include "mlts/timer.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <mutex>
#include <queue>
#include <vector>

template<typename T>
using t_ring_buffer = mlts::atomic_ring_buffer<T>;

TEST(ring_buffer, len)
{
    t_ring_buffer<int> rb{2};
    rb.put_one(1);
    rb.put_one(2);
    auto ret = rb.exist_len();
    auto ret2 = rb.empty_len();
    EXPECT_EQ(ret, 2);
    EXPECT_EQ(ret2, 0);
}

TEST(ring_buffer, put_get)
{
    t_ring_buffer<int> rb{2};
    rb.put_one(1);
    rb.put_one(2);
    int arr[2];
    auto exist_len = rb.exist_len();
    auto ret = rb.get(arr, 2);
    EXPECT_EQ(ret, 2);
    EXPECT_EQ(exist_len, 2);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
}

TEST(ring_buffer, over_1)
{
    t_ring_buffer<int> rb{2};
    rb.put_one(1);
    rb.put_one(2);
    auto ret1 = rb.put_one(3);
    auto get1 = rb.get_one();
    auto get2 = rb.get_one();
    auto get3 = rb.get_one();


    rb.put_one(4);
    rb.put_one(5);
    int arr2[2];
    rb.get(arr2, 2);


    t_ring_buffer<int> rb2{2};
    std::vector<int> arr{1, 2, 3};
    auto ret2 = rb2.put(arr);
    EXPECT_EQ(ret1, false);
    EXPECT_EQ(ret2, 2);

    EXPECT_EQ(*get1, 1);
    EXPECT_EQ(*get2, 2);
    EXPECT_EQ(get3.has_value(), false);

    EXPECT_EQ(arr2[0], 4);
    EXPECT_EQ(arr2[1], 5);
}

static inline int des_count{0};
struct tst
{
    tst() = default;

    ~tst()
    {
        ++des_count;
    }
    int v{};
};

TEST(ring_buffer, des)
{
    {
        t_ring_buffer<tst> rb{2};
    }
    EXPECT_EQ(des_count, 0);
    {
        t_ring_buffer<tst> rb{2};
        rb.put_one(tst{});
        rb.put_one(tst{});
    }
    EXPECT_EQ(des_count, 2);
}

TEST(ring_buffer, function)
{
    t_ring_buffer<std::function<int()>> rb(16);
    std::vector<int> vec{};
    for (int i = 0; i < 16; ++i)
    {
        vec.emplace_back(i);
    }

    for (int i = 0; i < 16; ++i)
    {
        std::function<int()> f= [vec, i]() {
            return vec[i];
        };
        rb.put_one(f);
    }

    for (int i = 0; i < 16; ++i)
    {
        auto ret = rb.get_one();
        EXPECT_EQ(vec[i], (*ret)());
    }
}

TEST(ring_buffer, multi_thread)
{
    constexpr int capacity = 1024;
    t_ring_buffer<int> buf{capacity};
    std::atomic_int sum{};
    int sum_res{};
    std::vector<int> args{};
    for (int i = 0; i < capacity; ++i)
    {
        args.emplace_back(i);
        sum_res += i;
    }


    std::thread t1([&]() {
        for (int i = 0; i < capacity;)
        {
            if (buf.put_one(args.at(i)))
            {
                ++i;
            }
        }
    });

    std::thread t2([&]() {
        for (int i = 0; i < capacity;)
        {
            auto res = buf.get_one();
            if (res)
            {
                sum += *res;
                ++i;
            }
        }
    });
    t1.join();
    t2.join();
    EXPECT_EQ(sum.load(), sum_res);
}