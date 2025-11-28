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

int32_t get_res(int fd) {

    uint32_t len = 0;

    // 4 bytes header
    char rbuf[4 + MAX_MSG_LEN];
    errno = 0;

    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading msg_size from conn")
                  << std::endl;
        return err;
    }

    memcpy(&len, rbuf, 4); // assume little endian

    if (len > MAX_MSG_LEN) {
        std::cerr << "Message too long from conn" << std::endl;
        return -1;
    }

    // Reply body
    char status_buf[4];
    uint32_t status_code = 0;
    err = read_full(fd, status_buf, 4);
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading status code from conn")
                  << std::endl;
        return err;
    }

    memcpy(&status_code, status_buf, 4); // assume little endian

    std::cout << "Status code: " << status_code << std::endl;

    err = read_full(fd, &rbuf[8], len - 4); // len include size of status code
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading message from conn")
                  << std::endl;
        return err;
    }

    std::cout << "Server message length: " << len << std::endl;
    std::cout << "Server message recieved: " << std::string(rbuf + 8, len - 4)
              << std::endl;
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
    memcpy(&wbuf[0], &len, 4);  // assume little endian

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

    std::vector<std::string> query_list = {
        "get",
        "key"
    };

    int32_t err = send_req_cmd(s_fd, query_list);
    if (err) {
        std::cerr << "Unable to send data to Server" << std::endl;
        return EXIT_FAILURE;
    }

    err = get_res(s_fd);
    if (err) {
        std::cerr << "Unable to get data from Server" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}