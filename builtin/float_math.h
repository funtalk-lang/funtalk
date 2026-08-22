#define MATH_OP_FUNTALK(name, func) \
Dynamic name(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) { \
    assert(args.size() == 1); \
    if (args[0].type == DynamicType::BOOL || args[0].type == DynamicType::INT || args[0].type == DynamicType::FLOAT) { \
        Dynamic a = cast(args[0], DynamicType::FLOAT, pool); \
        return Dynamic(DynamicType::FLOAT, std::bit_cast<uint64_t>(func(std::bit_cast<double>(a.value)))); \
    } \
    if (args[0].type == DynamicType::COMPLEX) { \
        return Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(func(pool.complexes.at(args[0].value)))); \
    } \
    if (args[0].type == DynamicType::MATRIX) { \
        return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(func(pool.matrices.at(args[0].value)))); \
    } \
    return createError("TypeError", "Expected first arg in (BOOL, INT, FLOAT, COMPLEX, MATRIX), got " + to_string(args[0].type), pool); \
}

MATH_OP_FUNTALK(SIN, std::sin);
MATH_OP_FUNTALK(COS, std::cos);
MATH_OP_FUNTALK(TAN, std::tan);
MATH_OP_FUNTALK(ASIN, std::asin);
MATH_OP_FUNTALK(ACOS, std::acos);
MATH_OP_FUNTALK(ATAN, std::atan);
MATH_OP_FUNTALK(SINH, std::sinh);
MATH_OP_FUNTALK(COSH, std::cosh);
MATH_OP_FUNTALK(TANH, std::tanh);
MATH_OP_FUNTALK(EXP, std::exp);
#define LOG2_FUNTALK(x) ([]<typename T>(const T& val) { \
if constexpr (requires { typename T::value_type; }) \
    return std::log(val) / std::log(2.0); \
    else \
        return std::log2(val); \
}(x))
MATH_OP_FUNTALK(LOG, std::log);
MATH_OP_FUNTALK(LOG2, LOG2_FUNTALK);
MATH_OP_FUNTALK(LOG10, std::log10);

#undef LOG2_FUNTALK
#undef MATH_OP_FUNTALK

#define MATH_OPM_FUNTALK(name, func) \
Dynamic name(TupleView args, CallPool& pool, std::atomic<bool>& interrupt) { \
    assert(args.size() == 1); \
    if (args[0].type == DynamicType::MATRIX) { \
        return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(func(pool.matrices.at(args[0].value)))); \
    } \
    return createError("TypeError", "Expected first arg MATRIX, got " + to_string(args[0].type), pool); \
}

MATH_OPM_FUNTALK(SINM, funtalk::sinm);
MATH_OPM_FUNTALK(COSM, funtalk::cosm);
MATH_OPM_FUNTALK(TANM, funtalk::tanm);
MATH_OPM_FUNTALK(ASINM, funtalk::asinm);
MATH_OPM_FUNTALK(ACOSM, funtalk::acosm);
MATH_OPM_FUNTALK(ATANM, funtalk::atanm);
MATH_OPM_FUNTALK(SINHM, funtalk::sinhm);
MATH_OPM_FUNTALK(COSHM, funtalk::coshm);
MATH_OPM_FUNTALK(TANHM, funtalk::tanhm);
MATH_OPM_FUNTALK(EXPM, funtalk::expm);
MATH_OPM_FUNTALK(LOGM, funtalk::logm);
MATH_OPM_FUNTALK(LOG2M, funtalk::log2m);
MATH_OPM_FUNTALK(LOG10M, funtalk::log10m);

#undef MATH_OPM_FUNTALK
