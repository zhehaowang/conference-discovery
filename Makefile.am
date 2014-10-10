ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AUTOMAKE_OPTIONS = subdir-objects

noinst_PROGRAMS = bin/test-both 
noinst_LTLIBRARIES = libs/libchrono-chat2013.la libs/libconference-discovery.la

bin_test_both_SOURCES = src/test.cpp \
  src/chatbuf.pb.cc \
  src/chrono-chat.cpp \
  src/conference-discovery-sync.cpp \
  src/sync-based-discovery.cpp
  
libs_libchrono_chat2013_la_SOURCES = src/chrono-chat.cpp \
  src/chatbuf.pb.cc
  
libs_libconference_discovery_la_SOURCES = src/sync-based-discovery.cpp \
  src/conference-discovery-sync.cpp
  
bin_test_both_CPPFLAGS = -I@BOOSTDIR@ -I@PROTOBUFDIR@ -I@NDNCPPLIB@ -I@CRYPTOLIB@ 
bin_test_both_LDFLAGS = -L@PROTOBUFDIR@ -L@NDNCPPLIB@ -L@CRYPTOLIB@ -lndn-cpp -lprotobuf -lcrypto

libs_libchrono_chat2013_la_CPPFLAGS = -I@PROTOBUFDIR@ -I@NDNCPPLIB@ -I@CRYPTOLIB@ 
libs_libchrono_chat2013_la_LDFLAGS = -L@PROTOBUFDIR@ -L@NDNCPPLIB@ -L@CRYPTOLIB@ -lndn-cpp -lcrypto -lprotobuf

libs_libconference_discovery_la_CPPFLAGS = -I@NDNCPPLIB@ -I@CRYPTOLIB@ 
libs_libconference_discovery_la_LDFLAGS = -L@NDNCPPLIB@ -L@CRYPTOLIB@ -lndn-cpp -lcrypto