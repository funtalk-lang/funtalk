#include "tuple.h"

Tuple::Tuple(CallScope& callScope) {
    const CompressedToken*& p = callScope.p;
    Scope& parent = callScope.parent;
    Scope& current = callScope.current;
    CallPool& pool = callScope.pool;
    std::atomic<bool>& interrupt = callScope.interrupt;
    RecursionCountGuard recursionCountGuard(pool);
    size_ = p->value;
    if (size_ == 0) {
        arr = nullptr;
        p++;
        return;
    }
    if (size_ > MAX_TUPLE_SIZE) {
        throw CallPanic("Cannot create tuple larger than " + std::to_string(MAX_TUPLE_SIZE));
    }
    arr = std::make_shared<Dynamic[]>(size_);
    p++;
    size_t j = 0;
    while (j != size_) {
        if (p->type >= CompressedTokenType::BOOL && p->type <= CompressedTokenType::STRING) {
            arr[j] = copy(Dynamic(static_cast<DynamicType>(p->type), p->value), pool);
            p++;
        } else if (p->type == CompressedTokenType::NAME) {
            Dynamic* var = current[p->value];
            if (var == nullptr) {
                throw RuntimeError("Variable '", RuntimeError::ID(p->value), "' is not defined");
            } else {
                arr[j] = copy(*var, pool);
            }
            p++;
        } else if (p->type == CompressedTokenType::CODE) {
            arr[j] = createObject(callScope);
        } else if (p->type == CompressedTokenType::ARR) {
            arr[j] = createArray(callScope);
        } else if (p->type == CompressedTokenType::TUPLE) {
            arr[j] = createTuple(callScope);
        } else if (p->type == CompressedTokenType::CALL) {
            arr[j] = call(callScope);
        } else {
            throw CallPanic("Invalid command in tuple: " + to_string(p->type));
        }
        j++;
    }
    if (j != size_) {
        throw CallPanic("This error cannot be real in tuple");
    }
}

bool Tuple::send(Dynamic d, uint64_t id, uint64_t senderID, CallPool& pool, std::atomic<bool>& interrupt) {
    RecursionCountGuard recursionCountGuard(pool);
    switch (static_cast<ID>(id)) {
        case ID::SET: {
            Validator v(Validator::Arg(DynamicType::TUPLE, {Validator::Arg(DynamicType::INT), Validator::Arg()}), "ARR.set");
            std::unique_ptr<const RuntimeError> e = v.get(d, pool);
            if (e) {
                throw RuntimeErrorInMessage(senderID, *e);
            }
            if (pool.tuples.at(d.value)[0].value >= size()) {
                throw RuntimeErrorInMessage(senderID, "Index out of range in ARR.set");
            }
            free((*this)[pool.tuples.at(d.value)[0].value], pool);
            (*this)[pool.tuples.at(d.value)[0].value] = copy(pool.tuples.at(d.value)[1], pool);
            return true;
        }
        case ID::PUSH: {
            push_back(copy(d, pool));
            return true;
        }
        case ID::MAP: {
            Validator v(Validator::Arg(DynamicType::FUN), "ARR.map");
            std::unique_ptr<const RuntimeError> e = v.get(d, pool);
            if (e) {
                throw RuntimeErrorInMessage(senderID, *e);
            }
            for (uint32_t i = 0; i < size(); i++) {
                Dynamic d1 = pool.funs.at(d.value).call(Dynamic(DynamicType::INT, i), pool.globalScope, pool.globalScope, pool, interrupt);
                if (d1.type != DynamicType::FUN) {
                    free((*this)[i], pool);
                    (*this)[i] = createError("CallError", "Cannot call " + to_string(d1.type), pool);
                    free(d1, pool);
                    continue;
                }
                Dynamic d2 = pool.funs.at(d1.value).call((*this)[i], pool.globalScope, pool.globalScope, pool, interrupt);
                free((*this)[i], pool);
                (*this)[i] = d2;
                free(d1, pool);
            }
            return true;
        }
        case ID::SCAN: {
            Validator v(Validator::Arg(DynamicType::TUPLE, {Validator::Arg(DynamicType::FUN), Validator::Arg()}), "ARR.scan");
            std::unique_ptr<const RuntimeError> e = v.get(d, pool);
            if (e) {
                throw RuntimeErrorInMessage(senderID, *e);
            }
            Dynamic state = pool.tuples.at(d.value)[1];
            for (Dynamic& item : *this) {
                Dynamic d1 = pool.funs.at(pool.tuples.at(d.value)[0].value).call(state, pool.globalScope, pool.globalScope, pool, interrupt);
                if (d1.type != DynamicType::FUN) {
                    free(item, pool);
                    item = createError("CallError", "Cannot call " + to_string(d1.type), pool);
                    free(d1, pool);
                    continue;
                }
                Dynamic item2 = pool.funs.at(d1.value).call(item, pool.globalScope, pool.globalScope, pool, interrupt);
                free(item, pool);
                item = item2;
                free(d1, pool);
                state = item;
            }
            return true;
        }
    }
    return false;
}

Dynamic Tuple::concat(DynamicType type, const Tuple& t, CallPool& pool) const {
    Tuple r = Tuple(size() + t.size());
    for (uint32_t i = 0; i < size(); i++) {
        r[i] = copy(begin()[i], pool);
    }
    for (uint32_t i = 0; i < t.size(); i++) {
        r[i + size()] = copy(t[i], pool);
    }
    return Dynamic(type, pool.tuples.alloc(r));
}
