#include "dynamic.h"

bool isHashable(Dynamic d, CallPool& pool) {
    if (d.type >= DynamicType::BOOL && d.type <= DynamicType::UINT || d.type == DynamicType::COMPLEX || d.type == DynamicType::MATRIX) {
        return true;
    }
    if (d.type == DynamicType::ARR || d.type == DynamicType::TUPLE) {
        for (Dynamic d1 : pool.tuples.at(d.value)) {
            if (!isHashable(d1, pool)) {
                return false;
            }
        }
        return true;
    }
    if (d.type == DynamicType::FUN) {
        for (Dynamic d1 : pool.funs.at(d.value).getArgs()) {
            if (!isHashable(d1, pool)) {
                return false;
            }
        }
        return true;
    }
    if (d.type == DynamicType::DICT) {
        if ((d.value >> 16) == 1) {
            return false;
        }
        for (auto [key, value] : pool.dicts.at(d.value)) {
            if (!isHashable(value, pool)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

Bytes to_string(Dynamic d, CallPool& pool) {
    switch (d.type) {
        case DynamicType::BOOL:
            return (d.value == 1) ? "TRUE" : "FALSE";
        case DynamicType::INT:
            return from_int64_to_string(d.value);
        case DynamicType::FLOAT:
            return from_double_to_string(std::bit_cast<double>(d.value));
        case DynamicType::CHAR:
            return std::u32string(1, static_cast<char32_t>(d.value));
        case DynamicType::STRING:
        case DynamicType::BYTES:
            return pool.bytes.at(d.value);
        case DynamicType::UINT:
            return from_bigint_to_string(pool.bytes.at(d.value));
        case DynamicType::OBJ:
            return Bytes("Obj at address: ") + from_uint64_to_string(d.value);
        case DynamicType::ARR: {
            std::string s;
            s.reserve(2 + pool.tuples.at(d.value).size() * 5);
            s += "[";
            for (Dynamic d1 : pool.tuples.at(d.value)) {
                s += to_string(d1, pool);
                s += ",";
            }
            s += "]";
            if (s.size() > MAX_BYTES_SIZE) {
                throw CallPanic("Cannot create string larger than " + std::to_string(MAX_BYTES_SIZE));
            }
            return s;
        }
        case DynamicType::TUPLE: {
            std::string s;
            s.reserve(2 + pool.tuples.at(d.value).size() * 5);
            s += "(";
            for (Dynamic d1 : pool.tuples.at(d.value)) {
                s += to_string(d1, pool);
                s += ",";
            }
            s += ")";
            if (s.size() > MAX_BYTES_SIZE) {
                throw CallPanic("Cannot create string larger than " + std::to_string(MAX_BYTES_SIZE));
            }
            return s;
        }
        case DynamicType::DICT: {
            std::string s;
            s.reserve(2 + pool.dicts.at(d.value).size() * 15);
            s += "Dict[";
            for (auto [key, value] : pool.dicts.at(d.value)) {
                s += "(";
                s += to_string(key, pool);
                s += ",";
                s += to_string(value, pool);
                s += "),";
            }
            s += "]";
            if (s.size() > MAX_BYTES_SIZE) {
                throw CallPanic("Cannot create string larger than " + std::to_string(MAX_BYTES_SIZE));
            }
            return s;
        }
        case DynamicType::FUN:
            return Bytes("Fun at address: ") + from_uint64_to_string(d.value);
        case DynamicType::ERROR:
            return pool.bytes.at(d.value & 0xFFFFFFFF) + ": " + pool.bytes.at(d.value >> 32);
        case DynamicType::ITERATE:
            return Bytes("Iterate at address: ") + from_uint64_to_string(d.value);
        case DynamicType::COMPLEX:
            return Bytes("Complex(") + from_double_to_string(pool.complexes.at(d.value).real()) + Bytes(", ") + from_double_to_string(pool.complexes.at(d.value).imag()) + Bytes(")");
        case DynamicType::MATRIX: {
            size_t max = 0;
            for (uint32_t j = 0; j < pool.matrices.at(d.value).h(); j++) {
                for (uint32_t i = 0; i < pool.matrices.at(d.value).w(); i++) {
                    max = std::max(max, std::to_string(pool.matrices.at(d.value).get(i, j)).size());
                }
            }
            std::string s;
            s.reserve((pool.matrices.at(d.value).size() * (max + 2)) + (pool.matrices.at(d.value).h() * 4) + 11);
            s += "Matrix(\n";
            for (uint32_t j = 0; j < pool.matrices.at(d.value).h(); j++) {
                s += "(";
                for (uint32_t i = 0; i < pool.matrices.at(d.value).w(); i++) {
                    std::string s1 = std::to_string(pool.matrices.at(d.value).get(i, j));
                    s += s1;
                    s += std::string(max - s1.size(), ' ');
                    s += ", ";
                }
                s += "),\n";
            }
            s += ")\n";
            if (s.size() > MAX_BYTES_SIZE) {
                throw CallPanic("Cannot create string larger than " + std::to_string(MAX_BYTES_SIZE));
            }
            return s;
        }
        case DynamicType::ASYNC:
            return Bytes("ASYNC at address: ") + from_uint64_to_string(d.value);
        case DynamicType::DEFERRED:
            return Bytes("DEFERRED at address: ") + from_uint64_to_string(d.value);
        default:
            return "Invalid type in std::string to_string(Dynamic d, CallPool& pool)";
    }
}

Bytes to_string(DynamicType type) {
    switch (type) {
        case DynamicType::BOOL:         return "BOOL";
        case DynamicType::INT:          return "INT";
        case DynamicType::FLOAT:        return "FLOAT";
        case DynamicType::CHAR:         return "CHAR";
        case DynamicType::STRING:       return "STRING";
        case DynamicType::BYTES:        return "BYTES";
        case DynamicType::UINT:         return "UINT";
        case DynamicType::OBJ:          return "OBJ";
        case DynamicType::ARR:          return "ARR";
        case DynamicType::TUPLE:        return "TUPLE";
        case DynamicType::DICT:         return "DICT";
        case DynamicType::FUN:          return "FUN";
        case DynamicType::ERROR:        return "ERROR";
        case DynamicType::ITERATE:       return "ITERATE";
        case DynamicType::COMPLEX:      return "COMPLEX";
        case DynamicType::MATRIX:       return "MATRIX";
        case DynamicType::ASYNC:        return "ASYNC";
        case DynamicType::DEFERRED:     return "DEFERRED";
        default:                        return "UNKNOWN DynamicType";
    }
}

DynamicType stringToDynamicType(const Bytes& s) {
    static const std::map<Bytes, DynamicType> typeMap = {
        {"BOOL",        DynamicType::BOOL},
        {"INT",         DynamicType::INT},
        {"FLOAT",       DynamicType::FLOAT},
        {"CHAR",        DynamicType::CHAR},
        {"STRING",      DynamicType::STRING},
        {"BYTES",       DynamicType::BYTES},
        {"UINT",        DynamicType::UINT},
        {"OBJ",         DynamicType::OBJ},
        {"ARR",         DynamicType::ARR},
        {"TUPLE",       DynamicType::TUPLE},
        {"DICT",        DynamicType::DICT},
        {"FUN",         DynamicType::FUN},
        {"ERROR",       DynamicType::ERROR},
        {"ITERATE",     DynamicType::ITERATE},
        {"COMPLEX",     DynamicType::COMPLEX},
        {"MATRIX",      DynamicType::MATRIX},
        {"ASYNC",       DynamicType::ASYNC},
        {"DEFERRED",    DynamicType::DEFERRED},
    };
    auto it = typeMap.find(s);
    if (it != typeMap.end()) {
        return it->second;
    }
    throw std::invalid_argument("Unknown DynamicType string: " + s);
}

void free(Dynamic d, CallPool& pool) {
    switch (d.type) {
        case DynamicType::STRING:
        case DynamicType::BYTES:
        case DynamicType::UINT:
            pool.bytes.free(d.value);
            break;
        case DynamicType::OBJ:
            for (auto [key, value] : pool.objects[d.value]) {
                free(value, pool);
            }
            pool.objects.free(d.value);
            break;
        case DynamicType::ARR:
        case DynamicType::TUPLE:
            for (Dynamic d1 : pool.tuples.at(d.value)) {
                free(d1, pool);
            }
            pool.tuples.free(d.value);
            break;
        case DynamicType::DICT:
            for (auto [key, value] : pool.dicts.at(d.value)) {
                free(value, pool);
                free(key, pool);
            }
            pool.dicts.free(d.value);
            break;
        case DynamicType::FUN:
            for (Dynamic d1 : pool.funs.at(d.value).getArgs()) {
                free(d1, pool);
            }
            pool.funs.free(d.value);
            break;
        case DynamicType::ERROR:
            pool.bytes.free(d.value & 0xFFFFFFFF);
            pool.bytes.free(d.value >> 32);
            break;
        case DynamicType::ITERATE:
            free(pool.iterators[d.value].init, pool);
            free(pool.iterators[d.value].fun, pool);
            for (const Iterate::Transformer& t : pool.iterators[d.value].transformers) {
                free(t.fun, pool);
                free(t.init, pool);
                free(t.state, pool);
            }
            pool.iterators.free(d.value);
            break;
        case DynamicType::COMPLEX:
            pool.complexes.free(d.value);
            break;
        case DynamicType::MATRIX:
            pool.matrices.free(d.value);
            break;
        case DynamicType::ASYNC:
            reinterpret_cast<Promise*>(d.value)->count--;
            break;
        case DynamicType::DEFERRED:
            reinterpret_cast<DeferredPromise*>(d.value)->count--;
            break;
        default:
            break;
    }
}

Dynamic copy(Dynamic d, CallPool& pool) {
    switch (d.type) {
        case DynamicType::STRING:
        case DynamicType::BYTES:
        case DynamicType::UINT:
            return Dynamic(d.type, pool.bytes.alloc(pool.bytes.at(d.value)));
        case DynamicType::OBJ: {
            uint16_t i = pool.objects.alloc(pool.objects[d.value]);
            for (auto& [key, value] : pool.objects[i]) {
                value = copy(value, pool);
            }
            return Dynamic(DynamicType::OBJ, i);
        }
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            Tuple t = pool.tuples.at(d.value);
            for (Dynamic& d1 : t) {
                d1 = copy(d1, pool);
            }
            return Dynamic(d.type, pool.tuples.alloc(t));
        }
        case DynamicType::DICT: {
            Dict dict;
            for (auto [key, value] : pool.dicts.at(d.value)) {
                dict[copy(key, pool)] = copy(value, pool);
            }
            return Dynamic(DynamicType::DICT, pool.dicts.alloc(dict));
        }
        case DynamicType::FUN: {
            FunView f = pool.funs.at(d.value);
            Tuple args = f.getArgs();
            for (Dynamic& d1 : args) {
                d1 = copy(d1, pool);
            }
            return Dynamic(DynamicType::FUN, pool.funs.alloc(Fun(f.getBegin(), args, f.getSize())));
        }
        case DynamicType::ERROR:
            return Dynamic(DynamicType::ERROR, pool.bytes.alloc(pool.bytes.at(d.value & 0xFFFFFFFF)) | (uint64_t(pool.bytes.alloc(pool.bytes.at(d.value >> 32))) << 32));
        case DynamicType::ITERATE: {
            uint16_t i = pool.iterators.alloc(pool.iterators[d.value]);
            Iterate& it = pool.iterators[d.value];
            it.init = copy(it.init, pool);
            it.init = copy(it.fun, pool);
            for (Iterate::Transformer& t : it.transformers) {
                t.fun = copy(t.fun, pool);
                t.init = copy(t.init, pool);
                t.state = copy(t.state, pool);
            }
            return Dynamic(DynamicType::ITERATE, i);
        }
        case DynamicType::COMPLEX:
            return Dynamic(DynamicType::COMPLEX, pool.complexes.alloc(pool.complexes.at(d.value)));
        case DynamicType::MATRIX:
            return Dynamic(DynamicType::MATRIX, pool.matrices.alloc(pool.matrices.at(d.value)));
        case DynamicType::ASYNC:
            reinterpret_cast<Promise*>(d.value)->count++;
            return d;
        case DynamicType::DEFERRED:
            reinterpret_cast<DeferredPromise*>(d.value)->count++;
            return d;
        default:
            return d;
    }
}

Dynamic copy(Dynamic d, CallPool& from, CallPool& to) {
    switch (d.type) {
        case DynamicType::STRING:
        case DynamicType::BYTES:
        case DynamicType::UINT:
            return Dynamic(d.type, to.bytes.alloc(from.bytes.at(d.value)));
        case DynamicType::OBJ: {
            throw CallPanic("Cannot copy OBJ to another pool");
        }
        case DynamicType::ARR:
        case DynamicType::TUPLE: {
            Tuple t = from.tuples.at(d.value);
            for (Dynamic& d1 : t) {
                d1 = copy(d1, from, to);
            }
            return Dynamic(d.type, to.tuples.alloc(t));
        }
        case DynamicType::DICT: {
            Dict dict = from.dicts.at(d.value);
            for (auto& [key, value] : dict) {
                value = copy(value, from, to);
            }
            return Dynamic(DynamicType::DICT, to.dicts.alloc(dict));
        }
        case DynamicType::FUN: {
            FunView f = from.funs.at(d.value);
            Tuple args = f.getArgs();
            for (Dynamic& d1 : args) {
                d1 = copy(d1, from, to);
            }
            return Dynamic(DynamicType::FUN, to.funs.alloc(Fun(f.getBegin(), args, f.getSize())));
        }
        case DynamicType::ERROR:
            return Dynamic(DynamicType::ERROR, to.bytes.alloc(from.bytes.at(d.value & 0xFFFFFFFF)) | (uint64_t(to.bytes.alloc(from.bytes.at(d.value >> 32))) << 32));
        case DynamicType::ITERATE: {
            uint16_t i = to.iterators.alloc(from.iterators[d.value]);
            Iterate& it = to.iterators[i];
            it.fun = copy(it.fun, from, to);
            it.init = copy(it.init, from, to);
            for (Iterate::Transformer& t : it.transformers) {
                t.fun = copy(t.fun, from, to);
                t.init = copy(t.init, from, to);
                t.state = copy(t.state, from, to);
            }
            return Dynamic(DynamicType::ITERATE, i);
        }
        case DynamicType::COMPLEX:
            return Dynamic(DynamicType::COMPLEX, to.complexes.alloc(from.complexes.at(d.value)));
        case DynamicType::MATRIX:
            return Dynamic(DynamicType::MATRIX, to.matrices.alloc(from.matrices.at(d.value)));
        case DynamicType::ASYNC:
            reinterpret_cast<Promise*>(d.value)->count++;
            return d;
        case DynamicType::DEFERRED:
            reinterpret_cast<DeferredPromise*>(d.value)->count++;
            return d;
        default:
            return d;
    }
}

Dynamic createObject(CallScope& callScope) {
    return Dynamic(DynamicType::OBJ, callScope.pool.objects.alloc(callScope));
}

Dynamic createArray(CallScope& callScope) {
    uint32_t i = callScope.pool.tuples.alloc(Tuple(callScope));
    return Dynamic(DynamicType::ARR, i);
}

Dynamic createTuple(CallScope& callScope) {
    uint32_t i = callScope.pool.tuples.alloc(Tuple(callScope));
    return Dynamic(DynamicType::TUPLE, i);
}

Dynamic createError(const Bytes& type, const Bytes& value, CallPool& pool) {
    return Dynamic(DynamicType::ERROR, pool.bytes.alloc(type) | (uint64_t(pool.bytes.alloc(value)) << 32));
}

Dynamic createPromise(Tuple t, std::chrono::milliseconds milliseconds, CallPool& pool) {
    Promise* promise = new Promise(std::move(t), milliseconds, pool);
    vm::promises.push(promise);
    return Dynamic(DynamicType::ASYNC, reinterpret_cast<uint64_t>(promise));
}

Dynamic createDeferredPromise(Tuple t, CallPool& pool, std::atomic<bool>& interrupt) {
    DeferredPromise* promise = new DeferredPromise(std::move(t), pool, interrupt);
    vm::deferreds.push(promise);
    return Dynamic(DynamicType::DEFERRED, reinterpret_cast<uint64_t>(promise));
}

Dynamic* Scope::operator[](uint64_t i) {
    if (object->find(i) != object->end()) {
        return &(*object)[i];
    }
    if (parent == nullptr) {
        return nullptr;
    }
    return (*parent)[i];
}

void createObject(Object*& obj, Scope& scope, CallPool& pool) {
    uint32_t i = pool.objects.alloc(Object(nullptr, pool));
    obj = &pool.objects[i];
    scope = obj->getScope();
}

Object::Object(CallScope& callScope) : scope(this, &callScope.parent), pool(&callScope.pool) {
    Scope s = Scope(this, &callScope.current);
    CallScope callScope1 = CallScope(callScope.p, scope, s, callScope.pool, callScope.interrupt);
    const CompressedToken*& p = callScope1.p;
    Scope& parent = callScope1.parent;
    Scope& current = callScope1.current;
    CallPool& pool = callScope1.pool;
    std::atomic<bool>& interrupt = callScope1.interrupt;
    RecursionCountGuard recursionCountGuard(pool);
    if (interrupt) {
        throw Interrupt();
    }
    uint64_t n = p->value;
    p++;
    while (n != 0) {
        if (p->type == CompressedTokenType::MOV) {
            mov(callScope1);
        } else if (p->type == CompressedTokenType::FUNC_DECL) {
            func_decl(p, current, pool);
        } else if (p->type == CompressedTokenType::MESSAGE_DECL) {
            message_decl(p, current, pool);
        } else {
            std::cout << '\n';
            printCompressedToken(*p);
            throw CallPanic("Unknown command in object");
        }
        n--;
    }
}

void movHelper(Object& o, size_t id, Dynamic f(CallScope&), CallScope& callScope) {
    Dynamic d = f(callScope);
    if (o.find(id) != o.end()) {
        free(o[id], callScope.pool);
    }
    o[id] = d;
}

void Object::mov(CallScope& callScope) {
    RecursionCountGuard recursionCountGuard(callScope.pool);
    const CompressedToken*& p = callScope.p;
    if (fields.size() > MAX_OBJ_SIZE) {
        throw CallPanic("Cannot create object larger than " + std::to_string(MAX_OBJ_SIZE));
    }
    p++;
    if (p->type != CompressedTokenType::NAME) {
        throw CallPanic("This error cannot be real in mov");
    }
    size_t id = p->value;
    p++;
    if (p->type >= CompressedTokenType::BOOL && p->type <= CompressedTokenType::STRING) {
        movHelper(*this, id, [](CallScope& callScope) {
            return copy(Dynamic(static_cast<DynamicType>(callScope.p->type), callScope.p->value), callScope.pool);
        }, callScope);
        p++;
    } else if (p->type == CompressedTokenType::NAME) {
        movHelper(*this, id, [](CallScope& callScope) {
            Dynamic* var = callScope.current[callScope.p->value];
            if (var) {
                return copy(*var, callScope.pool);
            } else {
                throw RuntimeError("Variable '", RuntimeError::ID(callScope.p->value), "' is not defined");
            }
        }, callScope);
        p++;
    } else if (p->type == CompressedTokenType::CODE) {
        movHelper(*this, id, createObject, callScope);
    } else if (p->type == CompressedTokenType::ARR) {
        movHelper(*this, id, createArray, callScope);
    } else if (p->type == CompressedTokenType::TUPLE) {
        movHelper(*this, id, createTuple, callScope);
    } else if (p->type == CompressedTokenType::CALL) {
        movHelper(*this, id, call, callScope);
    } else {
        throw CallPanic("Invalid CompressedTokenType in mov");
    }
}

void Object::func_decl(const CompressedToken*& p, Scope& scope, CallPool& pool) {
    Fun fun(p);
    const CompressedToken* end1 = p + (p->value & 0xFFFFFFFF) + 1;
    const CompressedToken* end2 = end1 + (p->value >> 32);
    p++;
    if (p->type != CompressedTokenType::NAME) {
        throw CallPanic("This error cannot be real in func_decl 1");
    }
    size_t id = p->value;
    p++;
    while (p != end1) {
        if (p->type != CompressedTokenType::NAME) {
            throw CallPanic("This error cannot be real in func_decl 2");
        }
        p++;
    }
    if (fields.find(id) != fields.end()) {
        if (fields[id].type == DynamicType::FUN) {
            free(fields[id], pool);
        } else {
            throw RuntimeError("Cannot declare function '", RuntimeError::ID(id), "'. This variable is already defined and is not a function");
        }
    }
    uint32_t i = pool.funs.alloc(fun);
    fields[id] = Dynamic(DynamicType::FUN, i);
    p = end2;
}

void Object::message_decl(const CompressedToken*& p, Scope& scope, CallPool& pool) {
    messages[p[1].value] = p;
    p += p->value >> 48;
}

template <typename Receiver, typename Sender, typename Func>
void sendHelper(Receiver& receiver, uint64_t id, Sender& sender, CallScope& callScope, RuntimeErrorInMessage e, Func f) {
    const CompressedToken*& p = callScope.p;
    Scope& parent = callScope.parent;
    Scope& current = callScope.current;
    CallPool& pool = callScope.pool;
    std::atomic<bool>& interrupt = callScope.interrupt;
    if (p[2].type >= CompressedTokenType::BOOL && p[2].type <= CompressedTokenType::STRING) {
        Dynamic d = Dynamic(static_cast<DynamicType>(p[2].type), p[2].value);
        if (!f(receiver, d, id, sender, parent, current, pool, interrupt)) {
            pool.errorHandler(e, *pool.globalScope.getObject(), pool, interrupt);
        }
        p += 3;
        return;
    }
    if (p[2].type == CompressedTokenType::NAME) {
        Dynamic* a = current[p[2].value];
        if (a == nullptr) {
            throw RuntimeErrorInMessage(id, "Variable '", RuntimeError::ID(p[2].value), "' is not defined");
        }
        if (!f(receiver, *a, id, sender, parent, current, pool, interrupt)) {
            pool.errorHandler(e, *pool.globalScope.getObject(), pool, interrupt);
        }
        p += 3;
        return;
    }
    p += 2;
    Dynamic d;
    switch (p->type) {
        case CompressedTokenType::CODE:
            d = createObject(callScope);
            break;
        case CompressedTokenType::ARR:
            d = createArray(callScope);
            break;
        case CompressedTokenType::TUPLE:
            d = createTuple(callScope);
            break;
        case CompressedTokenType::CALL:
            d = call(callScope);
            break;
        default:
            throw CallPanic("Invalid type in sendHelper");
    }
    if (!f(receiver, d, id, sender, parent, current, pool, interrupt)) {
        pool.errorHandler(e, *pool.globalScope.getObject(), pool, interrupt);
    }
    free(d, pool);
}

bool Object::send_(Dynamic d, uint64_t id, Object& sender, std::atomic<bool>& interrupt) {
    CallPool& pool = *this->pool;
    RecursionCountGuard recursionCountGuard(pool);
    if (const auto& it = messages.find(id); it != messages.end()) {
        const CompressedToken* p = it->second;
        if (p->type == CompressedTokenType::BUILT_IN_MESSAGE) {
            if (p->value >= pool.builtInMessages.size()) {
                throw CallPanic("Index out of range in Object::send_");
            }
            pool.builtInMessages[p->value](d, pool, interrupt);
            return true;
        }
        uint32_t argNameCount = (p->value & 0xFFFF) - 1;
        Object obj(&this->scope, pool);
        uint32_t argsCount = (d.type == DynamicType::TUPLE && pool.tuples.at(d.value).size() > 1) ? std::min(argNameCount, uint32_t(pool.tuples.at(d.value).size())) : std::min(1U, argNameCount);
        if (argsCount == 1) {
            obj.fields[p[2].value] = d;
        } else if (argsCount != 0) {
            if (d.type != DynamicType::TUPLE) {
                throw CallPanic("This error cannot be real in Object::send_");
            }
            TupleView t = pool.tuples.at(d.value);
            for (size_t i = 0; i < argsCount; i++) {
                obj.fields[p[i + 2].value] = t[i];
            }
        }
        Scope& s = obj.scope;
        uint32_t commandsCount = (p->value >> 16) & 0xFFFFFFFF;
        p += argNameCount + 2;
        CallScope callScope = CallScope(p, this->scope, s, pool, interrupt);
        while (commandsCount != 0) {
            try {
                if (p->type == CompressedTokenType::MOV) {
                    mov(callScope);
                } else if (p->type == CompressedTokenType::FUNC_DECL) {
                    func_decl(p, s, pool);
                } else if (p->type == CompressedTokenType::MESSAGE_DECL) {
                    message_decl(p, s, pool);
                } else if (p->type == CompressedTokenType::SEND) {
                    if (p[1].type == CompressedTokenType::NONLOCAL_MESSAGE) {
                        sendHelper(sender, p[1].value, *this, callScope, RuntimeErrorInMessage(id, " Sender cannot handle message ", RuntimeError::ID(p[1].value)), [](Object& sender, Dynamic d, uint64_t id, Object& obj, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) {
                            return sender.send(d, id, obj, pool, interrupt);
                        });
                    } else if (p[1].type == CompressedTokenType::MESSAGE) {
                        Dynamic* receiver = s[p[1].value & 0xFFFFFFFF];
                        if (receiver == nullptr) {
                            throw RuntimeErrorInMessage(id, "Variable '", RuntimeError::ID(p[1].value & 0xFFFFFFFF), "' is not defined");
                        }
                        uint64_t id1 = p[1].value >> 32;
                        switch (receiver->type) {
                            case DynamicType::OBJ: {
                                Object& receiverObj = pool.objects[receiver->value];
                                sendHelper(receiverObj, id1, *this, callScope, RuntimeErrorInMessage(id, " OBJ variable '", RuntimeError::ID(p[1].value & 0xFFFFFFFF), "' cannot handle message ", RuntimeError::ID(id1)), [](Object& receiverObj, Dynamic d, uint64_t id, Object& obj, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) {
                                    return receiverObj.send(d, id, obj, pool, interrupt);
                                });
                                break;
                            }
                            case DynamicType::ARR: {
                                uint32_t i = pool.tuples.edit(receiver->value);
                                Tuple& receiverArr = pool.tuples[i];
                                *receiver = Dynamic(DynamicType::ARR, i);
                                sendHelper(receiverArr, id1, id, callScope, RuntimeErrorInMessage(id, " ARR variable '", RuntimeError::ID(p[1].value & 0xFFFFFFFF), "' cannot handle message ", RuntimeError::ID(id1)), [](Tuple& receiverArr, Dynamic d, uint64_t id, uint64_t senderID, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) {
                                    return receiverArr.send(d, id, senderID, pool, interrupt);
                                });
                                break;
                            }
                            case DynamicType::DICT: {
                                uint32_t i = pool.dicts.edit(receiver->value);
                                Dict& receiverDict = pool.dicts[i];
                                *receiver = Dynamic(DynamicType::DICT, i);
                                sendHelper(receiverDict, id1, id, callScope, RuntimeErrorInMessage(id, " DICT variable '", RuntimeError::ID(p[1].value & 0xFFFFFFFF), "' cannot handle message ", RuntimeError::ID(id1)), [](Dict& receiverDict, Dynamic d, uint64_t id, uint64_t senderID, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) {
                                    return receiverDict.send(d, id, senderID, pool, interrupt);
                                });
                                break;
                            }
                            case DynamicType::ITERATE: {
                                Iterate& receiverIterate = pool.iterators[receiver->value];
                                sendHelper(receiverIterate, id1, id, callScope, RuntimeErrorInMessage(id, " ITERATE variable '", RuntimeError::ID(p[1].value & 0xFFFFFFFF), "' cannot handle message ", RuntimeError::ID(id1)), [](Iterate& receiverIterate, Dynamic d, uint64_t id, uint64_t senderID, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) {
                                    return receiverIterate.send(d, id, senderID, pool);
                                });
                                break;
                            }
                            default: {
                                pool.errorHandler(RuntimeErrorInMessage(id, receiver->type, " variable '", RuntimeError::ID(p[1].value & 0xFFFFFFFF), "' cannot handle any message"), *pool.globalScope.getObject(), pool, interrupt);
                                nullptr_t n;
                                sendHelper(n, id1, *this, callScope, RuntimeErrorInMessage(0), [](nullptr_t&, Dynamic, uint64_t, Object&, Scope&, Scope&, CallPool&, std::atomic<bool>&) {
                                    return true;
                                });
                                break;
                            }
                        }
                    } else {
                        std::cout << '\n';
                        printCompressedToken(*p);
                        throw CallPanic("Unknown command in Object::send 1");
                    }
                } else {
                    std::cout << '\n';
                    printCompressedToken(*p);
                    throw CallPanic("Unknown command in Object::send 2");
                }
            } catch (const RuntimeError& e) {
                throw RuntimeErrorInMessage(id, e);
            }
            commandsCount--;
        }
        return true;
    } else if (static_cast<ID>(id) != ID::NOT_IMPLEMENTED) {
        if (const auto& it = messages.find(static_cast<uint64_t>(ID::NOT_IMPLEMENTED)); it != messages.end()) {
            send(d, static_cast<uint64_t>(ID::NOT_IMPLEMENTED), sender, pool, interrupt);
        }
    }
    return false;
}

bool Object::send(Dynamic d, uint64_t id, Object& sender, CallPool& pool, std::atomic<bool>& interrupt) {
    if (interrupt) {
        throw Interrupt();
    }
    if (this->pool != &pool) {
        Dynamic d1 = copy(d, pool, *this->pool);
        bool b;
        try {
            b = send_(d1, id, sender, interrupt);
        } catch (const std::exception& e) {
            free(d1, *this->pool);
            throw;
        }
        free(d1, *this->pool);
        return b;
    }
    bool b = send_(d, id, sender, interrupt);
    return b;
}

#include "tuple.cpp"
#include "matrix.cpp"
#include "bytes.cpp"

Fun::Fun(FunView fun) : Fun(*fun) {}

Dynamic Fun::call(Dynamic d, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) const {
    RecursionCountGuard recursionCountGuard(pool);
    if (interrupt) {
        throw Interrupt();
    }
    if (begin->type == CompressedTokenType::BUILT_IN_FUN) {
        if (size == (args.size() - 1)) {
            if ((begin->value >> 32) >= pool.builtInFuns.size()) {
                throw CallPanic("Index out of range in Fun::call");
            }
            Tuple args1 = args;
            args1[size] = d;
            Dynamic res = pool.builtInFuns[begin->value >> 32](args1, pool, interrupt);
            return res;
        }
        Fun fun = *this;
        fun.args[fun.size] = d;
        for (Dynamic& d1 : fun.args) {
            d1 = copy(d1, pool);
        }
        fun.size++;
        return Dynamic(DynamicType::FUN, pool.funs.alloc(fun));
    }
    if (size == args.size()) {
        Object obj(&parent, pool);
        Scope s = Scope(&obj, &current);
        const CompressedToken* p = begin + 2;
        for (size_t i = 0; i < args.size(); i++) {
            obj[p->value] = args[i];
            p++;
        }
        obj[p->value] = d;
        p++;
        CallScope callScope = CallScope(p, parent, s, pool, interrupt);
        Dynamic res = ::call(callScope);
        return res;
    }
    Fun fun = *this;
    fun.args[fun.size] = d;
    for (Dynamic& d1 : fun.args) {
        d1 = copy(d1, pool);
    }
    fun.size++;
    return Dynamic(DynamicType::FUN, pool.funs.alloc(fun));
}

Dict::Dict(DictView d) : Dict(*d) {}

bool Dict::send(Dynamic d, uint64_t id, uint64_t senderID, CallPool& pool, std::atomic<bool>& interrupt) {
    RecursionCountGuard recursionCountGuard(pool);
    switch (static_cast<ID>(id)) {
        case ID::SET: {
            Validator v(Validator::Arg(DynamicType::TUPLE, {Validator::Arg(), Validator::Arg()}), "DICT.set");
            std::unique_ptr<const RuntimeError> e = v.get(d, pool);
            if (e) {
                throw RuntimeErrorInMessage(senderID, *e);
            }
            if (!isHashable(pool.tuples.at(d.value)[0], pool)) {
                throw RuntimeErrorInMessage(senderID, "in DICT.set: expected hashable key");
            }
            Dynamic key = copy(pool.tuples.at(d.value)[0], pool);
            if (find(key) != end()) {
                free(key, pool);
            }
            (*this)[key] = copy(pool.tuples.at(d.value)[1], pool);
            return true;
        }
        case ID::MAP: {
            Validator v(Validator::Arg(DynamicType::FUN), "DICT.map");
            std::unique_ptr<const RuntimeError> e = v.get(d, pool);
            if (e) {
                throw RuntimeErrorInMessage(senderID, *e);
            }
            for (auto& [key, value] : *this) {
                Dynamic d1 = pool.funs.at(d.value).call(key, pool.globalScope, pool.globalScope, pool, interrupt);
                if (d1.type != DynamicType::FUN) {
                    free(value, pool);
                    value = createError("CallError", "Cannot call " + to_string(d1.type), pool);
                    free(d1, pool);
                    continue;
                }
                Dynamic d2 = pool.funs.at(d1.value).call(value, pool.globalScope, pool.globalScope, pool, interrupt);
                free(value, pool);
                value = d2;
                free(d1, pool);
            }
            return true;
        }
    }
    return false;
}

uint16_t Dict::hash() const {
    uint16_t s = 0;
    for (auto [key, value] : *this) {
        s += funtalk::hash(key) ^ (funtalk::hash(value) * 0xAAAA);
    }
    return s;
}

Tuple Iterate::take(uint32_t n, CallPool& pool, std::atomic<bool>& interrupt) {
    Tuple t = Tuple(n);
    Dynamic state = copy(init, pool);
    uint32_t k = 0;
    uint32_t j = 0;
    for (uint32_t i = 0; i < transformers.size(); i++) {
        free(transformers[i].state, pool);
        transformers[i].state = copy(transformers[i].init, pool);
    }
    std::set<Dynamic> set;
    while (j < n) {
        if (k == 65536 * 2048) {
            throw CallPanic("iterations limit");
        }
        if (interrupt) {
            throw Interrupt();
        }
        bool filtered = false;
        Dynamic state1 = copy(state, pool);
        for (uint32_t i = 0; i < transformers.size(); i++) {
            if (transformers[i].type == MAP) {
                Dynamic state2 = pool.funs.at(transformers[i].fun.value).call(state1, pool.globalScope, pool.globalScope, pool, interrupt);
                free(state1, pool);
                state1 = state2;
            } else if (transformers[i].type == SCAN) {
                Dynamic d = pool.funs.at(transformers[i].fun.value).call(transformers[i].state, pool.globalScope, pool.globalScope, pool, interrupt);
                if (d.type != DynamicType::FUN) {
                    free(d, pool);
                    free(state1, pool);
                    free(state, pool);
                    throw CallPanic("expected FUN in scan");
                }
                Dynamic state2 = pool.funs.at(d.value).call(state1, pool.globalScope, pool.globalScope, pool, interrupt);
                free(d, pool);
                free(state1, pool);
                free(transformers[i].state, pool);
                transformers[i].state = state2;
                state1 = copy(state2, pool);
            } else if (transformers[i].type == FILTER) {
                Dynamic state2 = pool.funs.at(transformers[i].fun.value).call(state1, pool.globalScope, pool.globalScope, pool, interrupt);
                free(transformers[i].state, pool);
                transformers[i].state = state2;
                if (transformers[i].state.type != DynamicType::BOOL && transformers[i].state.type != DynamicType::INT) {
                    throw CallPanic("expected fun that returns BOOL or INT in filter");
                }
                if (transformers[i].state.value == 0) {
                    filtered = true;
                    break;
                }
            } else if (transformers[i].type == UNIQUE) {
                Dynamic state2 = pool.funs.at(transformers[i].fun.value).call(state1, pool.globalScope, pool.globalScope, pool, interrupt);
                if (!isHashable(state2, pool)) {
                    free(state2, pool);
                    throw CallPanic("expected fun that returns hashable value in unique");
                }
                free(transformers[i].state, pool);
                transformers[i].state = state2;
                if (set.find(transformers[i].state) != set.end()) {
                    filtered = true;
                    break;
                }
                set.insert(transformers[i].state);
            } else {
                throw CallPanic("This error cannot be real in Iterate");
            }
        }
        if (!filtered) {
            t[j] = state1;
            j++;
        }
        Dynamic state2 = pool.funs.at(fun.value).call(state, pool.globalScope, pool.globalScope, pool, interrupt);
        free(state, pool);
        state = state2;
        k++;
    }
    free(state, pool);
    return std::move(t);
}

bool Iterate::send(Dynamic d, uint64_t id, uint64_t senderID, CallPool& pool) {
    if (transformers.size() > MAX_ITERATE_SIZE) {
        throw CallPanic("Cannot create generator larger than " + std::to_string(MAX_ITERATE_SIZE));
    }
    switch (static_cast<ID>(id)) {
        case ID::MAP: {
            if (d.type != DynamicType::FUN) {
                throw RuntimeErrorInMessage(senderID, "Expected FUN in ITERATE.map, got ", d.type);
            }
            transformers.push_back({MAP, copy(d, pool)});
            return true;
        }
        case ID::SCAN: {
            if (d.type != DynamicType::TUPLE) {
                throw RuntimeErrorInMessage(senderID, "Expected TUPLE in ITERATE.scan, got ", d.type);
            }
            if (pool.tuples.at(d.value).size() != 2) {
                throw RuntimeErrorInMessage(senderID, "Expected 2 args in ITERATE.scan, got ", pool.tuples.at(d.value).size());
            }
            if (pool.tuples.at(d.value)[0].type != DynamicType::FUN) {
                throw RuntimeErrorInMessage(senderID, "Expected FUN as first arg in ITERATE.scan, got ", pool.tuples.at(d.value)[0].type);
            }
            transformers.push_back({SCAN, copy(pool.tuples.at(d.value)[0], pool), copy(pool.tuples.at(d.value)[1], pool)});
            return true;
        }
        case ID::FILTER: {
            if (d.type != DynamicType::FUN) {
                throw RuntimeErrorInMessage(senderID, "Expected FUN in ITERATE.filter, got ", d.type);
            }
            transformers.push_back({FILTER, copy(d, pool)});
            return true;
        }
    }
    return false;
}

Promise::Promise(Tuple t, std::chrono::milliseconds timeout, CallPool& pool) : end_time(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() + timeout)), pool(&pool), interrupt(false) {
    if (t.size() < 2) {
        throw RuntimeError("ASYNC expects tuple with size > 1");
    }
    if (t[0].type != DynamicType::FUN) {
        throw RuntimeError("ASYNC expects first tuple element FUN");
    }
    for (Dynamic& d : t) {
        d = copy(d, pool);
    }
    future = promise.get_future();
    try {
        vm::threads.push([this, t, &pool]() {
            Done d(done);
            Dynamic state = copy(t[0], pool);
            for (size_t i = 1; i < t.size(); i++) {
                if (state.type != DynamicType::FUN) {
                    for (Dynamic d : t) {
                        free(d, pool);
                    }
                    free(state, pool);
                    promise.set_value(createError("TypeError", "Cannot call " + to_string(state.type), pool));
                    return;
                }
                Dynamic d;
                try {
                    d = pool.funs.at(state.value).call(t[i], pool.globalScope, pool.globalScope, pool, interrupt);
                } catch (const std::exception& e) {
                    for (Dynamic d : t) {
                        free(d, pool);
                    }
                    promise.set_exception(std::current_exception());
                    return;
                }
                free(state, pool);
                state = d;
            }
            for (Dynamic d : t) {
                free(d, pool);
            }
            promise.set_value(state);
        });
    } catch (...) {
        done = true;
        promise.set_exception(std::current_exception());
    }
}

DeferredPromise::DeferredPromise(Tuple t, CallPool& pool, std::atomic<bool>& interrupt) : pool(&pool) {
    if (t.size() < 2) {
        throw RuntimeError("ASYNC expects tuple with size > 1");
    }
    if (t[0].type != DynamicType::FUN) {
        throw RuntimeError("ASYNC expects first tuple element FUN");
    }
    for (Dynamic& d : t) {
        d = copy(d, pool);
    }
    task = [this, t, &pool, &interrupt](std::promise<Dynamic> promise) {
        Dynamic state = copy(t[0], pool);
        for (size_t i = 1; i < t.size(); i++) {
            if (state.type != DynamicType::FUN) {
                for (Dynamic d : t) {
                    free(d, pool);
                }
                free(state, pool);
                promise.set_value(createError("TypeError", "Cannot call " + to_string(state.type), pool));
                return;
            }
            Dynamic d;
            try {
                d = pool.funs.at(state.value).call(t[i], pool.globalScope, pool.globalScope, pool, interrupt);
            } catch (const std::exception& e) {
                for (Dynamic d : t) {
                    free(d, pool);
                }
                promise.set_exception(std::current_exception());
                return;
            }
            free(state, pool);
            state = d;
        }
        for (Dynamic d : t) {
            free(d, pool);
        }
        promise.set_value(state);
    };
}

Dynamic call(CallScope& callScope) {
    const CompressedToken*& p = callScope.p;
    Scope& parent = callScope.parent;
    Scope& current = callScope.current;
    CallPool& pool = callScope.pool;
    std::atomic<bool>& interrupt = callScope.interrupt;
    RecursionCountGuard recursionCountGuard(pool);
    if (interrupt) {
        throw Interrupt();
    }
    if (p->type >= CompressedTokenType::BOOL && p->type <= CompressedTokenType::STRING) {
        Dynamic d = copy(Dynamic(static_cast<DynamicType>(p->type), p->value), pool);
        p++;
        return d;
    }
    if (p->type == CompressedTokenType::NAME) {
        Dynamic* var = current[p->value];
        if (var == nullptr) {
            throw RuntimeError("Variable '", RuntimeError::ID(p->value), "' is not defined");
        }
        p++;
        return copy(*var, pool);
    }
    if (p->type == CompressedTokenType::CODE) {
        return createObject(callScope);
    }
    if (p->type == CompressedTokenType::ARR) {
        return createArray(callScope);
    }
    if (p->type == CompressedTokenType::TUPLE) {
        return createTuple(callScope);
    }
    p++;
    uint64_t n = 2;
    Dynamic e;
    Dynamic fun;
    while (n) {
        if (p->type == CompressedTokenType::CALL) {
            if (fun.type != DynamicType::FUN && e.type != DynamicType::ERROR) {
                fun = call(callScope);
                if (fun.type != DynamicType::FUN) {
                    e = createError("CallError", "Cannot call " + to_string(fun.type), pool);
                }
            } else {
                Dynamic d = call(callScope);
                if (e.type == DynamicType::ERROR) {
                    free(d, pool);
                    return e;
                }
                Dynamic res = pool.funs.at(fun.value).call(d, parent, current, pool, interrupt);
                free(d, pool);
                free(fun, pool);
                return res;
            }
        } else if (p->type == CompressedTokenType::NAME) {
            if (fun.type != DynamicType::FUN && e.type != DynamicType::ERROR) {
                Dynamic* var = current[p->value];
                if (var == nullptr) {
                    throw RuntimeError("Variable '", RuntimeError::ID(p->value), "' is not defined");
                }
                p++;
                if (var->type != DynamicType::FUN) {
                    e = createError("CallError", "Cannot call " + to_string(var->type), pool);
                } else {
                    fun = copy(*var, pool);
                }
            } else {
                Dynamic* var = current[p->value];
                if (var == nullptr) {
                    throw RuntimeError("Variable '", RuntimeError::ID(p->value), "' is not defined");
                }
                p++;
                if (e.type == DynamicType::ERROR) {
                    return e;
                }
                Dynamic res = pool.funs.at(fun.value).call(*var, parent, current, pool, interrupt);
                free(fun, pool);
                return res;
            }
        } else if (p->type >= CompressedTokenType::BOOL && p->type <= CompressedTokenType::STRING) {
            if (fun.type != DynamicType::FUN && e.type != DynamicType::ERROR) {
                e = createError("CallError", "Cannot call " + to_string(static_cast<DynamicType>(p->type)), pool);
                p++;
            } else {
                Dynamic d = Dynamic(static_cast<DynamicType>(p->type), p->value);
                p++;
                if (e.type == DynamicType::ERROR) {
                    return e;
                }
                Dynamic res = pool.funs.at(fun.value).call(d, parent, current, pool, interrupt);
                free(fun, pool);
                return res;
            }
        } else if (p->type == CompressedTokenType::CODE) {
            if (fun.type != DynamicType::FUN && e.type != DynamicType::ERROR) {
                e = createError("CallError", "Cannot call " + to_string(DynamicType::OBJ), pool);
                free(createObject(callScope), pool);
            } else {
                Dynamic d = createObject(callScope);
                if (e.type == DynamicType::ERROR) {
                    free(d, pool);
                    return e;
                }
                Dynamic res = pool.funs.at(fun.value).call(d, parent, current, pool, interrupt);
                free(d, pool);
                free(fun, pool);
                return res;
            }
        } else if (p->type == CompressedTokenType::ARR) {
            if (fun.type != DynamicType::FUN && e.type != DynamicType::ERROR) {
                e = createError("CallError", "Cannot call " + to_string(DynamicType::ARR), pool);
                free(createArray(callScope), pool);
            } else {
                Dynamic d = createArray(callScope);
                if (e.type == DynamicType::ERROR) {
                    free(d, pool);
                    return e;
                }
                Dynamic res = pool.funs.at(fun.value).call(d, parent, current, pool, interrupt);
                free(d, pool);
                free(fun, pool);
                return res;
            }
        } else if (p->type == CompressedTokenType::TUPLE) {
            if (fun.type != DynamicType::FUN && e.type != DynamicType::ERROR) {
                e = createError("CallError", "Cannot call " + to_string(DynamicType::TUPLE), pool);
                free(createTuple(callScope), pool);
            } else {
                Dynamic d = createTuple(callScope);
                if (e.type == DynamicType::ERROR) {
                    free(d, pool);
                    return e;
                }
                Dynamic res = pool.funs.at(fun.value).call(d, parent, current, pool, interrupt);
                free(d, pool);
                free(fun, pool);
                return res;
            }
        } else {
            std::cout << '\n';
            printCompressedToken(*p);
            throw CallPanic("This error cannot be real 1");
        }
        n--;
    }
    std::cout << '\n';
    printCompressedToken(*p);
    throw CallPanic("This error cannot be real 2");
}

namespace funtalk {
    uint16_t hash(uint16_t a) {
        return a * 0x5555;
    }
    uint16_t hash(uint32_t a) {
        return hash(uint16_t(a)) + hash(uint16_t(a >> 16));
    }
    uint16_t hash(uint64_t a) {
        return hash(uint32_t(a)) + hash(uint32_t(a >> 32));
    }
    uint16_t hash(std::string_view s) {
        return std::hash<std::string_view>()(s);
    }
    uint16_t hash(Dynamic d) {
        return -d.value * hash(uint16_t(d.type));
    }
    uint16_t hash(TupleView t) {
        uint16_t s = 0;
        for (uint32_t i = 0; i < t.size(); i++) {
            s ^= hash(t[i]) * (i + 1) * 2;
        }
        return s;
    }
    uint16_t hash(FunView f) {
        return f.hash();
    }
    uint16_t hash(DictView d) {
        return d.hash();
    }
    uint16_t hash(std::complex<double> c) {
        return std::bit_cast<__uint128_t>(c) * 0x55555555;
    }
    uint16_t hash(MatrixView m) {
        uint64_t s = 0;
        for (uint32_t i = 0; i < m.size(); i++) {
            s ^= std::bit_cast<uint64_t>(m.begin()[i]) * (i + 1) * 0xAAAA;
            s ^= s >> 1;
        }
        return s;
    }
    uint16_t hash(BytesView b) {
        uint16_t s = 0;
        for (uint32_t i = 0; i < b.size(); i++) {
            s ^= uint16_t(b[i]) * (i + 1) * 2;
        }
        return s;
    }
}
