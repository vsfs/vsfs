# vim: ts=8:st=8:noexpandtab
# Makefile.am for Vsfs::masterd

CLEANFILES=*.gcov *.gcno *.gcda
BUILT_SOURCES = protobuf
#bin_PROGRAMS = masterd

noinst_LIBRARIES = libvsfs-masterd.a
libvsfs_masterd_a_SOURCES = \
  index_server_manager.cpp \
  index_server_manager.h \
  index_path_map.h \
  index_path_map.cpp \
  masterd.pb.cc \
  masterd.pb.h \
  partition_manager.cpp \
  partition_manager.h

protobuf: masterd.pb.h

masterd.pb.h: masterd.proto
	$(PROTOC) --cpp_out=. $<

#masterd_SOURCES = masterd.cpp
#masterd_LDADD = \
#  libvsfs-masterd.a

TESTS = \
  index_path_map_test \
  index_server_manager_test \
  partition_manager_test

check_PROGRAMS = $(TESTS)
LDADD = libvsfs-masterd.a ../common/libvsfs-common.a $(masterd_LDADD) \
	-lgtest -lgtest_main -lgmock \
	$(libthrift_LIBS) $(PROTOBUF_LIBS)
index_path_map_test_SOURCES = index_path_map_test.cpp
index_server_manager_test_SOURCES = index_server_manager_test.cpp
partition_manager_test_SOURCES = partition_manager_test.cpp

.PHONY: protobuf
