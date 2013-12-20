# Makefile.am for VSFS's C++ API

lib_LIBRARIES = libvsfs.a

libvsfs_a_SOURCES = \
  vsfs.h vsfs.cpp \
  vsfs_impl.h vsfs_impl.cpp

TESTS = \
  vsfs_test

check_PROGRAMS = $(TESTS)
LDADD = \
	libvsfs.a \
	${top_srcdir}/vsfs/client/libvsfs-client.a \
	${top_srcdir}/vsfs/rpc/libvsfs-rpc.a \
	${top_srcdir}/vsfs/index/libvsfs-index.a \
	${top_srcdir}/vsfs/common/libvsfs-common.a \
	-lgtest_main -lgtest -lgmock \
	$(libthrift_LIBS) -lthriftnb

vsfs_impl_test_SOURCES = vsfs_impl_test.cpp
vsfs_test_SOURCES = vsfs_test.cpp
