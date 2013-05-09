#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([vsfs], [0.0.1], [eddyxu@gmail.com])
AM_INIT_AUTOMAKE
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([vsfs/common/complex_query.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_LANG([C++])
AC_CHECK_PROG([HAVE_PROTOC], [protoc], [yes], [no])
if test x"$HAVE_PROTOC" != x"yes"; then
	AC_MSG_ERROR([Protobuf compiler was not found.])
fi
AC_SUBST(PROTOC, "protoc")

# Checks for libraries.
AC_CHECK_LIB([glog], [openlog], [], [AC_MSG_ERROR(google-glog was not found.)])

# Checks for header files.
AX_BOOST_BASE([1.48])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

AX_CXX_COMPILE_STDCXX_11(noext)
if test "$ac_cv_cxx_compile_cxx11" = no ; then
	AC_MSG_ERROR("It requires C++ 11 to build.!")
fi
PKG_CHECK_MODULES([libthrift], [thrift])
AC_CHECK_LIB([gflags], [main], [], [AC_MSG_ERROR(google-flags was not found.)])

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

#CXXFLAGS="-Wall -Wextra -Werror -pedantic -Wunused-value $CXXFLAGS"
CXXFLAGS="-Wall -Wextra -pedantic -Wunused-value $CXXFLAGS"
AM_CXXFLAGS="-I\$(top_srcdir)/lib/vobla -I\$(top_srcdir)/lib/gmock-1.6.0/src"
AM_CXXFLAGS="$AM_CXXFLAGS -I\$(top_srcdir)/lib/gmock-1.6.0/gtest/src"
AM_LDFLAGS="-L\$(top_srcdir)/lib/gmock-1.6.0/lib"
AM_LDFLAGS="$AM_LDFLAGS -L\$(top_srcdir)/lib/gmock-1.6.0/gtest/lib"

AC_SUBST(CXXFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(LDFLAGS, "$LDFLAGS -L\$(top_srcdir)/lib/vobla/vobla")
AC_SUBST(LIBS, "$LIBS -lvobla")

AC_CONFIG_FILES([Makefile
				 vsfs/common/Makefile
				 vsfs/index/Makefile
				 vsfs/rpc/Makefile
		         vsfs/Makefile])
AC_OUTPUT
