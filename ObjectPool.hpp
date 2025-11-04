#ifndef OBJECTPOOL_HPP
#define OBJECTPOOL_HPP

#include <vector>
#include <stdexcept>
#include <cstdint>

template<class T>
class ObjectPool {
public:
    explicit ObjectPool(size_t num) : data(sizeof(T) * num + alignof(T) - 1), used(num, false){
        if (num == 0) {
            throw std::invalid_argument("ObjectPool size cannot be zero");
        }
        auto base = reinterpret_cast<uintptr_t>(data.data());
        uintptr_t aligned = (base + alignof(T) - 1) & ~(alignof(T) - 1);
        memory = reinterpret_cast<T*>(aligned);
    }

    ~ObjectPool() {
        for (size_t i = 0; i < used.size(); ++i) {
            if (used[i]) { (memory + i)->~T(); }
        }
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    template<typename... Args>
    [[nodiscard]] T& alloc(Args&&... args) {
        int free_slot = -1;
        for (size_t i = 0; i < used.size(); i++) {
            if (!used[i]) {
                free_slot = static_cast<int>(i);
                break;
            }
        }
        if (free_slot == -1) {
            throw std::bad_alloc();
        }

        T* ptr = new(memory + free_slot) T{std::forward<Args>(args)...};
        used[free_slot] = true;
        size++;

        return *ptr;
    }

    void free(T& obj) {
        T* ptr = &obj;

        if (ptr < memory || ptr >= (memory + used.size())) {
            throw std::runtime_error("Object is not in pool");
        }

        auto byte_diff = reinterpret_cast<char*>(ptr) - reinterpret_cast<char*>(memory);
        if (static_cast<size_t>(byte_diff) % sizeof(T) != 0) {
            throw std::runtime_error("Invalid pointer");
        }

        auto index = static_cast<size_t>(byte_diff / sizeof(T));

        if (used[index]) {
            ptr->~T();
            used[index] = false;
            size--;
        } else {
            throw std::runtime_error("Object is already freed");
        }
    }

    [[nodiscard]] size_t get_capacity() const { return used.size(); }
    [[nodiscard]] size_t get_size() const { return size; }

private:
    std::vector<char> data{};
    T* memory{};
    std::vector<bool> used{};
    size_t size{};
};

#endif //OBJECTPOOL_HPP
