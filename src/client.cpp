#include "utils.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// Socket Configs
#define PORT_NO 1234            // Port number
#define IP_ADDR INADDR_LOOPBACK // 127.0.0.1

#define MAX_MSG_LEN 4069

std::string handle_str(int fd, char *rbuf, size_t &cursor) {
    // data len
    uint32_t data_len = 0;
    int err = read_full(fd, &rbuf[cursor], 4);
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading Data len from conn")
                  << std::endl;
        return "";
    }

    memcpy(&data_len, &rbuf[cursor], 4); // assume little endian
    cursor += 4;

    std::cout << "Data length: " << data_len << std::endl;

    err = read_full(fd, &rbuf[cursor], data_len);
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading message from conn")
                  << std::endl;
        return "";
    }

    std::string str = std::string(&rbuf[cursor], data_len);

    std::cout << "Server message recieved: " << str << std::endl;
    cursor += data_len;

    return str;
}

int64_t handle_int(int fd, char *rbuf, size_t &cursor) {
    int err = read_full(fd, &rbuf[cursor], 8);
    int64_t resp_int = 0;
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading message from conn")
                  << std::endl;
        return 0;
    }

    memcpy(&resp_int, &rbuf[cursor], 8);

    std::cout << "Server message recieved (integer): " << resp_int << std::endl;
    cursor += 8;

    return resp_int;
}

double handle_dbl(int fd, char *rbuf, size_t &cursor) {
    int err = read_full(fd, &rbuf[cursor], 8);
    double resp_dbl = 0;
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading message from conn")
                  << std::endl;
        return 0;
    }

    memcpy(&resp_dbl, &rbuf[cursor], 8);

    std::cout << "Server message recieved (double): " << resp_dbl << std::endl;
    cursor += 8;

    return resp_dbl;
}

void handle_arr(int fd, char *rbuf, size_t &cursor) {

    // read arr len
    int err = read_full(fd, &rbuf[cursor], 4);
    uint32_t arr_len = 0;
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading message from conn")
                  << std::endl;
        return;
    }

    memcpy(&arr_len, &rbuf[cursor], 4);

    std::cout << "Server message recieved (array), Length: " << arr_len
              << std::endl;
    cursor += 8;

    for (uint32_t idx = 0; idx < arr_len; idx++) {
        // read tag
        uint8_t resp_tag = 0;
        err = read_full(fd, &rbuf[cursor], 1);

        if (err) {
            std::cerr << (errno == 0 ? "EOF reading from conn"
                                     : "Error reading response tag from conn")
                      << std::endl;
            return;
        }

        memcpy(&resp_tag, &rbuf[cursor], 1); // assume little endian
        cursor += 1;

        std::cout << "Response tag: " << int(resp_tag) << std::endl;

        switch (resp_tag) {
        case TAG_NIL:
            std::cout << "Output is NULL" << std::endl;
            break;

        case TAG_INT:
            handle_int(fd, rbuf, cursor);
            break;

        case TAG_STR:
            handle_str(fd, rbuf, cursor);
            break;

        case TAG_DBL:
            handle_dbl(fd, rbuf, cursor);
            break;

        default:
            break;
        }
    }
}

