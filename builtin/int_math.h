Dynamic MOD_POW(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 3);
    if (args[0].type != DynamicType::INT) {
        return createError("TypeError", "Expected first arg INT, got " + to_string(args[0].type), pool);
    }
    if (args[1].type != DynamicType::INT) {
        return createError("TypeError", "Expected second arg INT, got " + to_string(args[1].type), pool);
    }
    if (args[2].type != DynamicType::INT) {
        return createError("TypeError", "Expected third arg INT, got " + to_string(args[2].type), pool);
    }
    int64_t base = args[0].value;
    int64_t exp = args[1].value;
    int64_t modulus = args[2].value;
    if (modulus <= 0) {
        return createError("ValueError", "Modulus must be positive", pool);
    }
    if (modulus == 1) {
        return Dynamic(DynamicType::INT, 0);
    }
    base = ((base % modulus) + modulus) % modulus;
    uint64_t u_exp = 0;
    if (exp < 0) {
        int64_t m0 = modulus;
        int64_t y = 0, x = 1;
        int64_t a = base;
        int64_t m = modulus;
        while (a > 1) {
            if (m == 0) break;
            int64_t q = a / m;
            int64_t t = m;
            m = a % m;
            a = t;
            t = y;
            y = x - q * y;
            x = t;
        }
        if (x < 0) x += m0;
        if ((__int128(x) * base) % modulus != 1) {
            return createError("ValueError", "Base and modulus are not coprime for negative exponent", pool);
        }
        base = x;
        u_exp = -static_cast<uint64_t>(exp);
    } else {
        u_exp = static_cast<uint64_t>(exp);
    }
    int64_t result = 1;
    while (u_exp > 0) {
        if (interrupt.load()) {
            return Dynamic(DynamicType::INT, 0);
        }
        if (u_exp & 1) {
            result = __int128(result) * base % modulus;
        }
        base = __int128(base) * base % modulus;
        u_exp >>= 1;
    }
    return Dynamic(DynamicType::INT, result);
}

Dynamic BIT_MATRIX_MUL(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type != DynamicType::INT) {
        return createError("TypeError", "Expected first arg INT, got " + to_string(args[0].type), pool);
    }
    if (args[1].type != DynamicType::INT) {
        return createError("TypeError", "Expected second arg INT, got " + to_string(args[1].type), pool);
    }
    std::array<uint8_t, 8> inputs = std::bit_cast<std::array<uint8_t, 8>>(args[0].value);
    std::array<uint8_t, 8> m = std::bit_cast<std::array<uint8_t, 8>>(args[1].value);
    for (uint8_t& a : inputs) {
        uint8_t result = 0;
        for (size_t i = 0; i < 8; i++) {
            uint8_t parity = std::popcount(static_cast<unsigned int>(a & m[i])) & 1;
            result |= (parity << i);
        }
        a = result;
    }
    return Dynamic(DynamicType::INT, std::bit_cast<uint64_t>(inputs));
}
