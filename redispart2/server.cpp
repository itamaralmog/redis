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
            return -1;  // error, or unexpected EOF
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
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Function to handle one request from a client
static int32_t one_request(int connfd) {
    // Buffer to hold the received message
    char rbuf[4 + k_max_msg + 1];

    // Read the 4-byte header which contains the length of the message
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    // Extract the message length
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // Read the actual message based on the length
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // Null-terminate the received message and print it
    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    // Prepare a reply message
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4); // Copy the length of the reply
    memcpy(&wbuf[4], reply, len); // Copy the reply content

    // Send the reply to the client
    return write_all(connfd, wbuf, 4 + len);
}

int main() {
    // Create a TCP socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket()");
        return 1;
    }

    // Set socket options to reuse the address
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Define the server address
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // Set port to 1234
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any IP

    // Bind the socket to the address and port
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        perror("bind()");
        return 1;
    }

    // Listen for incoming connections
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        perror("listen()");
        return 1;
    }

    // Accept and handle incoming connections in a loop
    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue; // Ignore failed accept
        }

        // Serve one client connection at a time
        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break; // Exit loop on error
            }
        }
        close(connfd); // Close the connection
    }
}
