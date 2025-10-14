#include "mlts/cache_object.hpp"
#include <gtest/gtest.h>

struct test_object
{
    void set(int v)
    {
        m_val += v;
    }
    int get() const
    {
        ++m_val;

        return m_val;
    }
    void set(std::string str)
    {
        m_str += str;
    }

    int ret_set()
    {
        return 10;
    }

    std::string get_str(int a) const
    {
        m_str += std::to_string(a);

        return m_str;
    }

    mutable int m_val{};
    mutable std::string m_str;
};
using cache_object_trait_test_object =
    mlts::cache_object_trait<decltype(&test_object::get), decltype(&test_object::get_str)>;

using cache_object_test_object = mlts::cache_object<test_object, cache_object_trait_test_object>;

TEST(cache_object, normal)
{

    cache_object_test_object ob{};
    auto value_1 = ob.get(&test_object::get);
    EXPECT_EQ(value_1, 1);

    auto str_1 = ob.get(&test_object::get_str, 1);

    EXPECT_EQ(str_1, "1");

    auto value_1t = ob.get(&test_object::get);
    EXPECT_EQ(value_1, value_1t);

    auto str_1t = ob.get(&test_object::get_str, 1);
    EXPECT_EQ(str_1, str_1t);

    ob.set([](auto& s) {
        s.set(2);
    });
    auto value_4 = ob.get(&test_object::get);
    EXPECT_EQ(value_4, 4);

    auto value_4t = ob.get(&test_object::get);
    EXPECT_EQ(value_4, value_4t);

    ob.set([](auto& s) {
        s.set("q");
    });

    auto str_2 = ob.get(&test_object::get_str, 1);
    EXPECT_EQ(str_2, "1q1");

    auto str_2t = ob.get(&test_object::get_str, 1);
    EXPECT_EQ(str_2, str_2t);
}

template<typename Mutex>
struct test_guard
{
    test_guard(Mutex& mu) : m_mu(mu)
    {
        mu.lock();
    }
    ~test_guard() noexcept
    {
        m_mu.unlock();
    }
    Mutex& m_mu;
};

struct mutex_for_co
{
    test_guard<std::mutex> write_lock() const noexcept
    {
        return test_guard<std::mutex>{m_mu};
    }

    test_guard<std::mutex> read_lock() const noexcept
    {
        return test_guard<std::mutex>{m_mu};
    }

    mutable std::mutex m_mu;
};


using cache_object_test_object_safe = mlts::cache_object<test_object, cache_object_trait_test_object, mutex_for_co>;

TEST(cache_object, multithread)
{
    constexpr auto size1 = sizeof(cache_object_test_object_safe);
    constexpr auto size2 = sizeof(cache_object_test_object);
    constexpr auto size3 = sizeof(std::mutex);
    cache_object_test_object_safe ob{};
    auto value_1 = ob.get(&test_object::get);
    EXPECT_EQ(value_1, 1);

    auto str_1 = ob.get(&test_object::get_str, 1);

    EXPECT_EQ(str_1, "1");

    auto value_1t = ob.get(&test_object::get);
    EXPECT_EQ(value_1, value_1t);

    auto str_1t = ob.get(&test_object::get_str, 1);
    EXPECT_EQ(str_1, str_1t);

    ob.set([](auto& s) {
        s.set(2);
    });
    auto value_4 = ob.get(&test_object::get);
    EXPECT_EQ(value_4, 4);

    auto value_4t = ob.get(&test_object::get);
    EXPECT_EQ(value_4, value_4t);

    ob.set([](auto& s) {
        s.set("q");
    });

    auto str_2 = ob.get(&test_object::get_str, 1);
    EXPECT_EQ(str_2, "1q1");

    auto str_2t = ob.get(&test_object::get_str, 1);
    EXPECT_EQ(str_2, str_2t);

    auto df1 = [](){
        return 1;
    };
    auto df2 = [str_2](){
        return 2;
    };
}