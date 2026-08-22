#include <iostream>
#include "../types/bytes.h"

int64_t mod_pow(int64_t base, int64_t exponent, int64_t modulus) {
    if (modulus == 1) return 0;
    int64_t result = 1;
    base = base % modulus;
    while (exponent > 0) {
        if (exponent & 1) {
            result = (result * base) % modulus;
        }
        base = (base * base) % modulus;
        exponent >>= 1;
    }
    return result;
}

void test_bigint() {
    for (uint32_t i = 0; i < 512; i++) {
        std::cout << "i = " << i << '\n';
        for (uint32_t j = 1; j < 512; j++) {
            for (uint32_t k = 1; k < 4; k++) {
                assert(from_uint64_to_bigint(i).bigint_mul(from_uint64_to_bigint(j)).to_uint64() == i * j);
                assert(from_uint64_to_bigint(i).bigint_pow(from_uint64_to_bigint(j), from_uint64_to_bigint(k)).to_uint64() == mod_pow(i, j, k));
            }
            assert(from_uint64_to_bigint(i).bigint_pow(from_uint64_to_bigint(j), from_uint64_to_bigint(255)).to_uint64() == mod_pow(i, j, 255));
            assert(from_uint64_to_bigint(i).bigint_pow(from_uint64_to_bigint(j), from_uint64_to_bigint(256)).to_uint64() == mod_pow(i, j, 256));
            assert(from_uint64_to_bigint(i).bigint_pow(from_uint64_to_bigint(j), from_uint64_to_bigint(257)).to_uint64() == mod_pow(i, j, 257));
            assert(from_uint64_to_bigint(i).bigint_pow(from_uint64_to_bigint(j), from_uint64_to_bigint(65535)).to_uint64() == mod_pow(i, j, 65535));
            assert(from_uint64_to_bigint(i).bigint_pow(from_uint64_to_bigint(j), from_uint64_to_bigint(65536)).to_uint64() == mod_pow(i, j, 65536));
            assert(from_uint64_to_bigint(i).bigint_pow(from_uint64_to_bigint(j), from_uint64_to_bigint(65537)).to_uint64() == mod_pow(i, j, 65537));
            assert(from_uint64_to_bigint(i).bigint_div(from_uint64_to_bigint(j)).to_uint64() == i / j);
            Bytes a = from_uint64_to_bigint(i);
            Bytes s = from_bigint_to_string(a);
            std::string s1 = std::to_string(i);
            std::string s2 = std::string(s.begin(), s.size());
            assert(s1 == s2);
        }
    }
}

void test_utf32(std::string u8) {
    Bytes s = Bytes(u8.c_str(), u8.size());
    std::u32string u32 = s.to_u32();
    std::string s1 = Bytes(u32);
    std::cout << s1 << '\n';
    assert(s1 == u8);
}

int main() {
    test_bigint();
    test_utf32("Hello");
    test_utf32("Привет");
    test_utf32("✅");
}
