#include "mlts/allocator.hpp"
#include "mlts/function.hpp"
#include "mlts/timer.hpp"
#include <array>
#include <functional>
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

TEST(function, test1)
{
    std::string ss = "osidfjsiosdfjdsfdsfdsofsoifjdiodjfiojiodfjiosjsiofdjiofjiofjoidjf";
    auto ss_len = ss.size();
    {

        mlts::function<int()> f3 = [ss] {
            std::string s = ss;
            return s.size();
        };
        auto rr1 = f3();
        mlts::function<int()> f4 = f3;
        auto rr2 = f4();
        EXPECT_EQ(rr1, ss_len);
        EXPECT_EQ(rr2, ss_len);
    }

    char buf[1024]{"nini"};

    auto buflen = 4;
    {
        mlts::function<int()> f3 = [buf] {
            std::string s = buf;
            return s.size();
        };
        auto rr1 = f3();
        mlts::function<int()> f4 = f3;
        auto rr2 = f4();
        EXPECT_EQ(rr1, buflen);
        EXPECT_EQ(rr2, buflen);
    }

    mlts::function<int()> f = [] {
        return 22;
    };
    auto r = f(mlts::no_exp_t);
    mlts::function<int()> f2 = f;
    // mlts::function<int()> f2 = std::move(f);
    auto r2 = f2(mlts::no_exp_t);

    {
        mlts::function<int()> f3 = [] {
            return 23;
        };
        f3.swap(f);
        auto r3 = f();
        EXPECT_EQ(r3, 23);
    }

    EXPECT_EQ(r.value, 22);
    EXPECT_EQ(r2.value, 22);
}

TEST(function, swap)
{
    std::string str;
    for (int i = 0; i < 8; ++i)
    {
        str += std::to_string(i);
    }
    std::string longstr;
    for (int i = 0; i < 128; ++i)
    {
        longstr += std::to_string(i);
    }

    mlts::function<std::string()> f1 = [str]() {
        return str;
    };
    mlts::function<std::string()> f2{};
    EXPECT_EQ(f2.operator bool(), false);

    f1.swap(f2);
    auto r1 = f2();
    EXPECT_EQ(str, r1);
    EXPECT_EQ(f1.operator bool(), false);

    mlts::function<std::string()> f3 = [longstr]() {
        return longstr;
    };
    mlts::function<std::string()> f4{};

    f4.swap(f3);

    EXPECT_EQ(longstr, f4());
    EXPECT_EQ(f3.operator bool(), false);

    f4.swap(f2);

    EXPECT_EQ(str, f4());
    EXPECT_EQ(longstr, f2());
}

TEST(function, trivial_swap)
{
    std::array<char, 8> str;
    for (int i = 0; i < 8; ++i)
    {
        str[i] = std::to_string(i)[0];
    }
    std::array<char, 128> longstr;
    for (int i = 0; i < 128; ++i)
    {
        longstr[i] = std::to_string(i)[0];
    }

    mlts::function<std::array<char, 8>()> f1 = [str]() {
        return str;
    };
    mlts::function<std::array<char, 8>()> f2{};
    EXPECT_EQ(f2.operator bool(), false);

    f1.swap(f2);
    auto r1 = f2();
    EXPECT_EQ(str, r1);
    EXPECT_EQ(f1.operator bool(), false);

    mlts::function<std::array<char, 128>()> f3 = [longstr]() {
        return longstr;
    };
    mlts::function<std::array<char, 128>()> f4{};

    f4.swap(f3);

    EXPECT_EQ(longstr, f4());
    EXPECT_EQ(f3.operator bool(), false);
}

TEST(function, copy)
{
    std::string str;
    for (int i = 0; i < 8; ++i)
    {
        str += std::to_string(i);
    }
    std::string longstr;
    for (int i = 0; i < 128; ++i)
    {
        longstr += std::to_string(i);
    }

    mlts::function<std::string()> f1 = [str]() {
        return str;
    };
    mlts::function<std::string()> f2{};

    f2.copy(f1);
    auto r1 = f1();
    auto r2 = f2();
    EXPECT_EQ(str, r1);
    EXPECT_EQ(str, r2);

    mlts::function<std::string()> f3 = [longstr]() {
        return longstr;
    };
    mlts::function<std::string()> f4{};
    f4.copy(f3);


    EXPECT_EQ(longstr, f3());
    EXPECT_EQ(longstr, f4());

    f1.copy(f4);
    EXPECT_EQ(longstr, f1());
    mlts::function<std::string()> f5{};
    f2.copy(f5);
    EXPECT_EQ(f2.operator bool(), false);
}

