#include <assert.h>        // For using assert
#include <stdint.h>        // For fixed-width integer types
#include <stdlib.h>        // For general utilities
#include <string.h>        // For string manipulation functions
#include <stdio.h>         // For standard I/O functions
#include <errno.h>         // For error number definitions
#include <unistd.h>        // For POSIX operating system API
#include <arpa/inet.h>     // For internet operations
#include <sys/socket.h>    // For socket definitions
#include <netinet/ip.h>    // For IP address structures

// Function to print messages to stderr
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// Function to print error messages and abort the program
static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// Function to read a full buffer from a file descriptor
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // Error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Function to write a full buffer to a file descriptor
static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // Error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

const size_t k_max_msg = 4096; // Maximum message size

// Function to send a request to the server
static int32_t send_req(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // Copy length to the buffer (assume little endian)
    memcpy(&wbuf[4], text, len); // Copy the message to the buffer
    return write_all(fd, wbuf, 4 + len); // Send the buffer
}

// Function to read a response from the server
static int32_t read_res(int fd) {
    // Buffer to hold the response
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4); // Read the 4-byte header
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // Extract the length (assume little endian)
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // Read the response body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // Null-terminate the response and print it
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}

int main() {
    // Create a TCP socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // Define the server address
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // Set port to 1234
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    // Connect to the server
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }

    // List of messages to send to the server
    const char *query_list[3] = {"hello1", "hello2", "hello3"};
    for (size_t i = 0; i < 3; ++i) {
        int32_t err = send_req(fd, query_list[i]); // Send each message
        if (err) {
            goto L_DONE;
        }
    }
    for (size_t i = 0; i < 3; ++i) {
        int32_t err = read_res(fd); // Read each response
        if (err) {
            goto L_DONE;
        }
    }

L_DONE:
    close(fd); // Close the socket
    return 0;
}
