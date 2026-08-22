#pragma once
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <cstdint>
#include <fstream>
#include <memory>
#include <cstring>
#include <utility>
#include <cmath>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <future>
#include <complex>
#include <cassert>
#include "gc.h"
#include "../compiler/compressor.h"

using nullptr_t = decltype(nullptr);

constexpr size_t MAX_VMS = 2;
constexpr size_t MAX_THREADS = 1;
constexpr int32_t MAX_CLIENTS = 8;
constexpr size_t MAX_TASKS = 8;
constexpr size_t MAX_OBJ_SIZE = 64;
constexpr size_t MAX_ARR_SIZE = 64;
constexpr size_t MAX_TUPLE_SIZE = 64;
constexpr size_t MAX_DICT_SIZE = 64;
constexpr size_t MAX_ITERATE_SIZE = 16;
constexpr size_t MAX_MATRIX_SIZE = 256;
constexpr int32_t RECURSION_LIMIT = 32;

enum class DynamicType : uint64_t {
    BOOL, INT, FLOAT, CHAR, STRING, BYTES, UINT, OBJ, ARR, TUPLE, DICT, FUN, ERROR, ITERATE, COMPLEX, MATRIX, ASYNC, DEFERRED
};

struct Dynamic {
    DynamicType type;
    uint64_t value;
    Dynamic() : type(DynamicType::BOOL), value(0) {}
    Dynamic(DynamicType type, uint64_t value) : type(type), value(value) {}
    auto operator<=>(const Dynamic& a) const = default;
};

bool isHashable(Dynamic d, CallPool& pool);

#include "bytes.h"

Bytes to_string(Dynamic d, CallPool& pool);
Bytes to_string(DynamicType type);
DynamicType stringToDynamicType(const Bytes& s);

using BuitinFun = Dynamic (*)(TupleView, CallPool& pool, std::atomic<bool>& interrupt);
using BuitinMessage = void (*)(Dynamic, CallPool& pool, std::atomic<bool>& interrupt);

void free(Dynamic d, CallPool& pool);
Dynamic copy(Dynamic d, CallPool& pool);
Dynamic copy(Dynamic d, CallPool& from, CallPool& to);

struct CallScope {
    const CompressedToken*& p;
    Scope& parent;
    Scope& current;
    CallPool& pool;
    std::atomic<bool>& interrupt;
    CallScope(const CompressedToken*& p, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) : p(p), parent(parent), current(current), pool(pool), interrupt(interrupt) {}
};

Dynamic createObject(CallScope& callScope);
Dynamic createArray(CallScope& callScope);
Dynamic createTuple(CallScope& callScope);
Dynamic createError(const Bytes& type, const Bytes& value, CallPool& pool);

namespace vm {
    template <class T>
    struct Queue {
        std::queue<T> q;
        std::mutex mutex;
        void push(T t) {
            std::lock_guard<std::mutex> lock(mutex);
            q.push(t);
        }
        bool pop_to(T& out) {
            std::lock_guard<std::mutex> lock(mutex);
            if (q.empty()) return false;
            out = std::move(q.front());
            q.pop();
            return true;
        }
        bool empty() {
            std::lock_guard<std::mutex> lock(mutex);
            return q.empty();
        }
    };
    class Thread {
        std::queue<std::function<void()>> tasks;
        std::mutex mutex;
        std::condition_variable cv;
        std::thread worker_thread;
        std::atomic<bool> stop = false;
    public:
        Thread() {
            worker_thread = std::thread([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->mutex);
                        this->cv.wait(lock, [this]() {
                            return this->stop || !this->tasks.empty();
                        });
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    if (task) {
                        task();
                    }
                }
            });
        }
        ~Thread() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                stop = true;
            }
            cv.notify_one();
            if (worker_thread.joinable()) {
                worker_thread.join();
            }
        }
        void push(std::function<void()> task) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                tasks.push(std::move(task));
            }
            cv.notify_one();
        }
        size_t size() {
            return tasks.size();
        }
    };
    class ThreadPool {
        Thread arr[MAX_THREADS]{};
        uint32_t i = 0;
    public:
        void push(std::function<void()> task) {
            if (arr[i].size() >= MAX_TASKS) {
                throw CallPanic("Too many tasks");
            }
            arr[i].push(std::move(task));
            i = (i + 1) % MAX_THREADS;
        }
    };
    inline ThreadPool threads;
    inline Queue<Promise*> promises;
    inline std::queue<DeferredPromise*> deferreds;
}

