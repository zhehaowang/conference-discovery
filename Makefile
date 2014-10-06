test: test.cpp test.h test-chrono-chat.cpp test-chrono-chat.h test-conference-discovery-sync.cpp test-conference-discovery-sync.h chatbuf.pb.cc
	g++ -Wall -ggdb test.cpp test-chrono-chat.cpp test-conference-discovery-sync.cpp chatbuf.pb.cc -I/opt/local/include -lndn-cpp -lpthread -lcrypto -lprotobuf -o test
chat-library: test-chrono-chat.cpp test-chrono-chat.h chatbuf.pb.cc
	g++ -c -Wall -ggdb test-chrono-chat.cpp chatbuf.pb.cc -I/opt/local/include -lndn-cpp -lpthread -lcrypto -lprotobuf
	ar rcs libchrono-chat2013.a test-chrono-chat.o chatbuf.pb.o
conference-discovery-library: test-conference-discovery-sync.cpp
	g++ -c -Wall -ggdb test-conference-discovery-sync.cpp -I/opt/local/include -lndn-cpp -lpthread -lcrypto
	ar rcs libconference-discovery.a test-conference-discovery-sync.o
clean:
	rm -rf *.o *.a test
	rm -rf .lib/