#pragma once
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <initializer_list>

/**
 * @brief Custom Dynamic Array (Vector) Data Structure for PBL2
 * Thay the cho std::vector, tu quan ly bo nho dong, amortized O(1) push_back
 */
template <typename T>
class MyVector {
private:
    T* data;
    size_t sz;
    size_t cap;

    void reallocate(size_t newCap) {
        if (newCap < sz) newCap = sz;
        if (newCap == 0) newCap = 1;

        T* newData = new T[newCap];
        for (size_t i = 0; i < sz; ++i) {
            newData[i] = std::move(data[i]);
        }
        delete[] data;
        data = newData;
        cap = newCap;
    }

public:
    // Iterator definition for range-based for loops
    class Iterator {
    private:
        T* ptr;
    public:
        Iterator(T* p) : ptr(p) {}
        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }
        Iterator& operator++() { ++ptr; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; ++ptr; return tmp; }
        Iterator& operator--() { --ptr; return *this; }
        Iterator operator--(int) { Iterator tmp = *this; --ptr; return tmp; }
        bool operator==(const Iterator& other) const { return ptr == other.ptr; }
        bool operator!=(const Iterator& other) const { return ptr != other.ptr; }
    };

    class ConstIterator {
    private:
        const T* ptr;
    public:
        ConstIterator(const T* p) : ptr(p) {}
        const T& operator*() const { return *ptr; }
        const T* operator->() const { return ptr; }
        ConstIterator& operator++() { ++ptr; return *this; }
        ConstIterator operator++(int) { ConstIterator tmp = *this; ++ptr; return tmp; }
        ConstIterator& operator--() { --ptr; return *this; }
        ConstIterator operator--(int) { ConstIterator tmp = *this; --ptr; return tmp; }
        bool operator==(const ConstIterator& other) const { return ptr == other.ptr; }
        bool operator!=(const ConstIterator& other) const { return ptr != other.ptr; }
    };

    MyVector() : data(nullptr), sz(0), cap(0) {}

    explicit MyVector(size_t initialCap) : data(nullptr), sz(0), cap(0) {
        reserve(initialCap);
    }

    MyVector(size_t count, const T& value) : data(nullptr), sz(0), cap(0) {
        reserve(count);
        for (size_t i = 0; i < count; ++i) {
            push_back(value);
        }
    }

    MyVector(std::initializer_list<T> init) : data(nullptr), sz(0), cap(0) {
        reserve(init.size());
        for (const auto& item : init) {
            push_back(item);
        }
    }

    // Copy Constructor
    MyVector(const MyVector& other) : data(nullptr), sz(other.sz), cap(other.cap) {
        if (cap > 0) {
            data = new T[cap];
            for (size_t i = 0; i < sz; ++i) {
                data[i] = other.data[i];
            }
        }
    }

    // Move Constructor
    MyVector(MyVector&& other) noexcept : data(other.data), sz(other.sz), cap(other.cap) {
        other.data = nullptr;
        other.sz = 0;
        other.cap = 0;
    }

    // Copy Assignment
    MyVector& operator=(const MyVector& other) {
        if (this != &other) {
            delete[] data;
            sz = other.sz;
            cap = other.cap;
            data = (cap > 0) ? new T[cap] : nullptr;
            for (size_t i = 0; i < sz; ++i) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    // Move Assignment
    MyVector& operator=(MyVector&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            sz = other.sz;
            cap = other.cap;
            other.data = nullptr;
            other.sz = 0;
            other.cap = 0;
        }
        return *this;
    }

    ~MyVector() {
        delete[] data;
    }

    void reserve(size_t newCap) {
        if (newCap > cap) {
            reallocate(newCap);
        }
    }

    void push_back(const T& value) {
        if (sz >= cap) {
            reserve(cap == 0 ? 4 : cap * 2);
        }
        data[sz++] = value;
    }

    void push_back(T&& value) {
        if (sz >= cap) {
            reserve(cap == 0 ? 4 : cap * 2);
        }
        data[sz++] = std::move(value);
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (sz >= cap) {
            reserve(cap == 0 ? 4 : cap * 2);
        }
        data[sz++] = T(std::forward<Args>(args)...);
    }

    void pop_back() {
        if (sz > 0) {
            --sz;
        }
    }

    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }

    T& at(size_t index) {
        if (index >= sz) throw std::out_of_range("MyVector::at: index out of range");
        return data[index];
    }

    const T& at(size_t index) const {
        if (index >= sz) throw std::out_of_range("MyVector::at: index out of range");
        return data[index];
    }

    T& front() { return data[0]; }
    const T& front() const { return data[0]; }

    T& back() { return data[sz - 1]; }
    const T& back() const { return data[sz - 1]; }

    size_t size() const { return sz; }
    size_t capacity() const { return cap; }
    bool empty() const { return sz == 0; }

    void clear() {
        sz = 0;
    }

    void remove_at(size_t index) {
        if (index >= sz) return;
        for (size_t i = index; i + 1 < sz; ++i) {
            data[i] = std::move(data[i + 1]);
        }
        --sz;
    }

    Iterator begin() { return Iterator(data); }
    Iterator end() { return Iterator(data + sz); }
    ConstIterator begin() const { return ConstIterator(data); }
    ConstIterator end() const { return ConstIterator(data + sz); }
};