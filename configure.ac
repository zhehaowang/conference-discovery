AC_PREREQ([2.59])
AC_INIT([ndnrtc-addon], [1.0], [wangzhehao410305@gmail.com], [ndn-entity-discovery])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR(src)

AM_INIT_AUTOMAKE([1.10 -Wall no-define foreign])

AC_ARG_VAR([BOOSTDIR], [Path to the directory which contains Boost library headers folder (default is /usr/local/include)])
AC_ARG_VAR([NDNCPPDIR], [Path to the directory which contains ndn-cpp library headers folder (default is /usr/local/include)])
AC_ARG_VAR([NDNCPPLIB], [Path to the directory which contains ndn-cpp library binaries (default is /usr/local/lib)])
AC_ARG_VAR([PTHREADDIR], [Path to the directory which contains pthread library headers (default is /usr/local/include)])
AC_ARG_VAR([PTHREADLIB], [Path to the directory which contains pthread library binaries (default is /usr/local/lib)])
AC_ARG_VAR([CRYPTODIR], [Path to the directory which contains libcrypto library headers (default is /usr/local/include)])
AC_ARG_VAR([CRYPTOLIB], [Path to the directory which contains libcrypto library binaries (default is /usr/local/lib)])
AC_ARG_VAR([PROTOBUFDIR], [Path to the directory which contains libcrypto library headers (default is /usr/local/include)])
AC_ARG_VAR([PROTOBUFLIB], [Path to the directory which contains libcrypto library binaries (default is /usr/local/lib)])

if test ${BOOSTDIR}; then
	BOOSTDIR=$(cd $BOOSTDIR; pwd)
else
	BOOSTDIR="/usr/local/include"
fi

if test ${NDNCPPDIR}; then
	NDNCPPDIR=$(cd $NDNCPPDIR; pwd)
else
	NDNCPPDIR="/usr/local/include"
fi

if test ${NDNCPPLIB}; then
	NDNCPPLIB=$(cd $NDNCPPLIB; pwd)
else
	NDNCPPLIB="/usr/local/lib"
fi

if test ${PTHREADDIR}; then
	PTHREADDIR=$(cd $PTHREADDIR; pwd)
else
	PTHREADDIR="/usr/include"
fi

if test ${PTHREADLIB}; then
	PTHREADLIB=$(cd $PTHREADLIB; pwd)
else
	PTHREADLIB="/usr/lib"
fi

if test ${CRYPTODIR}; then
	CRYPTODIR=$(cd $CRYPTODIR; pwd)
else
	CRYPTODIR="/usr/include"
fi

if test ${CRYPTOLIB}; then
	CRYPTOLIB=$(cd $CRYPTOLIB; pwd)
else
	CRYPTOLIB="/usr/lib"
fi

if test ${PROTOBUFDIR}; then
	PROTOBUFDIR=$(cd $PROTOBUFDIR; pwd)
else
	PROTOBUFDIR="/usr/local/include"
fi

if test ${PROTOBUFLIB}; then
	PROTOBUFLIB=$(cd $PROTOBUFLIB; pwd)
else
	PROTOBUFLIB="/usr/local/lib"
fi

# Checks for programs.
AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_LANG([C++])
LT_PREREQ([2.2])
LT_INIT()

# checking platform
case $host_os in
  darwin* )
		case "$host" in
		  *-darwin* | *-macos10*)
			if test -d /opt/local ; then
				CFLAGS="$CFLAGS -I/opt/local/include"
				CXXFLAGS="$CXXFLAGS -I/opt/local/include"
				CPPFLAGS="$CPPFLAGS -I/opt/local/include"
				LDFLAGS="$LDFLAGS -L/opt/local/lib"
			elif test -d /sw ; then
				CFLAGS="$CFLAGS -I/sw/include"
				CXXFLAGS="$CXXFLAGS -I/sw/include"
				CPPFLAGS="$CPPFLAGS -I/sw/include"
				LDFLAGS="$LDFLAGS -L/sw/lib"
			fi
			;;
		esac
		os_module=darwin
		
		;;
esac

# check for linker flags
#if [[[ $LDFLAGS =~ .*stdlib.* ]]]; then
#	AC_MSG_NOTICE([stdlib linker flag provided. adding $STDLIBFLAGS_LD skipped])
#else
#	if [[[ $STDLIBFLAGS_LD =~ .*libc\+\+.* ]]]; then
#		AX_CHECK_LINK_FLAG([-stdlib=libc++], [LDFLAGS="$LDFLAGS $STDLIBFLAGS_LD"])
#	else
#		AX_CHECK_LINK_FLAG([-stdlib=libstdc++], [LDFLAGS="$LDFLAGS $STDLIBFLAGS_LD"])
#	fi
#fi

# setting conditionals for automake
AM_CONDITIONAL([OS_DARWIN],  [test $os_module = darwin])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h sys/time.h iostream.h exception.h string.h])
# Checks for functions in header files
AC_CHECK_FUNCS([gettimeofday])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# -std=C++11 still being fixed
AX_CXX_COMPILE_STDCXX_11(, mandatory)

