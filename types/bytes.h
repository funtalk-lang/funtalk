#pragma once
#include <cstdint>
#include <string>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <iostream>

class Bytes {
    char* arr;
    uint32_t size_;
    bool owner = true;
public:
    Bytes() : arr(nullptr), size_(0) {}
    explicit Bytes(uint32_t size_) : size_(size_) {
        #ifndef FUNTALK_TEST_BYTES
        if (size_ > MAX_BYTES_SIZE) {
            throw CallPanic("Cannot create Bytes larger than " + std::to_string(MAX_BYTES_SIZE));
        }
        #endif
        if (size_ == 0) {
            arr = nullptr;
            return;
        }
        arr = new char[size_]{};
    }
    Bytes(const char* arr, uint32_t size_) : size_(size_) {
        #ifndef FUNTALK_TEST_BYTES
        if (size_ > MAX_BYTES_SIZE) {
            throw CallPanic("Cannot create Bytes larger than " + std::to_string(MAX_BYTES_SIZE));
        }
        #endif
        if (size_ == 0) {
            this->arr = nullptr;
            return;
        }
        this->arr = new char[size_]{};
        memcpy(this->arr, arr, size_);
    }
    Bytes(const char* arr) : Bytes(arr, std::strlen(arr)) {}
    Bytes(const std::string& s) : Bytes(s.c_str(), s.size()) {}
    Bytes(const std::u32string& s);
    Bytes(const Bytes& b) : size_(b.size_) {
        #ifndef FUNTALK_TEST_BYTES
        if (size_ > MAX_BYTES_SIZE) {
            throw CallPanic("Cannot create Bytes larger than " + std::to_string(MAX_BYTES_SIZE));
        }
        #endif
        if (size_ == 0) {
            arr = nullptr;
            return;
        }
        arr = new char[size_]{};
        memcpy(arr, b.arr, size_);
    }
    Bytes(Bytes&& b) : size_(b.size_) {
        if (b.owner || b.arr == nullptr) {
            arr = b.arr;
        } else {
            arr = new char[size_]{};
            memcpy(arr, b.arr, size_);
        }
        b.arr = nullptr;
        b.size_ = 0;
    }
    ~Bytes() {
        if (owner) {
            delete[] arr;
        }
    }
    Bytes& operator=(const Bytes& b) {
        if (this == &b) {
            return *this;
        }
        if (owner) {
            delete[] arr;
        }
        owner = true;
        size_ = b.size_;
        if (size_ == 0) {
            arr = nullptr;
            return *this;
        }
        arr = new char[size_]{};
        memcpy(arr, b.arr, size_);
        return *this;
    }
    Bytes& operator=(Bytes&& b) {
        if (this == &b) {
            return *this;
        }
        if (owner) {
            delete[] arr;
        }
        owner = true;
        size_ = b.size_;
        if (b.owner || b.arr == nullptr) {
            arr = b.arr;
        } else {
            arr = new char[size_]{};
            memcpy(arr, b.arr, size_);
        }
        b.arr = nullptr;
        b.size_ = 0;
        return *this;
    }
    uint32_t size() const {
        return size_;
    }
    char& operator[](uint32_t i) {
        assert(i < size());
        return arr[i];
    }
    char operator[](uint32_t i) const {
        assert(i < size());
        return arr[i];
    }
    char* begin() {
        return arr;
    }
    char* end() {
        return arr + size();
    }
    const char* begin() const {
        return arr;
    }
    const char* end() const {
        return arr + size();
    }
    bool operator==(const Bytes& b) const {
        if (size() != b.size()) {
            return false;
        }
        if (size() == 0) {
            return true;
        }
        return memcmp(arr, b.arr, size()) == 0;
    }
    auto operator<=>(const Bytes& b) const {
        size_t min_size = std::min(size(), b.size());
        if (min_size != 0) {
            int cmp = memcmp(arr, b.arr, min_size);
            if (cmp != 0) {
                return cmp <=> 0;
            }
        }
        return size() <=> b.size();
    }
    std::strong_ordering bigint_cmp(const Bytes& b) const {
        if (size() != b.size()) {
            return size() <=> b.size();
        }
        for (uint32_t i = size() - 1; i != -1; i--) {
            if (arr[i] != b[i]) {
                return uint8_t(arr[i]) <=> uint8_t(b[i]);
            }
        }
        return std::strong_ordering::equal;
    }
    Bytes operator~() const {
        Bytes r = *this;
        for (char& c : r) {
            c = ~c;
        }
        return r;
    }
    Bytes& operator^=(const Bytes& b) {
        uint32_t min = std::min(size(), b.size());
        for (uint32_t i = 0; i < min; i++) {
            arr[i] ^= b.arr[i];
        }
        return *this;
    }
    Bytes operator^(const Bytes& b) const {
        Bytes b1 = *this;
        b1 ^= b;
        return b1;
    }
    Bytes& operator&=(const Bytes& b) {
        uint32_t min = std::min(size(), b.size());
        for (uint32_t i = 0; i < min; i++) {
            arr[i] &= b.arr[i];
        }
        return *this;
    }
    Bytes operator&(const Bytes& b) const {
        Bytes b1 = *this;
        b1 &= b;
        return b1;
    }
    Bytes& operator|=(const Bytes& b) {
        uint32_t min = std::min(size(), b.size());
        for (uint32_t i = 0; i < min; i++) {
            arr[i] |= b.arr[i];
        }
        return *this;
    }
    Bytes operator|(const Bytes& b) const {
        Bytes b1 = *this;
        b1 |= b;
        return b1;
    }
    Bytes operator-() const {
        Bytes r = *this;
        for (char& c : r) {
            c = -c;
        }
        return r;
    }
    Bytes abs() const {
        Bytes r = *this;
        for (char& c : r) {
            c = std::abs(c);
        }
        return r;
    }
    Bytes sum(const Bytes& b) const {
        Bytes b1 = *this;
        for (uint32_t i = 0; i < std::min(size(), b.size()); i++) {
            b1[i] += b[i];
        }
        return b1;
    }
    Bytes& operator*=(const Bytes& b) {
        uint32_t min = std::min(size(), b.size());
        for (uint32_t i = 0; i < min; i++) {
            arr[i] *= b.arr[i];
        }
        return *this;
    }
    Bytes operator*(const Bytes& b) const {
        Bytes b1 = *this;
        b1 *= b;
        return b1;
    }
    Bytes& operator%=(const Bytes& b) {
        uint32_t min = std::min(size(), b.size());
        for (uint32_t i = 0; i < min; i++) {
            arr[i] %= b.arr[i];
        }
        return *this;
    }
    Bytes operator%(const Bytes& b) const {
        Bytes b1 = *this;
        b1 %= b;
        return b1;
    }
    Bytes operator+(const Bytes& b) const {
        if (b.size() == 0) {
            return *this;
        }
        if (size() == 0) {
            return b;
        }
        Bytes b1 = Bytes(size() + b.size());
        memcpy(b1.arr, arr, size());
        memcpy(b1.arr + size(), b.arr, b.size());
        return b1;
    }
    Bytes inv() const {
        static const uint8_t inv_box[256] = {0, 1, 129, 86, 193, 103, 43, 147, 225, 200, 180, 187, 150, 178, 202, 120, 241, 121, 100, 230, 90, 49, 222, 190, 75, 72, 89, 238, 101, 195, 60, 199, 249, 148, 189, 235, 50, 132, 115, 145, 45, 163, 153, 6, 111, 40, 95, 175, 166, 21, 36, 126, 173, 97, 119, 243, 179, 248, 226, 61, 30, 59, 228, 102, 253, 87, 74, 234, 223, 149, 246, 181, 25, 169, 66, 24, 186, 247, 201, 244, 151, 165, 210, 96, 205, 127, 3, 65, 184, 26, 20, 209, 176, 152, 216, 46, 83, 53, 139, 135, 18, 28, 63, 5, 215, 164, 177, 245, 188, 224, 250, 44, 218, 116, 124, 38, 113, 134, 159, 54, 15, 17, 158, 140, 114, 220, 51, 85, 255, 2, 172, 206, 37, 143, 117, 99, 240, 242, 203, 98, 123, 144, 219, 133, 141, 39, 213, 7, 33, 69, 12, 80, 93, 42, 252, 194, 229, 239, 122, 118, 204, 174, 211, 41, 105, 81, 48, 237, 231, 73, 192, 254, 130, 52, 161, 47, 92, 106, 13, 56, 10, 71, 233, 191, 88, 232, 76, 11, 108, 34, 23, 183, 170, 4, 155, 29, 198, 227, 196, 31, 9, 78, 14, 138, 160, 84, 131, 221, 236, 91, 82, 162, 217, 146, 251, 104, 94, 212, 112, 142, 125, 207, 22, 68, 109, 8, 58, 197, 62, 156, 19, 168, 185, 182, 67, 35, 208, 167, 27, 157, 136, 16, 137, 55, 79, 107, 70, 77, 57, 32, 110, 214, 154, 64, 171, 128};
        Bytes r = *this;
        for (char& c : r) {
            c = inv_box[c];
        }
        return r;
    }
    Bytes pow(const Bytes& b) const {
        auto mod_pow = [](uint8_t a, uint8_t b) -> uint8_t {
            if (a == 0) {
                return 0;
            }
            if (a == 1) {
                return 1;
            }
            uint16_t r = 1;
            for (uint8_t i = 0; i != b; i++) {
                r = r * a % 257;
            }
            return r;
        };
        Bytes b1 = *this;
        for (uint32_t i = 0; i < std::min(size(), b.size()); i++) {
            b1[i] = mod_pow(b1[i], b[i]);
        }
        return b1;
    }
    Bytes drop(int32_t n) const {
        uint32_t abs_n = n < 0 ? -n : n;
        assert(abs_n <= size());
        if (abs_n == size()) {
            return Bytes();
        }
        uint32_t new_size = size() - abs_n;
        Bytes res(new_size);
        if (n >= 0) {
            memcpy(res.arr, arr + n, new_size);
        } else {
            memcpy(res.arr, arr, new_size);
        }
        return res;
    }
    Bytes add1bit() const {
        if (size() == 0) {
            return Bytes();
        }
        uint8_t bit = 0;
        Bytes res;
        if (arr[size() - 1] & 128) {
            res = Bytes(size() + 1);
        } else {
            res = Bytes(size());
        }
        for (uint32_t i = 0; i < size(); i++) {
            res[i] = (uint8_t(arr[i]) << 1) | bit;
            bit = uint8_t(arr[i]) >> 7;
        }
        if (res.size() != size()) {
            res[size()] = bit;
        }
        return res;
    }
    Bytes bigint_shl(uint32_t i) const {
        if (size() == 0) {
            return Bytes();
        }
        Bytes r = *this;
        while ((i % 8) != 0) {
            r = r.add1bit();
            i--;
        }
        r = Bytes(i / 8) + r;
        return r;
    }
    Bytes drop1bit() const {
        if (size() == 0) {
            return Bytes();
        }
        Bytes res;
        if (arr[size() - 1] == 1) {
            uint8_t bit = 1;
            res = Bytes(size() - 1);
            for (uint32_t i = res.size() - 1; i != -1; i--) {
                res[i] = (uint8_t(arr[i]) >> 1) | (bit << 7);
                bit = arr[i] & 1;
            }
        } else {
            uint8_t bit = 0;
            res = Bytes(size());
            for (uint32_t i = size() - 1; i != -1; i--) {
                res[i] = (uint8_t(arr[i]) >> 1) | (bit << 7);
                bit = arr[i] & 1;
            }
        }
        return res;
    }
    Bytes trim() const {
        size_t nz = 0;
        while (nz < size() && arr[size() - nz - 1] == 0) nz++;
        if (nz == size()) {
            return Bytes();
        }
        Bytes trimmed(size() - nz);
        memcpy(trimmed.arr, arr, trimmed.size());
        return trimmed;
    }
    Bytes bigint_sum(const Bytes& b) const {
        if (size() == 0) {
            return b;
        }
        if (b.size() == 0) {
            return *this;
        }
        uint32_t max = std::max(size(), b.size());
        uint32_t min = std::min(size(), b.size());
        Bytes res = Bytes(max + 1);
        uint8_t borrow = 0;
        for (uint32_t i = 0; i < min; i++) {
            uint16_t a = uint16_t(uint8_t(arr[i])) + uint8_t(b[i]) + borrow;
            res[i] = a;
            borrow = a >> 8;
        }
        if (size() > b.size()) {
            for (uint32_t i = min; i < max; i++) {
                uint16_t a = uint16_t(uint8_t(arr[i])) + borrow;
                res[i] = a;
                borrow = a >> 8;
            }
        } else if (size() < b.size()) {
            for (uint32_t i = min; i < max; i++) {
                uint16_t a = uint16_t(uint8_t(b[i])) + borrow;
                res[i] = a;
                borrow = a >> 8;
            }
        }
        res[max] = borrow;
        return res.trim();
    }
    Bytes bigint_sub(const Bytes& b) const {
        if (b.size() == 0) {
            return *this;
        }
        uint32_t max = std::max(size(), b.size());
        uint32_t min = std::min(size(), b.size());
        Bytes res = Bytes(max + 1);
        uint8_t borrow = 0;
        for (uint32_t i = 0; i < min; i++) {
            uint16_t a = uint16_t(uint8_t(arr[i])) - uint8_t(b[i]) - borrow;
            res[i] = a;
            borrow = (a >> 8) & 1;
        }
        if (size() > b.size()) {
            for (uint32_t i = min; i < max; i++) {
                uint16_t a = uint16_t(uint8_t(arr[i])) - borrow;
                res[i] = a;
                borrow = (a >> 8) & 1;
            }
        } else if (size() < b.size()) {
            for (uint32_t i = min; i < max; i++) {
                uint16_t a = uint16_t(uint8_t(-b[i])) - borrow;
                res[i] = a;
                borrow = (a >> 8) & 1;
            }
        }
        res[max] = -borrow;
        return res.trim();
    }
    Bytes bigint_mod(const Bytes& b) const {
        assert(b.size() != 0);
        if (size() == 0) {
            return Bytes();
        }
        if (b.size() == 1 && b[0] == 1) {
            return Bytes();
        }
        if (bigint_cmp(b) == std::strong_ordering::less) {
            return *this;
        }
        Bytes r = Bytes();
        for (uint32_t i = size() * 8 - 1; i != -1; i--) {
            r = r.add1bit();
            if ((uint8_t(arr[i / 8]) >> (i % 8)) & 1) {
                if (r.size() == 0) {
                    r = Bytes(1);
                }
                r[0] |= 1;
            }
            if (b.bigint_cmp(r) != std::strong_ordering::greater) {
                r = r.bigint_sub(b);
            }
        }
        return r;
    }
    Bytes bigint_mul(const Bytes& b) const {
        if (size() == 0 || b.size() == 0) {
            return Bytes();
        }
        uint32_t max = std::max(size(), b.size());
        uint32_t min = std::min(size(), b.size());
        Bytes res = Bytes(max * 2);
        uint8_t borrow = 0;
        for (uint32_t j = 0; j < b.size(); j++) {
            for (uint32_t i = 0; i < size(); i++) {
                uint16_t a = uint16_t(uint8_t(arr[i])) * uint8_t(b[j]) + uint8_t(res[i + j]) + borrow;
                res[i + j] = a;
                borrow = a >> 8;
            }
            res[size() + j] += borrow;
            borrow = 0;
        }
        return res.trim();
    }
    Bytes bigint_div(const Bytes& b) const {
        assert(b.size() != 0);
        if (size() == 0) {
            return Bytes();
        }
        Bytes a = *this;
        Bytes r = Bytes(size());
        for (uint32_t i = size() * 8 - 1; i != -1; i--) {
            Bytes shl = b.bigint_shl(i);
            if (a.bigint_cmp(shl) != std::strong_ordering::less) {
                a = a.bigint_sub(shl);
                ((uint8_t*)r.arr)[i / 8] |= 1U << (i % 8);
            }
        }
        return r.trim();
    }
    Bytes bigint_pow(Bytes e, const Bytes& m) const {
        if (m.size() == 1 && m[0] == 1) {
            return Bytes();
        }
        Bytes r = Bytes(1);
        r[0] = 1;
        Bytes base = bigint_mod(m);
        while (e.size() != 0) {
            if (e[0] & 1) {
                r = r.bigint_mul(base).bigint_mod(m);
            }
            base = base.bigint_mul(base).bigint_mod(m);
            e = e.drop1bit();
        }
        return r;
    }
    uint64_t to_uint64() const {
        if (size() == 0) {
            return 0;
        }
        uint64_t i = 0;
        memcpy(&i, arr, std::min(size(), 8U));
        return i;
    }
    friend std::ostream& operator<<(std::ostream& os, const Bytes& b) {
        if (b.arr != nullptr && b.size() != 0) {
            os.write(b.arr, b.size());
        }
        return os;
    }
    operator std::string() const {
        return std::string(arr, size());
    }
    std::u32string to_u32() const {
        std::u32string result;
        size_t i = 0;
        while (i < size()) {
            uint32_t cp = 0;
            size_t bytes_to_read = 0;
            if ((arr[i] & 0x80) == 0x00) {
                cp = arr[i];
                bytes_to_read = 0;
            } else if ((arr[i] & 0xE0) == 0xC0) {
                cp = arr[i] & 0x1F;
                bytes_to_read = 1;
            } else if ((arr[i] & 0xF0) == 0xE0) {
                cp = arr[i] & 0x0F;
                bytes_to_read = 2;
            } else if ((arr[i] & 0xF8) == 0xF0) {
                cp = arr[i] & 0x07;
                bytes_to_read = 3;
            } else {
                cp = arr[i];
                bytes_to_read = 0;
            }
            i++;
            bool valid = true;
            for (size_t j = 0; j < bytes_to_read; ++j) {
                if (i >= size() || (arr[i] & 0xC0) != 0x80) {
                    valid = false;
                    break;
                }
                cp = (cp << 6) | (arr[i] & 0x3F);
                i++;
            }
            if (valid) {
                result.push_back(static_cast<char32_t>(cp));
            } else {
                result.push_back(U'\uFFFD');
            }
        }
        return result;
    }
    Bytes upper() const {
        std::u32string u32 = to_u32();
        std::string result;
        result.reserve(size());
        for (char32_t cp : u32) {
            uint32_t up = cp;
            if (cp >= 0x61 && cp <= 0x7A) {
                up = cp - 32;
            } else if (cp >= 0xE0 && cp <= 0xF6) {
                up = cp - 32;
            } else if (cp >= 0xF8 && cp <= 0xFE) {
                up = cp - 32;
            } else if (cp >= 0x03B1 && cp <= 0x03C9 && cp != 0x03C2) {
                up = cp - 32;
            } else if (cp == 0x03C2) {
                up = 0x03A3;
            } else if (cp >= 0x0430 && cp <= 0x044F) {
                up = cp - 32;
            } else if (cp >= 0x0450 && cp <= 0x045F && cp != 0x0451) {
                up = cp - 80;
            } else if (cp == 0x0451) {
                up = 0x0401;
            }
            if (up <= 0x7F) {
                result += static_cast<uint8_t>(up);
            } else if (up <= 0x7FF) {
                result += static_cast<uint8_t>(0xC0 | (up >> 6));
                result += static_cast<uint8_t>(0x80 | (up & 0x3F));
            } else if (up <= 0xFFFF) {
                result += static_cast<uint8_t>(0xE0 | (up >> 12));
                result += static_cast<uint8_t>(0x80 | ((up >> 6) & 0x3F));
                result += static_cast<uint8_t>(0x80 | (up & 0x3F));
            } else if (up <= 0x10FFFF) {
                result += static_cast<uint8_t>(0xF0 | (up >> 18));
                result += static_cast<uint8_t>(0x80 | ((up >> 12) & 0x3F));
                result += static_cast<uint8_t>(0x80 | ((up >> 6) & 0x3F));
                result += static_cast<uint8_t>(0x80 | (up & 0x3F));
            }
        }
        return Bytes(result.c_str(), result.size());
    }
    Bytes lower() const {
        std::u32string u32 = to_u32();
        std::string result;
        result.reserve(size());
        for (char32_t cp : u32) {
            uint32_t lp = cp;
            if (cp >= 0x41 && cp <= 0x5A) {
                lp = cp + 32;
            } else if (cp >= 0xC0 && cp <= 0xD6) {
                lp = cp + 32;
            } else if (cp >= 0xD8 && cp <= 0xDE) {
                lp = cp + 32;
            } else if (cp >= 0x0391 && cp <= 0x03A9 && cp != 0x03A2) {
                lp = cp + 32;
            } else if (cp >= 0x0410 && cp <= 0x042F) {
                lp = cp + 32;
            } else if (cp >= 0x0400 && cp <= 0x040F && cp != 0x0401) {
                lp = cp + 80;
            } else if (cp == 0x0401) {
                lp = 0x0451;
            }
            if (lp <= 0x7F) {
                result += static_cast<uint8_t>(lp);
            } else if (lp <= 0x7FF) {
                result += static_cast<uint8_t>(0xC0 | (lp >> 6));
                result += static_cast<uint8_t>(0x80 | (lp & 0x3F));
            } else if (lp <= 0xFFFF) {
                result += static_cast<uint8_t>(0xE0 | (lp >> 12));
                result += static_cast<uint8_t>(0x80 | ((lp >> 6) & 0x3F));
                result += static_cast<uint8_t>(0x80 | (lp & 0x3F));
            } else if (lp <= 0x10FFFF) {
                result += static_cast<uint8_t>(0xF0 | (lp >> 18));
                result += static_cast<uint8_t>(0x80 | ((lp >> 12) & 0x3F));
                result += static_cast<uint8_t>(0x80 | ((lp >> 6) & 0x3F));
                result += static_cast<uint8_t>(0x80 | (lp & 0x3F));
            }
        }
        return Bytes(result.c_str(), result.size());
    }
    friend class BytesView;
};

