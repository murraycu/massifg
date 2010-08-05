
AC_INIT([MassifG], [0.2], [jonn@openismus.com], [massifg], [http://openismus.com])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 no-define foreign])
AC_CONFIG_HEADERS([config.h])
AM_PROG_CC_C_O
AC_CONFIG_FILES([Makefile])

PKG_CHECK_MODULES([DEPS], [gtk+-2.0 >= 2.20 gmodule-export-2.0])
PKG_CHECK_MODULES([PARSER_DEPS], [glib-2.0 >= 2.6.0])

# For --enable-warnings*
DK_ARG_ENABLE_WARNINGS([MASSIFG_WARNING_FLAGS],
                       [-Wall -w1],
                       [-pedantic -Wall -Wextra -w1],
                       [GTK GDK GDK_PIXBUF G])

AC_OUTPUT



