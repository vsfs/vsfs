# vim: ts=8:st=8:noexpandtab
# Makefile for file index
CLEANFILES=*.gcov *.gcno *.gcov *.gcda

noinst_LIBRARIES = libvsfs-index.a
libvsfs_index_a_SOURCES = \
  hash_index.h hash_index.cpp \
  index_impl.h \
  index_info.h index_info.cpp \
  range_index.h range_index.cpp

TESTS = \
  hash_index_test \
  range_index_test

check_PROGRAMS = $(TESTS)

LDADD = libvsfs-index.a -lgtest -lgtest_main

range_index_test_SOURCES = range_index.h range_index_test.cpp
hash_index_test_SOURCES = hash_index.h hash_index_test.cpp
