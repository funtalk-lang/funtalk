#pragma once
#include "cast.h"

Dynamic NOT(Dynamic a, CallPool& pool) {
    switch (a.type) {
        case DynamicType::BOOL:
            return Dynamic(a.type, a.value ^ 1);
        case DynamicType::CHAR:
            return Dynamic(a.type, ~a.value & 0xFFFFFFFF);
        case DynamicType::INT:
        case DynamicType::BYTES:
        case DynamicType::UINT:
            return Dynamic(a.type, pool.bytes.alloc(~pool.bytes.at(a.value)));
        default:
            return createError("TypeError", "Cannot call NOT of " + to_string(a.type), pool);
    }
}

Dynamic AND(Dynamic a, Dynamic b, CallPool& pool) {
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call AND of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(a.type, a.value & b.value);
            break;
        case DynamicType::BYTES:
        case DynamicType::UINT:
            d = Dynamic(a.type, pool.bytes.alloc(pool.bytes.at(a.value) & pool.bytes.at(b.value)));
            break;
        default:
            d = createError("TypeError", "Cannot call AND of " + to_string(a.type), pool);
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

Dynamic OR(Dynamic a, Dynamic b, CallPool& pool) {
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call OR of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(a.type, a.value | b.value);
            break;
        case DynamicType::BYTES:
        case DynamicType::UINT:
            d = Dynamic(a.type, pool.bytes.alloc(pool.bytes.at(a.value) | pool.bytes.at(b.value)));
            break;
        default:
            d = createError("TypeError", "Cannot call OR of " + to_string(a.type), pool);
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

Dynamic XOR(Dynamic a, Dynamic b, CallPool& pool) {
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call XOR of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(a.type, a.value ^ b.value);
            break;
        case DynamicType::BYTES:
        case DynamicType::UINT:
            d = Dynamic(a.type, pool.bytes.alloc(pool.bytes.at(a.value) ^ pool.bytes.at(b.value)));
            break;
        default:
            d = createError("TypeError", "Cannot call XOR of " + to_string(a.type), pool);
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

Dynamic SHL(Dynamic a, Dynamic b, CallPool& pool) {
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call SHL of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(DynamicType::INT, a.value << b.value);
            break;
        default:
            d = createError("TypeError", "Cannot call SHL of " + to_string(a.type), pool);
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

Dynamic SHR(Dynamic a, Dynamic b, CallPool& pool) {
    CastStatus s;
    Dynamic err = cast(a, b, "Cannot call SHR of " + to_string(a.type) + " and " + to_string(b.type), s, pool);
    if (err.type == DynamicType::ERROR) {
        return err;
    }
    Dynamic d;
    switch (a.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            d = Dynamic(DynamicType::INT, a.value >> b.value);
            break;
        default:
            d = createError("TypeError", "Cannot call SHR of " + to_string(a.type), pool);
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
