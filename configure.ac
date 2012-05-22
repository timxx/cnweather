#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([cnweather], [0.2.1], [weitianleung@gmail.com], [cnweather], [https://github.com/timxx/cnweather])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

GETTEXT_PACKAGE=cnweather
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

# GSettings

GLIB_GSETTINGS


# Checks for libraries
PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= 3.2, gmodule-export-2.0])
PKG_CHECK_MODULES(LIBCURL, [libcurl >= 7.21])
PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.30])
PKG_CHECK_MODULES(SQLITE3, [sqlite3 >= 3.7])

AC_CONFIG_FILES([Makefile
		src/Makefile
        src/lib/Makefile
        src/test/Makefile
		data/Makefile
		data/icons/Makefile
		data/ui/Makefile
		data/com.timxx.cnweather.gschema.xml.in
		po/Makefile.in
		data/themes/Makefile
		data/themes/default/Makefile
                 ])

AC_OUTPUT

