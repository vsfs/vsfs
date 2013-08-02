AM_CPPFLAGS = -I../lib $(fuse_CFLAGS) -DFUSE_USE_VERSION=28 $(libthrift_CFLAGS) \
	      -I@top_srcdir@/lib/vobla

bin_PROGRAMS = mount.vsfs
noinst_LIBRARIES = libvsfs-fuse.a

libvsfs_fuse_a_SOURCES = \
  vsfs_ops.h vsfs_ops.cpp

libvsfs_fuse_a_LIBADD = \
  $(BOOST_SYSTEM_LIBS) \
  ../common/libvsfs-common.a

mount_vsfs_SOURCES = \
  vsfs.cpp
mount_vsfs_LDFLAGS = -L$(top_srcdir)/lib/vobla/vobla
mount_vsfs_LDADD = \
  libvsfs-fuse.a \
  ../client/libvsfs-client.a \
  ../rpc/libvsfs-rpc.a \
  ../index/libvsfs-index.a \
  ../common/libvsfs-common.a \
  -lvobla $(libthrift_LIBS) $(fuse_LIBS)

LDADD = $(vsfs_LDADD) $(GTEST_LIBS) $(GMOCK_LIBS)

TESTS = \
  vsfs_ops_test \
  testing/fuse_tests.py

check_PROGRAMS = vsfs_ops_test
vsfs_ops_test_SOURCES = vsfs_ops.h vsfs_ops.cpp vsfs_ops_test.cpp
