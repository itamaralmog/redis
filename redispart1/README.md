Explanation:

Server Code:

Setup Socket: Create a TCP socket using socket().

Set Options: Enable the SO_REUSEADDR option to allow the socket to be reused.

Bind: Assign a port and IP address to the socket using bind().

Listen: Start listening for incoming connections using listen().

Accept Loop: Accept connections in a loop using accept() and handle each client in the handle_client function.

Handle Client: In handle_client, read the message from the client using read() and respond with "world" using write().

Client Code:

Setup Socket: Create a TCP socket using socket().

Connect: Connect to the server at 127.0.0.1 on port 1234 using connect().

Send Message: Send the message "hello" to the server using write().

Read Response: Read the response from the server using read() and print it.

