#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <random>
void flood_worker() {
    sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dist(10, 2000);
    std::uniform_int_distribution<> byte_dist(0, 255);
    while (true) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) continue;
        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            close(sock);
            continue;
        }
        int size = size_dist(gen);
        std::vector<char> payload(size);
        for (int i = 0; i < size; ++i) {
            payload[i] = static_cast<char>(byte_dist(gen));
        }
        send(sock, payload.data(), payload.size(), 0);
        close(sock);
    }
}
int main() {
    std::cout << "Flood started\n";
    const unsigned int threads_count = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < threads_count; i++) {
        threads.emplace_back(flood_worker);
    }
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
    return 0;
}
