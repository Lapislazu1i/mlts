#include "mlts/allocator.hpp"
#include "mlts/lambda_box.hpp"
#include "mlts/lock_free_circular_queue.hpp"
#include "mlts/rc_type_container.hpp"
#include "mlts/timer.hpp"
#include <gtest/gtest.h>
#include <vector>


struct move_st
{
    move_st() = default;

    move_st(const move_st& v)
    {
        if (this == &v)
        {
            return;
        }
        m_val = v.m_val;
        m_is_move = v.m_is_move;
    }

    move_st& operator=(const move_st& v)
    {
        if (this == &v)
        {
            return *this;
        }
        m_val = v.m_val;
        m_is_move = v.m_is_move;
    }
    move_st(move_st&& v) noexcept
    {
        if (this == &v)
        {
            return;
        }
        m_val = v.m_val;
        v.m_val = 0;
        m_is_move = true;
    }
    move_st& operator=(move_st&& v) noexcept
    {
        if (this == &v)
        {
            return *this;
        }
        m_is_move = true;
        m_val = v.m_val;
        v.m_val = 0;
    }

    int m_val{};
    bool m_is_move{};
};

template<typename F>
class interval_func
{
public:
    interval_func(std::chrono::milliseconds ms, F&& f) : m_ms(ms), m_f(std::forward<F>(f))
    {
    }

    bool operator()() noexcept
    {
        auto cur = std::chrono::steady_clock::now();
        auto count = std::chrono::duration_cast<std::chrono::milliseconds>(cur - m_pre);
        if (count.count() > m_ms.count())
        {
            m_pre = cur;
            std::invoke(m_f);
            return true;
        }
        return false;
    }

    F m_f;
    std::chrono::milliseconds m_ms;
    std::chrono::steady_clock::time_point m_pre{std::chrono::steady_clock::now()};
};



TEST(lambda_box, static_ret_pass_by_ref)
{
    int res;
    auto lambda = [&res](int v) {
        res = v;
    };
    mlts::lambda_box<void(int), 1023> f(lambda);
    f(3);
    EXPECT_EQ(3, res);
}

TEST(lambda_box, static_ret_pass)
{
    mlts::lambda_box<int(int), 1023> f([](int v) {
        return 2 * v;
    });
    int res = f(3);
    EXPECT_EQ(6, res);
}

TEST(lambda_box, static_move)
{
    move_st gm{};

    mlts::lambda_box<bool(), 1023> fm([gm]() mutable {
        gm.m_val = 12;
        return gm.m_is_move;
    });
    mlts::lambda_box<bool(), 1023> f([]() {
        volatile move_st m{};
        return m.m_is_move;
    });
    mlts::lambda_box<bool(), 1023> f3 = std::move(fm);
    auto res = f();
    auto res2 = fm();
    auto res3 = f3();
    EXPECT_EQ(res, false);
    EXPECT_EQ(res2, true);
    EXPECT_EQ(0, gm.m_val);
}


TEST(lambda_box, dynamic_ret_pass_by_ref)
{
    int res;
    char buffer[1024]{};
    auto lambda = [&res, buffer](int v) {
        res = v;
    };
    mlts::lambda_box<void(int), 8> f(lambda);
    f(3);
    EXPECT_EQ(3, res);
}

TEST(lambda_box, dynamic_ret_pass)
{
    char buffer[1024]{};

    mlts::lambda_box<int(int), 8> f([buffer](int v) {
        return 2 * v;
    });
    int res = f(3);
    EXPECT_EQ(6, res);
}

TEST(lambda_box, dynamic_move)
{
    move_st gm{};
    char buffer[1024]{};

    mlts::lambda_box<bool(), 8> fm([gm, buffer]() mutable {
        gm.m_val = 12;
        return gm.m_is_move;
    });
    mlts::lambda_box<bool(), 8> f([buffer]() {
        volatile move_st m{};
        return m.m_is_move;
    });
    mlts::lambda_box<bool(), 8> f3 = std::move(fm);
    auto res = f();
    // auto res2 = fm();
    auto res3 = f3();
    EXPECT_EQ(res, false);
    EXPECT_EQ(res3, true);
    EXPECT_EQ(0, gm.m_val);
}

TEST(lambda_box, static_test_speed)
{
    char buffer[180]{};
    constexpr auto task_size = 10000000;
    std::vector<mlts::lambda_box<int(int), 31>> mlts_vec;
    std::vector<std::function<int(int)>> std_vec;
    mlts_vec.reserve(task_size);
    std_vec.reserve(task_size);
    std::vector<int> res(1);
    mlts::timer ti;
    for (int i = 0; i < task_size; ++i)
    {
        std_vec.emplace_back([buffer](int v) {
            return 2 * v;
        });
        // std::function<int(int)> f([buffer](int v) { return 2 * v; });
    }
    ti.start();

    for (auto& f : std_vec)
    {
        res[0] = f(1);
    }
    ti.end();
    auto std_res = ti.elapsed_time();

    for (int i = 0; i < task_size; ++i)
    {
        mlts_vec.emplace_back([buffer](int v) {
            return 2 * v;
        });
        // mlts::lambda_box<int(int), 31> f([buffer](int v) { return 2 * v; });
    }
    ti.start();

    for (auto& f : mlts_vec)
    {
        res[0] = f(1);
    }
    ti.end();
    auto mlts_res = ti.elapsed_time();


    // EXPECT_EQ(6, std_res.count());
    // EXPECT_EQ(61, mlts_res.count());
}
