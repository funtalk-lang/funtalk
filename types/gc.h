#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <fstream>
#include <memory>
#include <array>
#include <bit>
#include <bitset>
#include <typeinfo>
#include <iostream>
#include <atomic>

class Dynamic;
class Scope;
class Object;
class Tuple;
class TupleView;
class Fun;
class FunView;
class Dict;
class DictView;
struct Iterate;
class Matrix;
class MatrixView;
class Bytes;
class BytesView;
struct Promise;
struct DeferredPromise;
struct CallPool;

namespace funtalk {
    uint16_t hash(uint16_t);
    uint16_t hash(uint32_t);
    uint16_t hash(uint64_t);
    uint16_t hash(Dynamic);
    uint16_t hash(TupleView);
    uint16_t hash(FunView);
    uint16_t hash(DictView);
    uint16_t hash(std::complex<double>);
    uint16_t hash(MatrixView);
    uint16_t hash(BytesView);
}

#include "hash_map.h"

struct Bits {
    std::bitset<65536> arr;
    bool get(size_t i) const {
        return arr.test(i);
    }
    void set0(size_t i) {
        arr.reset(i);
    }
    void set1(size_t i) {
        arr.set(i);
    }
    void clear() {
        arr.reset();
    }
};

class GCPanic : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

constexpr int32_t GC_MAX = 65536;
constexpr int32_t GC_LOCAL_MAX = 65536 / 8;

template <class T, bool USE_MUTEX=true>
class GC {
    struct alignas(T) Data {
        uint8_t storage[sizeof(T)];
    };
    struct DummyMutex {
        constexpr void lock() noexcept {}
        constexpr void unlock() noexcept {}
        constexpr bool try_lock() noexcept { return true; }
    };
    using MutexType = std::conditional_t<USE_MUTEX, std::mutex, DummyMutex>;
    Data arr[65536]{};
    Bits bits;
    size_t chunk = 0;
    int32_t* const count;
    uint32_t size = 0;
    MutexType mutex;
    uint16_t alloc() {
        for (uint32_t j = 0; j < 256; j++) {
            for (uint32_t i = (chunk + j) % 256 * 256; i < ((chunk + j) % 256 + 1) * 256; i++) {
                if (!bits.get(i)) {
                    chunk = (chunk + 1) % 256;
                    bits.set1(i);
                    return i;
                }
            }
        }
        throw GCPanic("No free memory in GC");
    }
    void privateClear() {
        chunk = 0;
        size = 0;
        for (uint32_t i = 0; i < 65536; i++) {
            if (bits.get(i)) {
                bits.set0(i);
                (*count)--;
                reinterpret_cast<T*>(arr[i].storage)->~T();
            }
        }
    }
public:
    GC(int32_t& count) : count(&count) {}
    void clear() {
        std::lock_guard<MutexType> lock(mutex);
        privateClear();
    }
    GC& operator=(const GC& gc) {
        std::lock_guard<MutexType> lock(mutex);
        privateClear();
        bits = gc.bits;
        chunk = gc.chunk;
        for (uint32_t i = 0; i < 65536; i++) {
            if (bits.get(i)) {
                size++;
                if (size > GC_LOCAL_MAX) {
                    throw GCPanic("No free memory in GC");
                }
                (*count)++;
                if ((*count) > GC_MAX) {
                    throw GCPanic("No free memory in GC");
                }
                new (arr[i].storage) T(*reinterpret_cast<const T*>(gc.arr[i].storage));
            }
        }
        return *this;
    }
    ~GC() {
        std::lock_guard<MutexType> lock(mutex);
        privateClear();
    }
    template <class... Args>
    uint16_t alloc(Args&&... args) {
        std::lock_guard<MutexType> lock(mutex);
        uint16_t i = alloc();
        try {
            new (arr[i].storage) T(std::forward<Args>(args)...);
        } catch (...) {
            bits.set0(i);
            throw;
        }
        size++;
        if (size > GC_LOCAL_MAX) {
            throw GCPanic("No free memory in GC");
        }
        (*count)++;
        if ((*count) > GC_MAX) {
            throw GCPanic("No free memory in GC");
        }
        return i;
    }
    void free(uint16_t i) {
        std::lock_guard<MutexType> lock(mutex);
        if (!bits.get(i)) {
            throw GCPanic("Double free in GC" + std::string(typeid(T).name()) + ">, i = " + std::to_string(i));
        }
        size--;
        (*count)--;
        bits.set0(i);
        reinterpret_cast<const T*>(arr[i].storage)->~T();
    }
    T& operator[](uint16_t i) {
        std::lock_guard<MutexType> lock(mutex);
        if (!bits.get(i)) {
            throw GCPanic("Not allocated in GC<" + std::string(typeid(T).name()) + ">, i = " + std::to_string(i));
        }
        return *reinterpret_cast<T*>(arr[i].storage);
    }
    const T& operator[](uint16_t i) const {
        std::lock_guard<MutexType> lock(mutex);
        if (!bits.get(i)) {
            throw GCPanic("Not allocated in GC<" + std::string(typeid(T).name()) + ">, i = " + std::to_string(i));
        }
        return *reinterpret_cast<const T*>(arr[i].storage);
    }
};

template <class T, class T_view>
class Pool {
protected:
    struct Info {
        uint32_t i;
        uint32_t count;
    };
    GC<T, false> gc;
    HashMap<T_view, Info> map;
    std::mutex mutex;
public:
    Pool(int32_t& count) : gc(count) {}
    Pool& operator=(const Pool& pool) {
        std::lock_guard<std::mutex> lock(mutex);
        gc = pool.gc;
        map = pool.map;
        return *this;
    }
    uint16_t alloc(T_view t) {
        std::lock_guard<std::mutex> lock(mutex);
        if (auto it = map.find(t); it != map.end()) {
            it->second.count++;
            return it->second.i;
        }
        uint16_t i = gc.alloc(t);
        auto& info = map[gc[i]];
        info.i = i;
        info.count = 1;
        return info.i;
    }
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        map.clear();
        gc.clear();
    }
    void free(uint16_t i) {
        std::lock_guard<std::mutex> lock(mutex);
        T_view t = gc[i];
        auto& info = map[t];
        info.count--;
        if (info.count == 0) {
            map.erase(t);
            gc.free(i);
        }
    }
    T_view at(uint16_t i) {
        return gc[i];
    }
};

template <class T, class T_view>
struct SmartPool : protected Pool<T, T_view> {
    SmartPool(int32_t& count) : Pool<T, T_view>(count) {}
    uint32_t alloc(T_view t) {
        return Pool<T, T_view>::alloc(t);
    }
    uint32_t edit(uint32_t i) {
        if ((i >> 16) == 1) {
            return i;
        }
        uint16_t j = this->gc.alloc(Pool<T, T_view>::at(i));
        Pool<T, T_view>::free(i);
        return uint32_t(j) | (1 << 16);
    }
    void clear() {
        Pool<T, T_view>::clear();
    }
    void free(uint32_t i) {
        if ((i >> 16) == 1) {
            this->gc.free(i);
        } else {
            Pool<T, T_view>::free(i);
        }
    }
    T_view at(uint32_t i) {
        return this->gc[i];
    }
    T& operator[](uint32_t i) {
        if ((i >> 16) == 1) {
            return this->gc[i];
        }
        throw GCPanic("Cannot modify immutable object");
    }
};
