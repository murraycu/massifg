AC_INIT([MassifG], [0.3.0], [jonn@openismus.com], [massifg], [http://openismus.com])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 no-define foreign subdir-objects])
AC_CONFIG_HEADERS([src/config.h])
AM_PROG_CC_C_O
AC_CONFIG_FILES([Makefile docs/reference/Makefile])

# Initialize Libtool for creating libraries
LT_PREREQ([2.2.6])
LT_INIT

#Gtk-Doc
GTK_DOC_CHECK([1.14],[--docdir docs --flavour no-tmpl])
AC_CONFIG_MACRO_DIR(m4)

AC_DEFINE_UNQUOTED([INSTALL_PREFIX], ["$prefix"], [Define to the installation prefix])

PKG_CHECK_MODULES([DEPS], [gtk+-2.0 >= 2.20 gmodule-export-2.0 libgoffice-0.8])

# For --enable-warnings*
DK_ARG_ENABLE_WARNINGS([MASSIFG_WARNING_FLAGS],
                       [-Wall -Wno-unused-parameter -w1],
                       [-pedantic -Wall -Wextra -Wno-unused-parameter -w1],
                       [GTK GDK GDK_PIXBUF G])

AC_OUTPUT