Bytes operator+(const char* arr, const Bytes& b);
Bytes operator+(const std::string& s, const Bytes& b);
Bytes operator+(const std::u32string& s, const Bytes& b);
Bytes from_int64_to_string(int64_t i);
Bytes from_uint64_to_string(uint64_t i);
Bytes from_uint64_to_bigint(uint64_t i);
Bytes from_double_to_string(double d);
Bytes from_bigint_to_string(Bytes a);
Bytes from_string_to_bigint(const Bytes& b);
Bytes from_u32_to_string(const std::u32string& s);

class BytesView : public Bytes {
public:
    BytesView() = delete;
    BytesView(Bytes&& b) : Bytes() {
        this->arr = b.arr;
        this->size_ = b.size_;
        this->owner = b.owner;
        b.arr = nullptr;
        b.size_ = 0;
        b.owner = false;
    }
    BytesView(BytesView&& b) : Bytes() {
        this->arr = b.arr;
        this->size_ = b.size_;
        this->owner = b.owner;
        b.arr = nullptr;
        b.size_ = 0;
        b.owner = false;
    }
    BytesView(const Bytes& b) : Bytes() {
        this->arr = b.arr;
        this->size_ = b.size_;
        this->owner = false;
    }
    BytesView(const BytesView& b) : Bytes() {
        this->arr = b.arr;
        this->size_ = b.size_;
        this->owner = false;
    }
    BytesView(const char* arr) : Bytes(arr) {}
    BytesView(const std::string& s) : Bytes(s) {}
    BytesView(const std::u32string& s) : Bytes(s) {}
    BytesView& operator=(const Bytes& b) {
        if (this == &b) {
            return *this;
        }
        if (this->owner) {
            delete[] this->arr;
        }
        this->arr = b.arr;
        this->size_ = b.size_;
        this->owner = false;
        return *this;
    }
    BytesView& operator=(const BytesView& b) {
        if (this == &b) {
            return *this;
        }
        if (this->owner) {
            delete[] this->arr;
        }
        this->arr = b.arr;
        this->size_ = b.size_;
        this->owner = false;
        return *this;
    }
    BytesView& operator=(Bytes&& b) {
        if (this == &b) {
            return *this;
        }
        if (this->owner) {
            delete[] this->arr;
        }
        this->arr = b.arr;
        this->size_ = b.size_;
        this->owner = b.owner;
        b.arr = nullptr;
        b.size_ = 0;
        b.owner = false;
        return *this;
    }
    BytesView& operator=(BytesView&& b) {
        if (this == &b) {
            return *this;
        }
        if (this->owner) {
            delete[] this->arr;
        }
        this->arr = b.arr;
        this->size_ = b.size_;
        this->owner = b.owner;
        b.arr = nullptr;
        b.size_ = 0;
        b.owner = false;
        return *this;
    }
};

namespace std {
    template <>
    struct hash<BytesView> {
        size_t operator()(BytesView b) const noexcept {
            size_t s = 0;
            for (uint32_t i = 0; i < b.size(); i++) {
                s ^= size_t(uint8_t(b[i])) * (i + 1) * 0xAAAAAAAA;
            }
            return s;
        }
    };
}
