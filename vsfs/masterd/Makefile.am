# vim: ts=8:st=8:noexpandtab
# Makefile.am for Vsfs::masterd

CLEANFILES=*.gcov *.gcno *.gcda
BUILT_SOURCES = protobuf masterd_types.h
#bin_PROGRAMS = masterd

noinst_LIBRARIES = libvsfs-masterd.a
libvsfs_masterd_a_SOURCES = \
  index_server_manager.cpp \
  index_server_manager.h \
  index_namespace.h \
  index_namespace.cpp \
  masterd_constants.cpp \
  masterd_constants.h \
  masterd_types.cpp \
  masterd_types.h \
  masterd.pb.cc \
  masterd.pb.h \
  partition_manager.cpp \
  partition_manager.h

protobuf: masterd.pb.h

masterd_types.h: masterd.thrift
	thrift --gen cpp -out . $<

masterd.pb.h: masterd.proto
	$(PROTOC) --cpp_out=. $<

#masterd_SOURCES = masterd.cpp
#masterd_LDADD = \
#  libvsfs-masterd.a

TESTS = \
  index_namespace_test \
  index_server_manager_test \
  partition_manager_test

check_PROGRAMS = $(TESTS)
LDADD = libvsfs-masterd.a ../common/libvsfs-common.a $(masterd_LDADD) \
	-lgtest -lgtest_main -lgmock \
	$(libthrift_LIBS) $(PROTOBUF_LIBS)
index_namespace_test_SOURCES = index_namespace_test.cpp
index_server_manager_test_SOURCES = index_server_manager_test.cpp
partition_manager_test_SOURCES = partition_manager_test.cpp

.PHONY: protobuf
