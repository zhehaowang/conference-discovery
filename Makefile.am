ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AUTOMAKE_OPTIONS = subdir-objects

noinst_LTLIBRARIES = libs/libchrono-chat2013.la libs/libconference-discovery.la
noinst_PROGRAMS = bin/test-both bin/test-chat

libs_libchrono_chat2013_la_SOURCES = src/chrono-chat.cpp \
  src/chatbuf.pb.cc
  
libs_libconference_discovery_la_SOURCES = src/sync-based-discovery.cpp \
  src/conference-discovery-sync.cpp

libs_libchrono_chat2013_la_CPPFLAGS = -I$(top_srcdir)/include -I@PROTOBUFDIR@ -I@NDNCPPDIR@ -I@CRYPTODIR@ 
libs_libchrono_chat2013_la_LDFLAGS = -L@PROTOBUFLIB@ -L@NDNCPPLIB@ -L@CRYPTOLIB@ -lndn-cpp -lcrypto -lprotobuf

libs_libconference_discovery_la_CPPFLAGS = -I$(top_srcdir)/include -I@NDNCPPDIR@ -I@CRYPTODIR@ 
libs_libconference_discovery_la_LDFLAGS = -L@NDNCPPLIB@ -L@CRYPTOLIB@ -lndn-cpp -lcrypto

bin_test_both_SOURCES = tests/test-both.cpp 
  
bin_test_both_CPPFLAGS = -I$(top_srcdir)/include -I@BOOSTDIR@ -I@PROTOBUFDIR@ -I@NDNCPPDIR@ -I@CRYPTODIR@ 
bin_test_both_LDFLAGS = -L@PROTOBUFLIB@ -L@NDNCPPLIB@ -L@CRYPTOLIB@ -lndn-cpp -lprotobuf -lcrypto
bin_test_both_LDADD = libs/libchrono-chat2013.la libs/libconference-discovery.la

bin_test_chat_SOURCES = tests/test-chat.cpp
bin_test_chat_CPPFLAGS = -I$(top_srcdir)/include -I@BOOSTDIR@ -I@PROTOBUFDIR@ -I@NDNCPPDIR@ -I@CRYPTODIR@ 
bin_test_chat_LDFLAGS = -L@PROTOBUFLIB@ -L@NDNCPPLIB@ -L@CRYPTOLIB@ -lndn-cpp -lprotobuf -lcrypto
bin_test_chat_LDADD = libs/libchrono-chat2013.la