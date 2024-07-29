#include <stdio.h>          // Standard I/O functions
#include <string.h>         // String handling functions
#include <unistd.h>         // POSIX API for system calls
#include <arpa/inet.h>      // Definitions for internet operations
#include <sys/socket.h>     // Definitions for sockets
#include <netinet/ip.h>     // Definitions for internet protocols
#include <stdlib.h>

// Function to print error message and abort the program
static void die(const char *msg) {
    perror(msg);
    abort();
}

int main() {
    // Create a TCP socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        die("socket()");
    }

    // Define the server address to connect to
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(client_fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        die("connect()");
    }

    // Send a message to the server
    char message[] = "hello";
    write(client_fd, message, strlen(message));

    // Read the response from the server
    char buffer[64] = {};
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        die("read()");
    }
    printf("server says: %s\n", buffer);

    close(client_fd); // Close the client socket
    return 0;
}
