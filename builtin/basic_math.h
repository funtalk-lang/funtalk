Dynamic ABS(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return ABS(args[0], pool);
}

Dynamic ADD(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return ADD(args[0], args[1], pool);
}

Dynamic NEG(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return NEG(args[0], pool);
}

Dynamic MUL(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return MUL(args[0], args[1], pool);
}

Dynamic INV(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return INV(args[0], pool);
}

Dynamic MOD(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return MOD(args[0], args[1], pool);
}

Dynamic POW(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return POW(args[0], args[1], pool);
}

Dynamic MULM(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return MULM(args[0], args[1], pool);
}

Dynamic INVM(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return INVM(args[0], pool);
}

Dynamic POWM(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return POWM(args[0], args[1], pool);
}
