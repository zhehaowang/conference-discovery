test-exfil: test-conference-discovery-exfil.cpp test-exfil.cpp
	g++ -Wall -ggdb test-conference-discovery-exfil.cpp test-exfil.cpp -I/opt/local/include -lndn-cpp -lpthread -lcrypto -o test-exfil
test-memory-content-cache: test-memory-content-cache.cpp
	g++ -Wall -ggdb test-memory-content-cache.cpp -I/opt/local/include -lndn-cpp -lpthread -lcrypto -o test-memory-content-cache
test-sync-based: test-conference-discovery-sync.cpp test-sync-based.cpp
	g++ -Wall -ggdb test-conference-discovery-sync.cpp test-sync-based.cpp -I/opt/local/include -lndn-cpp -lpthread -lcrypto -o test-sync-based
clean:
	rm -rf test-exfil test-memory-content-cache
