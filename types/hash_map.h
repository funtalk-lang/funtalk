#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cstdint>

template <class Key, class Value>
class HashMap {
    struct Iterator {
        Key first;
        Value second;
    };
    std::vector<Iterator> arr[65536]{};
public:
    Value& operator[](Key key) {
        uint16_t i = funtalk::hash(key);
        for (Iterator& it : arr[i]) {
            if (it.first == key) {
                return it.second;
            }
        }
        arr[i].push_back({key, Value()});
        return arr[i][arr[i].size() - 1].second;
    }
    Iterator* end() {
        return nullptr;
    }
    Iterator* find(Key key) {
        uint16_t i = funtalk::hash(key);
        for (Iterator& it : arr[i]) {
            if (it.first == key) {
                return &it;
            }
        }
        return end();
    }
    void clear() {
        for (std::vector<Iterator>& v : arr) {
            v.clear();
        }
    }
    void erase(Key key) {
        std::erase_if(arr[funtalk::hash(key)], [key](const Iterator& it) { return it.first == key; });
    }
};

template <class Key, class Value>
struct SmallHashMap {
    struct Item {
        Key first;
        Value second;
        bool operator==(const Item&) const = default;
    };
    template <class T>
    struct Array {
        std::unique_ptr<T[]> arr;
        uint32_t size;
        Array() : arr(nullptr), size(0) {}
        Array(const Array& a) {
            size = a.size;
            if (size == 0) {
                arr = nullptr;
                return;
            }
            arr = std::make_unique<T[]>(size);
            std::copy(a.arr.get(), a.arr.get() + size, arr.get());
        }
        void push_back(const T& it) {
            std::unique_ptr<T[]> arr1 = std::make_unique<T[]>(size + 1);
            std::copy(arr.get(), arr.get() + size, arr1.get());
            arr1[size] = it;
            arr = std::move(arr1);
            size++;
        }
        T& operator[](size_t i) { return arr[i]; }
        const T& operator[](size_t i) const { return arr[i]; }
        T* begin() { return arr.get(); }
        T* end() { return arr.get() + size; }
        const T* begin() const { return arr.get(); }
        const T* end() const { return arr.get() + size; }
        const T* cbegin() const { return begin(); }
        const T* cend() const { return end(); }
        bool operator==(const Array& a) const {
            if (size != a.size) return false;
            for (uint32_t i = 0; i < size; i++) {
                if (arr[i] != a[i]) return false;
            }
            return true;
        }
    };
    template <class T, class ArrayType>
    struct Iterator {
        T* it;
        ArrayType* a;
        ArrayType* a_end;
        void settle() {
            while (it == a->end() && a != a_end) {
                a++;
                if (a != a_end) {
                    it = a->begin();
                } else {
                    it = nullptr;
                }
            }
        }
        Iterator& operator++() {
            if (a != a_end && it != a->end()) {
                it++;
            }
            settle();
            return *this;
        }
        Iterator operator++(int) {
            auto i = *this;
            ++(*this);
            return i;
        }
        T* operator->() const { return it; }
        T& operator*() const { return *it; }
        template <class OtherT, class OtherArrayType>
        bool operator==(const Iterator<OtherT, OtherArrayType>& other) const {
            return it == other.it && a == other.a;
        }
    };
    Array<Item> arr[16]{};
    uint32_t size_ = 0;
public:
    using iterator = Iterator<Item, Array<Item>>;
    using const_iterator = Iterator<const Item, const Array<Item>>;
    Value& operator[](Key key) {
        uint16_t i = funtalk::hash(key) & 15;
        for (Item& it : arr[i]) {
            if (it.first == key) return it.second;
        }
        arr[i].push_back({key, Value()});
        size_++;
        return arr[i][arr[i].size - 1].second;
    }
    iterator begin() {
        auto it = iterator{arr[0].begin(), arr, arr + 16};
        it.settle();
        return it;
    }
    iterator end() { return iterator{nullptr, arr + 16, arr + 16}; }
    const_iterator begin() const {
        auto it = const_iterator{arr[0].begin(), arr, arr + 16};
        it.settle();
        return it;
    }
    const_iterator end() const { return const_iterator{nullptr, arr + 16, arr + 16}; }
    const_iterator cbegin() const { return begin(); }
    const_iterator cend() const { return end(); }
    iterator find(Key key) {
        uint16_t i = funtalk::hash(key) & 15;
        for (Item& it : arr[i]) {
            if (it.first == key) return iterator{&it, arr + i, arr + 16};
        }
        return end();
    }
    const_iterator find(Key key) const {
        uint16_t i = funtalk::hash(key) & 15;
        for (const Item& it : arr[i]) {
            if (it.first == key) return const_iterator{&it, arr + i, arr + 16};
        }
        return cend();
    }
    uint32_t size() const { return size_; }
    bool operator==(const SmallHashMap&) const = default;
};
