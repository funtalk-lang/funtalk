#pragma once
#include "dynamic.h"

class Tuple {
    std::shared_ptr<Dynamic[]> arr;
    uint32_t size_ = 0;
public:
    Tuple() : size_(0), arr(nullptr) {}
    Tuple(uint32_t size_) : size_(size_) {
        if (size_ > MAX_TUPLE_SIZE) {
            throw CallPanic("Cannot create tuple larger than " + std::to_string(MAX_TUPLE_SIZE));
        }
        if (size_ == 0) {
            arr = nullptr;
            return;
        }
        arr = std::make_shared<Dynamic[]>(size_);
    }
    Tuple(CallScope& callScope);
    Tuple(const Tuple& t) : size_(t.size_) {
        if (size_ > MAX_TUPLE_SIZE) {
            throw CallPanic("Cannot create tuple larger than " + std::to_string(MAX_TUPLE_SIZE));
        }
        if (size_ == 0) {
            arr = nullptr;
            return;
        }
        arr = std::make_shared<Dynamic[]>(size_);
        memcpy(arr.get(), t.arr.get(), size_ * sizeof(Dynamic));
    }
    Tuple& operator=(const Tuple& t) {
        if (this == &t) {
            return *this;
        }
        size_ = t.size_;
        if (size_ == 0) {
            arr = nullptr;
            return *this;
        }
        arr = std::make_shared<Dynamic[]>(size_);
        memcpy(arr.get(), t.arr.get(), size_ * sizeof(Dynamic));
    }
    const Dynamic& operator[](size_t i) const {
        assert(i < size_);
        return arr[i];
    }
    Dynamic& operator[](size_t i) {
        assert(i < size_);
        return arr[i];
    }
    size_t size() const {
        return size_;
    }
    const Dynamic* begin() const {
        return arr.get();
    }
    const Dynamic* end() const {
        return arr.get() + size();
    }
    Dynamic* begin() {
        return arr.get();
    }
    Dynamic* end() {
        return arr.get() + size();
    }
    bool operator==(const Tuple& t) const {
        if (size() != t.size()) {
            return false;
        }
        for (uint32_t i = 0; i < size(); i++) {
            if (begin()[i] != t[i]) {
                return false;
            }
        }
        return true;
    }
    Tuple drop(int32_t n, CallPool& pool) const {
        uint32_t abs_n = n < 0 ? -n : n;
        assert(abs_n <= size());
        if (abs_n == size()) {
            return Tuple();
        }
        uint32_t new_size = size() - abs_n;
        Tuple res(new_size);
        if (n >= 0) {
            for (uint32_t i = 0; i < new_size; i++) {
                res[i] = copy(begin()[n + i], pool);
            }
        } else {
            for (uint32_t i = 0; i < new_size; i++) {
                res[i] = copy(begin()[i], pool);
            }
        }
        return res;
    }
    void push_back(Dynamic d) {
        if ((size_ + 1) > MAX_TUPLE_SIZE) {
            throw CallPanic("Cannot create tuple larger than " + std::to_string(MAX_TUPLE_SIZE));
        }
        size_++;
        if (size() == 1) {
            arr = std::make_shared<Dynamic[]>(size());
            arr[0] = d;
            return;
        }
        std::shared_ptr<Dynamic[]> arr1 = std::make_shared<Dynamic[]>(size());
        memcpy(arr1.get(), arr.get(), size() - 1);
        arr1.get()[size()] = d;
        arr = arr1;
    }
    void pop_back() {
        assert(size() != 0);
        size_--;
        if (size() == 0) {
            arr = nullptr;
            return;
        }
        std::shared_ptr<Dynamic[]> arr1 = std::make_shared<Dynamic[]>(size());
        memcpy(arr1.get(), arr.get(), size());
        arr = arr1;
    }
    bool send(Dynamic d, uint64_t id, uint64_t senderID, CallPool& pool, std::atomic<bool>& interrupt);
    Dynamic concat(DynamicType type, const Tuple& t, CallPool& pool) const;
    friend class TupleView;
};

class TupleView : public Tuple {
public:
    TupleView(const Tuple& t) : Tuple() {
        this->arr = t.arr;
        this->size_ = t.size_;
    }
    TupleView(const TupleView& t) : Tuple() {
        this->arr = t.arr;
        this->size_ = t.size_;
    }
    TupleView& operator=(const Tuple& t) {
        if (this == &t) {
            return *this;
        }
        this->arr = t.arr;
        this->size_ = t.size_;
        return *this;
    }
    TupleView& operator=(const TupleView& t) {
        if (this == &t) {
            return *this;
        }
        this->arr = t.arr;
        this->size_ = t.size_;
        return *this;
    }
};
