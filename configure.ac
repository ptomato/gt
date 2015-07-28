m4_define([_GT_API_VERSION_MACRO], [0])
m4_define([_GT_MINOR_VERSION_MACRO], [0])
m4_define([_GT_MICRO_VERSION_MACRO], [0])

dnl Initialization
dnl --------------
AC_INIT([gt],
    [_GT_API_VERSION_MACRO._GT_MINOR_VERSION_MACRO._GT_MICRO_VERSION_MACRO])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 subdir-objects dist-xz no-dist-gzip -Wall -Wno-portability])
AM_SILENT_RULES([yes])

# Before making a release, the GT_LT_VERSION string should be modified.
# The string is of the form c:r:a. Follow these instructions sequentially:
#
#  1. If the library source code has changed at all since the last update,
#     then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
#  2. If any interfaces have been added, removed, or changed since the last
#     update, increment current, and set revision to 0.
#  3. If any interfaces have been added since the last public release,
#     then increment age.
#  4. If any interfaces have been removed or changed since the last public
#     release, then set age to 0.
AC_SUBST([GT_LT_VERSION],[0:0:0])
AC_SUBST([GT_API_VERSION],[_GT_API_VERSION_MACRO])

dnl Required files and programs
dnl ---------------------------
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AC_PROG_SED
AM_PROG_VALAC
LT_INIT
PKG_PROG_PKG_CONFIG
GOBJECT_INTROSPECTION_REQUIRE([1])
AC_PATH_PROG([GIO_QUERYMODULES], [gio-querymodules])

dnl Required libraries
dnl ------------------
AC_SUBST([GT_REQUIRED_MODULES], ["glib-2.0 gio-2.0"])
AC_SUBST([GT_REQUIRED_MODULES_PRIVATE], [])
AC_SUBST([GT_PACKAGES], ["m4_foreach([package], [gio-2.0], [--pkg package])"])
PKG_CHECK_MODULES([GT], [$GT_REQUIRED_MODULES $GT_REQUIRED_MODULES_PRIVATE])
AC_SUBST([GIO_MODULE_DIR], [`$PKG_CONFIG --variable giomoduledir gio-2.0`])

dnl Output files
dnl ------------
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([gt-$GT_API_VERSION.pc:gt.pc.in], [],
    [GT_API_VERSION='$GT_API_VERSION'])
AC_OUTPUT
