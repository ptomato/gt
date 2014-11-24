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
lib_LTLIBRARIES = libgt.la
libgt_la_SOURCES = \
	src/mockfileinputstream.vala \
	src/mockfileoutputstream.vala \
	src/mockfile.vala \
	src/mockvfs.vala \
	$(NULL)
libgt_la_VALAFLAGS = \
	@GT_PACKAGES@ \
	-H gt.h \
	--vapi gt.vapi \
	$(NULL)
BUILT_SOURCES = gt.h
EXTRA_DIST += gt.vapi
CLEANFILES += gt.h gt.vapi

giomoduledir = $(GIO_MODULE_DIR)
giomodule_LTLIBRARIES = libgtmodule.la
libgtmodule_la_SOURCES = src/module.vala
libgtmodule_la_VALAFLAGS = \
	@GT_PACKAGES@ \
	--vapidir $(builddir) --pkg gt \
	$(NULL)
libgtmodule_la_LDFLAGS = \
	-module -avoid-version -export_dynamic -no-undefined \
	-export-symbols-regex '^g_io_module_(load|unload|query)' \
	$(NULL)
libgtmodule_la_LIBADD = libgt.la

## Install rules
## -------------
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
TEST_LINKER_FLAGS = libgt.la $(AM_LDFLAGS)

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
