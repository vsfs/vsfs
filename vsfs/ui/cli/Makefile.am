# vim: ts=8:st=8:noexpandtab

CLEANFILES = *.gcda *.gcno *.gcov

lib_LIBRARIES = libvsfs-ui-cli.a
bin_PROGRAMS = vsfs

LDADD = libvsfs-ui-cli.a \
	${top_srcdir}/vsfs/client/libvsfs-client.a \
	${top_srcdir}/vsfs/index/libvsfs-index.a \
	${top_srcdir}/vsfs/common/libvsfs-common.a \
	${top_srcdir}/vsfs/rpc/libvsfs-rpc.a \
	-lthrift -lthriftnb \
	-lvobla -lcrypto

libvsfs_ui_cli_a_SOURCES = \
  commands.h commands.cpp

vsfs_SOURCES = vsfs.cpp

vsfs.1: vsfs.1.ronn
	ronn $<

man_MANS = vsfs.1
