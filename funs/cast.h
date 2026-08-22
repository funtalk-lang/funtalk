#pragma once
#include <limits>
#include "../types//dynamic.h"

Dynamic cast(Dynamic d, DynamicType type, CallPool& pool);

bool to_bool(Dynamic d, CallPool& pool, Dynamic& err) {
    switch (d.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
            return d.value != 0;
        case DynamicType::BYTES:
        case DynamicType::UINT:
            for (uint8_t b : pool.bytes.at(d.value)) {
                if (b != 0) {
                    return true;
                }
            }
            return false;
        default:
            err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to BOOL", pool);
            return false;
    }
}

int64_t to_int(Dynamic d, CallPool& pool, Dynamic& err) {
    switch (d.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
            return d.value;
        case DynamicType::FLOAT:
            return std::bit_cast<double>(d.value);
        case DynamicType::CHAR:
            return d.value;
        case DynamicType::STRING:
            try {
                return std::stoll(pool.bytes.at(d.value));
            } catch (const std::invalid_argument& e) {
                err = createError("ValueError", e.what(), pool);
            } catch (const std::out_of_range& e) {
                err = createError("ValueError", e.what(), pool);
            }
            return -1;
        case DynamicType::BYTES:
        case DynamicType::UINT: {
            int64_t result = 0;
            std::memcpy(&result, pool.bytes.at(d.value).begin(), std::min(pool.bytes.at(d.value).size(), 8U));
            return result;
        }
        default:
            err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to INT", pool);
            return d.value;
    }
}

double to_float(Dynamic d, CallPool& pool, Dynamic& err) {
    switch (d.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
            return std::bit_cast<int64_t>(d.value);
        case DynamicType::FLOAT:
            return std::bit_cast<double>(d.value);
        case DynamicType::STRING:
            try {
                return std::stod(pool.bytes.at(d.value));
            } catch (const std::invalid_argument& e) {
                err = createError("ValueError", e.what(), pool);
            } catch (const std::out_of_range& e) {
                err = createError("ValueError", e.what(), pool);
            }
            return std::numeric_limits<double>::quiet_NaN();
        case DynamicType::BYTES:
            if (pool.bytes.at(d.value).size() != 8) {
                err = createError("ValueError", "Cannot cast bytes with size != 8 to FLOAT", pool);
                return std::numeric_limits<double>::quiet_NaN();
            } else {
                double result;
                std::memcpy(&result, pool.bytes.at(d.value).begin(), 8U);
                return result;
            }
        default:
            err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to FLOAT", pool);
            return std::numeric_limits<double>::quiet_NaN();
    }
}

char32_t to_char(Dynamic d, CallPool& pool, Dynamic& err) {
    switch (d.type) {
        case DynamicType::BOOL:
        case DynamicType::INT:
        case DynamicType::CHAR:
            return d.value;
        case DynamicType::BYTES:
        case DynamicType::UINT: {
            char32_t result = 0;
            std::memcpy(&result, pool.bytes.at(d.value).begin(), std::min(pool.bytes.at(d.value).size(), 4U));
            return result;
        }
        default:
            err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to CHAR", pool);
            return 0;
    }
}

Bytes to_bytes(Dynamic d, CallPool& pool, Dynamic& err) {
    switch (d.type) {
        case DynamicType::BOOL: {
            const auto a = std::bit_cast<std::array<char, 8>>(d.value);
            return Bytes(a.data(), 1);
        }
        case DynamicType::INT: {
            const auto a = std::bit_cast<std::array<char, 8>>(d.value);
            return Bytes(a.data(), 8);
        }
        case DynamicType::FLOAT: {
            const auto a = std::bit_cast<std::array<char, 8>>(d.value);
            return Bytes(a.data(), 8);
        }
        case DynamicType::CHAR: {
            const auto a = std::bit_cast<std::array<char, 8>>(d.value);
            return Bytes(a.data(), 4);
        }
        case DynamicType::STRING:
        case DynamicType::BYTES:
        case DynamicType::UINT:
            return pool.bytes.at(d.value);
        case DynamicType::COMPLEX: {
            const auto a = std::bit_cast<std::array<char, 16>>(pool.complexes.at(d.value));
            return Bytes(a.data(), 16);
        }
        default:
            err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to BYTES", pool);
            return Bytes();
    }
}

Bytes to_uint(Dynamic d, CallPool& pool, Dynamic& err) {
    switch (d.type) {
        case DynamicType::BOOL: {
            const auto a = std::bit_cast<std::array<char, 8>>(d.value);
            return Bytes(a.data(), 1);
        }
        case DynamicType::INT: {
            const auto a = std::bit_cast<std::array<char, 8>>(d.value);
            return Bytes(a.data(), 8);
        }
        case DynamicType::CHAR: {
            const auto a = std::bit_cast<std::array<char, 8>>(d.value);
            return Bytes(a.data(), 4);
        }
        case DynamicType::STRING:
            return from_string_to_bigint(pool.bytes.at(d.value));
        case DynamicType::BYTES:
        case DynamicType::UINT:
            return pool.bytes.at(d.value);
        case DynamicType::COMPLEX: {
            const auto a = std::bit_cast<std::array<char, 16>>(pool.complexes.at(d.value));
            return Bytes(a.data(), 16);
        }
        default:
            err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to UINT", pool);
            return Bytes();
    }
}

