#include "../src/utils.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define PORT_NO 1234
#define IP_ADDR "127.0.0.1"
#define MAX_MSG_LEN 4096

// Example/fuzzy commands to benchmark
std::vector<std::vector<std::string>> base_cmd_list = {
    {"set", "user:1:name", "Alice"},
    {"set", "user:2:name", "Bob"},
    {"set", "user:3:name", "Charlie"},
    {"get", "user:1:name"},
    {"get", "user:2:name"},
    {"del", "user:3:name"},
    {"zadd", "leaderboard", "100", "Alice"},
    {"zadd", "leaderboard", "200", "Bob"},
    {"zscore", "leaderboard", "Bob"},
    {"zquery", "leaderboard", "100", "Alice", "0", "3"},
    {"zrem", "leaderboard", "Alice"}};

// Generate random commands to simulate fuzziness
std::vector<std::vector<std::string>> generate_fuzzy_cmds(int n) {
    std::vector<std::vector<std::string>> cmds;
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> key_dist(1, 1000);
    std::uniform_int_distribution<int> score_dist(0, 1000);

    for (int i = 0; i < n; i++) {
        int choice = rng() % 5;
        int user_id = key_dist(rng);
        switch (choice) {
        case 0:
            cmds.push_back({"set", "user:" + std::to_string(user_id) + ":name",
                            "Name" + std::to_string(user_id)});
            break;
        case 1:
            cmds.push_back(
                {"get", "user:" + std::to_string(user_id) + ":name"});
            break;
        case 2:
            cmds.push_back(
                {"del", "user:" + std::to_string(user_id) + ":name"});
            break;
        case 3: {
            int score = score_dist(rng);
            cmds.push_back({"zadd", "leaderboard", std::to_string(score),
                            "user" + std::to_string(user_id)});
            break;
        }
        case 4: {
            int offset = rng() % 5;
            int limit = rng() % 10 + 1;
            cmds.push_back({"zquery", "leaderboard", "0",
                            "user" + std::to_string(user_id),
                            std::to_string(offset), std::to_string(limit)});
            break;
        }
        }
    }
    return cmds;
}

int connect_to_server() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT_NO);
    inet_pton(AF_INET, IP_ADDR, &addr.sin_addr);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }
    return fd;
}

int send_req_cmd(int fd, const std::vector<std::string> &cmd) {
    uint32_t len = 4; // 4 bytes for nstrings
    for (auto &s : cmd)
        len += 4 + s.size();

    if (len > MAX_MSG_LEN)
        return -1;

    char buf[4 + MAX_MSG_LEN];
    memcpy(buf, &len, 4);

    uint32_t n = cmd.size();
    memcpy(buf + 4, &n, 4);

    size_t cur = 8;
    for (auto &s : cmd) {
        uint32_t slen = s.size();
        memcpy(buf + cur, &slen, 4);
        memcpy(buf + cur + 4, s.data(), slen);
        cur += 4 + slen;
    }

    if (write(fd, buf, 4 + len) != (ssize_t)(4 + len))
        return -1;

    return 0;
}

void receive_res(int fd) {
    char header[4];
    if (read(fd, header, 4) <= 0)
        return;
    uint32_t msg_len;
    memcpy(&msg_len, header, 4);

    std::vector<char> buf(msg_len);
    if (read(fd, buf.data(), msg_len) <= 0)
        return;
}

// Benchmark thread
void benchmark_thread(int id, int n_repeats,
                      const std::vector<std::vector<std::string>> &cmds,
                      std::mutex &mtx, std::vector<double> &latencies) {
    int fd = connect_to_server();
    if (fd < 0)
        return;

    for (int i = 0; i < n_repeats; i++) {
        for (auto &cmd : cmds) {
            auto start = std::chrono::high_resolution_clock::now();
            if (send_req_cmd(fd, cmd) < 0) {
                std::cerr << "Thread " << id << ": failed to send command\n";
                continue;
            }
            receive_res(fd);
            auto end = std::chrono::high_resolution_clock::now();
            double ms =
                std::chrono::duration<double, std::milli>(end - start).count();

            std::lock_guard<std::mutex> lock(mtx);
            latencies.push_back(ms);
        }
    }
    close(fd);
}

int main() {
    const int n_threads = 4;
    const int n_repeats = 5000;

    std::cout << "Benchmark Started!\n";

    auto fuzzy_cmds = generate_fuzzy_cmds(50); // generate random commands
    std::vector<std::vector<std::string>> cmds = base_cmd_list;
    cmds.insert(cmds.end(), fuzzy_cmds.begin(), fuzzy_cmds.end());

    std::vector<std::thread> threads;
    std::vector<double> latencies;
    std::mutex mtx;

    auto t_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < n_threads; i++) {
        threads.emplace_back(benchmark_thread, i, n_repeats, std::ref(cmds),
                             std::ref(mtx), std::ref(latencies));
    }

    for (auto &t : threads)
        t.join();

    auto t_end = std::chrono::high_resolution_clock::now();
    double total_time = std::chrono::duration<double>(t_end - t_start).count();

    std::sort(latencies.begin(), latencies.end());

    double avg_latency =
        std::accumulate(latencies.begin(), latencies.end(), 0.0) /
        latencies.size();
    double min_latency = latencies.front();
    double max_latency = latencies.back();
    double p95 = latencies[static_cast<size_t>(latencies.size() * 0.95)];
    double p99 = latencies[static_cast<size_t>(latencies.size() * 0.99)];

    std::cout << "Benchmark finished:\n";

    std::cout << "==========================" << "\n";

    std::cout << "Total requests: " << latencies.size() << "\n";
    std::cout << "Average latency: " << avg_latency << " ms\n";
    std::cout << "Min latency: " << min_latency << " ms\n";
    std::cout << "Max latency: " << max_latency << " ms\n";
    std::cout << "p95 latency: " << p95 << " ms\n";
    std::cout << "p99 latency: " << p99 << " ms\n";
    std::cout << "Throughput: " << latencies.size() / total_time << " req/s\n";

    std::cout << "==========================" << "\n";

    return 0;
}
