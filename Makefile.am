## Definitions
## -----------
ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = @GT_CFLAGS@
AM_LDFLAGS = @GT_LIBS@

EXTRA_DIST =
CLEANFILES =
DISTCLEANFILES =
MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	INSTALL \
	tap-driver.sh \
	$(NULL)

## Build rules
## -----------
lib_LTLIBRARIES = libgt-@GT_API_VERSION@.la
libgt_@GT_API_VERSION@_la_SOURCES = \
	src/mockfileinputstream.vala \
	src/mockfileoutputstream.vala \
	src/mockfile.vala \
	src/mockvfs.vala \
	$(NULL)
libgt_@GT_API_VERSION@_la_VALAFLAGS = \
	@GT_PACKAGES@ \
	-H gt.h \
	--vapi gt-@GT_API_VERSION@.vapi \
	$(NULL)
libgt_@GT_API_VERSION@_la_LDFLAGS = -version-info @GT_LT_VERSION@ $(AM_LDFLAGS)
BUILT_SOURCES = gt.h
EXTRA_DIST += gt-@GT_API_VERSION@.vapi
CLEANFILES += gt.h gt-@GT_API_VERSION@.vapi

giomoduledir = $(GIO_MODULE_DIR)
giomodule_LTLIBRARIES = libgtmodule-@GT_API_VERSION@.la
libgtmodule_@GT_API_VERSION@_la_SOURCES = src/module.vala
libgtmodule_@GT_API_VERSION@_la_VALAFLAGS = \
	@GT_PACKAGES@ \
	--vapidir $(builddir) --pkg gt-@GT_API_VERSION@ \
	$(NULL)
libgtmodule_@GT_API_VERSION@_la_LDFLAGS = \
	-module -avoid-version -export_dynamic -no-undefined \
	-export-symbols-regex '^g_io_module_(load|unload|query)' \
	$(AM_LDFLAGS) \
	$(NULL)
libgtmodule_@GT_API_VERSION@_la_LIBADD = libgt-@GT_API_VERSION@.la

## Install rules
## -------------
gtincludedir = $(includedir)/gt-@GT_API_VERSION@/gt
gtinclude_HEADERS = gt.h

# Note that the template file is called library.pc.in, but generates a versioned
# .pc file using some magic in AC_CONFIG_FILES.
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gt-@GT_API_VERSION@.pc
DISTCLEANFILES += $(pkgconfig_DATA)
EXTRA_DIST += gt.pc.in

# It's required to run gio-querymodules after (un)installing a GIO module.
install-exec-hook:
	if test -n "$(GIO_QUERYMODULES)" -a -z "$(DESTDIR)"; then \
		$(GIO_QUERYMODULES) $(GIO_MODULE_DIR); \
	fi

uninstall-hook:
	if test -n "$(GIO_QUERYMODULES)" -a -z "$(DESTDIR)"; then \
		$(GIO_QUERYMODULES) $(GIO_MODULE_DIR); \
	fi

## Tests
## -----
noinst_PROGRAMS = \
	test-mockfile \
	test-gfileapi \
	$(NULL)
TEST_LINKER_FLAGS = libgt-@GT_API_VERSION@.la $(AM_LDFLAGS)

test_mockfile_SOURCES = test/mockfile.c gt.h
test_mockfile_LDFLAGS = $(TEST_LINKER_FLAGS)

test_gfileapi_SOURCES = test/gfileapi.c gt.h
test_gfileapi_LDFLAGS = $(TEST_LINKER_FLAGS)

TESTS = \
	test-mockfile \
	test-gfileapi \
	$(NULL)
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
LOG_DRIVER_FLAGS = --comments
LOG_COMPILER = $(top_srcdir)/tap-wrapper.sh
EXTRA_DIST += tap-wrapper.sh
AM_TESTS_ENVIRONMENT = export GIO_EXTRA_MODULES=$(builddir)/.libs;

-include $(top_srcdir)/git.mk
