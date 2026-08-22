Dynamic GET(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    switch (args[0].type) {
        case DynamicType::STRING:
        case DynamicType::BYTES:
        case DynamicType::UINT: {
            if (args[1].type == DynamicType::BOOL || args[1].type == DynamicType::INT) {
                if (args[1].value < pool.bytes.at(args[0].value).size()) {
                    return Dynamic(DynamicType::CHAR, pool.bytes.at(args[0].value)[args[1].value]);
                }
                if ((pool.bytes.at(args[0].value).size() + args[1].value) < pool.bytes.at(args[0].value).size()) {
                    return Dynamic(DynamicType::CHAR, pool.bytes.at(args[0].value)[pool.bytes.at(args[0].value).size() + args[1].value]);
                }
                return createError("IndexError", "Bytes index " + std::to_string(static_cast<int64_t>(args[1].type)) + " out of range", pool);
            }
            return createError("TypeError", "Expected second arg in (BOOL, INT), got " + to_string(args[1].type), pool);
        }
        case DynamicType::OBJ: {
            if (args[1].type == DynamicType::STRING) {
                if (auto it = vm::symbolTable.string_to_id.find(pool.bytes.at(args[1].value)); it != vm::symbolTable.string_to_id.end()) {
                    if (auto it1 = pool.objects[args[0].value].find(it->second); it1 != pool.objects[args[0].value].end()) {
                        return copy(it1->second, pool);
                    }
                    return createError("KeyError", "Key " + pool.bytes.at(args[1].value) + " does not exist", pool);
                }
                return createError("KeyError", "Key " + pool.bytes.at(args[1].value) + " out of symbolTable", pool);
            }
            return createError("TypeError", "Expected second arg STRING, got " + to_string(args[1].type), pool);
        }
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            if (args[1].type == DynamicType::BOOL || args[1].type == DynamicType::INT) {
                if (args[1].value < pool.tuples.at(args[0].value).size()) {
                    return copy(pool.tuples.at(args[0].value)[args[1].value], pool);
                }
                if ((pool.tuples.at(args[0].value).size() + args[1].value) < pool.tuples.at(args[0].value).size()) {
                    return copy(pool.tuples.at(args[0].value)[pool.tuples.at(args[0].value).size() + args[1].value], pool);
                }
                return createError("IndexError", "Array/Tuple index " + std::to_string(static_cast<int64_t>(args[1].value)) + " out of range", pool);
            }
            return createError("TypeError", "Expected second arg in (BOOL, INT), got " + to_string(args[1].type), pool);
        }
        case DynamicType::DICT: {
            if (auto it = pool.dicts.at(args[0].value).find(args[1]); it != pool.dicts.at(args[0].value).end()) {
                return copy(it->second, pool);
            }
            return createError("KeyError", "Key " + to_string(args[1], pool) + " does not exist", pool);
        }
        case DynamicType::ERROR: {
            if (args[1].type == DynamicType::BOOL || args[1].type == DynamicType::INT) {
                if (args[1].value == 0) {
                    return copy(Dynamic(DynamicType::STRING, pool.bytes.alloc(pool.bytes.at(args[0].value & 0xFFFFFFFF))), pool);
                }
                if (args[1].value == 1 || args[1].value == -1) {
                    return copy(Dynamic(DynamicType::STRING, pool.bytes.alloc(pool.bytes.at(args[0].value >> 32))), pool);
                }
                return createError("IndexError", "Error index " + std::to_string(static_cast<int64_t>(args[1].value)) + " out of range", pool);
            }
            return createError("TypeError", "Expected second arg in (BOOL, INT), got " + to_string(args[1].type), pool);
        }
        case DynamicType::COMPLEX: {
            if (args[1].type == DynamicType::BOOL || args[1].type == DynamicType::INT) {
                if (args[1].value == 0) {
                    return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(pool.complexes.at(args[1].value).real()));
                }
                if (args[1].value == 1 || args[1].value == -1) {
                    return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(pool.complexes.at(args[1].value).imag()));
                }
                return createError("IndexError", "Complex index " + std::to_string(static_cast<int64_t>(args[1].value)) + " out of range", pool);
            }
            return createError("TypeError", "Expected second arg in (BOOL, INT), got " + to_string(args[1].type), pool);
        }
        case DynamicType::MATRIX: {
            if (args[1].type == DynamicType::TUPLE) {
                if (pool.tuples.at(args[1].value).size() != 2) {
                    return createError("SizeError", "Expected tuple with size 2", pool);
                }
                uint64_t j;
                if (pool.tuples.at(args[1].value)[0].type == DynamicType::BOOL || pool.tuples.at(args[1].value)[0].type == DynamicType::INT) {
                    if (pool.tuples.at(args[1].value)[0].value < pool.matrices.at(args[0].value).h()) {
                        j = pool.tuples.at(args[1].value)[0].value;
                    } else if ((pool.matrices.at(args[0].value).h() + pool.tuples.at(args[1].value)[0].value) < pool.matrices.at(args[0].value).h()) {
                        j = pool.tuples.at(pool.matrices.at(args[0].value).h() + pool.tuples.at(args[1].value)[0].value)[0].value;
                    } else {
                        return createError("IndexError", "Matrix row index " + std::to_string(static_cast<int64_t>(pool.tuples.at(args[1].value)[0].value)) + " out of range", pool);
                    }
                } else {
                    return createError("TypeError", "Expected second arg tuple element at index 0 to be in (BOOL, INT), got " + to_string(pool.tuples.at(args[1].value)[0].type), pool);
                }
                uint64_t i;
                if (pool.tuples.at(args[1].value)[1].type == DynamicType::BOOL || pool.tuples.at(args[1].value)[1].type == DynamicType::INT) {
                    if (pool.tuples.at(args[1].value)[1].value < pool.matrices.at(args[0].value).w()) {
                        i = pool.tuples.at(args[1].value)[1].value;
                    } else if ((pool.matrices.at(args[0].value).w() + pool.tuples.at(args[1].value)[1].value) < pool.matrices.at(args[0].value).w()) {
                        i = pool.tuples.at(pool.matrices.at(args[0].value).w() + pool.tuples.at(args[1].value)[1].value)[1].value;
                    } else {
                        return createError("IndexError", "Matrix column index " + std::to_string(static_cast<int64_t>(pool.tuples.at(args[1].value)[1].value)) + " out of range", pool);
                    }
                } else {
                    return createError("TypeError", "Expected second arg tuple element at index 1 to be in (BOOL, INT), got " + to_string(pool.tuples.at(args[1].value)[1].type), pool);
                }
                return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(pool.matrices.at(args[0].value).get(i, j)));
            }
            return createError("TypeError", "Expected second arg in (TUPLE, ARR), got " + to_string(args[1].type), pool);
        }
        default:
            return createError("TypeError", "Expected first arg in (STRING, BYTES, UINT, OBJ, ARR, TUPLE, DICT, ERROR, COMPLEX, MATRIX), got " + to_string(args[0].type), pool);
    }
}

