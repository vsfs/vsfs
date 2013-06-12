# vim: ts=8:st=8:noexpandtab
#
# Makefile for VSFS meta server.
# Author: 2012 (c) Lei Xu <eddyxu@gmail.com>

AM_CPPFLAGS = @libthrift_CFLAGS@ -I@top_srcdir@/lib/vobla
CLEANFILES=*.gcno *.gcov *.gcda

#bin_PROGRAMS = metad

noinst_LIBRARIES = libvsfs-metad.a
libvsfs_metad_a_SOURCES = \
	meta_manager.h meta_manager.cpp
#	meta_server.h meta_server.cpp \
#	meta_server_handler.h meta_server_handler.cpp

#metad_SOURCES = metad.cpp

TESTS = \
	meta_manager_test
check_PROGRAMS = $(TESTS)

LDADD = \
	libvsfs-metad.a \
	../rpc/libvsfs-rpc.a \
	../common/libvsfs-common.a \
	-lgtest -lgtest_main -lgmock \
	@libthrift_LIBS@
meta_manager_test_SOURCES = meta_manager_test.cpp
