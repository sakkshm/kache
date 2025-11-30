#include <cassert>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <vector>

// ---------------- Serialization Functions ----------------
enum serial_tags { TAG_NIL, TAG_ERR, TAG_STR, TAG_INT, TAG_DBL, TAG_ARR };

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

void buf_append_u8(std::vector<uint8_t> &buf, const u_int8_t val) {
    buf.push_back(val);
}

void buf_append_u32(std::vector<uint8_t> &buf, const uint32_t data) {
    buf_append(buf, (const uint8_t *)&data, 4); // assume littlen-endian
}

void buf_append_i64(std::vector<uint8_t> &buf, const int64_t data) {
    buf_append(buf, (const uint8_t *)&data, 8); // assume littlen-endian
}

void buf_consume(std::vector<uint8_t> &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin() + n);
}

bool read_u8(const uint8_t *&curr, const uint8_t *end, uint8_t &out) {
    if (curr + 1 > end) {
        return false;
    }

    memcpy(&out, curr, 1);
    curr += 1;

    return true;
}

bool read_u32(const uint8_t *&curr, const uint8_t *end, uint32_t &out) {
    if (curr + 4 > end) {
        return false;
    }

    memcpy(&out, curr, 4);
    curr += 4;

    return true;
}

bool read_i64(const uint8_t *&curr, const uint8_t *end, uint64_t &out) {
    if (curr + 8 > end) {
        return false;
    }

    memcpy(&out, curr, 8);
    curr += 8;

    return true;
}

bool read_str(const uint8_t *&curr, const uint8_t *end, size_t n,
              std::string &out) {
    if (curr + n > end) {
        return false;
    }

    out.assign(curr, curr + n);
    curr += n;

    return true;
}

void out_nil(std::vector<uint8_t> &out) { buf_append_u8(out, TAG_NIL); }

void out_str(std::vector<uint8_t> &out, const char *s, size_t size) {
    buf_append_u8(out, TAG_STR);
    buf_append_u32(out, (uint32_t)size);
    buf_append(out, (const uint8_t *)s, size);
}
void out_int(std::vector<uint8_t> &out, int64_t val) {
    buf_append_u8(out, TAG_INT);
    buf_append_i64(out, val);
}
void out_arr(std::vector<uint8_t> &out, uint32_t n) {
    buf_append_u8(out, TAG_ARR);
    buf_append_u32(out, n);
}

// ---------------- Helper Functions ----------------

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