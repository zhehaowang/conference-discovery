test: test.cpp test.h test-chrono-chat.cpp test-chrono-chat.h test-conference-discovery-sync.cpp test-conference-discovery-sync.h chatbuf.pb.cc
	g++ -Wall -ggdb test.cpp test-chrono-chat.cpp test-conference-discovery-sync.cpp chatbuf.pb.cc -I/opt/local/include -lndn-cpp -lpthread -lcrypto -lprotobuf -o test
chat-library: test-chrono-chat.cpp test-chrono-chat.h
	g++ -Wall -ggdb test-chrono-chat.cpp -I/opt/local/include -lndn-cpp -lpthread -lcrypto -o test-chrono-chat.o
	#ar rcs libchronochat.a test-chrono-chat.o
clean:
	rm -rf test-chrono-chat test