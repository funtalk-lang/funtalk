Dynamic NOT(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 1);
    return NOT(args[0], pool);
}

Dynamic AND(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return AND(args[0], args[1], pool);
}

Dynamic OR(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return OR(args[0], args[1], pool);
}

Dynamic XOR(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return XOR(args[0], args[1], pool);
}

Dynamic SHL(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return SHL(args[0], args[1], pool);
}

Dynamic SHR(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) {
    assert(args.size() == 2);
    return SHR(args[0], args[1], pool);
}
