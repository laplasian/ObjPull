#include <gtest/gtest.h>
#include "ObjectPool.hpp"

struct TestObject { // Тестовый класс для проверки вызовов конструкторов и деструкторов
    inline static int constructor_calls = 0;
    inline static int destructor_calls = 0;
    inline static int param_constructor_calls = 0;

    int x, y;

    TestObject() : x(0), y(0) {constructor_calls++;}
    TestObject(int a, int b) : x(a), y(b) {param_constructor_calls++;}
    ~TestObject() {destructor_calls++;}

    TestObject(TestObject&& other) noexcept : x(other.x), y(other.y) {
        other.x = -1;
        other.y = -1;
    }

    static void reset() {
        constructor_calls = 0;
        destructor_calls = 0;
        param_constructor_calls = 0;
    }
};


template <typename T>
class ObjectPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestObject::reset();
    }
};

using MyTypes = ::testing::Types<int, double, TestObject>;
TYPED_TEST_CASE(ObjectPoolTest, MyTypes);

TYPED_TEST(ObjectPoolTest, AllocationAndDeallocation) {
    ObjectPool<TypeParam> pool(2);
    {
        auto obj1 = pool.alloc();
        ASSERT_NE(obj1, nullptr);

        auto obj2 = pool.alloc();
        ASSERT_NE(obj2, nullptr);
    }
    auto obj3 = pool.alloc();
    ASSERT_NE(obj3, nullptr);
}

TYPED_TEST(ObjectPoolTest, ThrowsWhenFull) {
    ObjectPool<TypeParam> pool(1);
    auto obj1 = pool.alloc();
    ASSERT_NE(obj1, nullptr);

    ASSERT_THROW(pool.alloc(), std::bad_alloc);
}

TYPED_TEST(ObjectPoolTest, ThrowsOnZeroSize) {
    ASSERT_THROW(ObjectPool<TypeParam> pool(0), std::invalid_argument);
}

TEST(ObjectPoolCustomTest, ConstructorArgumentsArePassed) {
    TestObject::reset();
    ObjectPool<TestObject> pool(1);
    EXPECT_EQ(TestObject::param_constructor_calls, 0);
    auto obj = pool.alloc(10, 20);
    EXPECT_EQ(TestObject::param_constructor_calls, 1);
    EXPECT_EQ(obj->x, 10);
    EXPECT_EQ(obj->y, 20);
}

TEST(ObjectPoolCustomTest, DestructorsAreCalled) {
    TestObject::reset();
    {
        ObjectPool<TestObject> pool(5);
        auto obj1 = pool.alloc();
        auto obj2 = pool.alloc();
    }
    EXPECT_EQ(TestObject::destructor_calls, 2);
}