# redis
redis steps
The changes in the client:

Explanation of the Code
This C code is a simple TCP client that connects to a server on localhost (127.0.0.1) on port 1234, sends a message "hello", and receives a response from the server.

Code Breakdown
Include Headers

c
Copy code
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
<stdlib.h>: Provides functions for memory allocation, process control, and conversions.
<string.h>: Provides functions for manipulating C strings and arrays.
<stdio.h>: Provides functions for input and output, including printf.
<errno.h>: Defines macros for reporting and retrieving error conditions through error codes.
<unistd.h>: Provides access to the POSIX operating system API, including close and read.
<arpa/inet.h>: Provides definitions for internet operations, such as htonl, ntohl, htons, and ntohs.
<sys/socket.h>: Defines socket structures and functions.
<netinet/ip.h>: Defines the sockaddr_in structure for IP sockets.
Error Handling Function

c
Copy code
static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}
This function prints an error message along with the current value of errno and then aborts the program.
It is used for error handling to provide meaningful error messages when system calls fail.
Main Function

c
Copy code
int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }
Creates a socket with AF_INET (IPv4), SOCK_STREAM (TCP), and the default protocol (0).
If the socket creation fails (fd < 0), it calls the die function to handle the error.
Setting Up Server Address

c
Copy code
struct sockaddr_in addr = {};
addr.sin_family = AF_INET;
addr.sin_port = ntohs(1234);
addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
Initializes a sockaddr_in structure for the server address.
sin_family is set to AF_INET (IPv4).
sin_port is set to 1234, using ntohs to ensure correct byte order (should be htons).
sin_addr.s_addr is set to INADDR_LOOPBACK (127.0.0.1), using ntohl to ensure correct byte order (should be htonl).
Connecting to Server

c
Copy code
int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
if (rv) {
    die("connect");
}
Attempts to connect to the server using the connect function.
If the connection fails (rv is non-zero), it calls the die function to handle the error.
Sending Message

c
Copy code
char msg[] = "hello";
write(fd, msg, strlen(msg));
Sends the message "hello" to the server using the write function.
Receiving Response

c
Copy code
char rbuf[64] = {};
ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
if (n < 0) {
    die("read");
}
printf("server says: %s\n", rbuf);
Reads the response from the server into rbuf using the read function.
If the read operation fails (n < 0), it calls the die function to handle the error.
Prints the server's response.
Closing Socket

c
Copy code
close(fd);
Closes the socket using the close function.
Correction in the Code
There is a small mistake in setting the port and address byte order. The correct functions to use are htons for the port and htonl for the address.

Here is the corrected part of the code:

c
Copy code
addr.sin_port = htons(1234);  // Corrected from ntohs to htons
addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // Corrected from ntohl to htonl
Summary
This code demonstrates a simple TCP client in C. It includes error handling, socket creation, connection to a server, sending a message, receiving a response, and closing the connection. The included headers provide necessary functions and definitions for these operations.

step 2:
The changes and more:
Original Code
Simpler Main Function
Only establishes a socket connection.
Sends a simple "hello" message using write.
Reads the response using read and prints it.
Enhanced Code
Additional Includes

Added #include <cassert> for assertions.
Constants and Helper Functions

Defined const size_t k_max_msg = 4096; to set a maximum message size.
Introduced msg function to print messages to stderr.
Added read_full and write_all functions to ensure full data transmission.
Message Handling Logic

Introduced the query function to handle message sending and receiving:
Prepends message length as a 4-byte header.
Sends the message and the header.
Reads the response header to get the length.
Reads the response body based on the length.
Prints the server's response.
Main Function Enhancements

The main function now uses the query function to send three messages ("hello1", "hello2", "hello3") to the server and handles errors.
Uses goto L_DONE for error handling and cleanup.
Specific Code Differences
Simplified vs Enhanced Error Handling
Original Error Handling:

c
Copy code
if (fd < 0) {
    perror("socket()");
}
Simple perror calls without terminating the program.
Enhanced Error Handling:

c
Copy code
if (fd < 0) {
    die("socket()");
}
Calls the die function which prints an error and aborts the program.
Message Handling
Original Simple Message Handling:

c
Copy code
char msg[] = "hello";
write(fd, msg, strlen(msg));

char rbuf[64] = {};
ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
if (n < 0) {
    perror("read");
}
printf("server says: %s\n", rbuf);
Enhanced Message Handling:

c
Copy code
int32_t err = query(fd, "hello1");
if (err) {
    goto L_DONE;
}
err = query(fd, "hello2");
if (err) {
    goto L_DONE;
}
err = query(fd, "hello3");
if (err) {
    goto L_DONE;
}

L_DONE:
    close(fd);
    return 0;
Use of Helper Functions
Original:

Directly calls write and read.
Enhanced:

Uses write_all and read_full to ensure the complete transmission and reception of data.
Uses query to handle sending messages and reading responses with length prefixes.
Goto for Cleanup
Original:

Does not use goto for cleanup.
Enhanced:

