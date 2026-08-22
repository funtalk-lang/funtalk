#pragma once
#include "cast.h"

Dynamic ABS(Dynamic a, CallPool& pool) {
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
            return Dynamic(DynamicType::INT, std::abs(std::bit_cast<int64_t>(a.value)));
        case DynamicType::FLOAT:
            return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::abs(std::bit_cast<double>(a.value))));
        case DynamicType::BYTES:
            return Dynamic(DynamicType::BYTES, pool.bytes.alloc(pool.bytes.at(a.value).abs()));
        case DynamicType::COMPLEX:
            return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::abs(pool.complexes.at(a.value))));
        case DynamicType::MATRIX:
            return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(std::abs(pool.matrices.at(a.value))));
        default:
            return createError("TypeError", "Cannot call ABS of " + to_string(a.type), pool);
    }
}

Dynamic ADD(Dynamic a, Dynamic b, CallPool& pool) {
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call ADD of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
            d = Dynamic(DynamicType::INT, a.value + b.value);
            break;
        case DynamicType::FLOAT:
            d = Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::bit_cast<double>(a.value) + std::bit_cast<double>(b.value)));
            break;
        case DynamicType::CHAR:
            d = Dynamic(DynamicType::CHAR, (a.value + b.value) & 0xFFFFFFFF);
            break;
        case DynamicType::STRING:
            if (pool.bytes.at(a.value).size() + pool.bytes.at(b.value).size() > MAX_BYTES_SIZE) {
                throw CallPanic("Cannot create string larger than " + std::to_string(MAX_BYTES_SIZE));
            }
            d = Dynamic(DynamicType::STRING, pool.bytes.alloc(pool.bytes.at(a.value) + pool.bytes.at(b.value)));
            break;
        case DynamicType::BYTES:
            d = Dynamic(DynamicType::STRING, pool.bytes.alloc(pool.bytes.at(a.value).sum(pool.bytes.at(b.value))));
            break;
        case DynamicType::ARR:
        case DynamicType::TUPLE:
            d = pool.tuples.at(a.value).concat(d.type, pool.tuples.at(b.value), pool);
            break;
        case DynamicType::DICT: {
            Dict dict;
            for (auto [key, value] : pool.dicts.at(a.value)) {
                if (pool.dicts.at(b.value).find(key) == pool.dicts.at(b.value).end()) {
                    dict[key] = copy(value, pool);
                } else {
                    Dynamic d = cast(pool.dicts.at(b.value)[key], pool.dicts.at(a.value)[key].type, pool);
                    dict[key] = ADD(value, d, pool);
                    free(d, pool);
                }
            }
            for (auto [key, value] : pool.dicts.at(b.value)) {
                if (pool.dicts.at(a.value).find(key) == pool.dicts.at(a.value).end()) {
                    dict[key] = copy(value, pool);
                }
            }
            d = Dynamic(DynamicType::DICT, pool.dicts.alloc(dict));
            break;
        }
        case DynamicType::COMPLEX:
            d = Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(pool.complexes.at(a.value) + pool.complexes.at(b.value)));
            break;
        case DynamicType::MATRIX:
            if (pool.matrices.at(a.value).w() != pool.matrices.at(b.value).w() || pool.matrices.at(a.value).h() != pool.matrices.at(b.value).h()) {
                return createError("ValueError", "Cannot call ADD of matrices with different size", pool);
            }
            d = Dynamic(DynamicType::MATRIX, pool.matrices.alloc(pool.matrices.at(a.value) + pool.matrices.at(b.value)));
            break;
        default:
            d = createError("TypeError", "Cannot call ADD of " + to_string(a.type) + " and " + to_string(b.type), pool);
            break;
    }
    if (s == CastStatus::NONE) {
        return d;
    }
    if (s == CastStatus::A) {
        free(a, pool);
        return d;
    }
    free(b, pool);
    return d;
}

Dynamic NEG(Dynamic a, CallPool& pool) {
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
            return Dynamic(DynamicType::INT, -a.value);
        case DynamicType::FLOAT:
            return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(-std::bit_cast<double>(a.value)));
        case DynamicType::BYTES:
            return Dynamic(DynamicType::BYTES, pool.bytes.alloc(-pool.bytes.at(a.value)));
        case DynamicType::COMPLEX:
            return Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(-pool.complexes.at(a.value)));
        case DynamicType::MATRIX:
            return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(-pool.matrices.at(a.value)));
        default:
            return createError("TypeError", "Cannot call NEG of " + to_string(a.type), pool);
    }
}

