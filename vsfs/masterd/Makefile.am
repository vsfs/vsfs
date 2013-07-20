# vim: ts=8:st=8:noexpandtab
# Makefile.am for Vsfs::masterd

CLEANFILES=*.gcov *.gcno *.gcda
BUILT_SOURCES = masterd_types.h
bin_PROGRAMS = masterd
EXTRA_DIST = masterd.thrift

noinst_LIBRARIES = libvsfs-masterd.a
libvsfs_masterd_a_SOURCES = \
  index_namespace.cpp \
  index_namespace.h \
  index_namespace_interface.h \
  master_controller.cpp \
  master_controller.h \
  master_server.cpp \
  master_server.h \
  namespace.h \
  namespace.cpp \
  masterd_constants.cpp \
  masterd_constants.h \
  masterd_types.cpp \
  masterd_types.h \
  partition_manager.cpp \
  partition_manager.h \
  server_manager.cpp \
  server_manager.h

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
  partition_manager_test \
  server_manager_test

check_PROGRAMS = $(TESTS)
LDADD = libvsfs-masterd.a $(masterd_LDADD) \
	-lgtest -lgtest_main -lgmock

index_namespace_test_SOURCES = index_namespace_test.cpp
master_controller_test_SOURCES = master_controller_test.cpp mock_index_namespace.h
partition_manager_test_SOURCES = partition_manager_test.cpp
server_manager_test_SOURCES = server_manager_test.cpp
