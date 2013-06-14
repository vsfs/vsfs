# vim: ts=8:st=8:noexpandtab
#
# Makefile for vsfs rpc definations.

LDADD = @libthrift_LIBS@
noinst_LIBRARIES = libvsfs-rpc.a
BUILT_SOURCES = vsfs_constants.h

libvsfs_rpc_a_SOURCES = \
  IndexServer.h IndexServer.cpp \
  MasterServer.h MasterServer.cpp \
  MetaServer.h MetaServer.cpp \
  rpc_client.h \
  thrift_utils.h thrift_utils.cpp \
  vsfs_constants.h vsfs_constants.cpp \
  vsfs_types.h vsfs_types.cpp

vsfs_constants.h: vsfs.thrift
	thrift --gen cpp -out . $<

TESTS = \
  thrift_utils_test

check_PROGRAMS = $(TESTS)

test_SOURCES = testdata/test_constants.h \
	       testdata/test_constants.cpp \
	       testdata/test_types.cpp \
	       testdata/test_types.h

testdata/test_constants.h: testdata/test.thrift
	thrift --gen cpp -out testdata $<

thrift_utils_test_SOURCES = thrift_utils_test.cpp $(test_SOURCES) \
	                    testdata/test_constants.h
thrift_utils_test_CXXFLAGS = $(AM_CXXFLAGS) $(libthrift_CFLAGS)
thrift_utils_test_LDADD = -lgtest -lgtest_main @libthrift_LIBS@

EXTRA_DIST = \
  vsfs.thrift
