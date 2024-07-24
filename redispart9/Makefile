

# client:
# 	g++ -Wall -Wextra -O2 -g client.cpp common.hpp -o client

# server:
# 	g++ -Wall -Wextra -O2 -g server.cpp hashtable.cpp hashtable.hpp common.hpp avl.hpp avl.cpp zset.cpp zset.hpp -o server

# test: 
# 	g++ -Wall -Wextra -O2 -g avl.hpp test_offset.cpp -o test

# clean:
# 	rm -rf server client

# Compiler and flags
# CXX = g++
# CXXFLAGS = -Wall -Wextra -O2 -g

# make: client server

# # Targets and their dependencies
# client: client.cpp common.hpp
# 	$(CXX) $(CXXFLAGS) client.cpp -o client

# server: server.cpp hashtable.cpp hashtable.hpp list.hpp common.hpp avl.hpp avl.cpp zset.cpp zset.hpp heap.cpp heap.hpp
# 	$(CXX) $(CXXFLAGS) server.cpp hashtable.cpp avl.cpp zset.cpp -o server

# test: test_heap.cpp heap.hpp heap.cpp
# 	$(CXX) $(CXXFLAGS) test_heap.cpp -o test

# clean:
# 	rm -rf server client test

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -g

# Sources and targets
SRCS = server.cpp hashtable.cpp avl.cpp zset.cpp heap.cpp  # Add heap.cpp if it contains heap_update implementation
OBJS = $(SRCS:.cpp=.o)

# Targets and their dependencies
all: server test

server: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

test: test_heap.cpp heap.hpp heap.cpp
	$(CXX) $(CXXFLAGS) test_heap.cpp -o test

clean:
	rm -rf server $(OBJS) test
