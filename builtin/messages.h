#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

template <class T>
void printHelper(Dynamic arg) {
    T* promise = reinterpret_cast<T*>(arg.value);
    promise->push([](Dynamic d, CallPool& pool) {
        std::cout << to_string(d, pool) << '\n';
    }, [](std::exception_ptr e) {
        try {
            std::rethrow_exception(e);
        } catch (const RuntimeErrorInMessage& e) {
            std::cout << vm::to_string(e) << '\n';
        } catch (const RuntimeError& e) {
            std::cout << vm::to_string(e) << '\n';
        } catch (const std::exception& e) {
            std::cout << e.what() << '\n';
        }
    });
}

void print(Dynamic arg, CallPool& pool, std::atomic<bool>& interrupt) {
    if (arg.type == DynamicType::ASYNC) {
        printHelper<Promise>(arg);
    } else if (arg.type == DynamicType::DEFERRED) {
        printHelper<DeferredPromise>(arg);
    } else {
        std::cout << to_string(arg, pool) << '\n';
    }
}

void send_response(int client_fd, const std::string& status, const std::string& content_type, BytesView body) {
    std::string response =
    "HTTP/1.1 " + status + "\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: POST, OPTIONS, GET\r\n"
    "Access-Control-Allow-Headers: Content-Type\r\n"
    "Content-Type: " + content_type + "\r\n"
    "Content-Length: " + std::to_string(body.size()) + "\r\n"
    "Connection: close\r\n"
    "\r\n" + body;
    try {
        send(client_fd, response.c_str(), response.length(), 0);
    } catch (...) {}
}

template <class T>
void sendHelper(int client_fd, Dynamic arg) {
    T* promise = reinterpret_cast<T*>(arg.value);
    promise->push([client_fd](Dynamic d, CallPool& pool) {
        send_response(client_fd, "200 OK", "text/plain", to_string(d, pool));
        vm::clientCount--;
        close(client_fd);
    }, [client_fd](std::exception_ptr e) {
        try {
            std::rethrow_exception(e);
        } catch (const RuntimeErrorInMessage& e) {
            send_response(client_fd, "400 Bad Request", "text/plain", vm::to_string(e));
        } catch (const RuntimeError& e) {
            send_response(client_fd, "400 Bad Request", "text/plain", vm::to_string(e));
        } catch (const std::exception& e) {
            send_response(client_fd, "400 Bad Request", "text/plain", e.what());
        }
        vm::clientCount--;
        close(client_fd);
    });
}

void send(Dynamic arg, CallPool& pool, std::atomic<bool>& interrupt) {
    Validator v(Validator::Arg(DynamicType::TUPLE, {Validator::Arg(DynamicType::INT), Validator::Arg()}), "send");
    std::unique_ptr<const RuntimeError> e = v.get(arg, pool);
    if (e) {
        throw *e;
    }
    TupleView t = pool.tuples.at(arg.value);
    int client_fd = t[0].value;
    if (t[1].type == DynamicType::ASYNC) {
        sendHelper<Promise>(client_fd, t[1]);
    } else if (t[1].type == DynamicType::DEFERRED) {
        sendHelper<DeferredPromise>(client_fd, t[1]);
    } else {
        send_response(client_fd, "200 OK", "text/plain", to_string(t[1], pool));
        vm::clientCount--;
        close(client_fd);
    }
}

void handle_client(int client_fd, const std::map<Bytes, Bytes>& get_map, const std::map<Bytes, Bytes>& post_map, std::atomic<bool>& interrupt) {
    constexpr int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        vm::clientCount--;
        close(client_fd);
        #ifdef DEBUG
        std::cerr << "Error setting timeout" << std::endl;
        #endif
        return;
    }
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            #ifdef DEBUG
            std::cerr << "Client read operation timed out (408)." << std::endl;
            #endif
            send_response(client_fd, "408 Request Timeout", "text/plain", "The server timed out waiting for the request.");
        } else {
            #ifdef DEBUG
            std::cerr << "Read error occurred: " << errno << std::endl;
            #endif
        }
        vm::clientCount--;
        close(client_fd);
        return;
    }
    if (bytes_received == 0) {
        #ifdef DEBUG
        std::cout << "Client closed connection prematurely." << std::endl;
        #endif
        vm::clientCount--;
        close(client_fd);
        return;
    }
    buffer[bytes_received] = '\0';
    std::string request_str(buffer, bytes_received);
    std::istringstream request_stream(request_str);
    std::string method, path, version;
    if (!(request_stream >> method >> path >> version)) {
        #ifdef DEBUG
        std::cout << "Invalid request. Looks like a tcp flood\n" << std::endl;
        #endif
        vm::clientCount--;
        close(client_fd);
        return;
    }
    #ifdef DEBUG
    std::cout << "Received: " << method << " " << path << std::endl;
    #endif
    if (method == "OPTIONS") {
        send_response(client_fd, "204 No Content", "text/plain", "");
        vm::clientCount--;
        close(client_fd);
        return;
    }
    if (method == "GET") {
        if (auto it = get_map.find(path); it != get_map.end()) {
            std::cout << it->first << " " << it->second << vm::symbolTable.string_to_id.at(it->second) << '\n';
            vm::vmpool[0].globalScope.getObject()->send(Dynamic(DynamicType::INT, client_fd), vm::symbolTable.string_to_id.at(it->second), *vm::vmpool[0].messageObj, *vm::vmpool[0].messageObj->getPool(), interrupt);
        } else {
            send_response(client_fd, "400 Bad Request", "text/plain", "Invalid GET Endpoint");
        }
        return;
    }
    if (method == "POST") {
        size_t content_length = 0;
        std::string line;
        while (std::getline(request_stream, line) && line != "\r") {
            if (line.rfind("Content-Length:", 0) == 0) {
                content_length = std::stoul(line.substr(15));
            }
        }
        size_t header_end = request_str.find("\r\n\r\n");
        std::string body = "";
        if (header_end != std::string::npos) {
            body = request_str.substr(header_end + 4);
        }
        while (body.length() < content_length) {
            bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) break;
            buffer[bytes_received] = '\0';
            body.append(buffer, bytes_received);
        }
        #ifdef DEBUG
        std::cout << "POST Body Payload: " << body << std::endl;
        #endif
        if (auto it = post_map.find(path); it != post_map.end()) {
            Tuple t(2);
            t[0] = Dynamic(DynamicType::INT, client_fd);
            t[1] = Dynamic(DynamicType::BYTES, vm::vmpool[0].messageObj->getPool()->bytes.alloc(Bytes(body)));
            Dynamic d = Dynamic(DynamicType::TUPLE, vm::vmpool[0].messageObj->getPool()->tuples.alloc(t));
            vm::vmpool[0].globalScope.getObject()->send(d, vm::symbolTable.string_to_id.at(it->second), *vm::vmpool[0].messageObj, *vm::vmpool[0].messageObj->getPool(), interrupt);
            free(d, *vm::vmpool[0].messageObj->getPool());
        } else {
            send_response(client_fd, "400 Bad Request", "text/plain", "Invalid POST Endpoint");
        }
    } else {
        send_response(client_fd, "405 Method Not Allowed", "text/plain", "Only GET, POST and OPTIONS allowed.");
        vm::clientCount--;
        close(client_fd);
        return;
    }
}

