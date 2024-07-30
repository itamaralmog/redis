#include <assert.h>        // For assert function
#include <stdint.h>        // For fixed-width integer types
#include <stdlib.h>        // For general utilities like malloc and free
#include <string.h>        // For memory manipulation functions
#include <stdio.h>         // For standard I/O functions
#include <errno.h>         // For error number definitions
#include <fcntl.h>         // For file control options
#include <poll.h>          // For polling function
#include <unistd.h>        // For POSIX API
#include <arpa/inet.h>     // For internet operations
#include <sys/socket.h>    // For socket definitions
#include <netinet/ip.h>    // For IP address structures
#include <vector>          // For dynamic array

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

// Function to set a file descriptor to non-blocking mode
static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
    }
    flags |= O_NONBLOCK;
    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}

const size_t k_max_msg = 4096; // Maximum message size

// Connection states
enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,  // Mark the connection for deletion
};

// Structure representing a connection
struct Conn {
    int fd = -1;
    uint32_t state = STATE_REQ;
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

// Function to store a connection in the vector
static void conn_put(std::vector<Conn *> &fd2conn, Conn *conn) {
    if (fd2conn.size() <= (size_t)conn->fd) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}

// Function to accept a new connection
static int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // Error
    }
    fd_set_nb(connfd); // Set new connection to non-blocking mode
    Conn *conn = (Conn *)malloc(sizeof(Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn_put(fd2conn, conn);
    return 0;
}

// Function prototypes
static void state_req(Conn *conn);
static void state_res(Conn *conn);

// Function to process a single request
static bool try_one_request(Conn *conn) {
    if (conn->rbuf_size < 4) {
        return false; // Not enough data in buffer
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }
    if (4 + len > conn->rbuf_size) {
        return false; // Not enough data in buffer
    }

    // Process the request
    printf("client says: %.*s\n", len, &conn->rbuf[4]);
    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;

    // Remove the processed request from buffer
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    // Change state to response
    conn->state = STATE_RES;
    state_res(conn);

    return (conn->state == STATE_REQ);
}

// Function to fill the read buffer
static bool try_fill_buffer(Conn *conn) {
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false; // Non-blocking read
    }
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        msg(conn->rbuf_size > 0 ? "unexpected EOF" : "EOF");
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    while (try_one_request(conn)) {}
    return (conn->state == STATE_REQ);
}

// Function to handle the request state
static void state_req(Conn *conn) {
    while (try_fill_buffer(conn)) {}
}

// Function to flush the write buffer
static bool try_flush_buffer(Conn *conn) {
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        return false; // Non-blocking write
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if (conn->wbuf_sent == conn->wbuf_size) {
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    return true;
}

// Function to handle the response state
static void state_res(Conn *conn) {
    while (try_flush_buffer(conn)) {}
}

// Function to handle connection I/O
static void connection_io(Conn *conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0);  // Unexpected state
    }
}

int main() {
    // Create a socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // Allow reuse of local addresses
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Bind the socket to address and port
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_ANY);  // wildcard address 0.0.0.0
    if (bind(fd, (const sockaddr *)&addr, sizeof(addr)) < 0) {
        die("bind()");
    }

    // Listen for incoming connections
    if (listen(fd, SOMAXCONN) < 0) {
        die("listen()");
    }

    // Set the listening socket to non-blocking mode
    fd_set_nb(fd);

    // Vector to store connections, indexed by file descriptor
    std::vector<Conn *> fd2conn;

    // Event loop
    while (true) {
        std::vector<struct pollfd> poll_args;

        // Add the listening socket to poll arguments
        poll_args.push_back({fd, POLLIN, 0});

        // Add active connections to poll arguments
        for (Conn *conn : fd2conn) {
            if (!conn) continue;
            struct pollfd pfd = {conn->fd, (conn->state == STATE_REQ) ? POLLIN : POLLOUT, POLLERR};
            poll_args.push_back(pfd);
        }

        // Poll for events
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if (rv < 0) {
            die("poll()");
        }

        // Process active connections
        for (size_t i = 1; i < poll_args.size(); ++i) {
            if (poll_args[i].revents) {
                Conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END) {
                    // Close and free connection
                    fd2conn[conn->fd] = nullptr;
                    close(conn->fd);
                    free(conn);
                }
            }
        }

        // Accept new connections
        if (poll_args[0].revents) {
            accept_new_conn(fd2conn, fd);
        }
    }

    return 0;
}