Dynamic createPromise(Tuple t, std::chrono::milliseconds milliseconds, CallPool& pool);
Dynamic createDeferredPromise(Tuple t, CallPool& pool, std::atomic<bool>& interrupt);

class RecursionError : public std::exception {
public:
    const char* what() const noexcept override {
        return "RecursionError";
    }
};

class Interrupt : public std::exception {
public:
    const char* what() const noexcept override {
        return "Interrupt";
    }
};

class RuntimeError : public std::exception {
public:
    enum ArgType {
        STRING, DYNAMIC_TYPE, NAME, INT
    };
    union ArgValue {
        uint64_t i;
        const char* s;
        ArgValue(uint64_t i) : i(i) {}
        ArgValue(const char* s) : s(s) {}
    };
    struct ID {
        uint64_t id;
        ID(uint64_t id) : id(id) {}
    };
    struct Arg {
        ArgType type;
        ArgValue value;
        Arg(const char* s) : type(STRING), value(s) {}
        Arg(DynamicType type) : type(DYNAMIC_TYPE), value(static_cast<uint64_t>(type)) {}
        Arg(ID id) : type(NAME), value(id.id) {}
        Arg(uint64_t i) : type(INT), value(i) {}
    };
    std::vector<Arg> args;
    template<typename... Args>
    explicit RuntimeError(Args&&... arguments) {
        (args.emplace_back(std::forward<Args>(arguments)), ...);
    }
    virtual ~RuntimeError() noexcept override = default;
    const char* what() const noexcept override {
        return "RuntimeError";
    }
};

class RuntimeErrorInMessage : public RuntimeError {
public:
    const uint64_t messageID;
    template<typename... Args>
    RuntimeErrorInMessage(uint64_t messageID, Args&&... arguments) : RuntimeError(std::forward<Args>(arguments)...), messageID(messageID) {}
    RuntimeErrorInMessage(uint64_t messageID, const RuntimeError& e) : RuntimeError(e), messageID(messageID) {}
    const char* what() const noexcept override {
        return "RuntimeErrorInMessage";
    }
};

class Scope {
    Object* object;
    Scope* parent;
public:
    Scope() : object(nullptr), parent(nullptr) {}
    Scope(Object* object, Scope* parent) : object(object), parent(parent) {}
    Scope(const Scope& scope) : object(scope.object), parent(scope.parent) {}
    Scope(Scope&& scope) : object(scope.object), parent(scope.parent) {
        scope.object = nullptr;
        scope.parent = nullptr;
    }
    Object* getObject() const {
        return object;
    }
    Scope* getParent() const {
        return parent;
    }
    Dynamic* operator[](uint64_t i);
    Scope& operator=(const Scope&) = default;
    bool operator==(const Scope& scope) const = default;
};

void createObject(Object*& obj, Scope& scope, CallPool& pool);

