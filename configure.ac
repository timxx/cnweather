#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([cnWeather], [0.1.0], [weitianleung@gmail.com], [cnweather], [https://github.com/timxx/cnWeather])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

GETTEXT_PACKAGE=cnWeather
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GETTEXT package name])

# libtools
LT_INIT
AC_PROG_LIBTOOL

IT_PROG_INTLTOOL([0.40.0])
AM_GLIB_GNU_GETTEXT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([string.h] [locale.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK

# Checks for libraries
PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= 3.2])
PKG_CHECK_MODULES(LIBCURL, [libcurl])

AC_CONFIG_FILES([Makefile
		src/Makefile
        src/lib/Makefile
        src/test/Makefile
		data/Makefile
		po/Makefile.in
                 ])

AC_OUTPUT

