#include "builder.h"
// g++ main.cpp compiler/*.cpp types/dynamic.cpp -std=c++20 -fsanitize=address -o funtalk && ./funtalk main1.fun
// g++ tests/flood.cpp -o test && ./test
// g++ tests/slowloris.cpp -o test && ./test
// g++ tests/bytes.cpp types/bytes.cpp -DFUNTALK_TEST_BYTES -std=c++20 -o test && ./test
int main(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::invalid_argument("Expected 1 or more args");
    }
    std::atomic<bool> interrupt = false;
    std::unique_ptr<VM> vm1 = nullptr;
    try {
        vm1 = std::make_unique<VM>(build(argv[1], interrupt));
        vm1->main(std::vector<std::string>(argv + 2, argv + argc), interrupt);
    } catch (const RuntimeErrorInMessage& e) {
        #ifdef DEBUG
        std::cerr << "In main vm main: " << vm::to_string(e) << '\n';
        #endif
    } catch (const RuntimeError& e) {
        #ifdef DEBUG
        std::cerr << "In main vm main: " << vm::to_string(e) << '\n';
        #endif
    } catch (const std::exception& e) {
        #ifdef DEBUG
        std::cerr << "In main vm main: " << e.what() << '\n';
        #endif
    }
    std::vector<Promise*> promises;
    auto f1 = [&promises]() {
        if (!vm::promises.empty()) {
            Promise* promise;
            vm::promises.pop_to(promise);
            if (promise->isDone()) {
                if (promise->count == 0) {
                    promise->exec();
                    delete promise;
                } else {
                    promises.push_back(promise);
                }
            } else if (promise->isExpired()) {
                promise->interrupt = true;
                vm::promises.push(promise);
            } else {
                vm::promises.push(promise);
            }
        }
        for (Promise*& promise : promises) {
            if (promise->count == 0) {
                promise->exec();
                delete promise;
                promise = nullptr;
            }
        }
        std::erase_if(promises, [](Promise* p) { return p == nullptr; });
    };
    std::vector<DeferredPromise*> deferreds;
    auto f2 = [&deferreds]() {
        if (!vm::deferreds.empty()) {
            DeferredPromise* promise = vm::deferreds.front();
            if (promise->count == 0) {
                promise->exec();
                vm::deferreds.pop();
                delete promise;
            } else {
                vm::deferreds.pop();
                deferreds.push_back(promise);
            }
        }
        for (DeferredPromise*& promise : deferreds) {
            if (promise->count == 0) {
                promise->exec();
                delete promise;
                promise = nullptr;
            }
        }
        std::erase_if(deferreds, [](DeferredPromise* p) { return p == nullptr; });
    };
    while (true) {
        if (vm::tasks.size() == 0 && vm::promises.empty() && promises.empty() && deferreds.empty()) {
            break;
        }
        for (auto task : vm::tasks) {
            try {
                task();
            } catch (const RuntimeErrorInMessage& e) {
                #ifdef DEBUG
                std::cerr << "In main task loop: " << vm::to_string(e) << '\n';
                #endif
            } catch (const RuntimeError& e) {
                #ifdef DEBUG
                std::cerr << "In main task loop: " << vm::to_string(e) << '\n';
                #endif
            } catch (const std::exception& e) {
                #ifdef DEBUG
                std::cerr << "In main task loop: " << e.what() << '\n';
                #endif
            }
        }
        f1();
        f2();
    }
    #ifdef DEBUG
    std::cout << "Program finished\n";
    #endif
    return 0;
}