class Object {
protected:
    SmallHashMap<uint64_t, Dynamic> fields;
    SmallHashMap<uint64_t, const CompressedToken*> messages;
    Scope scope;
    CallPool* pool;
public:
    Object(Scope* parent, CallPool& pool) : scope(this, parent), pool(&pool) {}
    Object(CallScope& callScope);
    Object(const Object& obj) : fields(obj.fields), messages(obj.messages), scope(this, obj.scope.getParent()), pool(obj.pool) {}
    void mov(CallScope& callScope);
    friend void movHelper(Object& o, size_t id, Dynamic f(CallScope&), CallScope& callScope);
    void func_decl(const CompressedToken*& p, Scope& scope, CallPool& pool);
    void message_decl(const CompressedToken*& p, Scope& scope, CallPool& pool);
    bool send_(Dynamic d, uint64_t id, Object& sender, std::atomic<bool>& interrupt);
    bool send(Dynamic d, uint64_t id, Object& sender, CallPool& pool, std::atomic<bool>& interrupt);
    Dynamic& at(uint64_t i) {
        auto it = fields.find(i);
        assert(it != fields.end());
        return it->second;
    }
    const Dynamic& at(uint64_t i) const {
        auto it = fields.find(i);
        assert(it != fields.cend());
        return it->second;
    }
    Dynamic& operator[](uint64_t i) {
        return fields[i];
    }
    const Dynamic& operator[](uint64_t i) const {
        return at(i);
    }
    const CompressedToken* getMessage(uint64_t i) const {
        auto it = messages.find(i);
        assert(it != messages.cend());
        return it->second;
    }
    void setMessage(uint64_t i, CompressedToken* p) {
        if (messages.size() > MAX_OBJ_SIZE) {
            throw CallPanic("Cannot create object larger than " + std::to_string(MAX_OBJ_SIZE));
        }
        messages[i] = p;
    }
    Scope getScope() const {
        return scope;
    }
    CallPool* getPool() const {
        return pool;
    }
    SmallHashMap<uint64_t, Dynamic>::const_iterator begin() const {
        return fields.cbegin();
    }
    SmallHashMap<uint64_t, Dynamic>::const_iterator end() const {
        return fields.cend();
    }
    SmallHashMap<uint64_t, Dynamic>::iterator begin() {
        return fields.begin();
    }
    SmallHashMap<uint64_t, Dynamic>::iterator end() {
        return fields.end();
    }
    SmallHashMap<uint64_t, Dynamic>::const_iterator find(uint64_t i) const {
        return fields.find(i);
    }
    size_t size() const {
        return fields.size();
    }
    Object& operator=(const Object&) = default;
    bool operator==(const Object& o) const = default;
};

#include "tuple.h"

class Fun {
    const CompressedToken* begin;
    Tuple args;
    size_t size;
public:
    Fun() : begin(nullptr), size(0) {}
    Fun(const CompressedToken* begin, Tuple args, size_t size) : begin(begin), args(std::move(args)), size(size) {}
    Fun(const CompressedToken* begin) : begin(begin), args((begin->type == CompressedTokenType::BUILT_IN_FUN) ? (begin->value & 0xFFFFFFFF) : (begin->value & 0xFFFFFFFF) - 2), size(0) {}
    Fun(FunView fun);
    Fun(const Fun& fun) = default;
    Fun(Fun&& fun) : begin(fun.begin) {
        args = std::move(fun.args);
        size = fun.size;
        fun.size = 0;
    }
    const CompressedToken* getBegin() const {
        return begin;
    }
    TupleView getArgs() const {
        return args;
    }
    size_t getSize() const {
        return size;
    }
    Dynamic call(Dynamic d, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) const;
    Fun& operator=(const Fun&) = default;
    uint16_t hash() const {
        return reinterpret_cast<size_t>(begin) * ((size << 1) | 1);
    }
    bool operator==(const Fun&) const = default;
};

class FunView {
    const Fun* p;
public:
    FunView() : p(nullptr) {}
    FunView(const Fun& fun) : p(&fun) {}
    FunView(const FunView& fun) = default;
    const CompressedToken* getBegin() const {
        return p->getBegin();
    }
    TupleView getArgs() const {
        return p->getArgs();
    }
    size_t getSize() const {
        return p->getSize();
    }
    uint16_t hash() const {
        return p->hash();
    }
    Dynamic call(Dynamic d, Scope& parent, Scope& current, CallPool& pool, std::atomic<bool>& interrupt) const {
        return p->call(d, parent, current, pool, interrupt);
    }
    const Fun& operator*() const {
        return *p;
    }
    FunView& operator=(const FunView& fun) = default;
    bool operator==(FunView f) const {
        return *p == *f.p;
    }
    bool operator==(nullptr_t) const {
        return p == nullptr;
    }
};

Dynamic call(CallScope& callScope);

