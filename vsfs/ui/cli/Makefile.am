# vim: ts=8:st=8:noexpandtab

CLEANFILES = *.gcda *.gcno *.gcov *.1
EXTRA_DIST = \
  vsfs.1.ronn

lib_LIBRARIES = libvsfs-ui-cli.a
bin_PROGRAMS = vsfs

LDADD = libvsfs-ui-cli.a \
	${top_srcdir}/vsfs/client/libvsfs-client.a \
	${top_srcdir}/vsfs/index/libvsfs-index.a \
	${top_srcdir}/vsfs/common/libvsfs-common.a \
	${top_srcdir}/vsfs/rpc/libvsfs-rpc.a \
	-lgtest_main -lgtest -lgmock \
	-lthrift -lthriftnb \
	-lvobla -lcrypto

libvsfs_ui_cli_a_SOURCES = \
  commands.h commands.cpp

vsfs_SOURCES = vsfs.cpp
vsfs_LDADD = libvsfs-ui-cli.a \
	${top_srcdir}/vsfs/client/libvsfs-client.a \
	${top_srcdir}/vsfs/index/libvsfs-index.a \
	${top_srcdir}/vsfs/common/libvsfs-common.a \
	${top_srcdir}/vsfs/rpc/libvsfs-rpc.a \
	-lthrift -lthriftnb \
	-lvobla -lcrypto

vsfs.1: vsfs.1.ronn
	RONN_ORGANIZATION='VSFS ${VERSION}' \
        RONN_MANUAL='VSFS Manual' \
	ronn $<

man_MANS = vsfs.1

TESTS = \
	commands_test

check_PROGRAMS = $(TESTS)

commands_test_SOURCES = commands_test.cpp
