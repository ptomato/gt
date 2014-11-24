dnl Initialization
dnl --------------
AC_INIT([gt], [0.0.0])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11 subdir-objects dist-xz no-dist-gzip -Wall -Wno-portability])
AM_SILENT_RULES([yes])

dnl Versioning variables
dnl --------------------
GT_CURRENT=0  # Increment if interface changes
GT_REVISION=0 # Increment if source changes; set 0 if interface changes
GT_AGE=0 # Increment if interfaces added; set 0 if removed
LT_VERSION_INFO="$GT_CURRENT:$GT_REVISION:$GT_AGE"
AC_SUBST([LT_VERSION_INFO])

dnl Required files and programs
dnl ---------------------------
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AM_PROG_VALAC
LT_INIT
PKG_PROG_PKG_CONFIG
AC_PATH_PROG([GIO_QUERYMODULES], [gio-querymodules])

dnl Required libraries
dnl ------------------
AC_SUBST([GT_PACKAGES], ["m4_foreach([package], [gio-2.0], [--pkg package])"])
PKG_CHECK_MODULES([GT], [glib-2.0 gio-2.0])
AC_SUBST([GIO_MODULE_DIR], [`$PKG_CONFIG --variable giomoduledir gio-2.0`])

dnl Output files
dnl ------------
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