Tuple to_tuple(Dynamic d, CallPool& pool, Dynamic& err) {
    switch (d.type) {
        case DynamicType::STRING: {
            std::u32string s = pool.bytes.at(d.value).to_u32();
            Tuple t = Tuple(s.size());
            for (uint32_t i = 0; i < s.size(); i++) {
                t[i] = Dynamic(DynamicType::CHAR, s[i]);
            }
            return t;
        }
        case DynamicType::BYTES: {
            Tuple t = Tuple(pool.bytes.at(d.value).size());
            for (uint32_t i = 0; i < pool.bytes.at(d.value).size(); i++) {
                t[i] = Dynamic(DynamicType::INT, pool.bytes.at(d.value)[i]);
            }
            return t;
        }
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            Tuple t = Tuple(pool.tuples.at(d.value).size());
            for (uint32_t i = 0; i < pool.tuples.at(d.value).size(); i++) {
                t[i] = copy(pool.tuples.at(d.value)[i], pool);
            }
            return t;
        }
        case DynamicType::DICT: {
            Tuple t = Tuple(pool.dicts.at(d.value).size());
            uint32_t i = 0;
            for (auto [key, value] : pool.dicts.at(d.value)) {
                Tuple pair = Tuple(2);
                pair[0] = copy(key, pool);
                pair[1] = copy(value, pool);
                t[i] = Dynamic(DynamicType::TUPLE, pool.tuples.alloc(pair));
                i++;
            }
            return t;
        }
        case DynamicType::COMPLEX: {
            Tuple t = Tuple(2);
            t[0] = Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(pool.complexes.at(d.value).real()));
            t[1] = Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(pool.complexes.at(d.value).imag()));
            return t;
        }
        default:
            err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to ARR/TUPLE", pool);
            return Tuple();
    }
}

Dict to_dict(Dynamic d, CallPool& pool, Dynamic& err) {
    switch (d.type) {
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            Dict dict = Dict();
            for (Dynamic d1 : pool.tuples.at(d.value)) {
                if (d1.type == DynamicType::ARR || d1.type == DynamicType::TUPLE) {
                    if (pool.tuples.at(d1.value).size() != 2) {
                        err = createError("ItemValueError", "Cannot cast ARR/TUPLE to DICT because the item size is not 2", pool);
                        return std::move(dict);
                    }
                    dict[copy(pool.tuples.at(d1.value)[0], pool)] = copy(pool.tuples.at(d1.value)[1], pool);
                } else {
                    err = createError("ItemTypeError", "Cannot cast ARR/TUPLE to DICT because the item type is " + to_string(d1.type), pool);
                    return std::move(dict);
                }
            }
            return std::move(dict);
        }
        case DynamicType::DICT: {
            Dict dict = Dict();
            for (auto [key, value] : pool.dicts.at(d.value)) {
                dict[copy(key, pool)] = copy(value, pool);
            }
            return std::move(dict);
        }
        default:
            err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to DICT", pool);
            return Dict();
    }
}

Matrix to_matrix(Dynamic d, CallPool& pool, Dynamic& err) {
    if (d.type == DynamicType::ARR || d.type == DynamicType::TUPLE) {
        uint32_t h = pool.tuples.at(d.value).size();
        if (h == 0) {
            err = createError("ValueError", "Cannot cast empty arr/tuple to MATRIX", pool);
            return Matrix();
        }
        uint32_t w;
        if (pool.tuples.at(d.value)[0].type == DynamicType::ARR || pool.tuples.at(d.value)[0].type == DynamicType::TUPLE) {
            w = pool.tuples.at(pool.tuples.at(d.value)[0].value).size();
        } else {
            err = createError("ItemTypeError", "Cannot cast ARR/TUPLE to MATRIX because the item type is " + to_string(pool.tuples.at(d.value)[0].type), pool);
            return Matrix();
        }
        for (uint32_t j = 0; j < h; j++) {
            if (pool.tuples.at(d.value)[j].type == DynamicType::ARR || pool.tuples.at(d.value)[j].type == DynamicType::TUPLE) {
                if (pool.tuples.at(pool.tuples.at(d.value)[j].value).size() != w) {
                    err = createError("ItemValueError", "Cannot cast ARR/TUPLE to MATRIX because the items has different sizes", pool);
                    return Matrix();
                }
            } else {
                err = createError("ItemTypeError", "Cannot cast ARR/TUPLE to MATRIX: expected item type is ARR/TUPLE, got " + to_string(pool.tuples.at(d.value)[j].type), pool);
                return Matrix();
            }
        }
        Matrix m = Matrix(w, h);
        for (uint32_t j = 0; j < h; j++) {
            for (uint32_t i = 0; i < w; i++) {
                m.get(i, j) = std::bit_cast<double>(cast(pool.tuples.at(pool.tuples.at(d.value)[j].value)[i], DynamicType::FLOAT, pool).value);
            }
        }
        return std::move(m);
    } else if (d.type == DynamicType::MATRIX) {
        return pool.matrices.at(d.value);
    } else {
        err = createError("TypeError", "Cannot cast " + to_string(d.type) + " to MATRIX", pool);
        return Matrix();
    }
}

