#vim: ts=8:st=8:noexpandtab

BUILT_SOURCES = log_types.h
CLEANFILES = *.gcda *.gcno *.gcov

noinst_LIBRARIES = libvsfs-common.a
libvsfs_common_a_SOURCES = \
  complex_query.h complex_query.cpp \
  file_info.h file_info.cpp \
  hash_util.h hash_util.cpp \
  key_value_store.h \
  leveldb_store.h leveldb_store.cpp \
  log_constants.h log_constants.cpp \
  log_manager.h \
  log_types.h log_types.cpp \
  mock_key_value_store.h \
  mock_leveldb_store.h \
  posix_path.h posix_path.cpp \
  thread.h

log_types.h: log.thrift
	thrift --gen cpp -out . $<

TESTS = \
	complex_query_test \
	leveldb_store_test \
	log_manager_test \
	posix_path_test

check_PROGRAMS = $(TESTS)

LDADD = libvsfs-common.a \
	@top_srcdir@/lib/vobla/vobla/libvobla.la \
	@top_srcdir@/vsfs/rpc/libvsfs-rpc.a \
	-lgtest_main -lgtest -lgmock \
	$(libthrift_LIBS) \
	$(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB)

complex_query_test_SOURCES = complex_query.h complex_query.cpp \
			     complex_query_test.cpp
leveldb_store_test_SOURCES = leveldb_store_test.cpp
log_manager_test_SOURCES = log_manager_test.cpp
posix_path_test_SOURCES = posix_path.h posix_path.cpp posix_path_test.cpp
