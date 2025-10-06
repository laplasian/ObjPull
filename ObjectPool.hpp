#ifndef OBJECTPOOL_HPP
#define OBJECTPOOL_HPP
#include <iostream>
#include <ostream>
#include <vector>

namespace detale {

}

template<class T>
class ObjectPool {
public:
    explicit ObjectPool(size_t num) {
        memory = static_cast<int *>(malloc(num * sizeof(T)));
        used.resize(num, false);
        if (memory == nullptr) {
            throw std::bad_alloc();
        }
    };
    ~ObjectPool() {
        delete[] memory;
    }

    template<typename... Args>
    T& alloc(Args&&... args) { // валидаторы
        int vacation = -1;
        for (int i = 0; i < used.size(); i++) {
            if (used[i] == 0) {
                used[i] = 1;
                vacation = i;
                break;
            }
        }
        if (vacation == -1) {
            vacation = expand();
        }

        T * ptr = new(&memory[vacation]) T(std::forward<Args>(args)...); // валидаторы
        return *ptr;
    }

private:
    T * memory = nullptr;
    std::vector<bool> used{};

    int expand() {
        int size = used.size();

        int* mem = static_cast<int *>(realloc(memory, size * sizeof(T) * 2));
        if (mem == nullptr) {
            throw std::bad_alloc();
        }
        memory = mem;

        return size;
    }
};

#endif //OBJECTPOOL_HPP
