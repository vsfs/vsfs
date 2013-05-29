#vim: ts=8:st=8:noexpandtab

#BUILT_SOURCES = protobuf
CLEANFILES = *.pb.h *.pb.cc *.gcda *.gcno *.gcov

noinst_LIBRARIES = libvsfs-common.a
libvsfs_common_a_SOURCES = \
  complex_query.h complex_query.cpp \
  file_info.h file_info.cpp \
  hash_util.h hash_util.cpp \
  key_value_store.h \
  leveldb_store.h leveldb_store.cpp \
  mock_key_value_store.h \
  posix_path.h posix_path.cpp \
  thread.h

protobuf: log.pb.h

log.pb.h: log.proto
	protoc --cpp_out=. $<

TESTS = \
	complex_query_test \
	leveldb_store_test \
	posix_path_test

check_PROGRAMS = $(TESTS)

LDADD = libvsfs-common.a \
	@top_srcdir@/lib/vobla/vobla/libvobla.la \
	-lgtest_main -lgtest \
	$(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB)

complex_query_test_SOURCES = complex_query.h complex_query.cpp \
			     complex_query_test.cpp
posix_path_test_SOURCES = posix_path.h posix_path.cpp posix_path_test.cpp
leveldb_store_test_SOURCES = leveldb_store_test.cpp

.PHONY: protobuf
