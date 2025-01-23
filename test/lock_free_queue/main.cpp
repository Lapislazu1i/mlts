#include "mlts/allocator.hpp"
#include "mlts/lock_free_queue.hpp"
#include "mlts/timer.hpp"
#include <sstream>
#include <fstream>
#include <functional>
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
    int max_int{100000};
    int thread_size = 16;
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
    bool op;
    do
    {
        int tmp_int{};
        op = queue.pop_front(tmp_int);
        if (op)
        {
            free_res += tmp_int;
        }
    } while (op == true);
    std::stringstream ss{};
    ss << "normal " << normal_time << "free " << free_time << "\n";
    fprintf(stdout, "%s",ss.str().c_str());
    EXPECT_EQ(free_res, normal_res);
}

TEST(lock_free_queue, mul_thread_push_back_func_cmp_res)
{
    std::mutex mutex{};
    constexpr int max_int{100000};
    constexpr int thread_size = 16;
    std::vector<std::thread> threads{};
    using test_queue_type = mlts::lock_free_queue<std::function<int()>>;
    std::unique_ptr<test_queue_type> queue_ptr = std::make_unique<test_queue_type>();
    auto& queue = *queue_ptr;
    mlts::timer ti{};
    ti.start();
    for (int i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([&queue, max_int]() {
            for (int j = 0; j < max_int; ++j)
            {
                queue.push_back([j]() { return j; });
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
    std::queue<std::function<int()>, std::deque<std::function<int()>>> normal_queue;
    ti.start();

    for (int i = 0; i < thread_size; ++i)
    {
        normal_threads.emplace_back([&mutex, max_int, &normal_queue]() {
            for (int j = 0; j < max_int; ++j)
            {
                std::scoped_lock lk(mutex);
                normal_queue.push([j]() { return j; });
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
        normal_res += normal_queue.front()();
        normal_queue.pop();
    }

    std::int64_t free_res{};
    bool op;
    do
    {
        std::function<int()> tmp_int;
        op = queue.pop_front(tmp_int);
        if (op)
        {
            free_res += tmp_int();
        }
    } while (op == true);
    std::stringstream ss{};
    ss << "normal " << normal_time << "free " << free_time << "\n";
    fprintf(stdout, "%s",ss.str().c_str());
    EXPECT_EQ(free_res, normal_res);
}

TEST(lock_free_queue, mul_thread_push_back_func_cmp_res_with_static_mp_sc_circular_fifo_allocator)
{
    std::mutex mutex{};
    constexpr int max_int{100000};
    constexpr int thread_size = 16;
    std::vector<std::thread> threads{};
    using test_queue_type =
        mlts::lock_free_queue<std::function<int()>,
                              mlts::mp_sc_circular_fifo_allocator<mlts::lock_free_queue_node<std::function<int()>>,
                                                                  (max_int + 1) * (thread_size)>>;
    std::unique_ptr<test_queue_type> queue_ptr = std::make_unique<test_queue_type>();
    auto& queue = *queue_ptr;
    mlts::timer ti{};
    ti.start();
    for (int i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([&queue, max_int]() {
            for (int j = 0; j < max_int; ++j)
            {
                queue.push_back([j]() { return j; });
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
    std::queue<std::function<int()>, std::deque<std::function<int()>>> normal_queue;
    ti.start();

    for (int i = 0; i < thread_size; ++i)
    {
        normal_threads.emplace_back([&mutex, max_int, &normal_queue]() {
            for (int j = 0; j < max_int; ++j)
            {
                std::scoped_lock lk(mutex);
                normal_queue.push([j]() { return j; });
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
        normal_res += normal_queue.front()();
        normal_queue.pop();
    }

    std::int64_t free_res{};
    bool op;
    do
    {
        std::function<int()> tmp_int;
        op = queue.pop_front(tmp_int);
        if (op)
        {
            free_res += tmp_int();
        }
    } while (op == true);
    std::stringstream ss{};
    ss << "normal " << normal_time << "free " << free_time << "\n";
    fprintf(stdout, "%s",ss.str().c_str());
    EXPECT_EQ(free_res, normal_res);
}


TEST(lock_free_queue, mul_thread_pop_font_push_back_func_cmp_res)
{
    std::mutex mutex{};
    constexpr int max_int{100000};
    constexpr int thread_size = 128;
    std::vector<std::thread> threads{};
    using test_queue_type =
        mlts::lock_free_queue<std::function<int()>>;
    std::unique_ptr<test_queue_type> queue_ptr = std::make_unique<test_queue_type>();
    auto& queue = *queue_ptr;
    mlts::timer ti{};
    ti.start();
    for (int i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([&queue, max_int]() {
            for (int j = 0; j < max_int; ++j)
            {
                queue.push_back([j]() { return j; });
            }
        });
    }
    auto all_size = thread_size * max_int;
    std::int64_t free_res{};
    bool op;
    for (auto i = 0; i < all_size;)
    {
        std::function<int()> tmp_int;
        op = queue.pop_front(tmp_int);
        if (op)
        {
            ++i;
            free_res += tmp_int();
        }
    }

    for (auto& th : threads)
    {
        th.join();
    }
    ti.end();
    auto free_time = ti.elapsed_time();

    std::vector<std::thread> normal_threads{};
    std::queue<std::function<int()>, std::deque<std::function<int()>>> normal_queue;
    ti.start();

    for (int i = 0; i < thread_size; ++i)
    {
        normal_threads.emplace_back([&mutex, max_int, &normal_queue]() {
            for (int j = 0; j < max_int; ++j)
            {
                std::scoped_lock lk(mutex);
                normal_queue.push([j]() { return j; });
            }
        });
    }
    for (auto& th : normal_threads)
    {
        th.join();
    }

    std::int64_t normal_res{};

    for (auto i = 0; i < all_size;)
    {
        std::scoped_lock lk(mutex);
        if (not normal_queue.empty())
        {
            normal_res += normal_queue.front()();
            normal_queue.pop();
            ++i;
        }
    }

    ti.end();
    auto normal_time = ti.elapsed_time();

    std::int64_t right_res{};
    for (int i = 0; i < max_int; ++i)
    {
        right_res += i;
    }
    right_res *= thread_size;


    std::stringstream ss{};
    ss << "normal " << normal_time << "free " << free_time << "\n";
    fprintf(stdout, "%s",ss.str().c_str());
    EXPECT_EQ(free_res, normal_res);
}

TEST(lock_free_queue, mul_thread_pop_font_push_back_func_cmp_res_with_static_mp_sc_circular_fifo_allocator)
{
    std::mutex mutex{};
    constexpr int max_int{100000};
    constexpr int thread_size = 128;
    std::vector<std::thread> threads{};
    using test_queue_type =
        mlts::lock_free_queue<std::function<int()>,
                              mlts::mp_sc_circular_fifo_allocator<mlts::lock_free_queue_node<std::function<int()>>,
                                                                  (max_int + 1) * (thread_size)>>;
    std::unique_ptr<test_queue_type> queue_ptr = std::make_unique<test_queue_type>();
    auto& queue = *queue_ptr;
    mlts::timer ti{};
    ti.start();
    for (int i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([&queue, max_int]() {
            for (int j = 0; j < max_int; ++j)
            {
                queue.push_back([j]() { return j; });
            }
        });
    }
    auto all_size = thread_size * max_int;
    std::int64_t free_res{};
    bool op;
    for (auto i = 0; i < all_size;)
    {
        std::function<int()> tmp_int;
        op = queue.pop_front(tmp_int);
        if (op)
        {
            ++i;
            free_res += tmp_int();
        }
    }

    for (auto& th : threads)
    {
        th.join();
    }
    ti.end();
    auto free_time = ti.elapsed_time();

    std::vector<std::thread> normal_threads{};
    std::queue<std::function<int()>, std::deque<std::function<int()>>> normal_queue;
    ti.start();

    for (int i = 0; i < thread_size; ++i)
    {
        normal_threads.emplace_back([&mutex, max_int, &normal_queue]() {
            for (int j = 0; j < max_int; ++j)
            {
                std::scoped_lock lk(mutex);
                normal_queue.push([j]() { return j; });
            }
        });
    }
    for (auto& th : normal_threads)
    {
        th.join();
    }

    std::int64_t normal_res{};

    for (auto i = 0; i < all_size;)
    {
        std::scoped_lock lk(mutex);
        if (not normal_queue.empty())
        {
            normal_res += normal_queue.front()();
            normal_queue.pop();
            ++i;
        }
    }

    ti.end();
    auto normal_time = ti.elapsed_time();

    std::int64_t right_res{};
    for (int i = 0; i < max_int; ++i)
    {
        right_res += i;
    }
    right_res *= thread_size;


    std::stringstream ss{};
    ss << "normal " << normal_time << "free " << free_time << "\n";
    fprintf(stdout, "%s",ss.str().c_str());
    EXPECT_EQ(free_res, normal_res);
}


TEST(lock_free_queue, single_thread_pop_front_cmp_res)
{
    std::mutex mutex{};
    std::vector<std::thread> threads{};
    mlts::lock_free_queue<int> queue{};
    int max_int{100000};
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

    std::int64_t right_res{};
    for (int i = 0; i < max_int; ++i)
    {
        right_res += i;
    }
    right_res *= thread_size;

    EXPECT_EQ(free_res, right_res);
}


TEST(lock_free_queue, single_thread_pop_front_func_cmp_res)
{
    std::mutex mutex{};
    std::vector<std::thread> threads{};
    mlts::lock_free_queue<std::function<int()>> queue{};
    int max_int{100000};
    int thread_size = 16;

    for (int i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([&queue, max_int]() {
            for (int j = 0; j < max_int; ++j)
            {
                queue.push_back([j]() { return j; });
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
                std::function<int()> tmp_int{};
                op = queue.pop_front(tmp_int);
                if (op)
                {
                    mutex.lock();
                    free_res += tmp_int();
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

    std::int64_t right_res{};
    for (int i = 0; i < max_int; ++i)
    {
        right_res += i;
    }
    right_res *= thread_size;

    EXPECT_EQ(free_res, right_res);
}

TEST(lock_free_queue, single_thread_pop_front_func_cmp_res_with_mp_sc_circular_fifo_allocator)
{
    std::mutex mutex{};
    constexpr int max_int{100000};
    constexpr int thread_size = 16;
    std::vector<std::thread> threads{};
    using normal_queue = mlts::lock_free_queue<
        std::function<int()>,
        mlts::mp_sc_circular_fifo_allocator<mlts::lock_free_queue_node<std::function<int()>>, max_int * thread_size>>;
    std::unique_ptr<normal_queue> queue_ptr = std::make_unique<normal_queue>();
    auto& queue = *queue_ptr;

    for (int i = 0; i < thread_size; ++i)
    {
        threads.emplace_back([&queue, max_int]() {
            for (int j = 0; j < max_int; ++j)
            {
                queue.push_back([j]() { return j; });
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
                std::function<int()> tmp_int{};
                op = queue.pop_front(tmp_int);
                if (op)
                {
                    mutex.lock();
                    free_res += tmp_int();
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

    std::int64_t right_res{};
    for (int i = 0; i < max_int; ++i)
    {
        right_res += i;
    }
    right_res *= thread_size;

    EXPECT_EQ(free_res, right_res);
}