Uses goto L_DONE to centralize cleanup logic after sending and receiving messages.
Summary
The enhanced code provides more robust error handling, ensures full data transmission and reception, and structures the logic for sending and receiving messages using helper functions. The original code is a simpler version with straightforward message handling and basic error reporting. The enhancements improve reliability and readability, especially for larger or more complex message exchanges.

step3:
The changes and more:

Original Code
Combined Query Function:
The original version has a single query function that handles both sending a request and reading a response.
Enhanced Code
Split Query Function:
The query function is split into two separate functions: send_req and read_res.
Specific Differences and Explanations
Function Definitions
Original query Function:

Combines sending the request and reading the response in one function.
Enhanced Code:

send_req Function: Handles sending the request.

c
Copy code
static int32_t send_req(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text, len);
    return write_all(fd, wbuf, 4 + len);
}
read_res Function: Handles reading the response.

c
Copy code
static int32_t read_res(int fd) {
    // 4 bytes header
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

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}
Main Function Logic
Original:

Uses query three times in sequence to send requests and read responses.
c
Copy code
int32_t err = query(fd, "hello1");
if (err) {
    goto L_DONE;
}
err = query(fd, "hello2");
if (err) {
    goto L_DONE;
}
err = query(fd, "hello3");
if (err) {
    goto L_DONE;
}
Enhanced:

Sends all requests first using send_req and then reads all responses using read_res.
c
Copy code
const char *query_list[3] = {"hello1", "hello2", "hello3"};
for (size_t i = 0; i < 3; ++i) {
    int32_t err = send_req(fd, query_list[i]);
    if (err) {
        goto L_DONE;
    }
}
for (size_t i = 0; i < 3; ++i) {
    int32_t err = read_res(fd);
    if (err) {
        goto L_DONE;
    }
}
Error Handling
Original:

The main function directly handles errors after each query call.
Enhanced:

Uses the same error handling approach but applies it separately to the send_req and read_res loops.
Summary of Differences
Function Splitting:

The original query function was split into send_req for sending requests and read_res for reading responses.
Sequential vs. Pipelined Requests:

The original code sends and reads one request at a time.
The enhanced code sends all requests first and then reads all responses, which can be more efficient for pipelined requests.
Main Function Structure:

The main function was updated to iterate over sending and then reading operations, improving clarity and separation of concerns.

step4:
The changes in the client code:

Argument Handling and Command Building
Previous Code:

The previous version used a predefined list of commands (query_list) within the main function.
cpp
Copy code
const char *query_list[3] = {"hello1", "hello2", "hello3"};
for (size_t i = 0; i < 3; ++i) {
    int32_t err = send_req(fd, query_list[i]);
    if (err) {
        goto L_DONE;
    }
}
New Code:

The new version constructs the commands dynamically from command-line arguments using std::vector<std::string>.
cpp
Copy code
std::vector<std::string> cmd;
for (int i = 1; i < argc; ++i) {
    cmd.push_back(argv[i]);
}
int32_t err = send_req(fd, cmd);
if (err) {
    goto L_DONE;
}
Request Sending
Previous Code:

The previous version's send_req function was designed to send a single command as a string.
cpp
Copy code
static int32_t send_req(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text, len);
    return write_all(fd, wbuf, 4 + len);
}
New Code:

The new version's send_req function is enhanced to send multiple commands in a vector.
cpp
Copy code
static int32_t send_req(int fd, const std::vector<std::string> &cmd) {
    uint32_t len = 4;
    for (const std::string &s : cmd) {
        len += 4 + s.size();
    }
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(&wbuf[0], &len, 4);  // assume little endian
    uint32_t n = cmd.size();
    memcpy(&wbuf[4], &n, 4);
    size_t cur = 8;
    for (const std::string &s : cmd) {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }
    return write_all(fd, wbuf, 4 + len);
}
Response Reading and Printing
Previous Code:

The previous version's read_res function read and printed the server response without distinguishing the response code.
cpp
Copy code
static int32_t read_res(int fd) {
    // 4 bytes header
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

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}
New Code:

The new version's read_res function extracts and prints a response code from the server response.
cpp
Copy code
static int32_t read_res(int fd) {
    // 4 bytes header
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

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // print the result
    uint32_t rescode = 0;
    if (len < 4) {
        msg("bad response");
        return -1;
    }
    memcpy(&rescode, &rbuf[4], 4);
    printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
    return 0;
}
Summary of Differences
Argument Handling and Command Building:

The new version uses command-line arguments to build the commands dynamically.
Request Sending:

The send_req function now handles multiple commands in a vector rather than a single string.
Response Reading and Printing:

The read_res function now extracts and prints a response code along with the server response.
These changes improve the flexibility and functionality of the client code, allowing it to handle multiple commands and response codes more effectively.