Dynamic SIZE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    switch (args[0].type) {
        case DynamicType::STRING:
        case DynamicType::BYTES:
        case DynamicType::UINT:
            return Dynamic(DynamicType::INT, pool.bytes.at(args[0].value).size());
        case DynamicType::OBJ:
            return Dynamic(DynamicType::INT, pool.objects[args[0].value].size());
        case DynamicType::ARR:
        case DynamicType::TUPLE:
            return Dynamic(DynamicType::INT, pool.tuples.at(args[0].value).size());
        case DynamicType::DICT:
            return Dynamic(DynamicType::INT, pool.dicts.at(args[0].value).size());
        case DynamicType::ERROR:
            return Dynamic(DynamicType::INT, 2);
        case DynamicType::COMPLEX:
            return Dynamic(DynamicType::INT, 2);
        case DynamicType::MATRIX: {
            Tuple t = Tuple(2);
            t[0] = Dynamic(DynamicType::INT, pool.matrices.at(args[0].value).w());
            t[1] = Dynamic(DynamicType::INT, pool.matrices.at(args[0].value).h());
            return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(t));
        }
        default:
            return createError("TypeError", "Expected first arg in (STRING, BYTES, UINT, OBJ, ARR, TUPLE, DICT, ERROR, COMPLEX, MATRIX), got " + to_string(args[0].type), pool);
    }
}

Dynamic DROP(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type != DynamicType::BOOL && args[0].type != DynamicType::INT) {
        return createError("TypeError", "Expected first arg in (BOOL, INT), got " + to_string(args[0].type), pool);
    }
    if (args[0].value == 0) {
        return copy(args[1], pool);
    }
    switch (args[1].type) {
        case DynamicType::STRING:
        case DynamicType::BYTES:
        case DynamicType::UINT: {
            if (args[0].value < pool.bytes.at(args[1].value).size() || (args[0].value + pool.bytes.at(args[1].value).size()) < pool.bytes.at(args[1].value).size()) {
                return Dynamic(args[1].type, pool.bytes.alloc(pool.bytes.at(args[1].value).drop(args[0].value)));
            }
            return createError("IndexError", "Bytes index " + std::to_string(static_cast<int64_t>(args[0].value)) + " out of range", pool);
        }
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            if (args[0].value < pool.tuples.at(args[1].value).size() || (args[0].value + pool.tuples.at(args[1].value).size()) < pool.tuples.at(args[1].value).size()) {
                return Dynamic(args[1].type, pool.tuples.alloc(pool.tuples.at(args[1].value).drop(args[0].value, pool)));
            }
            return createError("IndexError", "Array/Tuple index " + std::to_string(static_cast<int64_t>(args[1].value)) + " out of range", pool);
        }
        default:
            return createError("TypeError", "Expected second arg in (STRING, BYTES, UINT, ARR, TUPLE), got " + to_string(args[0].type), pool);
    }
}

