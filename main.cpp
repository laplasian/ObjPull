#include <iostream>
#include "ObjectPool.hpp"

int main() {

    ObjectPool<int> p(10);

    auto a = p.alloc(1);
    auto b = p.alloc(2);
    auto c = p.alloc(3);
    std::cout << a << b << c << std::endl;

    return 0;
}