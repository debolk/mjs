dnl Process this file with autoconf to produce a configure script.
AC_INIT([mjs],[4.3])

AC_PREREQ([2.69])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile src/Makefile src/plugin/Makefile tools/Makefile tests/Makefile doc/Makefile])
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([-Wall -Wno-extra-portability -Werror subdir-objects foreign])

AC_LANG([C])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

LT_INIT([dlopen])

PKG_CHECK_MODULES([XINE], [libxine])
PKG_CHECK_MODULES([CURSES], [ncursesw panelw])
PKG_CHECK_MODULES([MICROHTTPD], [libmicrohttpd])
dnl Check for CHECK library
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_CHECK_HEADER([dlfcn.h], [], AC_MSG_ERROR([Could not find dlfcn headers.]))
AC_CHECK_LIB(dl, dlclose, , AC_MSG_ERROR([Could not find dlfcn library]), -ldl)

AC_MSG_CHECKING([for APREQ2])
AS_IF([which apreq2-config &> /dev/null],
  [
    APREQ2_CFLAGS=`apreq2-config --includes`
    APREQ2_LIBS=`apreq2-config --link-ld`
    AC_SUBST([APREQ2_CFLAGS])
    AC_SUBST([APREQ2_LIBS])
	AC_MSG_RESULT([yes])
  ],
  [
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Could not find apreq2-config, please install the apreq2 library and headers.])
  ]
)

AC_OUTPUT