Dynamic TAKE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type == DynamicType::BOOL || args[0].type == DynamicType::INT) {
        if (args[1].type == DynamicType::ITERATE) {
            if (std::bit_cast<int64_t>(args[0].value) < 0) {
                return createError("ValueError", "Unexpected negative int: " + std::to_string(std::bit_cast<int64_t>(args[0].value)), pool);
            }
            if (args[0].value > MAX_TUPLE_SIZE) {
                return createError("ValueError", "Cannot create tuple with size " + std::to_string(args[0].value) + ", the max size is " + std::to_string(MAX_TUPLE_SIZE), pool);
            }
            try {
                return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(pool.iterators[args[1].value].take(args[0].value, pool, interrupt)));
            } catch (const CallPanic& e) {
                return createError("TakeError", e.what(), pool);
            }
        }
        return createError("TypeError", "Expected second arg ITERATE, got " + to_string(args[1].type), pool);
    }
    return createError("TypeError", "Expected first arg in (BOOL, INT), got " + to_string(args[0].type), pool);
}

Dynamic FIND(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type != DynamicType::FUN) {
        return createError("TypeError", "Expected first arg FUN, got " + to_string(args[0].type), pool);
    }
    switch (args[1].type) {
        case DynamicType::OBJ: {
            for (auto [key, value] : pool.objects[args[1].value]) {
                Dynamic d1 = pool.funs.at(args[0].value).call(value, pool.globalScope, pool.globalScope, pool, interrupt);
                if (d1.type == DynamicType::ERROR) {
                    return d1;
                }
                Dynamic d2 = cast(d1, DynamicType::BOOL, pool);
                if (d2.type == DynamicType::ERROR) {
                    free(d1, pool);
                    return d2;
                }
                if (d2.value) {
                    Tuple t(2);
                    t[0] = Dynamic(DynamicType::STRING, pool.bytes.alloc(Bytes(vm::symbolTable.id_to_string.at(key))));
                    t[1] = copy(value, pool);
                    free(d1, pool);
                    return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(t));
                }
                free(d1, pool);
            }
            return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(Tuple()));
        }
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            for (int i = 0; i < pool.tuples.at(args[1].value).size(); i++) {
                Dynamic d1 = pool.funs.at(args[0].value).call(pool.tuples.at(args[1].value)[i], pool.globalScope, pool.globalScope, pool, interrupt);
                if (d1.type == DynamicType::ERROR) {
                    return d1;
                }
                Dynamic d2 = cast(d1, DynamicType::BOOL, pool);
                if (d2.type == DynamicType::ERROR) {
                    free(d1, pool);
                    return d2;
                }
                if (d2.value) {
                    Tuple t(2);
                    t[0] = Dynamic(DynamicType::INT, i);
                    t[1] = copy(pool.tuples.at(args[1].value)[i], pool);
                    free(d1, pool);
                    return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(t));
                }
                free(d1, pool);
            }
            return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(Tuple()));
        }
        case DynamicType::DICT: {
            for (auto [key, value] : pool.dicts.at(args[1].value)) {
                Dynamic d1 = pool.funs.at(args[0].value).call(value, pool.globalScope, pool.globalScope, pool, interrupt);
                if (d1.type == DynamicType::ERROR) {
                    return d1;
                }
                Dynamic d2 = cast(d1, DynamicType::BOOL, pool);
                if (d2.type == DynamicType::ERROR) {
                    free(d1, pool);
                    return d2;
                }
                if (d2.value) {
                    Tuple t(2);
                    t[0] = copy(key, pool);
                    t[1] = copy(value, pool);
                    free(d1, pool);
                    return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(t));
                }
                free(d1, pool);
            }
            return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(Tuple()));
        }
        default:
            return createError("TypeError", "Expected second arg in (OBJ, ARR, TUPLE, DICT), got " + to_string(args[1].type), pool);
    }
}

Dynamic REDUCE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 3);
    if (args[0].type != DynamicType::FUN) {
        return createError("TypeError", "Expected first arg FUN, got " + to_string(args[0].type), pool);
    }
    switch (args[1].type) {
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            Dynamic state = copy(args[2], pool);
            for (int i = 0; i < pool.tuples.at(args[1].value).size(); i++) {
                Dynamic d1 = pool.funs.at(args[0].value).call(state, pool.globalScope, pool.globalScope, pool, interrupt);
                if (d1.type == DynamicType::ERROR) {
                    free(state, pool);
                    return d1;
                }
                if (d1.type != DynamicType::FUN) {
                    free(d1, pool);
                    return createError("ValueError", "Excpected fun of two args, got fun of one arg", pool);
                }
                Dynamic d2 = pool.funs.at(d1.value).call(pool.tuples.at(args[1].value)[i], pool.globalScope, pool.globalScope, pool, interrupt);
                free(state, pool);
                state = d2;
            }
            return state;
        }
        default:
            return createError("TypeError", "Expected second arg ARR/TUPLE, got " + to_string(args[1].type), pool);
    }
}
