#include <cassert>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

enum resp_status_code { OK, RES_NX, RES_ERR };

int32_t read_full(int fd, char *buf, size_t n) {

    while (n > 0) {
        ssize_t rv = read(fd, buf, n);

        if (rv <= 0) {
            // error or unexpected EOF
            return -1;
        }

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }

    return 0;
}

int32_t write_all(int fd, const char *buf, size_t n) {

    while (n > 0) {
        ssize_t rv = write(fd, buf, n);

        if (rv <= 0) {
            // error or unexpected EOF
            return -1;
        }

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }

    return 0;
}

// enable non-blocking mode for fds
void fd_set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        std::cerr << "Unable to set O_NONBLOCK flag!" << std::endl;
    }
}