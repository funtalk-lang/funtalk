Dynamic ARG(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    if (args[0].type == DynamicType::COMPLEX) {
        return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::arg(pool.complexes.at(args[0].value))));
    } else {
        return createError("TypeError", "Expected first arg COMPLEX, got " + to_string(args[0].type), pool);
    }
}

Dynamic CONJ(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    if (args[0].type == DynamicType::COMPLEX) {
        return Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(std::conj(pool.complexes.at(args[0].value))));
    } else {
        return createError("TypeError", "Expected first arg COMPLEX, got " + to_string(args[0].type), pool);
    }
}

Dynamic NORM(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    if (args[0].type == DynamicType::COMPLEX) {
        return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(std::norm(pool.complexes.at(args[0].value))));
    } else {
        return createError("TypeError", "Expected first arg COMPLEX, got " + to_string(args[0].type), pool);
    }
}

Dynamic POLAR(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    Dynamic a = cast(args[0], DynamicType::FLOAT, pool);
    if (args[0].type != DynamicType::BOOL && args[0].type != DynamicType::INT && args[0].type != DynamicType::FLOAT) {
        free(a, pool);
        return createError("TypeError", "Expected first arg in (BOOL, INT, FLOAT), got " + to_string(args[0].type), pool);
    }
    Dynamic b = cast(args[1], DynamicType::FLOAT, pool);
    if (args[1].type != DynamicType::BOOL && args[1].type != DynamicType::INT && args[1].type != DynamicType::FLOAT) {
        free(b, pool);
        return createError("TypeError", "Expected second arg in (BOOL, INT, FLOAT), got " + to_string(args[1].type), pool);
    }
    return Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(std::polar(std::bit_cast<double>(a.value), std::bit_cast<double>(b.value))));
}
