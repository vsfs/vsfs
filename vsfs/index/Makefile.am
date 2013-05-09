# vim: ts=8:st=8:noexpandtab
# Makefile for file index
CLEANFILES=*.gcov *.gcno *.gcov *.gcda

noinst_LIBRARIES = libvsfs-index.a
libvsfs_index_a_SOURCES = \
	index_info.h index_info.cpp \
	range_index.h range_index.cpp

TESTS = \
	range_index_test

check_PROGRAMS = $(TESTS)

LDADD = libvsfs-index.a -lgtest -lgtest_main

range_index_test_SOURCES = range_index.h range_index_test.cpp
