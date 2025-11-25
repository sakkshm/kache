#include "utils.cpp"
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

int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);

    if (len > MAX_MSG_LEN) {
        std::cerr << "Message too long for protocol" << std::endl;
        return -1;
    }

    // Send request
    char wbuf[4 + MAX_MSG_LEN];

    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);

    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }

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
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        std::cerr << (errno == 0 ? "EOF reading from conn"
                                 : "Error reading message from conn")
                  << std::endl;
        return err;
    }

    std::cout << "Server message length: " << len << std::endl;
    std::cout << "Server message recieved: " << std::string(rbuf + 4, len)
              << std::endl;
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
        "hello1",
        "hello2",
        "hello3",
    };

    for (const std::string &s : query_list) {
        int32_t err = query(s_fd, s.data());
        if (err) {
            std::cerr << "Unable to send data to Server" << std::endl;
            return EXIT_FAILURE;
        }
    }

    int32_t err = query(s_fd, "Hello");
    if (err) {
        std::cerr << "Unable to send data to Server" << std::endl;
        return EXIT_FAILURE;
    }

    err = query(s_fd, "World!");
    if (err) {
        std::cerr << "Unable to send data to Server" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}