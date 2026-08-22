#include "bytes.h"

Bytes::Bytes(const std::u32string& s) : Bytes(from_u32_to_string(s)) {}

Bytes operator+(const char* arr, const Bytes& b) {
    return Bytes(arr) + b;
}
Bytes operator+(const std::string& s, const Bytes& b) {
    return Bytes(s) + b;
}
Bytes operator+(const std::u32string& s, const Bytes& b) {
    return Bytes(s) + b;
}

Bytes from_int64_to_string(int64_t i) {
    return Bytes(std::to_string(i));
}

Bytes from_uint64_to_string(uint64_t i) {
    return Bytes(std::to_string(i));
}

Bytes from_uint64_to_bigint(uint64_t i) {
    if (i == 0) {
        return Bytes();
    }
    return Bytes(reinterpret_cast<char*>(&i), 8).trim();
}

Bytes from_double_to_string(double d) {
    return std::to_string(d);
}

Bytes from_bigint_to_string(Bytes a) {
    std::string s = "";
    Bytes ten = Bytes(1);
    ten[0] = 10;
    while (a.size() != 0) {
        Bytes mod = a.bigint_mod(ten);
        if (mod.size() == 0) {
            s = '0' + s;
        } else {
            s = char(uint8_t(mod[0]) % 10 + '0') + s;
        }
        a = a.bigint_div(ten);
    }
    if (s == "") {
        s = "0";
    }
    return Bytes(s.c_str(), s.size());
}

Bytes from_string_to_bigint(const Bytes& b) {
    Bytes r = Bytes();
    Bytes m = Bytes(1);
    m[0] = 1;
    Bytes ten = Bytes(1);
    ten[0] = 10;
    Bytes a = Bytes(1);
    for (uint8_t c : b) {
        a[0] = c - '0';
        r = r.bigint_sum(a).bigint_mul(m);
        m = m.bigint_mul(ten);
    }
    return r;
}

Bytes from_u32_to_string(const std::u32string& s) {
    #ifndef FUNTALK_TEST_BYTES
    if (s.size() > MAX_BYTES_SIZE) {
        throw CallPanic("Cannot create Bytes larger than " + std::to_string(MAX_BYTES_SIZE));
    }
    #endif
    std::string res;
    for (char32_t cp : s) {
        if (cp <= 0x7F) {
            res.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            res.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            res.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0x10FFFF) {
            res.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            res.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return Bytes(res.data(), static_cast<uint32_t>(res.size()));
}