# Check for Boost shared pointers
AC_MSG_CHECKING([for boost::shared_ptr])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <boost/shared_ptr.hpp>]]
    [[boost::shared_ptr<int> have_shared_ptr;]])
], [
    AC_MSG_RESULT([yes])
    HAVE_BOOST_SHARED_PTR=1
], [
    AC_MSG_RESULT([no])
    HAVE_BOOST_SHARED_PTR=0
])
AC_DEFINE_UNQUOTED([HAVE_BOOST_SHARED_PTR], $HAVE_BOOST_SHARED_PTR, [1 if have the `boost::shared_ptr' class.])

# Check for NDN-CPP
SAVED_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -I$NDNCPPDIR -I$BOOSTDIR"
SAVED_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -L$NDNCPPLIB"
SAVED_LIBS=$LIBS
LIBS="$LIBS -lndn-cpp"

AC_CHECK_HEADERS([ndn-cpp/interest.hpp],,[AC_MSG_FAILURE([can't find interest.hpp])])
AC_MSG_CHECKING([for NDN-CPP library...])

AC_LINK_IFELSE(
 	[AC_LANG_PROGRAM([#include <ndn-cpp/interest.hpp>],
 		[ndn::Interest interest])],
 		[AC_MSG_RESULT([yes])],
 		[AC_MSG_FAILURE([can't find NDN-CPP lib])]
)

AC_MSG_CHECKING([if NDN-CPP library was compiled for using boost::shared_ptr])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([
		#include <ndn-cpp/interest.hpp>
		#include <boost/shared_ptr.hpp>
		],
		[
			ndn::ptr_lib::shared_ptr<int> ptrlibPtr(new int(5));
			boost::shared_ptr<int> boostPtr(new int(6));
			ptrlibPtr = boostPtr;
		]
	)],
	[AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])]
	[AC_MSG_FAILURE([NDN-CPP installation at $NDNCPPBIN does not provide support for boost::shared_ptr])]	
)

CPPFLAGS=$SAVED_CPPFLAGS
LDFLAGS=$SAVED_LDFLAGS
LIBS=$SAVED_LIBS

# Check for pthread library
SAVED_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -I${PTHREADDIR}"
SAVED_LDFLAGS=$LDFLAGS
LDFLAGS="$LDLFAGS -L$PTHREADLIB"
SAVED_LIBS="$LIBS"

AC_CHECK_HEADERS([pthread.h],,[AC_MSG_FAILURE([can't find pthread headers])])
AC_CHECK_LIB([pthread], [pthread_atfork],,[AC_MSG_FAILURE([can't find pthread library])])

CPPFLAGS=$SAVED_CPPFLAGS
LDFLAGS=$SAVED_LDFLAGS
LIBS=$SAVED_LIBS

# Check for crypto library
SAVED_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -I${CRYPTODIR}"
SAVED_LDFLAGS=$LDFLAGS
LDFLAGS="$LDLFAGS -L$CRYPTOLIB"
SAVED_LIBS="$LIBS"
LIBS="$LIBS -lcrypto"

AC_CHECK_HEADERS([openssl/rand.h],,[AC_MSG_FAILURE([can't find crypto headers])])
AC_CHECK_LIB([crypto], [RAND_bytes],,[AC_MSG_FAILURE([can't find crypto library])])

CPPFLAGS=$SAVED_CPPFLAGS
LDFLAGS=$SAVED_LDFLAGS
LIBS=$SAVED_LIBS

# Check for protobuf library
SAVED_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -I${PROTOBUFDIR}"
SAVED_LDFLAGS=$LDFLAGS
LDFLAGS="$LDLFAGS -L$PROTOBUFLIB"
SAVED_LIBS="$LIBS"
LIBS="$LIBS -lprotobuf"

AC_MSG_CHECKING([for Protobuf])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#include <google/protobuf/message.h>]]
    [[google::protobuf::Message *message;]])
], [
    AC_MSG_RESULT([yes])
    HAVE_PROTOBUF=1
], [
    AC_MSG_RESULT([no])
    HAVE_PROTOBUF=0
])
AC_DEFINE_UNQUOTED([HAVE_PROTOBUF], $HAVE_PROTOBUF, [1 if have Google Protobuf.])
if test "$HAVE_PROTOBUF" == "1" ; then
  AC_CHECK_LIB([protobuf], [main], [],
               [AC_MSG_FAILURE([have protobuf headers but no libprotobuf])])
  # libprotobuf needs libpthread.
  AC_CHECK_LIB([pthread], [main], [],
               [AC_MSG_FAILURE([have libprotobuf but no libpthread])])
fi

LIBS=$SAVED_LIBS
CPPFLAGS=$SAVED_CPPFLAGS
LDFLAGS=$SAVED_LDFLAGS

AC_OUTPUT

AC_OUTPUT(Makefile)