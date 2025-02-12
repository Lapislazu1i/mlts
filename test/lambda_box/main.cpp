#include "mlts/allocator.hpp"
#include "mlts/lambda_box.hpp"
#include "mlts/lock_free_circular_queue.hpp"
#include "mlts/rc_type_container.hpp"
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

TEST(lambda_box, static_ret_pass_by_ref)
{
    int res;
    auto lambda = [&res](int v) { res = v; };
    mlts::lambda_box<void(int), 1024> f(lambda);
    f(3);
    EXPECT_EQ(3, res);
}

TEST(lambda_box, static_ret_pass)
{
    mlts::lambda_box<int(int), 1024> f([](int v) { return 2 * v; });
    int res = f(3);
    EXPECT_EQ(6, res);
}

TEST(lambda_box, static_move)
{
    move_st gm{};

    mlts::lambda_box<bool(), 1024> fm([gm]() mutable {
        gm.m_val = 12;
        return gm.m_is_move;
    });
    mlts::lambda_box<bool(), 1024> f([]() {
        volatile move_st m{};
        return m.m_is_move;
    });
    mlts::lambda_box<bool(), 1024> f3 = std::move(fm);
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
    auto lambda = [&res, buffer](int v) { res = v; };
    mlts::lambda_box<void(int), 8> f(lambda);
    f(3);
    EXPECT_EQ(3, res);
}

TEST(lambda_box, dynamic_ret_pass)
{
    char buffer[1024]{};

    mlts::lambda_box<int(int), 8> f([buffer](int v) { return 2 * v; });
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
    auto res2 = fm();
    auto res3 = f3();
    EXPECT_EQ(res, false);
    EXPECT_EQ(res2, true);
    EXPECT_EQ(0, gm.m_val);
}