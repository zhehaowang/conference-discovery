# Make only tested with libndn-cpp compiled with boost shared pointers and functions;
# The compilation process could fail with libndn-cpp compiled with std shared pointers and functions.

test: test.cpp test.h chrono-chat.cpp chrono-chat.h conference-discovery-sync.cpp conference-discovery-sync.h sync-based-discovery.cpp sync-based-discovery.h chatbuf.pb.cc
	g++ -std=c++11 -Wall -ggdb test.cpp chrono-chat.cpp sync-based-discovery.cpp conference-discovery-sync.cpp chatbuf.pb.cc -lndn-cpp -lpthread -lcrypto -lprotobuf -o test
chat-library: chrono-chat.cpp chrono-chat.h chatbuf.pb.cc
	g++ -std=c++11 -c -Wall -ggdb chrono-chat.cpp chatbuf.pb.cc -lndn-cpp -lpthread -lcrypto -lprotobuf
	ar rcs libchrono-chat2013.a chrono-chat.o chatbuf.pb.o
conference-discovery-library: sync-based-discovery.cpp conference-discovery-sync.cpp
	g++ -std=c++11 -c -Wall -ggdb sync-based-discovery.cpp conference-discovery-sync.cpp -lndn-cpp -lpthread -lcrypto
	ar rcs libconference-discovery.a conference-discovery-sync.o
clean:
	rm -rf *.o *.a test
	rm -rf .lib/