TEST(function, test_move_only)
{
    std::unique_ptr<int> p{std::make_unique<int>(11)};
    mlts::function<int()> f3 = [pp = std::move(p)]() {
        return *pp;
    };
    auto r = f3();
    EXPECT_EQ(r, 11);
    mlts::function<int()> f4 = std::move(f3);
    auto r2= f4();
    EXPECT_EQ(r2, 11);
}

#ifndef _DEBUG
constexpr auto per_run_count = 100000000;
constexpr auto per_trivial_run_count = 1000000000;
constexpr auto per_small_run_count = 1000000000;
constexpr auto per_trivial_small_run_count = 5000000000;

TEST(function, performance_mlts)
{
    std::string longstr;
    for (int i = 0; i < 128; ++i)
    {
        longstr += std::to_string(i);
    }
    mlts::function<std::string()> f = [longstr]() {
        return longstr;
    };

    for (size_t i = 0; i < per_run_count; ++i)
    {
        volatile std::string r = f();
    }
}

TEST(function, performance_std)
{
    std::string longstr;
    for (int i = 0; i < 128; ++i)
    {
        longstr += std::to_string(i);
    }
    std::function<std::string()> f = [longstr]() {
        return longstr;
    };

    for (size_t i = 0; i < per_run_count; ++i)
    {
        volatile std::string r = f();
    }
}

TEST(function, performance_small_mlts)
{
    std::string longstr;
    for (int i = 0; i < 8; ++i)
    {
        longstr += std::to_string(i);
    }
    mlts::function<std::string()> f = [longstr]() {
        return longstr;
    };

    for (size_t i = 0; i < per_small_run_count; ++i)
    {
        volatile std::string r = f();
    }
}

TEST(function, performance_small_std)
{
    std::string longstr;
    for (int i = 0; i < 8; ++i)
    {
        longstr += std::to_string(i);
    }
    std::function<std::string()> f = [longstr]() {
        return longstr;
    };

    for (size_t i = 0; i < per_small_run_count; ++i)
    {
        volatile std::string r = f();
    }
}


TEST(function, performance_trivial_mlts)
{
    std::array<char, 128> longstr;
    for (int i = 0; i < 128; ++i)
    {
        longstr[i] = std::to_string(i)[0];
    }
    mlts::function<std::array<char, 128>()> f = [longstr]() noexcept {
        return longstr;
    };

    for (size_t i = 0; i < per_trivial_run_count; ++i)
    {
        volatile std::array<char, 128> r = f();
    }
}

TEST(function, performance_trivial_std)
{
    std::array<char, 128> longstr;
    for (int i = 0; i < 128; ++i)
    {
        longstr[i] = std::to_string(i)[0];
    }
    std::function<std::array<char, 128>()> f = [longstr]() noexcept {
        return longstr;
    };

    for (size_t i = 0; i < per_trivial_run_count; ++i)
    {
        volatile std::array<char, 128> r = f();
    }
}

TEST(function, performance_trivial_small_mlts)
{
    std::array<char, 8> longstr;
    for (int i = 0; i < 8; ++i)
    {
        longstr[i] = std::to_string(i)[0];
    }
    mlts::function<std::array<char, 8>()> f = [longstr]() noexcept {
        return longstr;
    };

    for (size_t i = 0; i < per_trivial_small_run_count; ++i)
    {
        volatile std::array<char, 8> r = f();
    }
}

TEST(function, performance_trivial_small_std)
{
    std::array<char, 8> longstr;
    for (int i = 0; i < 8; ++i)
    {
        longstr[i] = std::to_string(i)[0];
    }
    std::function<std::array<char, 8>()> f = [longstr]() noexcept {
        return longstr;
    };

    for (size_t i = 0; i < per_trivial_small_run_count; ++i)
    {
        volatile std::array<char, 8> r = f();
    }
}

#endif