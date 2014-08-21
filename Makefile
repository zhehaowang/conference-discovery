test-exfil: test-conference-discovery-exfil.cpp test-exfil.cpp
	g++ -Wall -ggdb test-conference-discovery-exfil.cpp test-exfil.cpp -I/opt/local/include -lndn-cpp -lpthread -lcrypto -o test-exfil

clean:
	rm -rf test-exfil
