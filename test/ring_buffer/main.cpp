#include "mlts/lambda_box.hpp"
#include "mlts/thread_pool.hpp"
#include "mlts/timer.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <mutex>
#include <queue>
#include <vector>
#include "mlts/ring_buffer.hpp"


TEST(ring_buffer, len)
{
    mlts::ring_buffer<int> rb{2};
    rb.put_one(1);
    rb.put_one(2);
    auto ret = rb.exist_len();
    auto ret2 = rb.empty_len();
    EXPECT_EQ(ret, 2);
    EXPECT_EQ(ret2, 0);
}

TEST(ring_buffer, put_get)
{
    mlts::ring_buffer<int> rb{2};
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
    mlts::ring_buffer<int> rb{2};
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


    mlts::ring_buffer<int> rb2{2};
    std::vector<int> arr{1,2,3};
    auto ret2 = rb2.put(arr);
    EXPECT_EQ(ret1, false);
    EXPECT_EQ(ret2, 2);

    EXPECT_EQ(*get1, 1);
    EXPECT_EQ(*get2, 2);
    EXPECT_EQ(get3.has_value(), false);

    EXPECT_EQ(arr2[0], 4);
    EXPECT_EQ(arr2[1], 5);


}

