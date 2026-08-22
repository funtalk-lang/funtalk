Dynamic INT(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::INT, pool);
}

Dynamic ROUND(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    Dynamic d = cast(args[0], DynamicType::FLOAT, pool);
    if (d.type == DynamicType::ERROR) {
        return d;
    }
    return Dynamic(DynamicType::INT, int64_t(std::round(std::bit_cast<double>(d.value))));
}

Dynamic FLOOR(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    Dynamic d = cast(args[0], DynamicType::FLOAT, pool);
    if (d.type == DynamicType::ERROR) {
        return d;
    }
    return Dynamic(DynamicType::INT, int64_t(std::floor(std::bit_cast<double>(d.value))));
}

Dynamic CEIL(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    Dynamic d = cast(args[0], DynamicType::FLOAT, pool);
    if (d.type == DynamicType::ERROR) {
        return d;
    }
    return Dynamic(DynamicType::INT, int64_t(std::ceil(std::bit_cast<double>(d.value))));
}

Dynamic FLOAT(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::FLOAT, pool);
}

Dynamic CHAR(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::CHAR, pool);
}

Dynamic STRING(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::STRING, pool);
}

Dynamic BYTES(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::BYTES, pool);
}

Dynamic UINT(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::UINT, pool);
}

Dynamic ARR(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::ARR, pool);
}

Dynamic TUPLE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::TUPLE, pool);
}

Dynamic DICT(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::DICT, pool);
}

Dynamic ITERATE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type != DynamicType::FUN) {
        return createError("TypeError", "Expected first arg FUN, got " + to_string(args[0].type), pool);
    }
    return Dynamic(DynamicType::ITERATE, pool.iterators.alloc(copy(args[1], pool), copy(args[0], pool)));
}

Dynamic COMPLEX(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    if (args[0].type == DynamicType::ARR || args[0].type == DynamicType::TUPLE) {
        TupleView t = pool.tuples.at(args[0].value);
        Dynamic d1 = cast(t[0], DynamicType::FLOAT, pool);
        if (d1.type == DynamicType::ERROR) {
            return d1;
        }
        Dynamic d2 = cast(t[1], DynamicType::FLOAT, pool);
        if (d2.type == DynamicType::ERROR) {
            return d2;
        }
        return Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(std::complex<double>(std::bit_cast<double>(d1.value), std::bit_cast<double>(d2.value))));
    }
    return createError("TypeError", "Expected first arg ARR/TUPLE, got " + to_string(args[0].type), pool);
}

Dynamic MATRIX(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return cast(args[0], DynamicType::MATRIX, pool);
}

Dynamic EVAL(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 5);
    if (args[0].type != DynamicType::STRING) {
        return createError("TypeError", "Expected first arg STRING, got " + to_string(args[0].type), pool);
    }
    if (args[1].type != DynamicType::OBJ) {
        return createError("TypeError", "Expected second arg OBJ, got " + to_string(args[1].type), pool);
    }
    if (args[4].type != DynamicType::STRING) {
        return createError("TypeError", "Expected fourth arg STRING, got " + to_string(args[4].type), pool);
    }
    std::unique_ptr<VM> vm = nullptr;
    try {
        vm = std::make_unique<VM>(pool.bytes.at(args[0].value), &pool.objects[args[1].value], interrupt);
    } catch (const FunTalkTokenizationError& e) {
        return createError("TokenizationError", e.what(), pool);
    } catch (const RuntimeError& e) {
        return createError("InternalError", vm::to_string(e), pool);
    } catch (const std::exception& e) {
        return createError("InternalError", e.what(), pool);
    }
    vm->messageObj->send(args[3], vm::symbolTable.string_to_id.at("main"), *pool.globalScope.getObject(), pool, interrupt);
    if (args[2].type == DynamicType::ARR || args[2].type == DynamicType::TUPLE) {
        Tuple t = pool.tuples.at(args[2].value);
        std::vector<std::string> v;
        v.reserve(t.size());
        for (Dynamic d : t) {
            v.push_back(to_string(d, pool));
        }
        vm->main(v, interrupt);
    } else {
        vm->main({to_string(args[2], pool)}, interrupt);
    }
    if (auto it = vm::symbolTable.string_to_id.find(pool.bytes.at(args[4].value)); it != vm::symbolTable.string_to_id.end()) {
        if (auto it1 = vm->messageObj->find(it->second); it1 != vm->messageObj->end()) {
            Dynamic res = copy(it1->second, pool);
            return res;
        }
        return createError("KeyError", "Key " + pool.bytes.at(args[4].value) + " does not exist", pool);
    }
    return createError("KeyError", "Key " + pool.bytes.at(args[4].value) + " out of symbolTable", pool);
}

Dynamic ASYNC(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type != DynamicType::ARR && args[0].type != DynamicType::TUPLE) {
        return createError("TypeError", "Expected first arg ARR/TUPLE, got " + to_string(args[0].type), pool);
    }
    if (args[1].type != DynamicType::INT) {
        return createError("TypeError", "Expected second arg INT, got " + to_string(args[1].type), pool);
    }
    if (args[1].value == 0) {
        return createDeferredPromise(pool.tuples.at(args[0].value), vm::vmpool[0], interrupt);
    }
    return createPromise(pool.tuples.at(args[0].value), std::chrono::milliseconds(args[1].value), vm::vmpool[0]);
}

Dynamic TYPE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return Dynamic(DynamicType::STRING, pool.bytes.alloc(to_string(args[0].type)));
}

Dynamic CAST(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[1].type == DynamicType::STRING) {
        try {
            return cast(args[0], stringToDynamicType(pool.bytes.at(args[1].value)), pool);
        } catch (const std::invalid_argument& e) {
            return createError("ValueError", pool.bytes.at(args[1].value) + " is not a name of type", pool);
        }
    }
    return createError("TypeError", "Expected second arg STRING, got " + to_string(args[1].type), pool);
}
