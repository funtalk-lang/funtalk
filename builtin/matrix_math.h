Dynamic DET(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type != DynamicType::MATRIX) {
        return createError("TypeError", "Expected first arg MATRIX, got " + to_string(args[0].type), pool);
    }
    if (pool.matrices.at(args[0].value).w() != pool.matrices.at(args[0].value).h()) {
        return createError("ValueError", "Expected square matrix", pool);
    }
    return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(pool.matrices.at(args[0].value).det()));
}

Dynamic TRANSPOSE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    if (args[0].type != DynamicType::MATRIX) {
        return createError("TypeError", "Expected first arg MATRIX, got " + to_string(args[0].type), pool);
    }
    if (pool.matrices.at(args[0].value).w() != pool.matrices.at(args[0].value).h()) {
        return createError("ValueError", "Expected square matrix", pool);
    }
    return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(pool.matrices.at(args[0].value).transpose()));
}

Dynamic GET_ROW(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type != DynamicType::MATRIX) {
        return createError("TypeError", "Expected first arg MATRIX, got " + to_string(args[0].type), pool);
    }
    if (args[1].type != DynamicType::INT) {
        return createError("TypeError", "Expected second arg INT, got " + to_string(args[1].type), pool);
    }
    if (args[1].value >= pool.matrices.at(args[0].value).h()) {
        return createError("IndexError", "Matrix row index " + to_string(args[1].type) + " out of range", pool);
    }
    Tuple t(pool.matrices.at(args[0].value).w());
    for (uint32_t i = 0; i < t.size(); i++) {
        t[i] = Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(pool.matrices.at(args[0].value).get(args[1].value, i)));
    }
    return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(t));
}

Dynamic GET_COL(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    if (args[0].type != DynamicType::MATRIX) {
        return createError("TypeError", "Expected first arg MATRIX, got " + to_string(args[0].type), pool);
    }
    if (args[1].type != DynamicType::INT) {
        return createError("TypeError", "Expected second arg INT, got " + to_string(args[1].type), pool);
    }
    if (args[1].value >= pool.matrices.at(args[0].value).w()) {
        return createError("IndexError", "Matrix column index " + to_string(args[1].type) + " out of range", pool);
    }
    Tuple t(pool.matrices.at(args[0].value).h());
    for (uint32_t i = 0; i < t.size(); i++) {
        t[i] = Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(pool.matrices.at(args[0].value).get(i, args[1].value)));
    }
    return Dynamic(DynamicType::TUPLE, pool.tuples.alloc(t));
}
