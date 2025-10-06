#ifndef OBJECTPOOL_HPP
#define OBJECTPOOL_HPP
#include <iostream>
#include <ostream>
#include <vector>

namespace detale {
    template <typename T>
    concept Object = requires(T t)
    {
        new T();
    };
}

template<detale::Object T>
class ObjectPool {
public:
    explicit ObjectPool(size_t num) {
        memory = static_cast<T *>(malloc(num * sizeof(T)));
        used.resize(num, false);
        if (memory == nullptr) {
            throw std::bad_alloc();
        }
    };
    ~ObjectPool() {
        delete[] memory;
    }

    template<typename... Args> // валидаторы
    T& alloc(Args&&... args) {
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

        T * ptr = new(&memory[vacation]) T(std::forward<Args>(args)...);
        return *ptr;
    }

    void free(T * ptr) {
    };
    void free(size_t i) {
    }

private:
    T * memory = nullptr;
    std::vector<bool> used{};

    int expand() {
        int size = used.size();

        T* mem = static_cast<T *>(realloc(memory, size * sizeof(T) * 2));
        if (mem == nullptr) {
            throw std::bad_alloc();
        }
        memory = mem;

        return size;
    }
};

#endif //OBJECTPOOL_HPP