enum class CastStatus {
    NONE, A, B
};

Dynamic cast(Dynamic& a, Dynamic& b, const Bytes& err, CastStatus& s, CallPool& pool) {
    if (a.type == b.type) {
        s = CastStatus::NONE;
        return Dynamic();
    }
    Dynamic b_ = cast(b, a.type, pool);
    if (b_.type == DynamicType::ERROR) {
        free(b_, pool);
        Dynamic a_ = cast(a, b.type, pool);
        if (a_.type == DynamicType::ERROR) {
            free(a_, pool);
            return createError("TypeError", err, pool);
        }
        a = a_;
        s = CastStatus::A;
    } else {
        b = b_;
        s = CastStatus::B;
    }
    return Dynamic();
}

Dynamic cast(Dynamic d, DynamicType type, CallPool& pool) {
    if (d.type == DynamicType::OBJ) {
        return createError("CastError", "Cannot cast OBJ", pool);
    }
    if (type == DynamicType::OBJ) {
        return createError("CastError", "Cannot cast to OBJ", pool);
    }
    if (type == DynamicType::FUN) {
        return createError("CastError", "Cannot cast to FUN", pool);
    }
    if (type == DynamicType::ERROR) {
        return createError("CastError", "Cannot cast to ERROR", pool);
    }
    if (d.type == DynamicType::ITERATE) {
        return createError("CastError", "Cannot cast ITERATE", pool);
    }
    if (type == DynamicType::ITERATE) {
        return createError("CastError", "Cannot cast to ITERATE", pool);
    }
    if (type == DynamicType::COMPLEX) {
        return createError("CastError", "Cannot cast to COMPLEX", pool);
    }
    if (d.type == DynamicType::ASYNC) {
        return createError("CastError", "Cannot cast ASYNC", pool);
    }
    if (type == DynamicType::ASYNC) {
        return createError("CastError", "Cannot cast to ASYNC", pool);
    }
    if (d.type == DynamicType::DEFERRED) {
        return createError("CastError", "Cannot cast DEFERRED", pool);
    }
    if (type == DynamicType::DEFERRED) {
        return createError("CastError", "Cannot cast to DEFERRED", pool);
    }
    Dynamic err;
    switch (type) {
        case DynamicType::BOOL: {
            Dynamic r = Dynamic(DynamicType::BOOL, to_bool(d, pool, err));
            if (err.type == DynamicType::ERROR) {
                return err;
            }
            return r;
        }
        case DynamicType::INT: {
            Dynamic r = Dynamic(DynamicType::INT, to_int(d, pool, err));
            if (err.type == DynamicType::ERROR) {
                return err;
            }
            return r;
        }
        case DynamicType::FLOAT: {
            Dynamic r = Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(to_float(d, pool, err)));
            if (err.type == DynamicType::ERROR) {
                return err;
            }
            return r;
        }
        case DynamicType::CHAR: {
            Dynamic r = Dynamic(DynamicType::CHAR, to_char(d, pool, err));
            if (err.type == DynamicType::ERROR) {
                return err;
            }
            return r;
        }
        case DynamicType::STRING: {
            return Dynamic(DynamicType::STRING, pool.bytes.alloc(to_string(d, pool)));
        }
        case DynamicType::BYTES: {
            Dynamic r = Dynamic(DynamicType::BYTES, pool.bytes.alloc(to_bytes(d, pool, err)));
            if (err.type == DynamicType::ERROR) {
                free(r, pool);
                return err;
            }
            return r;
        }
        case DynamicType::UINT: {
            Dynamic r = Dynamic(DynamicType::UINT, pool.bytes.alloc(to_uint(d, pool, err)));
            if (err.type == DynamicType::ERROR) {
                free(r, pool);
                return err;
            }
            return r;
        }
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            Dynamic r = Dynamic(type, pool.tuples.alloc(to_tuple(d, pool, err)));
            if (err.type == DynamicType::ERROR) {
                free(r, pool);
                return err;
            }
            return r;
        }
        case DynamicType::DICT: {
            Dynamic r = Dynamic(DynamicType::DICT, pool.dicts.alloc(to_dict(d, pool, err)));
            if (err.type == DynamicType::ERROR) {
                free(r, pool);
                return err;
            }
            return r;
        }
        case DynamicType::MATRIX: {
            Dynamic r = Dynamic(DynamicType::MATRIX, pool.matrices.alloc(to_matrix(d, pool, err)));
            if (err.type == DynamicType::ERROR) {
                free(r, pool);
                return err;
            }
            return r;
        }
        default:
            throw CallPanic("cast");
    }
}
