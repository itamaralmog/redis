

# client:
# 	g++ -Wall -Wextra -O2 -g client.cpp common.hpp -o client

# server:
# 	g++ -Wall -Wextra -O2 -g server.cpp hashtable.cpp hashtable.hpp common.hpp avl.hpp avl.cpp zset.cpp zset.hpp -o server

# test: 
# 	g++ -Wall -Wextra -O2 -g avl.hpp test_offset.cpp -o test

# clean:
# 	rm -rf server client

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -g

make: client server

# Targets and their dependencies
client: client.cpp common.hpp
	$(CXX) $(CXXFLAGS) client.cpp -o client

server: server.cpp hashtable.cpp hashtable.hpp list.hpp common.hpp avl.hpp avl.cpp zset.cpp zset.hpp
	$(CXX) $(CXXFLAGS) server.cpp hashtable.cpp avl.cpp zset.cpp -o server

test: test_offset.cpp avl.hpp
	$(CXX) $(CXXFLAGS) test_offset.cpp -o test

clean:
	rm -rf server client test