class Dict : public std::map<Dynamic, Dynamic> {
public:
    Dict() {}
    Dict(DictView d);
    Dynamic& operator[](const Dynamic& d) {
        if (size() > MAX_DICT_SIZE) {
            throw CallPanic("Cannot create dict larger than " + std::to_string(MAX_DICT_SIZE));
        }
        return std::map<Dynamic, Dynamic>::operator[](d);
    }
    bool send(Dynamic d, uint64_t id, uint64_t senderID, CallPool& pool, std::atomic<bool>& interrupt);
    uint16_t hash() const;
};

class DictView {
    const Dict* p;
public:
    DictView(const Dict& dict) : p(&dict) {}
    const Dynamic& operator[](Dynamic d) const {
        return (*p).at(d);
    }
    const Dict& operator*() const {
        return *p;
    }
    size_t size() const {
        return p->size();
    }
    std::map<Dynamic, Dynamic>::const_iterator begin() const {
        return p->begin();
    }
    std::map<Dynamic, Dynamic>::const_iterator end() const {
        return p->end();
    }
    std::map<Dynamic, Dynamic>::const_iterator find(Dynamic d) const {
        return p->find(d);
    }
    uint16_t hash() const {
        return p->hash();
    }
    bool operator==(DictView d) const {
        return *p == *d.p;
    }
};

struct Iterate {
    Dynamic init;
    Dynamic fun;
    enum Type {
        MAP, SCAN, FILTER, UNIQUE
    };
    struct Transformer {
        Type type;
        Dynamic fun;
        Dynamic init;
        Dynamic state;
    };
    std::vector<Transformer> transformers;
    Iterate() {}
    Iterate(Dynamic init, Dynamic fun) : init(init), fun(std::move(fun)) {}
    Tuple take(uint32_t n, CallPool& pool, std::atomic<bool>& interrupt);
    bool send(Dynamic d, uint64_t id, uint64_t senderID, CallPool& pool);
};

#include "matrix.h"

class Promise {
    std::promise<Dynamic> promise;
    std::future<Dynamic> future;
    std::atomic<bool> done = false;
    const std::chrono::system_clock::time_point end_time;
    CallPool* pool;
    struct Task {
        std::function<void(Dynamic, CallPool&)> main;
        std::function<void(std::exception_ptr)> error;
    };
    std::queue<Task> tasks;
    std::mutex tasks_mutex;
public:
    struct Done {
        std::atomic<bool>* const done;
        Done(std::atomic<bool>& done) : done(&done) {}
        ~Done() {
            (*done) = true;
        }
    };
    std::atomic<uint32_t> count = 1;
    std::atomic<bool> interrupt;
    Promise(Tuple t, std::chrono::milliseconds timeout, CallPool& pool);
    void push(std::function<void(Dynamic, CallPool&)> task, std::function<void(std::exception_ptr)> error) {
        std::lock_guard<std::mutex> lock(tasks_mutex);
        tasks.push({std::move(task), std::move(error)});
    }
    void exec();
    bool isDone() const {
        return done;
    }
    bool isExpired() const {
        return std::chrono::system_clock::now() > end_time;
    }
    Dynamic get() {
        return future.get();
    }
};

class DeferredPromise {
    CallPool* pool;
    struct Task {
        std::function<void(Dynamic, CallPool&)> main;
        std::function<void(std::exception_ptr)> error;
    };
    std::queue<Task> tasks;
    std::function<void(std::promise<Dynamic>)> task;
public:
    uint32_t count = 1;
    DeferredPromise(Tuple t, CallPool& pool, std::atomic<bool>& interrupt);
    void push(std::function<void(Dynamic, CallPool&)> task, std::function<void(std::exception_ptr)> error) {
        tasks.push({std::move(task), std::move(error)});
    }
    void exec();
};

enum class ID {
    NOT_IMPLEMENTED = 1, SET, PUSH, MAP, SCAN, FILTER, UNIQUE, MAIN
};

using ErrorHandler = void (*)(const RuntimeErrorInMessage&, Object&, CallPool&, std::atomic<bool>&);

