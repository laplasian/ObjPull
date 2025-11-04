cmake_minimum_required(VERSION 3.30)
project(ObjPull)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wcast-qual -Wshadow
            -D_GLIBCXX_ASSERTIONS
            -D_GLIBCXX_DEBUG
            -D_GLIBCXX_DEBUG_PEDANTIC
            -D_GLIBCXX_SANITIZE_VECTOR
)

enable_testing()

add_executable(ObjPull
        ObjectPool.hpp
        tests.cpp)

target_link_libraries(ObjPull GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(ObjPull)
