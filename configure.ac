#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([process], [1.0], [victor.lavaud@gmail.com])
AC_CONFIG_SRCDIR([src/process.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
LT_INIT
DX_INIT_DOXYGEN([process])

AC_ARG_ENABLE( [gnome],
               [AS_HELP_STRING([--enable-gnome],[enable listing Gnome applications])],
               [case "${enableval}" in
                    yes) gnome=true ;;
                    no)  gnome=false ;;
                    *)   AC_MSG_ERROR([bad value ${enableval} for --enable-gnome]) ;;
                esac],[gnome=false]
             )
AM_CONDITIONAL([GNOME],[test "x$gnome" = "xtrue"])

# Checks for programs.
AC_PROG_CXX
AC_PROG_OBJC
AC_PROG_OBJCXX
AC_LANG_PUSH([Objective C])
AC_CHECK_HEADERS([Foundation/Foundation.h])
AC_CHECK_HEADERS([AppKit/NSRunningApplication.h])
AC_CHECK_HEADERS([AppKit/NSWorkspace.h])
AC_LANG_POP

if test "x$gnome" = "xtrue"; then
    CPPFLAGS="$CPPFLAGS -DPS_GNOME"
    PKG_CHECK_MODULES([WNCK],[libwnck-3.0])
fi

AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])
# Checks for libraries.
BOOST_REQUIRE([1.51])
BOOST_SYSTEM
BOOST_FILESYSTEM

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T

# Checks for library functions.

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    tests/Makefile])
AC_OUTPUT
