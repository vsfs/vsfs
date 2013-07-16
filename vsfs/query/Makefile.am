#vim: ts=8:st=8:noexpandtab

noinst_LIBRARIES = libvsfs-query.a
libvsfs_query_a_SOURCES = \
			  ast.h ast.cpp

LDADD = libvsfs-query.a \
	$(GTEST_LIBS) -lgtest_main
TESTS = ast_test
check_PROGRAMS = $(TESTS)
ast_test_SOURCES = ast_test.cpp
