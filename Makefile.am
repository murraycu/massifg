
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_CPPFLAGS = $(MASSIFG_WARNING_FLAGS)

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# Building of main application
bin_PROGRAMS = bin/massifg
bin_massifg_SOURCES = src/massifg.c
bin_massifg_CPPFLAGS = $(DEPS_CFLAGS) $(MASSIFG_WARNING_FLAGS)
bin_massifg_LDADD = libmassifg.la $(DEPS_LIBS)

# Temporary library
noinst_LTLIBRARIES = libmassifg.la
libmassifg_la_SOURCES = \
		src/massifg_application.c src/massifg_application.h \
		src/massifg_parser.c src/massifg_parser.h src/massifg_parser_private.h\
                src/massifg_graph.c src/massifg_graph.h src/massifg_graph_private.h\
		src/massifg_utils.c src/massifg_utils.h \
		src/massifg_gtkui.c src/massifg_gtkui.h
libmassifg_la_CPPFLAGS = ${bin_massifg_CPPFLAGS}

# Distribution
dist_noinst_SCRIPTS = autogen.sh

desktopdir = $(datadir)/applications
dist_desktop_DATA = data/massifg.desktop

uidir = $(pkgdatadir)
dist_ui_DATA = data/massifg.glade \
               data/menu.ui

# XXX: docs/gtk-doc.make is disted explicitly because if that is not done,
# make on the tarball fails (but not make distcheck !) because it is looking
# for that file. However, docs/reference/Makefile.in in the tarball does actually
# have the contents of gtk-doc.make included
# Other projects using very similar setup like libepc seems to work when this file is not disted
dist_noinst_DATA = \
	docs/gtk-doc.make \
	tests/massif-output-2snapshots.txt \
	tests/massif-output-glom.txt \
	tests/massif-output-broken-800.txt

# Unit/Functional tests setup
apptest_PROGRAMS = tests/application
apptestdir = $(checkdir)
check_PROGRAMS = tests/common tests/utils tests/parser tests/graph

tests_common_SOURCES = tests/common.c tests/common.h
tests_common_CPPFLAGS = $(bin_massifg_CPPFLAGS) $(CUSTOM_WFLAGS) -I$(top_srcdir)/src
tests_common_LDADD = $(bin_massifg_LDADD)

tests_graph_SOURCES = tests/graph.c
tests_graph_CPPFLAGS = $(bin_massifg_CPPFLAGS) $(CUSTOM_WFLAGS) -I$(top_srcdir)/src
tests_graph_LDADD = $(bin_massifg_LDADD)

tests_parser_SOURCES = tests/parser.c
tests_parser_CPPFLAGS = $(bin_massifg_CPPFLAGS) $(CUSTOM_WFLAGS) -I$(top_srcdir)/src
tests_parser_LDADD = $(bin_massifg_LDADD)

tests_application_SOURCES = tests/application.c
tests_application_CPPFLAGS = $(bin_massifg_CPPFLAGS) $(CUSTOM_WFLAGS) -I$(top_srcdir)/src
tests_application_LDADD = $(bin_massifg_LDADD)

tests_utils_SOURCES = tests/utils.c
tests_utils_CPPFLAGS = $(bin_massifg_CPPFLAGS) $(CUSTOM_WFLAGS) -I$(top_srcdir)/src
tests_utils_LDADD = $(bin_massifg_LDADD)

# Documentation
gtkdoc_builddir=$(top_builddir)/docs/reference/
gtkdoc_distdir=$(top_distdir)/docs/reference/
gtkdoc_srcdir=$(top_srcdir)/docs/reference/

all-local: $(noinst_LTLIBRARIES)
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) all

clean-local:
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) clean
	rm -f src/*.stamp src/*.tmp

distclean-local:
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) distclean

docs: $(noinst_LTLIBRARIES)
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) docs

install-data-local:
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) install-data

uninstall-local:
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) uninstall

dist-hook:
	mkdir -p $(gtkdoc_distdir)
	cp $(gtkdoc_srcdir)/Makefile.am $(gtkdoc_srcdir)/Makefile.in $(gtkdoc_distdir)
	cd $(gtkdoc_builddir) && $(MAKE) $(AM_MAKEFLAGS) \
	distdir="../../$(gtkdoc_distdir)" \
	top_distdir="../../$(top_distdir)" \
	dist-hook

.PHONY: docs

# ----------------------------------------------
# Rules for hooking up the unit/functional tests
GTESTER = gtester
GTESTER_REPORT = gtester-report

# app-test: run application tests. This is separate from test/check because these tests
# require an X-server, and should be verified manually
app-test: ${check_PROGRAMS}
	@test -z "${apptest_PROGRAMS}" || top_srcdir=${top_srcdir} ${GTESTER} --verbose ${apptest_PROGRAMS}

# test: run all tests
test: ${check_PROGRAMS}
	@test -z "${check_PROGRAMS}" || top_srcdir=${top_srcdir} ${GTESTER} --verbose ${check_PROGRAMS}

# test-report: run tests and generate report
# perf-report: run tests with -m perf and generate report
# full-report: like test-report: with -m perf and -m slow
test-report perf-report full-report: ${check_PROGRAMS}
	@ignore_logdir=true; \
	if test -z "$$GTESTER_LOGDIR"; then \
	  GTESTER_LOGDIR=`mktemp -d "\`pwd\`/.testlogs-XXXXXX"`; export GTESTER_LOGDIR; \
	  ignore_logdir=false; \
	fi; \
	test -z "${check_PROGRAMS}" || { \
	  case $@ in \
	  test-report) test_options="-k";; \
	  perf-report) test_options="-k -m=perf";; \
	  full-report) test_options="-k -m=perf -m=slow";; \
	  esac; \
	  if test -z "$$GTESTER_LOGDIR"; then	\
	    top_srcdir=${top_srcdir} ${GTESTER} --verbose $$test_options -o test-report.xml ${check_PROGRAMS}; \
	  elif test -n "${check_PROGRAMS}"; then \
	    top_srcdir=${top_srcdir} ${GTESTER} --verbose $$test_options -o `mktemp "$$GTESTER_LOGDIR/log-XXXXXX"` ${check_PROGRAMS}; \
	  fi; \
	}; \
	$$ignore_logdir || { \
	  echo '<?xml version="1.0"?>' > $@.xml; \
	  echo '<report-collection>'  >> $@.xml; \
	  for lf in `ls -L "$$GTESTER_LOGDIR"/.`; do \
	    sed '1,1s/^<?xml\b[^>?]*?>//' <"$$GTESTER_LOGDIR"/"$$lf" >> $@.xml; \
	  done; \
	  echo >> $@.xml; \
	  echo '</report-collection>' >> $@.xml; \
	  rm -rf "$$GTESTER_LOGDIR"/; \
	  ${GTESTER_REPORT} --version 2>/dev/null 1>&2; test "$$?" != 0 || ${GTESTER_REPORT} $@.xml >$@.html; \
	}
# run make test as part of make check
check-local: test

.PHONY: test test-report perf-report full-report