void parse_handle_http_arg(Dynamic arg, int& port, std::map<Bytes, Bytes>& get_map, std::map<Bytes, Bytes>& post_map, CallPool& pool) {
    if (arg.type != DynamicType::TUPLE) {
        throw RuntimeError("Error in handle_http: Expected TUPLE, got ", arg.type);
    }
    TupleView t = pool.tuples.at(arg.value);
    if (t.size() < 2) {
        throw RuntimeError("Error in handle_http: Expected 2 args or more, got ", t.size());
    }
    if (t[0].type != DynamicType::INT) {
        throw RuntimeError("Error in handle_http: Expected first arg INT, got ", t[0].type);
    }
    port = t[0].value;
    for (uint32_t i = 1; i < t.size(); i++) {
        if (t[i].type != DynamicType::TUPLE) {
            throw RuntimeError("Error in handle_http: Expected tuple args after the first arg, got ", t[i].type);
        }
        TupleView t1 = pool.tuples.at(t[i].value);
        if (t1.size() != 3) {
            throw RuntimeError("Error in handle_http: Expected tuple args with size 3, got ", t1.size());
        }
        for (uint32_t j = 0; j < 3; j++) {
            if (t1[j].type != DynamicType::STRING) {
                throw RuntimeError("Error in handle_http: Expected STRING, in tuples, got ", t1[j].type);
            }
        }
        if (pool.bytes.at(t1[0].value) == "get") {
            get_map[pool.bytes.at(t1[1].value)] = pool.bytes.at(t1[2].value);
        } else if (pool.bytes.at(t1[0].value) == "post") {
            post_map[pool.bytes.at(t1[1].value)] = pool.bytes.at(t1[2].value);
        } else {
            throw RuntimeError("Error in handle_http: Expected \"get\" or \"post\" as method");
        }
    }
}

void handle_http(Dynamic arg, CallPool& pool, std::atomic<bool>& interrupt) {
    #ifdef DEBUG
    std::cout << "handle_http\n";
    #endif
    std::map<Bytes, Bytes>* get_map = new std::map<Bytes, Bytes>();
    std::map<Bytes, Bytes>* post_map = new std::map<Bytes, Bytes>();
    int port;
    parse_handle_http_arg(arg, port, *get_map, *post_map, pool);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw RuntimeError("in handle_http: Failed to create socket");
    }
    timeval accept_tv;
    accept_tv.tv_sec = 1;
    accept_tv.tv_usec = 0;
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &accept_tv, sizeof(accept_tv)) < 0) {
        #ifdef DEBUG
        std::cerr << "Error setting accept timeout on server socket" << std::endl;
        #endif
        close(server_fd);
        return;
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_fd);
        throw RuntimeError("in handle_http: Bind failed");
    }
    if (listen(server_fd, 256) < 0) {
        close(server_fd);
        throw RuntimeError("in handle_http: Listen failed");
    }
    #ifdef DEBUG
    std::cout << "Server successfully running on port " << port << "...\n";
    #endif
    vm::tasks.push_back([server_fd, get_map, post_map, &interrupt]() {
        vm::clientCount++;
        sockaddr_in client_address{};
        socklen_t client_len = sizeof(client_address);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
        if (client_fd < 0) {
            vm::clientCount--;
            return;
        }
        if (vm::clientCount > MAX_CLIENTS) {
            close(client_fd);
            vm::clientCount--;
            return;
        }
        handle_client(client_fd, *get_map, *post_map, interrupt);
    });
}