step5:
more changes in the client code:
1. Handling of Responses:
Previous Version:
Responses were handled in a straightforward manner, simply printing the server's response string.
c
Copy code
static int32_t read_res(int fd) {
    // 4 bytes header
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

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // print the result
    uint32_t rescode = 0;
    if (len < 4) {
        msg("bad response");
        return -1;
    }
    memcpy(&rescode, &rbuf[4], 4);
    printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
    return 0;
}
Current Version:
A more sophisticated on_response function is introduced to handle different types of responses. It can process and print various response types, such as strings, integers, arrays, errors, and nil values.
c
Copy code
static int32_t on_response(const uint8_t *data, size_t size) {
    if (size < 1) {
        msg("bad response");
        return -1;
    }
    switch (data[0]) {
    case SER_NIL:
        printf("(nil)\n");
        return 1;
    case SER_ERR:
        if (size < 1 + 8) {
            msg("bad response");
            return -1;
        }
        {
            int32_t code = 0;
            uint32_t len = 0;
            memcpy(&code, &data[1], 4);
            memcpy(&len, &data[1 + 4], 4);
            if (size < 1 + 8 + len) {
                msg("bad response");
                return -1;
            }
            printf("(err) %d %.*s\n", code, len, &data[1 + 8]);
            return 1 + 8 + len;
        }
    case SER_STR:
        if (size < 1 + 4) {
            msg("bad response");
            return -1;
        }
        {
            uint32_t len = 0;
            memcpy(&len, &data[1], 4);
            if (size < 1 + 4 + len) {
                msg("bad response");
                return -1;
            }
            printf("(str) %.*s\n", len, &data[1 + 4]);
            return 1 + 4 + len;
        }
    case SER_INT:
        if (size < 1 + 8) {
            msg("bad response");
            return -1;
        }
        {
            int64_t val = 0;
            memcpy(&val, &data[1], 8);
            printf("(int) %ld\n", val);
            return 1 + 8;
        }
    case SER_ARR:
        if (size < 1 + 4) {
            msg("bad response");
            return -1;
        }
        {
            uint32_t len = 0;
            memcpy(&len, &data[1], 4);
            printf("(arr) len=%u\n", len);
            size_t arr_bytes = 1 + 4;
            for (uint32_t i = 0; i < len; ++i) {
                int32_t rv = on_response(&data[arr_bytes], size - arr_bytes);
                if (rv < 0) {
                    return rv;
                }
                arr_bytes += (size_t)rv;
            }
            printf("(arr) end\n");
            return (int32_t)arr_bytes;
        }
    default:
        msg("bad response");
        return -1;
    }
}

static int32_t read_res(int fd) {
    // 4 bytes header
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

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // print the result
    int32_t rv = on_response((uint8_t *)&rbuf[4], len);
    if (rv > 0 && (uint32_t)rv != len) {
        msg("bad response");
        rv = -1;
    }
    return rv;
}
2. Addition of Enumeration for Response Types:
Current Version:
Added an enumeration to define response types, improving code readability and maintainability.
c
Copy code
enum {
    SER_NIL = 0,
    SER_ERR = 1,
    SER_STR = 2,
    SER_INT = 3,
    SER_ARR = 4,
};
3. Handling of Different Response Types:
Current Version:
on_response function handles different response types (nil, error, string, integer, array) by checking the first byte of the response and processing accordingly.
4. Overall Improvements:
Current Version:
Introduced more robust error checking and response handling.
Added detailed parsing and processing for different response types.
Enhanced debug and error messages to provide clearer information.
These changes make the current code more capable of handling various types of responses from the server and provide more detailed output, making it easier to debug and understand server responses.

step6:
final steps in the client code:
1. New Include Directive:
Current Version:
Includes a new header file common.hpp which contains shared definitions and functions.
cpp
Copy code
// proj
#include "common.hpp"
2. Additional Response Type Handling:
Current Version:
Introduces a new response type SER_DBL for handling double-precision floating-point numbers.
cpp
Copy code
case SER_DBL:
    if (size < 1 + 8) {
        msg("bad response");
        return -1;
    }
    {
        double val = 0;
        memcpy(&val, &data[1], 8);
        printf("(dbl) %g\n", val);
        return 1 + 8;
    }
3. Enumeration in Header File:
Current Version:
Moves the enumeration for response types (SER_NIL, SER_ERR, SER_STR, SER_INT, SER_DBL, SER_ARR) to the common.hpp header file.
cpp
Copy code
enum {
    SER_NIL = 0,
    SER_ERR = 1,
    SER_STR = 2,
    SER_INT = 3,
    SER_DBL = 4,
    SER_ARR = 5,
};
4. Inline Hash Function:
Current Version:
Adds an inline function str_hash in common.hpp to compute a hash of a given string.
cpp
Copy code
inline uint64_t str_hash(const uint8_t *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}
5. Macro for Container:
Current Version:
Adds a macro container_of in common.hpp to get the container structure from a member pointer.
cpp
Copy code
#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})
6. Overall Code Organization:
Current Version:
Improved organization by moving shared components into a common header file (common.hpp). This allows for better code reuse and maintainability.
These changes enhance the functionality and organization of the code by adding support for new data types (double), introducing utility functions and macros, and improving the code structure with a common header file.







