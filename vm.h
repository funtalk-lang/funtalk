#pragma once
#include "types/dynamic.h"

namespace vm {
    void handle(const RuntimeErrorInMessage& e, Object& obj, CallPool& pool, std::atomic<bool>& interrupt);
    std::unordered_map<std::string, std::pair<BuitinFun, uint32_t>> createFuns();
    std::unordered_map<std::string, BuitinMessage> createMessages();
    const std::unordered_map<std::string, std::pair<BuitinFun, uint32_t>> funs = createFuns();
    const std::unordered_map<std::string, BuitinMessage> messages = createMessages();
    VMPool vmpool;
    CallPool* pool = new CallPool();
    Object* funObj;
    Scope funScope;
    Object* messageObj;
    Scope messageScope;
    CompressedToken* builtInFunTokens;
    CompressedToken* builtInMessageTokens;
    std::vector<std::function<void()>> tasks;
    std::unordered_map<int, int> client_to_pooli;
    std::atomic<int32_t> clientCount = 0;
    SymbolTable createSymbolTable() {
        createObject(funObj, funScope, *pool);
        createObject(messageObj, messageScope, *pool);
        SymbolTable symbolTable;
        symbolTable.push_back("");
        symbolTable.push_back("NOT_IMPLEMENTED");
        symbolTable.push_back("set");
        symbolTable.push_back("push");
        symbolTable.push_back("map");
        symbolTable.push_back("scan");
        symbolTable.push_back("filter");
        symbolTable.push_back("unique");
        symbolTable.push_back("main");
        symbolTable.push_back("error");
        pool->errorHandler = vm::handle;
        pool->builtInFuns.reserve(vm::funs.size());
        pool->builtInMessages.reserve(vm::messages.size());
        vm::builtInFunTokens = new CompressedToken[vm::funs.size()];
        vm::builtInMessageTokens = new CompressedToken[vm::messages.size()];
        uint16_t count = 0;
        for (auto it : funs) {
            uint64_t i = symbolTable.push_back(it.first);
            builtInFunTokens[count] = CompressedToken(CompressedTokenType::BUILT_IN_FUN, it.second.second | (uint64_t(count) << 32));
            Fun fun = Fun(builtInFunTokens + count);
            uint16_t j = pool->funs.alloc(fun);
            (*funObj)[i] = Dynamic(DynamicType::FUN, j);
            pool->builtInFuns.push_back(it.second.first);
            count++;
        }
        count = 0;
        for (auto it : messages) {
            uint64_t i = symbolTable.push_back(it.first);
            builtInMessageTokens[count] = CompressedToken(CompressedTokenType::BUILT_IN_MESSAGE, count);
            messageObj->setMessage(i, builtInMessageTokens + count);
            pool->builtInMessages.push_back(it.second);
            count++;
        }
        return symbolTable;
    }
    SymbolTable symbolTable = createSymbolTable();
    std::mutex symbolTableMutex;

    Bytes to_string(const RuntimeError& e) {
        std::string s = "Error: ";
        for (RuntimeError::Arg arg : e.args) {
            switch (arg.type) {
                case RuntimeError::STRING:
                    s += arg.value.s;
                    break;
                case RuntimeError::DYNAMIC_TYPE:
                    s += to_string(static_cast<DynamicType>(arg.value.i));
                    break;
                case RuntimeError::NAME:
                    s += vm::symbolTable.id_to_string.at(arg.value.i);
                    break;
                case RuntimeError::INT:
                    s += std::to_string(arg.value.i);
                    break;
            }
        }
        return s;
    }

    Bytes to_string(const RuntimeErrorInMessage& e) {
        std::string s = "Error in '" + vm::symbolTable.id_to_string.at(e.messageID) + "': ";
        for (RuntimeError::Arg arg : e.args) {
            switch (arg.type) {
                case RuntimeError::STRING:
                    s += arg.value.s;
                    break;
                case RuntimeError::DYNAMIC_TYPE:
                    s += to_string(static_cast<DynamicType>(arg.value.i));
                    break;
                case RuntimeError::NAME:
                    s += vm::symbolTable.id_to_string.at(arg.value.i);
                    break;
                case RuntimeError::INT:
                    s += std::to_string(arg.value.i);
                    break;
            }
        }
        return s;
    }

    void handle(const RuntimeErrorInMessage& e, Object& obj, CallPool& pool, std::atomic<bool>& interrupt) {
        Dynamic d = Dynamic(DynamicType::STRING, pool.messageObj->getPool()->bytes.alloc(to_string(e)));
        obj.send(d, vm::symbolTable.string_to_id.at("error"), *pool.messageObj, *pool.messageObj->getPool(), interrupt);
        free(d, *pool.messageObj->getPool());
    }
}

void Promise::exec() {
    Dynamic d;
    std::exception_ptr eptr = nullptr;
    try {
        d = get();
    } catch (const std::exception& e) {
        eptr = std::current_exception();
    }
    if (eptr) {
        while (true) {
            Task task;
            {
                std::lock_guard<std::mutex> lock(tasks_mutex);
                if (tasks.empty()) {
                    break;
                }
                task = std::move(tasks.front());
                tasks.pop();
            }
            task.error(eptr);
        }
    } else {
        while (true) {
            Task task;
            {
                std::lock_guard<std::mutex> lock(tasks_mutex);
                if (tasks.empty()) {
                    break;
                }
                task = std::move(tasks.front());
                tasks.pop();
            }
            task.main(d, *pool);
        }
    }
}

