#include <gtest/gtest.h>
#include "ObjectPool.hpp"

struct TestObject {
    inline static int constructor_calls = 0;
    inline static int destructor_calls = 0;
    inline static int param_constructor_calls = 0;

    int x{};
    int y{};

    TestObject() { constructor_calls++; }
    TestObject(int a, int b) : x(a), y(b) {
        constructor_calls++;
        param_constructor_calls++;
    }
    ~TestObject() { destructor_calls++; }

    TestObject(const TestObject&) = delete;
    TestObject& operator=(const TestObject&) = delete;
    TestObject(TestObject&&) = delete;
    TestObject& operator=(TestObject&&) = delete;

    template<class T>
    void free_from_pool(ObjectPool<T>& pool) { pool.free(*this); }


    static void reset() {
        constructor_calls = 0;
        destructor_calls = 0;
        param_constructor_calls = 0;
    }
};

class ObjectPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestObject::reset();
    }
};

TEST(ObjectPoolInitTest, ThrowsOnZeroSize) {
    EXPECT_THROW(ObjectPool<int> pool(0), std::invalid_argument);
}

TEST(ObjectPoolInitTest, CorrectCapacityAndSize) {
    ObjectPool<int> pool(10);
    EXPECT_EQ(pool.get_capacity(), 10);
    EXPECT_EQ(pool.get_size(), 0);
}

TEST_F(ObjectPoolTest, Alloc) {
    ObjectPool<TestObject> pool(2);
    EXPECT_EQ(pool.get_size(), 0);

    pool.alloc();
    EXPECT_EQ(pool.get_size(), 1);
    EXPECT_EQ(TestObject::constructor_calls, 1);

    pool.alloc();
    EXPECT_EQ(pool.get_size(), 2);
    EXPECT_EQ(TestObject::constructor_calls, 2);
}

TEST_F(ObjectPoolTest, AllocWithArguments) {
    ObjectPool<TestObject> pool(1);
    auto& obj = pool.alloc(10, 20);

    EXPECT_EQ(TestObject::param_constructor_calls, 1);
    EXPECT_EQ(obj.x, 10);
    EXPECT_EQ(obj.y, 20);
}

TEST_F(ObjectPoolTest, ThrowsWhenFull) {
    ObjectPool<TestObject> pool(1);
    pool.alloc();
    EXPECT_THROW(pool.alloc(), std::bad_alloc);
}

TEST_F(ObjectPoolTest, Free) {
    ObjectPool<TestObject> pool(1);
    auto& obj = pool.alloc();
    EXPECT_EQ(pool.get_size(), 1);
    EXPECT_EQ(TestObject::destructor_calls, 0);

    pool.free(obj);
    EXPECT_EQ(pool.get_size(), 0);
    EXPECT_EQ(TestObject::destructor_calls, 1);
}

TEST_F(ObjectPoolTest, Reallocation) {
    ObjectPool<TestObject> pool(1);
    auto& obj1 = pool.alloc();
    pool.free(obj1);

    auto& obj2 = pool.alloc();
    pool.free(obj2);

    EXPECT_EQ(pool.get_size(), 0);
    EXPECT_EQ(TestObject::constructor_calls, 2);
    EXPECT_EQ(TestObject::destructor_calls, 2);
}

TEST_F(ObjectPoolTest, ThrowsOnDoubleFree) {
    ObjectPool<TestObject> pool(1);
    auto& obj = pool.alloc();
    pool.free(obj);
    EXPECT_THROW(pool.free(obj), std::runtime_error);
}

TEST_F(ObjectPoolTest, ThrowsInvalidArgForFree) {
    ObjectPool<TestObject> pool(1);
    auto& obj = pool.alloc();
    TestObject not_in_pool{};
    EXPECT_THROW(pool.free(not_in_pool), std::runtime_error);
}

TEST_F(ObjectPoolTest, TDestructorsCallByPoolDestructor) {
    {
        ObjectPool<TestObject> pool(5);
        pool.alloc();
        pool.alloc();
        EXPECT_EQ(TestObject::destructor_calls, 0);
    }
    EXPECT_EQ(TestObject::destructor_calls, 2);
}

TEST_F(ObjectPoolTest, Complex) {
    ObjectPool<TestObject> pool(3);
    auto& obj1 = pool.alloc();
    auto& obj2 = pool.alloc();
    auto& obj3 = pool.alloc(1, 2);

    EXPECT_EQ(pool.get_size(), 3);
    EXPECT_EQ(TestObject::constructor_calls, 3);
    EXPECT_EQ(TestObject::param_constructor_calls, 1);

    pool.free(obj2);
    EXPECT_EQ(pool.get_size(), 2);
    EXPECT_EQ(TestObject::destructor_calls, 1);

    auto& obj4 = pool.alloc(3, 4);
    EXPECT_EQ(TestObject::constructor_calls, 4);
    EXPECT_EQ(pool.get_size(), 3);
    EXPECT_EQ(TestObject::param_constructor_calls, 2);

    pool.free(obj1);
    pool.free(obj3);
    pool.free(obj4);

    EXPECT_EQ(pool.get_size(), 0);
    EXPECT_EQ(TestObject::destructor_calls, 4);
}

TEST_F(ObjectPoolTest, FreeingObjectFromAnotherPool) {
    ObjectPool<TestObject> pool1(1);
    ObjectPool<TestObject> pool2(1);

    auto& obj_from_pool1 = pool1.alloc();

    EXPECT_THROW(pool2.free(obj_from_pool1), std::runtime_error);
    EXPECT_NO_THROW(pool1.free(obj_from_pool1));
}

TEST_F(ObjectPoolTest, FreeingHeapAllocatedObject) {
    ObjectPool<TestObject> pool(1);
    TestObject* heap_obj = new TestObject();

    EXPECT_THROW(pool.free(*heap_obj), std::runtime_error);

    delete heap_obj;
}

TEST_F(ObjectPoolTest, FreeingPointerToMiddleOfObject) {
    ObjectPool<TestObject> pool(1);
    auto& obj = pool.alloc();

    char* ptr_to_middle = reinterpret_cast<char*>(&obj) + sizeof(int);
    TestObject* invalid_ptr = reinterpret_cast<TestObject*>(ptr_to_middle);

    EXPECT_THROW(pool.free(*invalid_ptr), std::runtime_error);
}

TEST_F(ObjectPoolTest, FreeingInvalidPointerInsidePoolRange) {
    ObjectPool<char> pool(100);
    char& obj = pool.alloc();
    char* invalid_ptr = &obj + 5;

    EXPECT_THROW(pool.free(*invalid_ptr), std::runtime_error);
}

TEST_F(ObjectPoolTest, FreeingItself) {
    ObjectPool<TestObject> pool(1);
    auto& obj = pool.alloc();

    EXPECT_NO_THROW(obj.free_from_pool(pool));
    EXPECT_EQ(pool.get_size(), 0);
    EXPECT_EQ(TestObject::destructor_calls, 1);

    EXPECT_THROW(pool.free(obj), std::runtime_error);
}

struct alignas(64) HighlyAlignedObject {
    char data[64];
};

TEST(ObjectPoolAlignmentTest, HandlesHighlyAlignedObjects) {
    ObjectPool<HighlyAlignedObject> pool(2);
    auto& obj1 = pool.alloc();
    auto& obj2 = pool.alloc();

    EXPECT_EQ(reinterpret_cast<uintptr_t>(&obj1) % 64, 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&obj2) % 64, 0);
}