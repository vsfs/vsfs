# vim: ts=8:st=8:noexpandtab

LDADD = \
  ../masterd/libvsfs-masterd.a \
  ../client/libvsfs-client.a \
  ../rpc/libvsfs-rpc.a \
  $(libthrift_LIBS) $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB) \
  -lgtest -lgtest_main -lgmock

TESTS = \
  client_metadata_test

noinst_PROGRAMS = $(TESTS)

client_metadata_test_SOURCES = client_metadata_test.cpp

test: integration-test

integration-test: $(TESTS)
	for t in $<; do ./$$t; done

.PHONY: test integration-test
