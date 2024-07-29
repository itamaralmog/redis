#include <stdio.h>          // Standard I/O functions
#include <string.h>         // String handling functions
#include <unistd.h>         // POSIX API for system calls
#include <arpa/inet.h>      // Definitions for internet operations
#include <sys/socket.h>     // Definitions for sockets
#include <netinet/ip.h>     // Definitions for internet protocols

// Print a message to stderr
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

// Function to handle client communication
static void handle_client(int connfd) {
    char buffer[64] = {};
    // Read data from client
    ssize_t n = read(connfd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", buffer);

    // Respond to client
    char response[] = "world";
    write(connfd, response, strlen(response));
}

int main() {
    // Create a TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket()");
        return 1;
    }

    // Allow socket address reuse
    int val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Bind the socket to an address and port
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (const sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind()");
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen()");
        return 1;
    }

    // Accept and handle incoming connections in an infinite loop
    while (1) {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            perror("accept()");
            continue;
        }

        // Handle the client communication
        handle_client(connfd);
        close(connfd); // Close the connection with the client
    }

    close(server_fd); // Close the server socket
    return 0;
}
