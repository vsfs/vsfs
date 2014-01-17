# vim: ts=8:st=8:noexpandtab
# Makefile.am for Vsfs::masterd

CLEANFILES=*.gcov *.gcno *.gcda
BUILT_SOURCES = masterd_types.h
bin_PROGRAMS = masterd
EXTRA_DIST = masterd.thrift

noinst_LIBRARIES = libvsfs-masterd.a libvsfs-masterd-test.a
libvsfs_masterd_a_SOURCES = \
  index_namespace.cpp \
  index_namespace.h \
  index_namespace_interface.h \
  master_controller.cpp \
  master_controller.h \
  master_server.cpp \
  master_server.h \
  masterd_constants.cpp \
  masterd_constants.h \
  masterd_types.cpp \
  masterd_types.h \
  mt_hashmap.h \
  namespace.cpp \
  namespace.h \
  partition_manager.cpp \
  partition_manager.h \
  partition_manager_interface.h

libvsfs_masterd_test_a_SOURCES = \
  testing/local_masterd_cluster.h \
  testing/local_masterd_cluster.cpp

EXTRA_DIST += \
  $(libvsfs_masterd_a_SOURCES)

masterd_types.h: masterd.thrift
	thrift --gen cpp -out . $<

masterd_SOURCES = masterd.cpp
masterd_LDADD = \
  libvsfs-masterd.a \
  $(top_srcdir)/vsfs/rpc/libvsfs-rpc.a \
  $(top_srcdir)/vsfs/common/libvsfs-common.a \
  $(libthrift_LIBS) -lthriftnb

TESTS = \
  index_namespace_test \
  master_controller_test \
  mt_hashmap_test \
  namespace_test \
  partition_manager_test

check_PROGRAMS = $(TESTS)
LDADD = libvsfs-masterd.a libvsfs-masterd-test.a $(masterd_LDADD) \
	-lgtest -lgtest_main -lgmock

index_namespace_test_SOURCES = index_namespace_test.cpp
master_controller_test_SOURCES = master_controller_test.cpp mock_index_namespace.h
mt_hashmap_test_SOURCES = mt_hashmap_test.cpp mt_hashmap.h
namespace_test_SOURCES = namespace_test.cpp
partition_manager_test_SOURCES = partition_manager_test.cpp

INTEGRATION_TESTS = \
		    testing/masterd_integration_test

noinst_PROGRAMS = $(INTEGRATION_TESTS)
testing_masterd_integration_test_SOURCES = testing/masterd_integration_test.cpp

test: check integration-test

integration-test: $(INTEGRATION_TESTS)
	for t in $< ; do ./$$t; done

.PHONY: test integration-test
