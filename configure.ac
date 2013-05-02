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

# Checks for libraries.
AC_CHECK_LIB([glog], [openlog], [], [AC_MSG_ERROR(google-glog was not found.)])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

AX_CXX_COMPILE_STDCXX_11(noext)
if test "$ac_cv_cxx_compile_cxx11" = no ; then
	AC_MSG_ERROR("It requires C++ 11 to build.!")
fi

AC_ARG_ENABLE([gcov],
		      [AS_HELP_STRING([--enable-gcov],
				  [enable coverage test (default: no).])],
			  [gcov_enabled=${enableval}], [gcov_enabled=no])
if test "x${gcov_enabled}" = xyes; then
  CFLAGS = "$CFLAGS -fprofile-arcs -ftest-coverage --coverage"
  CXXFLAGS="$CXXFLAGS -fprofile-arcs -ftest-coverage --coverage"
  LDFLAGS="$LDFLAGS -fprofile-arcs"
  LIBS="$LIBS -lgcov"
fi

# Checks for library functions.

CFLAGS="-Wall -Wextra -Werror -pedantic -Wunused-value $CFLAGS"
CXXFLAGS="$CFLAGS $CXXFLAGS"

AM_CXXFLAGS="-I\$(top_srcdir)/lib/vobla -I\$(top_srcdir)/lib/gmock-1.6.0/src"
AM_CXXFLAGS="$AM_CXXFLAGS -I\$(top_srcdir)/lib/gmock-1.6.0/gtest/src"
AM_LDFLAGS="-L\$(top_srcdir)/lib/gmock-1.6.0/lib"
AM_LDFLAGS="$AM_LDFLAGS -L\$(top_srcdir)/lib/gmock-1.6.0/gtest/lib"

AC_SUBST(CXXFLAGS)
AC_SUBST(AM_LDFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(LDFLAGS, "$LDFLAGS")
AC_SUBST(LIBS, "$LIBS")

AC_CONFIG_FILES([Makefile
				 vsfs/common/Makefile
		         vsfs/Makefile])
AC_OUTPUT
