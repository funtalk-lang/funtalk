Dynamic EQ(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return EQ(args[0], args[1], pool);
}

Dynamic NEQ(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return NEQ(args[0], args[1], pool);
}

Dynamic LT(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return LT(args[0], args[1], pool);
}

Dynamic LE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return LE(args[0], args[1], pool);
}

Dynamic GT(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return GT(args[0], args[1], pool);
}

Dynamic GE(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return GE(args[0], args[1], pool);
}
