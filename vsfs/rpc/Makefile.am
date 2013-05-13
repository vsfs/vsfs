# vim: ts=8:st=8:noexpandtab
#
# Makefile for vsfs rpc definations.

AM_CXXFLAGS = @libthrift_CFLAGS@
LDADD = @libthrift_LIBS@
noinst_LIBRARIES = libvsfs-rpc.a
BUILT_SOURCES = vsfs_constants.h

libvsfs_rpc_a_SOURCES = \
  IndexServer.h IndexServer.cpp \
  MasterDaemon.h MasterDaemon.cpp \
  MetaServer.h MetaServer.cpp \
  rpc_client.h \
  vsfs_constants.h vsfs_constants.cpp \
  vsfs_types.h vsfs_types.cpp

vsfs_constants.h: vsfs.thrift
	thrift --gen cpp $<
	mv gen-cpp/* .
	rm -rf gen-cpp
