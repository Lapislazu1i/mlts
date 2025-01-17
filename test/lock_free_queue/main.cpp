#include "lock_free_queue.hpp"
#include "timer.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <mutex>
#include <queue>
#include <vector>


using fq_node_type = mlts::lock_free_queue_node<int>;
struct test_allocator
{
    ~test_allocator()
    {
        s_callback(m_address);
    }
    fq_node_type* allocate(size_t count)
    {
        fq_node_type* tmp{};
        tmp = (fq_node_type*)malloc(sizeof(fq_node_type) * count);
        if (tmp == nullptr) [[unlikely]]
        {
            throw std::bad_alloc{};
        }
        m_address[tmp]++;
        return tmp;
    }
    void deallocate(fq_node_type* p, size_t count)
    {
        free(p);
        m_address[p]--;
    }
    std::map<fq_node_type*, int> m_address{};
    static std::function<void(const std::map<fq_node_type*, int>&)> s_callback;
};
std::function<void(const std::map<fq_node_type*, int>&)> test_allocator::s_callback = nullptr;

TEST(lock_free_queue, pop_front)
{
    mlts::lock_free_queue<int> queue{};

    int count{10};
    for (int i = 2; i < count; ++i)
    {
        queue.push_back(i);
    }
    int val{};
    auto op = queue.pop_front(val);

    EXPECT_EQ(op, true);
    EXPECT_EQ(val, 2);
}

TEST(lock_free_queue, destroy)
{
    std::map<fq_node_type*, int> ptr_map{};
    test_allocator::s_callback = [&ptr_map](const std::map<fq_node_type*, int>& m) { ptr_map = m; };

    {
        mlts::lock_free_queue<int, test_allocator> queue;
        queue.push_back(1);
        queue.push_back(2);
        queue.push_back(3);
    }

    bool has_leak{false};

    for (const auto& [k, v] : ptr_map)
    {
        if (v != 0)
        {
            has_leak = true;
            break;
        }
    }
    EXPECT_EQ(ptr_map.size(), 3 + 1);
    EXPECT_EQ(has_leak, false);

    // queue.
}

TEST(lock_free_queue, mul_thread_push_back_cmp_res)
{
    std::mutex mutex{};
    std::vector<std::thread> threads{};
    mlts::lock_free_queue<int> queue{};
    queue.push_back(0);
    int max_int{100000};
    int thread_size = 8;
    mlts::timer ti{};
    ti.start();
    for (int i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([&queue, max_int]() {
            for (int j = 0; j < max_int; ++j)
            {
                queue.push_back(j);
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }
    ti.end();
    auto free_time = ti.elapsed_time();

    std::vector<std::thread> normal_threads{};
    std::queue<int, std::deque<int>> normal_queue;
    ti.start();

    for (int i = 0; i < thread_size; ++i)
    {
        normal_threads.emplace_back([&mutex, max_int, &normal_queue]() {
            for (int j = 0; j < max_int; ++j)
            {
                std::scoped_lock lk(mutex);
                normal_queue.push(j);
            }
        });
    }
    for (auto& th : normal_threads)
    {
        th.join();
    }
    ti.end();
    auto normal_time = ti.elapsed_time();

    std::int64_t right_res{};
    for (int i = 0; i < max_int; ++i)
    {
        right_res += i;
    }
    right_res *= thread_size;

    std::int64_t normal_res{};

    while (not normal_queue.empty())
    {
        normal_res += normal_queue.front();
        normal_queue.pop();
    }

    std::int64_t free_res{};
    std::int64_t free_res2{};
    // std::ofstream ofs("d:\\benchmark.txt");
    // ofs << "free:" << free_time << "\n" << "normal:" << normal_time << "\n";

    bool op;
    do
    {
        int tmp_int{};
        op = queue.pop_front(tmp_int);
        free_res += tmp_int;
    } while (op == true);


    EXPECT_EQ(free_res, normal_res);
    // EXPECT_EQ(free_res, free_res2);
}

TEST(lock_free_queue, mul_thread_pop_front_cmp_res)
{
    std::mutex mutex{};
    std::vector<std::thread> threads{};
    mlts::lock_free_queue<int> queue{};
    queue.push_back(0);
    int max_int{1000};
    int thread_size = 16;

    for (int i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([&queue, max_int]() {
            for (int j = 0; j < max_int; ++j)
            {
                queue.push_back(j);
            }
        });
    }


    std::int64_t free_res{};

    std::vector<std::thread> pop_threads{};

    size_t all_len = thread_size * max_int;

    for (int i = 0; i < 1; ++i)
    {
        pop_threads.emplace_back([&queue, &free_res, &mutex, &all_len]() {
            bool op;

            for (size_t j = 0; j < all_len;)
            {

                int tmp_int{};
                op = queue.pop_front(tmp_int);
                if (op)
                {
                    mutex.lock();
                    free_res += tmp_int;
                    mutex.unlock();
                    ++j;
                }
            }
        });
    }

    for (auto& th : pop_threads)
    {
        th.join();
    }

    for (auto& th : threads)
    {
        th.join();
    }

    // need pop again
    int tmp_int{};
    auto op = queue.pop_front(tmp_int);
    free_res += tmp_int;
    std::int64_t right_res{};
    for (int i = 0; i < max_int; ++i)
    {
        right_res += i;
    }
    right_res *= thread_size;

    std::int64_t rit{};
    for (int i = 0; i < thread_size; ++i)
    {
        for (int j = 0; j < max_int; ++j)
        {
            rit += j;
        }
    }

    EXPECT_EQ(free_res, right_res);
}


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}