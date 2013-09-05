# vim: ts=8:st=8:noexpandtab
#
# Makefile for VSFS index server.
# Author: 2012 (c) Lei Xu <eddyxu@gmail.com>

AM_CPPFLAGS = @libthrift_CFLAGS@
#SUBDIRS = . testing
CLEANFILES=*.gcno *.gcov *.gcda
bin_PROGRAMS = indexd

noinst_LIBRARIES = libvsfs-indexd.a
libvsfs_indexd_a_SOURCES = \
	index_manager.h index_manager.cpp \
	index_controller.h index_controller.cpp \
        index_controller_interface.h \
	index_server.h index_server.cpp

indexd_SOURCES = indexd.cpp
indexd_LDADD = \
	       libvsfs-indexd.a \
	       ../rpc/libvsfs-rpc.a \
	       ../index/libvsfs-index.a \
	       ../common/libvsfs-common.a \
	       @libthrift_LIBS@ -lthriftnb $(libprotobuf_LIBS)

TESTS = \
	index_manager_test \
	index_controller_test

check_PROGRAMS = $(TESTS)

LDADD = \
	libvsfs-indexd.a \
	../rpc/libvsfs-rpc.a \
	../index/libvsfs-index.a \
	../common/libvsfs-common.a \
	-lvobla -lgtest -lgmock -lgmock_main @libthrift_LIBS@

index_manager_test_SOURCES = index_manager_test.cpp
index_controller_test_SOURCES = index_controller_test.cpp
