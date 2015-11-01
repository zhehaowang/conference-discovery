NdnRtc-Addon:  Chat and Conference Discovery Addon for NdnRtc
==============================================================

Prerequisites
=============

* Required: ndn-cpp built with boost shared pointers.
* Required: libprotobuf
* Required: libcrypto
* Tested with OSX 10.9.5 and OSX 10.10

Build
=====
To build in a terminal, change directory to the NdnRtc-Addon root.  Enter:

    ./configure
    make .proto
    make

Files
=====
This makes the following libraries:

* libs/libchrono-chat2013.la
* libs/libconference-discovery.la

This makes the following test files:

* bin/test-both
* bin/test-chat

Development
===========
Follow Development Prerequisites above for your platform.
Now you can add source code files and update Makefile.am.
After updating, change directory to the NDN-CPP root and enter the following to build the Makefile:

    ./autogen.sh

To build again, follow the instructions above (./configure, make, etc.)

For older versions of automake, may also need to run autogen.sh; Try commenting AC\_CHECK\_HEADER\_STDBOOL if reported missing (occured on memoria)

Working with ndn-cpp-0.7 and NDNRTC
===========
To use this with ndnrtc, may want to use this configure command (as NDNRTC is compiled with libstdc++):
<pre>
./configure NDNCPPDIR=<include path of ndn-cpp compiled with libstdc++> NDNCPPLIB=<lib path of ndn-cpp compiled with libstdc++> PROTOBUFDIR=<include path of protobuf compiled with libstdc++> PROTOBUFLIB=<lib path of ndn-cpp compiled with libstdc++> CXXFLAGS="-stdlib=libstdc++"
</pre>
Per onInterest deprecation in ndn-cpp-0.7, onInterest function signatures were changed to onInterestCallback

_Known Issue_: Double check if configure's doing things correctly, link stage command seems to prefer /usr/local/lib/libndn-cpp.so.0 and does not seem to recognize NDNCPPLIB option, tested on memoria.

If symbol not found appeared during linking stage, the most likely cause is that some of the dependencies are compiled with libstdc++, while others with libc++; the ones with libstdc++ usually have __1 in their function symbol.