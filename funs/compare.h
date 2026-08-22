#pragma once
#include "cast.h"

Dynamic EQ(Dynamic a, Dynamic b, CallPool& pool) {
    if (a == b) {
        return Dynamic(DynamicType::BOOL, true);
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call EQ of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::OBJ:
            d = Dynamic(DynamicType::BOOL, pool.objects[a.value] == pool.objects[b.value]);
            break;
        case DynamicType::ARR:
            d = Dynamic(DynamicType::BOOL, pool.tuples.at(a.value) == pool.tuples.at(b.value));
            break;
        case DynamicType::DICT:
            d = Dynamic(DynamicType::BOOL, pool.dicts.at(a.value) == pool.dicts.at(b.value));
            break;
        case DynamicType::COMPLEX:
            d = Dynamic(DynamicType::BOOL, pool.complexes.at(a.value) == pool.complexes.at(b.value));
            break;
        default:
            d = Dynamic(DynamicType::BOOL, a == b);
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

Dynamic NEQ(Dynamic a, Dynamic b, CallPool& pool) {
    if (a == b) {
        return Dynamic(DynamicType::BOOL, false);
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call NEQ of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::OBJ:
            d = Dynamic(DynamicType::BOOL, pool.objects[a.value] != pool.objects[b.value]);
            break;
        case DynamicType::ARR:
            d = Dynamic(DynamicType::BOOL, pool.tuples.at(a.value) != pool.tuples.at(b.value));
            break;
        case DynamicType::DICT:
            d = Dynamic(DynamicType::BOOL, pool.dicts.at(a.value) != pool.dicts.at(b.value));
            break;
        case DynamicType::COMPLEX:
            d = Dynamic(DynamicType::BOOL, pool.complexes.at(a.value) != pool.complexes.at(b.value));
            break;
        default:
            d = Dynamic(DynamicType::BOOL, a != b);
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

Dynamic LT(Dynamic a, Dynamic b, CallPool& pool) {
    if (a == b) {
        return Dynamic(DynamicType::BOOL, false);
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call LT of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(DynamicType::BOOL, a.value < b.value);
            break;
        case DynamicType::FLOAT:
            d = Dynamic(DynamicType::BOOL, std::bit_cast<double>(a.value) < std::bit_cast<double>(b.value));
            break;
        case DynamicType::STRING:
        case DynamicType::BYTES:
            d = Dynamic(DynamicType::BOOL, pool.bytes.at(a.value) < pool.bytes.at(b.value));
            break;
        case DynamicType::UINT:
            d = Dynamic(DynamicType::BOOL, pool.bytes.at(a.value).bigint_cmp(pool.bytes.at(b.value)) == std::strong_ordering::less);
            break;
        default:
            d = createError("TypeError", "Cannot call LT of " + to_string(a.type), pool);
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

Dynamic GT(Dynamic a, Dynamic b, CallPool& pool) {
    if (a == b) {
        return Dynamic(DynamicType::BOOL, false);
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call GT of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(DynamicType::BOOL, a.value < b.value);
            break;
        case DynamicType::FLOAT:
            d = Dynamic(DynamicType::BOOL, std::bit_cast<double>(a.value) > std::bit_cast<double>(b.value));
            break;
        case DynamicType::STRING:
        case DynamicType::BYTES:
            d = Dynamic(DynamicType::BOOL, pool.bytes.at(a.value) > pool.bytes.at(b.value));
            break;
        case DynamicType::UINT:
            d = Dynamic(DynamicType::BOOL, pool.bytes.at(a.value).bigint_cmp(pool.bytes.at(b.value)) == std::strong_ordering::greater);
            break;
        default:
            d = createError("TypeError", "Cannot call GT of " + to_string(a.type), pool);
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

Dynamic LE(Dynamic a, Dynamic b, CallPool& pool) {
    if (a == b) {
        return Dynamic(DynamicType::BOOL, true);
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call LE of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(DynamicType::BOOL, a.value <= b.value);
            break;
        case DynamicType::FLOAT:
            d = Dynamic(DynamicType::BOOL, std::bit_cast<double>(a.value) <= std::bit_cast<double>(b.value));
            break;
        case DynamicType::STRING:
        case DynamicType::BYTES:
            d = Dynamic(DynamicType::BOOL, pool.bytes.at(a.value) <= pool.bytes.at(b.value));
            break;
        case DynamicType::UINT:
            d = Dynamic(DynamicType::BOOL, pool.bytes.at(a.value).bigint_cmp(pool.bytes.at(b.value)) != std::strong_ordering::greater);
            break;
        default:
            d = createError("TypeError", "Cannot call LE of " + to_string(a.type), pool);
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

Dynamic GE(Dynamic a, Dynamic b, CallPool& pool) {
    if (a == b) {
        return Dynamic(DynamicType::BOOL, true);
    }
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call GE of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(DynamicType::BOOL, a.value >= b.value);
            break;
        case DynamicType::FLOAT:
            d = Dynamic(DynamicType::BOOL, std::bit_cast<double>(a.value) >= std::bit_cast<double>(b.value));
            break;
        case DynamicType::STRING:
        case DynamicType::BYTES:
            d = Dynamic(DynamicType::BOOL, pool.bytes.at(a.value) >= pool.bytes.at(b.value));
            break;
        case DynamicType::UINT:
            d = Dynamic(DynamicType::BOOL, pool.bytes.at(a.value).bigint_cmp(pool.bytes.at(b.value)) != std::strong_ordering::less);
            break;
        default:
            d = createError("TypeError", "Cannot call GE of " + to_string(a.type), pool);
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
