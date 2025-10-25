#ifndef OBJECTPOOL_HPP
#define OBJECTPOOL_HPP

#include <vector>
#include <stdexcept>

namespace detail {
    template <typename T>
    concept Object = requires(T t)
    {
        new T();
    };
}

template<detail::Object T>
class ObjectPool {
public:
    explicit ObjectPool(size_t num): data(sizeof(T) * num), used(num, false) {
        if (num == 0) {
            throw std::invalid_argument("ObjectPool size cannot be zero");
        }
        memory = reinterpret_cast<T*>(data.data());
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
    T& alloc(Args&&... args) {
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
        auto byte_diff = reinterpret_cast<char*>(ptr) - reinterpret_cast<char*>(memory);

        if (byte_diff < 0 || static_cast<size_t>(byte_diff) >= data.size() * sizeof(T)) {
            throw std::runtime_error("Object is not in pool");
        }
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

    [[nodiscard]] size_t get_capacity() const { return used.size(); };
    [[nodiscard]] size_t get_size() const { return size; };

private:
    std::vector<char> data{};
    T* memory{};
    std::vector<bool> used{};
    size_t size {};
};

#endif //OBJECTPOOL_HPP
