# vim: ts=8:st=8:noexpandtab

CLEANFILES = *.gcda *.gcno *.gcov

lib_LIBRARIES = libvsfs-client.a

LDADD = libvsfs-client.a \
	../index/libvsfs-index.a \
	../common/libvsfs-common.a \
	../rpc/libvsfs-rpc.a \
	-lthrift -lthriftnb \
	-lvobla -lcrypto -lgtest -lgtest_main -lgmock

libvsfs_client_a_SOURCES = \
  vsfs_client.h \
  mock_vsfs_client.h \
  vsfs_rpc_client.cpp vsfs_rpc_client.h

TESTS = vsfs_rpc_client_test

check_PROGRAMS = $(TESTS)
vsfs_rpc_client_test_SOURCES = vsfs_rpc_client_test.cpp
