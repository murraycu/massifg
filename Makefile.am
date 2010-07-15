AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = $(DEPS_CFLAGS)

# Building of main application
bin_PROGRAMS = bin/massifg
bin_massifg_SOURCES = src/massifg.c \
		src/massifg_parser.c src/massifg_parser.h \
                src/massifg_graph.c src/massifg_graph.h

bin_massifg_LDADD = $(DEPS_LIBS)

# Distribution
dist_noinst_SCRIPTS = autogen.sh

# Unit/Functional tests setup
check_PROGRAMS = tests/parser
tests_parser_SOURCES = tests/parser.c
tests_parser_CPPFLAGS = $(PARSER_DEPS_CFLAGS) $(CUSTOM_WFLAGS) -I$(top_srcdir)/src
tests_parser_LDADD = ${PARSER_DEPS_LIBS}


# ----------------------------------------------
# Rules for hooking up the unit/functional tests
GTESTER = gtester
GTESTER_REPORT = gtester-report

# test: run all tests
test: ${check_PROGRAMS}
	@test -z "${check_PROGRAMS}" || ${GTESTER} --verbose ${check_PROGRAMS}

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
	    ${GTESTER} --verbose $$test_options -o test-report.xml ${check_PROGRAMS}; \
	  elif test -n "${check_PROGRAMS}"; then \
	    ${GTESTER} --verbose $$test_options -o `mktemp "$$GTESTER_LOGDIR/log-XXXXXX"` ${check_PROGRAMS}; \
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
