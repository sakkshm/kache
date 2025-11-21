#include <cstdint>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// Socket Configs
#define PORT_NO 1234            // Port number
#define IP_ADDR INADDR_LOOPBACK // 127.0.0.1

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

    // Sending message to server
    char wbuf[] = "hello";
    ssize_t n = write(s_fd, wbuf, strlen(wbuf));

    std::cout << "Sending to server: " << wbuf << std::endl;

    if (n < 0) {
        std::cerr << "Unable to write to server..." << std::endl;
        return EXIT_FAILURE;
    }

    // Recieveing message from server
    char rbuf[64] = {};

    n = read(s_fd, &rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        std::cerr << "Unable to read from s_fd: " << s_fd << std::endl;
        return EXIT_FAILURE;
    } else {
        rbuf[n] = '\0';
        std::cout << "Server message recieved: " << rbuf << std::endl;
    }

    return EXIT_SUCCESS;
}