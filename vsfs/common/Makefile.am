#vim: ts=8:st=8:noexpandtab

#BUILT_SOURCES = protobuf
CLEANFILES = *.pb.h *.pb.cc *.gcda *.gcno *.gcov

noinst_LIBRARIES = libvsfs-common.a
libvsfs_common_a_SOURCES = \
			   complex_query.h complex_query.cpp \
			   posix_path.h posix_path.cpp \
			   thread.h

protobuf: log.pb.h

log.pb.h: log.proto
	protoc --cpp_out=. $<

TESTS = \
	complex_query_test \
	posix_path_test

check_PROGRAMS = $(TESTS)

LDADD = libvsfs-common.a \
	@top_srcdir@/lib/vobla/vobla/libvobla.la \
	-lgtest_main -lgtest

complex_query_test_SOURCES = complex_query.h complex_query.cpp \
			     complex_query_test.cpp
posix_path_test_SOURCES = posix_path.h posix_path.cpp posix_path_test.cpp

.PHONY: protobuf
