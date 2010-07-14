AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

AM_CPPFLAGS = $(DEPS_CFLAGS)

bin_PROGRAMS = bin/massifg
bin_massifg_SOURCES = src/massifg.c \
		src/massifg_parser.c src/massifg_parser.h \
                src/massifg_graph.c src/massifg_graph.h

bin_massifg_LDADD = $(DEPS_LIBS)

dist_noinst_SCRIPTS = autogen.sh
