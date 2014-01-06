#vim: ts=8:st=8:noexpandtab

BUILT_SOURCES = log_types.h
CLEANFILES = *.gcda *.gcno *.gcov
EXTRA_DIST = log.thrift

noinst_LIBRARIES = libvsfs-common.a
libvsfs_common_a_SOURCES = \
  complex_query.h complex_query.cpp \
  file_handler.h \
  file.h file.cpp \
  key_value_store.h \
  leveldb_store.h leveldb_store.cpp \
  object_storage_manager.h object_storage_manager.cpp \
  log_constants.h log_constants.cpp \
  log_manager.h \
  log_types.h log_types.cpp \
  mock_key_value_store.h \
  mock_leveldb_store.h \
  path_util.h path_util.cpp \
  posix_file_handler.h posix_file_handler.cpp \
  posix_path.h posix_path.cpp \
  posix_storage_manager.h posix_storage_manager.cpp \
  server_map.h server_map.cpp \
  storage_manager.h \
  test_leveldb_store.h test_leveldb_store.cpp \
  thread.h \
  types.h

EXTRA_DIST += \
  complex_query.h complex_query.cpp \
  file_handler.h \
  file_object.h file_object.cpp \
  key_value_store.h \
  leveldb_store.h leveldb_store.cpp \
  log_constants.h log_constants.cpp \
  log_manager.h \
  log_types.h log_types.cpp \
  mock_key_value_store.h \
  mock_leveldb_store.h \
  path_util.h path_util.cpp \
  posix_file_handler.h posix_file_handler.cpp \
  posix_path.h posix_path.cpp \
  posix_storage_manager.h posix_storage_manager.cpp \
  server_map.h server_map.cpp \
  storage_manager.h \
  test_leveldb_store.h test_leveldb_store.cpp \
  thread.h \
  types.h


log_types.h: log.thrift
	thrift --gen cpp -out . $<

analyze_srcs = $(filter %.cpp, $(libvsfs_common_a_SOURCES))
analyze_plists = $(analyze_srcs:%.cpp=%.plist)

MOSTLYCLEANFILES = $(analyze_plists)

$(analyze_plists): %.plist: %.cpp
	@echo "  Static analyzing" $@
	@clang -I${top_srcdir} -I${top_srcdir}/lib/vobla ${CXXFLAGS} \
		${LDFLAGS} --analyze $< -o $@
	@oclint -fatal-assembler-warnings ${@:.plist=.cpp} -- \
		-I${top_srcdir} -I${top_srcdir}/lib/vobla ${CXXFLAGS}

static-check: $(analyze_plists)

TESTS = \
	complex_query_test \
	leveldb_store_test \
	log_manager_test \
	object_storage_manager_test \
	path_util_test \
	posix_path_test \
	server_map_test

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
object_storage_manager_test_SOURCES = object_storage_manager_test.cpp
path_util_test_SOURCES = path_util_test.cpp
posix_path_test_SOURCES = posix_path.h posix_path.cpp posix_path_test.cpp
server_map_test_SOURCES = server_map_test.cpp

EXTRA_DIST += \
  complex_query_test.cpp \
  leveldb_store_test.cpp \
  log_manager_test.cpp \
  posix_path_test.cpp \
  server_map_test.cpp

.PHONY: static-check