int32_t get_res(int fd) {

    uint32_t len = 0;

    // Read buffer
    char rbuf[4 + MAX_MSG_LEN];
    size_t cursor = 0;
    errno = 0;

    // Read resp. length
    int32_t err = read_full(fd, &rbuf[cursor], 4);

    std::cout << "========================================" << std::endl;

    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading msg_size from conn")
                  << std::endl;
        return err;
    }

    memcpy(&len, &rbuf[cursor], 4); // assume little endian
    cursor += 4;

    if (len > MAX_MSG_LEN) {
        std::cerr << "Message too long from conn" << std::endl;
        return -1;
    }

    std::cout << "Server response length: " << len << std::endl;

    // Resp body
    // Read status code
    uint32_t status_code = 0;
    err = read_full(fd, &rbuf[cursor], 4);
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading status code from conn")
                  << std::endl;
        return err;
    }

    memcpy(&status_code, &rbuf[cursor], 4); // assume little endian
    cursor += 4;

    std::cout << "Status code: " << status_code << std::endl;

    // check status codes
    if (status_code != OK) {

        switch (status_code) {
        case UNKNOWN_CMD:
            std::cout << "UNKNOWN_CMD: Unknown command found" << std::endl;
            break;

        case RES_NX:
            std::cout << "RES_NX: Entry not found" << std::endl;
            break;

        case RES_ERR:
            std::cout << "RES_ERR: Error in generating response" << std::endl;
            break;

        case ERR_BAD_ARG:
            std::cout << "ERR_BAD_ARG: Bad Arguments" << std::endl;
            break;

        case ERR_BAD_TYPE:
            std::cout << "ERR_BAD_TYPE: Wrong type for this action"
                      << std::endl;
            break;

        default:
            break;
        }

        return -1;
    }

    // read tag
    uint8_t resp_tag = 0;
    err = read_full(fd, &rbuf[cursor], 1);

    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading response tag from conn")
                  << std::endl;
        return err;
    }

    memcpy(&resp_tag, &rbuf[cursor], 1); // assume little endian
    cursor += 1;

    std::cout << "Response tag: " << int(resp_tag) << std::endl;

    switch (resp_tag) {
    case TAG_NIL:
        std::cout << "Output is NULL" << std::endl;
        break;

    case TAG_INT:
        handle_int(fd, rbuf, cursor);
        break;

    case TAG_STR:
        handle_str(fd, rbuf, cursor);
        break;

    case TAG_DBL:
        handle_dbl(fd, rbuf, cursor);
        break;

    case TAG_ARR:
        handle_arr(fd, rbuf, cursor);
        break;

    default:
        break;
    }

    std::cout << "========================================" << std::endl;

    return 0;
}

int32_t send_req_cmd(int fd, std::vector<std::string> cmd) {
    uint32_t len = 4;
    for (const std::string &s : cmd) {
        len += 4 + s.size();
    }
    if (len > MAX_MSG_LEN) {
        return -1;
    }

    char wbuf[4 + MAX_MSG_LEN];

    // write full msg size
    memcpy(&wbuf[0], &len, 4); // assume little endian

    // write no. of strings
    uint32_t n = cmd.size();
    memcpy(&wbuf[4], &n, 4);

    // for each str in cmd
    // write size(str)
    // write str
    size_t cur = 8;
    for (const std::string &s : cmd) {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }

    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }

    return 0;
}

int main(void) {

    std::cout << "Trying to create a socket..." << std::endl;

    int s_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_fd == -1) {
        std::cerr << "Unable to create a socket" << std::endl;
        return EXIT_FAILURE;
    } else {
        std::cout << "Socket created successfully!" << std::endl;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;             // IPv4
    addr.sin_port = htons(PORT_NO);        // port number
    addr.sin_addr.s_addr = htonl(IP_ADDR); // IP addr

    // connect socket to addr
    char ip_str[INET_ADDRSTRLEN]; // buffer for IPv4 string
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
    std::cout << "Connecting to server at: " << ip_str << ":" << PORT_NO
              << std::endl;

    if (connect(s_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        std::cerr << "Unable to connect socket to server" << std::endl;
        return EXIT_FAILURE;
    } else {
        std::cout << "Socket successfully connected to server!" << std::endl;
    }

    std::cout << "========================================" << std::endl;

    std::vector<std::vector<std::string>> cmd_list = {
        // Hashmap Commands
        {"set", "user:1:name", "Alice"},
        {"set", "user:2:name", "Bob"},
        {"set", "user:3:name", "Charlie"},
        {"get", "user:2:name"},
        {"expire", "user:1:name", "5000"},
        {"persist", "user:1:name"},
        {"del", "user:3:name"},

        // Sorted Set (ZSet) Commands
        {"zadd", "leaderboard", "100", "Alice"},
        {"zadd", "leaderboard", "200", "Bob"},
        {"zadd", "leaderboard", "150", "Charlie"},
        {"zadd", "leaderboard", "250", "Diana"},
        {"zadd", "leaderboard", "180", "Eve"},
        {"zscore", "leaderboard", "Bob"},
        {"zrem", "leaderboard", "Alice"}, 

        // Sorted Set traversal queries
        {"zquery", "leaderboard", "150", "Charlie", "0", "3"},
        {"zquery", "leaderboard", "200", "Bob", "0", "2"}
    };

    for (auto cmd : cmd_list) {
        std::cout << "Sending command: ";
        for (auto c : cmd) {
            std::cout << c << " ";
        }

        std::cout << std::endl;

        int32_t err = send_req_cmd(s_fd, cmd);
        if (err) {
            std::cerr << "Unable to send data to Server" << std::endl;
            return EXIT_FAILURE;
        }
    }

    while (true) {
        int32_t err = get_res(s_fd);
        if (err) {
            std::cerr << "Unable to get data from Server" << std::endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}