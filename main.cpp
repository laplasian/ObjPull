#include <iostream>
#include "ObjectPool.hpp"

class Point {
    int m_x, m_y;
public:
    Point()
        : m_x(0), m_y(0) {}
    Point(int x, int y)
        : m_x(x), m_y(y) {}
};


int main() {

    ObjectPool<int> p(10);

    auto a = p.alloc(1);
    auto b = p.alloc(2);
    auto c = p.alloc(3);
    std::cout << *a << *b << *c << std::endl;

    ObjectPool<Point> pp(10);

    auto p1 = pp.alloc();
    auto p2 = pp.alloc(0, 1);

    return 0;
}