Dynamic MUL(Dynamic a, Dynamic b, CallPool& pool) {
    if ((a.type >= DynamicType::BOOL && a.type <= DynamicType::FLOAT) && b.type == DynamicType::MATRIX) {
        a = cast(a, DynamicType::FLOAT, pool);
        return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(std::bit_cast<double>(a.value) * pool.matrices.at(b.value)));
    }
    if (a.type == DynamicType::MATRIX && (b.type >= DynamicType::BOOL && b.type <= DynamicType::FLOAT)) {
        b = cast(b, DynamicType::FLOAT, pool);
        return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(pool.matrices.at(a.value) * std::bit_cast<double>(b.value)));
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call MUL of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
            d = Dynamic(a.type, std::bit_cast<int64_t>(a.value) * std::bit_cast<int64_t>(b.value));
            break;
        case DynamicType::FLOAT:
            d = Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::bit_cast<double>(a.value) * std::bit_cast<double>(b.value)));
            break;
        case DynamicType::CHAR:
            d = Dynamic(DynamicType::CHAR, (a.value * b.value) & 0xFFFFFFFF);
            break;
        case DynamicType::BYTES:
            d = Dynamic(DynamicType::BYTES, pool.bytes.alloc(pool.bytes.at(a.value).pow(pool.bytes.at(b.value))));
            break;
        case DynamicType::COMPLEX:
            d = Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(pool.complexes.at(a.value) * pool.complexes.at(b.value)));
            break;
        case DynamicType::MATRIX:
            if (pool.matrices.at(a.value).w() != pool.matrices.at(b.value).w() || pool.matrices.at(a.value).h() != pool.matrices.at(b.value).h()) {
                return createError("ValueError", "Cannot call MUL of matrices with different size", pool);
            }
            d = Dynamic(DynamicType::MATRIX, pool.matrices.alloc(pool.matrices.at(a.value) * pool.matrices.at(b.value)));
            break;
        default:
            d = createError("TypeError", "Cannot call MUL of " + to_string(a.type) + " and " + to_string(b.type), pool);
            break;
    }
    if (s == CastStatus::NONE) {
        return d;
    }
    if (s == CastStatus::A) {
        free(a, pool);
        return d;
    }
    free(b, pool);
    return d;
}

Dynamic MULM(Dynamic a, Dynamic b, CallPool& pool) {
    if (a.type != DynamicType::MATRIX) {
        return createError("TypeError", "Expected first arg MATRIX, got " + to_string(a.type), pool);
    }
    if (b.type != DynamicType::MATRIX) {
        return createError("TypeError", "Expected second arg MATRIX, got " + to_string(b.type), pool);
    }
    if (pool.matrices.at(a.value).w() != pool.matrices.at(b.value).h()) {
        return createError("ValueError", "Incompatible matrix dimensions for MULM", pool);
    }
    return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(funtalk::mulm(pool.matrices.at(a.value), pool.matrices.at(b.value))));
}

Dynamic INV(Dynamic a, CallPool& pool) {
    switch (a.type) {
        case DynamicType::FLOAT:
            return Dynamic(a.type, std::bit_cast<uint64_t>(1.0 / std::bit_cast<double>(a.value)));
        case DynamicType::BYTES:
            return Dynamic(DynamicType::BYTES, pool.bytes.alloc(pool.bytes.at(a.value).inv()));
        case DynamicType::COMPLEX:
            return Dynamic(a.type, pool.complexes.alloc(1.0 / pool.complexes.at(a.value)));
        case DynamicType::MATRIX:
            return Dynamic(a.type, pool.matrices.alloc(pool.matrices.at(a.value).inv()));
        default:
            return createError("TypeError", "Cannot call INV of " + to_string(a.type), pool);
    }
}

Dynamic INVM(Dynamic a, CallPool& pool) {
    if (a.type != DynamicType::MATRIX) {
        return createError("TypeError", "Expected first arg square matrix" + to_string(a.type), pool);
    }
    if (pool.matrices.at(a.value).w() != pool.matrices.at(a.value).h()) {
        return createError("ValueError", "Expected first arg square matrix", pool);
    }
    return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(funtalk::invm(pool.matrices.at(a.value))));
}