void DeferredPromise::exec() {
    Dynamic d;
    std::exception_ptr eptr = nullptr;
    std::promise<Dynamic> promise;
    std::future<Dynamic> future = promise.get_future();
    task(std::move(promise));
    try {
        d = future.get();
    } catch (const std::exception& e) {
        eptr = std::current_exception();
    }
    if (eptr) {
        while (true) {
            Task task;
            {
                if (tasks.empty()) {
                    break;
                }
                task = std::move(tasks.front());
                tasks.pop();
            }
            task.error(eptr);
        }
    } else {
        while (true) {
            Task task;
            {
                if (tasks.empty()) {
                    break;
                }
                task = std::move(tasks.front());
                tasks.pop();
            }
            task.main(d, *pool);
        }
    }
}

struct VMPoolGuard {
    size_t i;
    VMPoolGuard() : i(vm::vmpool.alloc()) {}
    ~VMPoolGuard() {
        vm::vmpool.free(i);
    }
};

struct MessageObjGuard {
    size_t i;
    Object* messageObj;
    MessageObjGuard(Object* messageObj) : messageObj(messageObj), i(messageObj->getPool()->objects.alloc(*messageObj)) {}
    ~MessageObjGuard() {
        messageObj->getPool()->objects.free(i);
    }
};

struct VM {
    std::unique_ptr<VMPoolGuard> poolGuard = nullptr;
    std::unique_ptr<MessageObjGuard> messageObjGuard = nullptr;
    Object* currentObj;
    size_t currentObjI;
    std::vector<CompressedToken> compressedTokens;
    Object* messageObj;
    VM(std::string code, Object* messageObj, std::atomic<bool>& interrupt) {
        code = processText(std::move(code));
        if (!checkBrackets(code)) {
            throw FunTalkTokenizationError("brackets not correct");
        }
        std::vector<Token> tokens = fixTokens(tokenize(split(code)));
        #ifdef DEBUG
        for (auto t : tokens) {
            printToken(t);
        }
        #endif
        if (tokens.empty()) {
            throw FunTalkTokenizationError("Nothing to tokenize");
        }
        std::lock_guard<std::mutex> lock(vm::symbolTableMutex);
        vm::symbolTable.insert_from_tokens(tokens);
        compress(compressedTokens, vm::symbolTable, &*tokens.cbegin(), &*tokens.cend());
        #ifdef DEBUG
        for (CompressedToken t : compressedTokens) {
            printCompressedToken(t);
        }
        #endif
        poolGuard = std::make_unique<VMPoolGuard>();
        if (poolGuard->i == -1) {
            throw CallPanic("No free vms");
        }
        CallPool& pool = vm::vmpool[poolGuard->i];
        for (CompressedToken& t : compressedTokens) {
            if (t.type == CompressedTokenType::STRING) {
                t.value = pool.bytes.alloc(Bytes(vm::symbolTable.id_to_string.at(t.value)));
            }
        }
        messageObjGuard = std::make_unique<MessageObjGuard>(messageObj);
        this->messageObj = &messageObj->getPool()->objects[messageObjGuard->i];
        for (auto& [key, value] : *this->messageObj) {
            value = copy(value, *this->messageObj->getPool());
        }
        pool.funs = vm::pool->funs;
        pool.builtInFuns = vm::pool->builtInFuns;
        pool.builtInMessages = vm::pool->builtInMessages;
        pool.errorHandler = vm::pool->errorHandler;
        pool.messageObj = this->messageObj;
        const CompressedToken* p = &*compressedTokens.cbegin();
        currentObj = nullptr;
        CallScope callScope = CallScope(p, vm::funScope, vm::funScope, pool, interrupt);
        currentObjI = pool.objects.alloc(callScope);
        currentObj = &pool.objects[currentObjI];
        pool.globalScope = currentObj->getScope();
    }
    VM(const VM&) = delete;
    VM(VM&&) = default;
    void main(std::vector<std::string> args, std::atomic<bool>& interrupt) {
        CallPool& pool = vm::vmpool[poolGuard->i];
        Tuple t(args.size());
        for (uint32_t i = 0; i < args.size(); i++) {
            Bytes b = args[i];
            t[i] = Dynamic(DynamicType::STRING, messageObj->getPool()->bytes.alloc(b));
        }
        Dynamic d = Dynamic(DynamicType::TUPLE, messageObj->getPool()->tuples.alloc(t));
        try {
            #ifdef DEBUG
            std::cout << "VM::main started, pooli: " << poolGuard->i << '\n';
            #endif
            currentObj->send(d, vm::symbolTable.string_to_id.at("main"), *messageObj, *messageObj->getPool(), interrupt);
            #ifdef DEBUG
            std::cout << "VM::main done without exception, pooli: " << poolGuard->i << '\n';
            #endif
            free(d, *messageObj->getPool());
        } catch (const std::exception& e) {
            free(d, *messageObj->getPool());
            throw;
        }
    }
};
