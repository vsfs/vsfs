#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([vsfs], [0.0.1], [eddyxu@gmail.com])
AM_INIT_AUTOMAKE([color-tests parallel-tests])
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([vsfs/common/complex_query.h])
AC_CONFIG_HEADERS([config.h])

RPM_RELEASE=0
AC_SUBST(RPM_RELEASE)
if test -d ".git" ; then
	AC_CHECK_PROG(GIT_CHECK, git, yes)
	if test x"$GIT_CHECK" = x"yes"; then
		RPM_RELEASE=`if expr index $(git describe --always) '-' > /dev/null ; then git describe --always | cut -d- -f2- | tr '-' '.' ; else echo "0"; fi`
	fi
fi
AC_MSG_NOTICE([RPM_RELEASE='$RPM_RELEASE'])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_CHECK_PROG([RONN], [ronn], yes)
if test x"$RONN" != x"yes"; then
	AC_MSG_ERROR([Please install ronn ruby before compiling.])
fi
AC_LANG([C++])

# Checks for libraries.
AC_CHECK_LIB([glog], [openlog], [], [AC_MSG_ERROR(google-glog was not found.)])
AC_CHECK_LIB([gflags],[getenv],[],[AC_MSG_ERROR([google-gflags was not found.])])
# check is snappy-devel is installed, needed by leveldb
AC_CHECK_LIB([snappy], [snappy_compress], [], [AC_MSG_ERROR([libsnappy was not found.])])
# use system leveldb
AC_CHECK_LIB([leveldb], [leveldb_open], [], [AC_MSG_ERROR([libleveldb was not found.])], [-lsnappy -lpthread])
AC_CHECK_LIB([event], [main], [], [AC_MSG_ERROR([libevent was not found.])])
PKG_CHECK_MODULES([libthrift], [thrift-nb])
PKG_CHECK_MODULES([fuse], [fuse >= 2.8.0], [],
				  [AC_MSG_ERROR(fuse was not found.)])

AC_ARG_ENABLE([python],
			  AS_HELP_STRING([--enable-python], [enable python API]),
			  AC_DEFINE([HAVE_PYTHON], [1], [Define if enable python API.]))
AM_CONDITIONAL([BUILD_PYTHON], [test x"$enable_python" = "xyes"])

AS_IF([test "x$enable_python" = "xyes"], [
AX_PYTHON_DEVEL
AX_BOOST_PYTHON([1.53])
])


# Checks for header files.
AX_BOOST_BASE([1.53])
AX_BOOST_FILESYSTEM([1.53])
AX_BOOST_SYSTEM([1.53])
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([attr/xattr.h], [], [AC_MSG_ERROR([attr/xattr.h was not found.])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_C_INLINE


AX_CXX_COMPILE_STDCXX_11(noext)
if test "$ac_cv_cxx_compile_cxx11" = no ; then
	AC_MSG_ERROR("It requires C++ 11 to build.!")
fi

AC_ARG_ENABLE([gcov],
		      [AS_HELP_STRING([--enable-gcov],
				  [enable coverage test (default: no).])],
			  [gcov_enabled=${enableval}], [gcov_enabled=no])
if test "x${gcov_enabled}" = xyes; then
  CXXFLAGS="$CXXFLAGS -fprofile-arcs -ftest-coverage --coverage"
  LDFLAGS="$LDFLAGS -fprofile-arcs"
  LIBS="$LIBS -lgcov"
fi

# Checks for library functions.
AC_CHECK_FUNCS([strerrno])
AC_FUNC_CHOWN
AC_FUNC_MALLOC
AC_FUNC_ERROR_AT_LINE

#CXXFLAGS="-Wall -Wextra -Werror -pedantic -Wunused-value $CXXFLAGS"
CXXFLAGS="-Wall -Wextra -static -pedantic -Wunused-value $CXXFLAGS"
AM_CXXFLAGS="-I\$(top_srcdir)/lib/vobla -isystem \$(top_srcdir)/lib/gmock-1.7.0/include"
AM_CXXFLAGS="$AM_CXXFLAGS -isystem \$(top_srcdir)/lib/gmock-1.7.0/gtest/include"
AM_CXXFLAGS="$AM_CXXFLAGS \$(libthrift_CFLAGS)"
AM_LDFLAGS="-L\$(top_srcdir)/lib/gmock-1.7.0/lib -L\$(top_srcdir)/lib/vobla/vobla"
AM_LDFLAGS="$AM_LDFLAGS -L\$(top_srcdir)/lib/gmock-1.7.0/gtest/lib"

AC_SUBST(CXXFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(LIBS, "$LIBS -lvobla")

AC_CONFIG_FILES([Makefile
				 contrib/vsfs.spec
				 vsfs/api/Makefile
				 vsfs/api/cpp/Makefile
				 vsfs/api/py/Makefile
				 vsfs/client/Makefile
				 vsfs/common/Makefile
				 vsfs/fuse/Makefile
				 vsfs/index/Makefile
				 vsfs/indexd/Makefile
				 vsfs/masterd/Makefile
				 vsfs/query/Makefile
				 vsfs/rpc/Makefile
				 vsfs/testing/Makefile
				 vsfs/ui/Makefile
				 vsfs/ui/cli/Makefile
		         vsfs/Makefile
				 ])
AC_OUTPUT