Dynamic MOD(Dynamic a, Dynamic b, CallPool& pool) {
    if (a.type == DynamicType::MATRIX && (b.type >= DynamicType::BOOL && b.type <= DynamicType::FLOAT)) {
        b = cast(b, DynamicType::FLOAT, pool);
        return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(pool.matrices.at(a.value) % std::bit_cast<double>(b.value)));
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call MOD of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::INT:
            d = Dynamic(a.type, std::bit_cast<int64_t>(a.value) % std::bit_cast<int64_t>(b.value));
            break;
        case DynamicType::FLOAT:
            d = Dynamic(a.type, std::bit_cast<uint64_t>(std::fmod(std::bit_cast<double>(a.value), std::bit_cast<double>(b.value))));
            break;
        case DynamicType::CHAR:
            d = Dynamic(a.type, static_cast<int32_t>(a.value) % static_cast<int32_t>(b.value));
            break;
        case DynamicType::BYTES:
            d = Dynamic(DynamicType::BYTES, pool.bytes.alloc(pool.bytes.at(a.value) % pool.bytes.at(b.value)));
            break;
        default:
            d = createError("TypeError", "Cannot call MOD of " + to_string(a.type), pool);
            break;
    }
    if (s == CastStatus::NONE) {
        return d;
    }
    if (s == CastStatus::A) {
        free(a, pool);
        return d;
    }
    free(b, pool);
    return d;
}

Dynamic POW(Dynamic a, Dynamic b, CallPool& pool) {
    if (a.type == DynamicType::MATRIX && b.type == DynamicType::INT) {
        if (pool.matrices.at(a.value).w() != pool.matrices.at(a.value).h()) {
            return createError("TypeError", "Expected first arg square matrix", pool);
        }
        return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(std::pow(pool.matrices.at(a.value), std::bit_cast<int64_t>(b.value))));
    }
    if ((a.type == DynamicType::BOOL || a.type == DynamicType::INT) && b.type == DynamicType::FLOAT) {
        a = cast(a, DynamicType::FLOAT, pool);
        if (std::bit_cast<double>(a.value) < 0 && std::bit_cast<double>(b.value) != static_cast<int64_t>(std::bit_cast<double>(b.value))) {
            return Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(std::pow(std::complex<double>(std::bit_cast<double>(a.value), 0), std::complex<double>(std::bit_cast<double>(b.value), 0))));
        }
        return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::pow(std::bit_cast<double>(a.value), std::bit_cast<double>(b.value))));
    }
    if (a.type == DynamicType::FLOAT && (b.type == DynamicType::BOOL || b.type == DynamicType::INT)) {
        b = cast(b, DynamicType::FLOAT, pool);
        return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::pow(std::bit_cast<double>(a.value), std::bit_cast<double>(b.value))));
    }
    if ((a.type == DynamicType::BOOL || a.type == DynamicType::INT) && (b.type == DynamicType::BOOL || b.type == DynamicType::INT)) {
        a = cast(a, DynamicType::FLOAT, pool);
        b = cast(b, DynamicType::FLOAT, pool);
        if (std::bit_cast<double>(a.value) < 0 && std::bit_cast<double>(b.value) != static_cast<int64_t>(std::bit_cast<double>(b.value))) {
            return Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(std::pow(std::complex<double>(std::bit_cast<double>(a.value), 0), std::complex<double>(std::bit_cast<double>(b.value), 0))));
        }
        return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::pow(std::bit_cast<double>(a.value), std::bit_cast<double>(b.value))));
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call POW of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::FLOAT:
            if (std::bit_cast<double>(a.value) < 0 && std::bit_cast<double>(b.value) != static_cast<int64_t>(std::bit_cast<double>(b.value))) {
                d = Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(std::pow(std::complex<double>(std::bit_cast<double>(a.value), 0), std::complex<double>(std::bit_cast<double>(b.value), 0))));
            } else {
                d = Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::pow(std::bit_cast<double>(a.value), std::bit_cast<double>(b.value))));
            }
            break;
        case DynamicType::BYTES:
            return Dynamic(DynamicType::BYTES, pool.bytes.alloc(pool.bytes.at(a.value).pow(pool.bytes.at(b.value))));
        case DynamicType::COMPLEX:
            d = Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(std::pow(pool.complexes.at(a.value), pool.complexes.at(b.value))));
            break;
        default:
            d = createError("TypeError", "Cannot call POW of " + to_string(a.type) + " and " + to_string(b.type), pool);
            break;
    }
    if (s == CastStatus::NONE) {
        return d;
    }
    if (s == CastStatus::A) {
        free(a, pool);
        return d;
    }
    free(b, pool);
    return d;
}

Dynamic POWM(Dynamic a, Dynamic b, CallPool& pool) {
    if (a.type != DynamicType::MATRIX) {
        return createError("TypeError", "Expected first arg square matrix" + to_string(a.type), pool);
    }
    if (pool.matrices.at(a.value).w() != pool.matrices.at(a.value).h()) {
        return createError("ValueError", "Expected first arg square matrix", pool);
    }
    if (b.type > DynamicType::INT) {
        return createError("TypeError", "Expected second arg in (BOOL, INT), got " + to_string(b.type), pool);
    }
    return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(funtalk::powm(pool.matrices.at(a.value), b.value)));
}
