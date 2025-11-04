#include <gtest/gtest.h>
#include "ObjectPool.hpp"
#include <vector>
#include <numeric>
#include <algorithm>

typedef struct Point {
    int x;
    int y;
} Point;

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
    EXPECT_THROW(ObjectPool<Point> pool(0), std::invalid_argument);
}

TEST(ObjectPoolInitTest, CorrectCapacityAndSize) {
    ObjectPool<Point> pool(10);
    EXPECT_EQ(pool.get_capacity(), 10);
    EXPECT_EQ(pool.get_size(), 0);
}

TEST_F(ObjectPoolTest, Alloc) {
    ObjectPool<TestObject> pool(2);
    EXPECT_EQ(pool.get_size(), 0);

    (void)pool.alloc();
    EXPECT_EQ(pool.get_size(), 1);
    EXPECT_EQ(TestObject::constructor_calls, 1);

    (void)pool.alloc();
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
    (void)pool.alloc();
    EXPECT_THROW((void)pool.alloc(), std::bad_alloc);
}

TEST_F(ObjectPoolTest, AllocPoint) {
    ObjectPool<Point> pool(1);
    auto& obj = pool.alloc(10, 20);

    EXPECT_EQ(obj.x, 10);
    EXPECT_EQ(obj.y, 20);
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
    (void)pool.alloc();
    TestObject not_in_pool{};
    EXPECT_THROW(pool.free(not_in_pool), std::runtime_error);
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
    auto* heap_obj = new TestObject();

    EXPECT_THROW(pool.free(*heap_obj), std::runtime_error);

    delete heap_obj;
}

TEST_F(ObjectPoolTest, FreeingPointerBeforeMemory) {
    ObjectPool<TestObject> pool(1);
    auto& obj = pool.alloc();

    char* ptr_before_mem = reinterpret_cast<char*>(&obj) - 1;
    auto* invalid_ptr = reinterpret_cast<TestObject*>(ptr_before_mem);

    EXPECT_THROW(pool.free(*invalid_ptr), std::runtime_error);
}

TEST_F(ObjectPoolTest, FreeingPointerAfterMemory) {
    ObjectPool<char> pool(1);
    auto& obj = pool.alloc();
    char* invalid_ptr = &obj + 1;

    EXPECT_THROW(pool.free(*invalid_ptr), std::runtime_error);
}

TEST_F(ObjectPoolTest, FreeingInvalidPointerInsideMemory) {
    ObjectPool<int> pool(10);
    auto& obj1 = pool.alloc();
    int* invalid_ptr = reinterpret_cast<int*>(reinterpret_cast<char*>(&obj1) + 1);

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


TEST_F(ObjectPoolTest, TDtorCallByPoolDtor) {
    {
        ObjectPool<TestObject> pool(5);
         [[maybe_unused]] auto& a = pool.alloc();
        (void)pool.alloc();
        EXPECT_EQ(TestObject::destructor_calls, 0);
    }
    EXPECT_EQ(TestObject::destructor_calls, 2);
}

TEST_F(ObjectPoolTest, StupidFreeOrder) {
    ObjectPool<TestObject> pool(4);
    auto& obj1 = pool.alloc();
    auto& obj2 = pool.alloc();
    auto& obj3 = pool.alloc();
    auto& obj4 = pool.alloc();

    pool.free(obj2);
    pool.free(obj1);
    pool.free(obj4);
    pool.free(obj3);

    EXPECT_EQ(pool.get_size(), 0);
    EXPECT_EQ(TestObject::destructor_calls, 4);
}

TEST_F(ObjectPoolTest, StressTest) {
    const size_t pool_size = 1000;
    ObjectPool<TestObject> pool(pool_size);
    std::vector<TestObject*> objects;

    for (size_t i = 0; i < pool_size; ++i) {
        objects.push_back(&pool.alloc(i, i * 2));
    }
    EXPECT_EQ(pool.get_size(), pool_size);
    EXPECT_EQ(TestObject::constructor_calls, pool_size);
    EXPECT_EQ(TestObject::param_constructor_calls, pool_size);

    for (size_t i = 0; i < objects.size(); i += 2) {
        pool.free(*objects[i]);
        objects[i] = nullptr;
    }
    EXPECT_EQ(pool.get_size(), pool_size / 2);
    EXPECT_EQ(TestObject::destructor_calls, pool_size / 2);

    for (size_t i = 0; i < objects.size(); i += 2) {
        objects[i] = &pool.alloc(i, i * 2);
    }
    EXPECT_EQ(pool.get_size(), pool_size);
    EXPECT_EQ(TestObject::constructor_calls, pool_size + pool_size / 2);

    for (size_t i = objects.size(); i-- > 0;) {
        if (i % 2 != 0) {
            pool.free(*objects[i]);
            objects[i] = nullptr;
        }
    }
    EXPECT_EQ(pool.get_size(), pool_size / 2);
    EXPECT_EQ(TestObject::destructor_calls, pool_size);

    for (auto* obj : objects) {
        if (obj != nullptr) {
            pool.free(*obj);
        }
    }
    EXPECT_EQ(pool.get_size(), 0);
    EXPECT_EQ(TestObject::destructor_calls, pool_size + pool_size / 2);
}


struct alignas(512) HighlyAlignedObject {
    int64_t i;
    char data[256 - sizeof(i)];
};

struct HighlyAlignedObject1 {
    int64_t i;
    char data[512 - sizeof(i)];
};

struct HighlyAlignedObject2 {
    char data[512];
};

TEST(ObjectPoolAlignmentTest, HandlesHighlyAlignedObjects) {
    ObjectPool<HighlyAlignedObject> pool(2);
    auto& obj1 = pool.alloc();
    auto& obj2 = pool.alloc();

    EXPECT_EQ(reinterpret_cast<uintptr_t>(&obj1) % alignof(HighlyAlignedObject), 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&obj2) % alignof(HighlyAlignedObject), 0);

    ObjectPool<HighlyAlignedObject1> pool1(2);
    auto& obj11 = pool1.alloc();
    auto& obj21 = pool1.alloc();

    EXPECT_EQ(reinterpret_cast<uintptr_t>(&obj11) % alignof(HighlyAlignedObject1), 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&obj21) % alignof(HighlyAlignedObject1), 0);

    ObjectPool<HighlyAlignedObject2> pool2(2);
    auto& obj12 = pool2.alloc();
    auto& obj22 = pool2.alloc();

    EXPECT_EQ(reinterpret_cast<uintptr_t>(&obj12) % alignof(HighlyAlignedObject2), 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&obj22) % alignof(HighlyAlignedObject2), 0);

    std::cout << sizeof(HighlyAlignedObject) << " " << alignof(HighlyAlignedObject) << std::endl;
    std::cout << sizeof(HighlyAlignedObject1) << " " << alignof(HighlyAlignedObject1) << std::endl;
    std::cout << sizeof(HighlyAlignedObject2) << " " << alignof(HighlyAlignedObject2) << std::endl;
}
