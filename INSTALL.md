NdnRtc-Addon:  Chat and Conference Discovery Addon for NdnRtc
==============================================================

Prerequisites
=============

* Required: ndn-cpp built with boost shared pointers.
* Required: libprotobuf
* Required: libcrypto
* Tested with OSX 10.9.5

Build
=====
To build in a terminal, change directory to the NdnRtc-Addon root.  Enter:

    ./configure
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
