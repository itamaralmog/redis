#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <cassert>

const size_t k_max_msg = 4096; // Maximum message size

// Function to print messages to stderr
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// Function to read a full buffer from a file descriptor
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1; // Error or unexpected EOF
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
            return -1; // Error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Function to send a query to the server and read the response
static int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1; // Message too long
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4); // Copy the length of the message (assuming little endian)
    memcpy(&wbuf[4], text, len); // Copy the message content

    // Send the message to the server
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }

    // Read the response header (4 bytes)
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    // Get the length of the response
    memcpy(&len, rbuf, 4); // Assume little endian
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

    // Print the response
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}

int main() {
    // Create a TCP socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket()");
        return 1;
    }

    // Define the server address
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        perror("connect");
        return 1;
    }

    // Send multiple queries to the server
    if (query(fd, "hello1") < 0) goto L_DONE;
    if (query(fd, "hello2") < 0) goto L_DONE;
    if (query(fd, "hello3") < 0) goto L_DONE;

L_DONE:
    close(fd); // Close the socket
    return 0;
}

