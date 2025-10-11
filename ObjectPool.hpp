#ifndef OBJECTPOOL_HPP
#define OBJECTPOOL_HPP

#include <functional>
#include <memory>
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
    explicit ObjectPool(size_t num) {
        if (num == 0) {
            throw std::invalid_argument("ObjectPool size cannot be zero");
        }
        memory = static_cast<T*>(::operator new(sizeof(T) * num));
        used.resize(num, false);
    }

    ~ObjectPool() {
        for (size_t i = 0; i < used.size(); ++i) {
            if (used[i]) { reinterpret_cast<T*>(&memory[i])->~T(); }
        }
        ::operator delete[](memory);
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;
    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    template<typename... Args>
    decltype(auto) alloc(Args&&... args) {
        int free_slot = -1;
        for (size_t i = 0; i < used.size(); i++) {
            if (!used[i]) {
                used[i] = true;
                free_slot = static_cast<int>(i);
                break;
            }
        }
        if (free_slot == -1) {
            throw std::bad_alloc();
        }

        T* ptr = new(&memory[free_slot]) T(std::forward<Args>(args)...);

        using Deleter = std::function<void(T*)>;
        return std::unique_ptr<T, Deleter>(ptr, [this](T* p) { this->free(p); });
    }

    void free(T* ptr) {
        if (!ptr) return;

        auto index = static_cast<size_t>(reinterpret_cast<T*>(ptr) - memory);

        if (index < used.size() && used[index]) {
            ptr->~T();
            used[index] = false;
        } else {
            throw std::runtime_error("ObjectPool out of memory");
        }
    }

private:
    T* memory = nullptr;
    std::vector<bool> used{};
};

#endif //OBJECTPOOL_HPP
