#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([process], [1.0], [victor.lavaud@gmail.com])
AC_CONFIG_SRCDIR([src/process.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
LT_INIT

AC_ARG_ENABLE( [cocoa], 
               [AS_HELP_STRING([enable listing Cocoa application])],
               [case "${enableval}" in
                    yes) cocoa=true ;;
                    no)  cocoa=false ;;
                    *)   AC_MSG_ERROR([bad value ${enableval} for --enable-cocoa]) ;;
                esac],[cocoa=false]
             )
AM_CONDITIONAL([COCOA],[test "x$cocoa" = "xtrue"])

# Checks for programs.
AC_PROG_CXX
AC_PROG_OBJC

if test "x$cocoa" = "xtrue"; then
    CPPFLAGS="$CPPFLAGS -DPS_COCOA"
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
