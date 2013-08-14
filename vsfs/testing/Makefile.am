# vim: ts=8:st=8:noexpandtab

LDADD = \
  libvsfs-testing.a \
  ../masterd/libvsfs-masterd-test.a \
  ../masterd/libvsfs-masterd.a \
  ../indexd/libvsfs-indexd.a \
  ../index/libvsfs-index.a \
  ../client/libvsfs-client.a \
  ../rpc/libvsfs-rpc.a \
  ../common/libvsfs-common.a \
  $(libthrift_LIBS) $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB) \
  -lgtest -lgtest_main -lgmock

noinst_LIBRARIES = \
  libvsfs-testing.a

libvsfs_testing_a_SOURCES = \
  local_vsfs_cluster.cpp \
  local_vsfs_cluster.h

INTEGRATION_TESTS = \
  client_metadata_test \
  command_test

noinst_PROGRAMS = $(INTEGRATION_TESTS)

client_metadata_test_SOURCES = client_metadata_test.cpp
command_test_SOURCES = command_test.cpp

test: integration-test

integration-test: $(INTEGRATION_TESTS)
	for t in $<; do ./$$t; done

.PHONY: test integration-test