struct CallPool {
    int32_t count = 0;
    Pool<Bytes, BytesView>                  bytes       = Pool<Bytes, BytesView>(count);
    SmartPool<Tuple, TupleView>             tuples      = SmartPool<Tuple, TupleView>(count);
    GC<Object>                              objects     = GC<Object>(count);
    Pool<Fun, FunView>                      funs        = Pool<Fun, FunView>(count);
    SmartPool<Dict, DictView>               dicts       = SmartPool<Dict, DictView>(count);
    GC<Iterate>                             iterators   = GC<Iterate>(count);
    Pool<std::complex<double>, std::complex<double>> complexes = Pool<std::complex<double>, std::complex<double>>(count);
    Pool<Matrix, MatrixView>                matrices    = Pool<Matrix, MatrixView>(count);
    std::vector<BuitinFun> builtInFuns;
    std::vector<BuitinMessage> builtInMessages;
    ErrorHandler errorHandler;
    Scope globalScope;
    Object* messageObj;
    std::atomic<int32_t> recursion_count = 0;
    void clear() {
        bytes.clear();
        tuples.clear();
        objects.clear();
        funs.clear();
        dicts.clear();
        iterators.clear();
        complexes.clear();
        matrices.clear();
        assert(count == 0);
        recursion_count = 0;
    }
    CallPool() {}
    CallPool(const CallPool&) = delete;
    ~CallPool() {
        clear();
    }
};

struct RecursionCountGuard {
    std::atomic<int32_t>& recursion_count;
    RecursionCountGuard(CallPool& pool) : recursion_count(pool.recursion_count) {
        recursion_count++;
        if (recursion_count > RECURSION_LIMIT) {
            throw RecursionError();
        }
    }
    ~RecursionCountGuard() {
        recursion_count--;
    }
};

struct Validator {
    struct Arg {
        bool any;
        DynamicType type;
        std::vector<Arg> items;
        Arg() : any(true) {}
        Arg(DynamicType type) : any(false), type(type) {}
        Arg(DynamicType type, std::vector<Arg> items) : any(false), type(type), items(std::move(items)) {}
    };
    Arg arg;
    const char* name;
    Validator(const Arg& arg, const char* name) : arg(arg), name(name) {}
    std::unique_ptr<const RuntimeError> get(Dynamic d, CallPool& pool) {
        if (arg.any) {
            return nullptr;
        }
        if (d.type != arg.type) {
            return std::make_unique<const RuntimeError>("in ", name, ": expected ", arg.type, ", got ", d.type);
        }
        if (arg.type != DynamicType::TUPLE) {
            return nullptr;
        }
        TupleView t = pool.tuples.at(d.value);
        if (t.size() != arg.items.size()) {
            return std::make_unique<const RuntimeError>("in ", name, ": expected tuple with size ", arg.items.size(), ", got ", t.size());
        }
        for (size_t i = 0; i < t.size(); i++) {
            if (arg.items[i].any) {
                continue;
            }
            if (t[i].type != arg.items[i].type) {
                return std::make_unique<const RuntimeError>("in ", name, ": expected element at index ", i, " to be ", arg.items[i].type, ", got ", t[i].type);
            }
            if (t[i].type != DynamicType::TUPLE) {
                continue;
            }
            TupleView t1 = pool.tuples.at(t[i].value);
            if (t1.size() != arg.items[i].items.size()) {
                return std::make_unique<const RuntimeError>("in ", name, ": expected inner tuple at index ", i, " with size ", arg.items[i].items.size(), ", got ", t1.size());
            }
        }
        return nullptr;
    }
};

class VMPool {
    CallPool* pools = new CallPool[MAX_VMS]{};
    bool bits[MAX_VMS]{};
    std::mutex pool_mutex;
public:
    size_t alloc() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        for (size_t i = 0; i < MAX_VMS; i++) {
            if(!bits[i]) {
                bits[i] = true;
                return i;
            }
        }
        return -1;
    }
    void free(size_t i) {
        if (i == -1) {
            return;
        }
        std::lock_guard<std::mutex> lock(pool_mutex);
        assert(i < MAX_VMS);
        pools[i].clear();
        bits[i] = false;
    }
    CallPool& operator[](size_t i) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        assert(i < MAX_VMS);
        assert(bits[i]);
        return pools[i];
    }
    size_t size() const {
        return MAX_VMS;
    }
    CallPool* begin() {
        return pools;
    }
    CallPool* end() {
        return pools + MAX_VMS;
    }
};
