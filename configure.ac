#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([process], [1.0], [victor.lavaud@gmail.com])
AC_CONFIG_SRCDIR([src/process.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])
LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

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
    tests/Makefile])
AC_OUTPUT
