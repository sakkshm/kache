#include <cstdint>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// Socket Configs
#define PORT_NO 1234 // Port number
#define IP_ADDR 0    // wildcard IP 0.0.0.0

void handle_connection(int conn_fd) {
    char rbuf[256] = {};

    ssize_t n = read(conn_fd, &rbuf, sizeof(rbuf) - 1);

    if (n < 0) {
        std::cerr << "Unable to read from conn_fd: " << conn_fd << std::endl;
        return;
    } else if (n == 0) {
        std::cout << "Client closed connection" << std::endl;
        return;
    } else {
        rbuf[n] = '\0';
        std::cout << "Client message recieved: " << rbuf << std::endl;
    }

    char wbuf[] = "world";
    n = write(conn_fd, wbuf, strlen(wbuf));

    if (n < 0) {
        std::cerr << "Unable to write to conn_fd: " << conn_fd << std::endl;
        return;
    }
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

    // setting SO_REUSEADDR to prevent TIME_WAIT and reuse addresses
    int val = 1;
    if (setsockopt(s_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        std::cerr << "Unable to set socket option: SO_REUSEADDR" << std::endl;
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;             // IPv4
    addr.sin_port = htons(PORT_NO);        // port number
    addr.sin_addr.s_addr = htonl(IP_ADDR); // IP addr

    // bind socket to addr
    std::cout << "Trying to bind socket to addr..." << std::endl;

    if (bind(s_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        std::cerr << "Unable to bind socket to addr" << std::endl;
        return EXIT_FAILURE;
    } else {
        std::cout << "Socket bound successfully to addr!" << std::endl;
    }

    // listen to socket
    if (listen(s_fd, SOMAXCONN) == -1) {
        std::cerr << "Unable to listen to socket" << std::endl;
        return EXIT_FAILURE;
    } else {
        char ip_str[INET_ADDRSTRLEN]; // buffer for IPv4 string
        inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
        std::cout << "Listening on " << ip_str << ":" << PORT_NO << std::endl;
    }

    while (true) {
        // Accept connections
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);

        int conn_fd = accept(s_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (conn_fd == -1) {
            // error
            std::cerr << "Unable to connect to this client..." << std::endl;
            continue;
        }

        handle_connection(conn_fd);

        // close client conn_fd
        close(conn_fd);
    }

    // close socket fd
    close(s_fd);

    return EXIT_SUCCESS;
}