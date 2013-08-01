# vim: ts=8:st=8:noexpandtab

lib_LIBRARIES = libvsfs-client.a

LDADD = @libthrift_LIBS@ \
	libvsfs-client.a \
	../index/libvsfs-index.a \
	../common/libvsfs-common.a \
	../rpc/libvsfs-rpc.a \
	-lvobla -lcrypto -lgtest -lgtest_main

vsfsutil_SOURCES = vsfsutil.cpp \
		   commands.h commands.cpp \
		   rpc_client.h

libvsfs_client_a_SOURCES = \
			   vsfs_client.h \
			   vsfs_rpc_client.cpp vsfs_rpc_client.h

TESTS = vsfs_rpc_client_test

check_PROGRAMS = $(TESTS)
vsfs_rpc_client_test_SOURCES = vsfs_rpc_client_test.cpp
