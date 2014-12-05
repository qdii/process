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

# Check for libraries and headers
AC_LANG_PUSH([Objective C])
AC_CHECK_HEADERS([Foundation/Foundation.h])
AC_CHECK_HEADERS([AppKit/NSRunningApplication.h])
AC_CHECK_HEADERS([AppKit/NSWorkspace.h])
AX_APPEND_LINK_FLAGS(["-framework AppKit"])
AX_APPEND_LINK_FLAGS(["-framework Foundation"])
AC_LANG_POP

AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([windows.h])
AC_CHECK_HEADERS([gdiplus.h])
AC_CHECK_HEADERS([shellapi.h])
AC_CHECK_HEADERS([winbase.h])
AC_CHECK_HEADERS([ole2.h])
AC_CHECK_HEADERS([codecvt])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([assert.h])

AC_CHECK_HEADERS([signal.h])
AC_CHECK_HEADERS([iomanip])
AC_CHECK_HEADERS([iterator])
AC_CHECK_HEADERS([sstream])
AC_CHECK_HEADERS([utility])
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([fstream])
AC_CHECK_HEADERS([vector])
AC_CHECK_HEADERS([memory])
AC_CHECK_FUNCS([kill])
AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM(
                    [[#include <utility>]],
                    [[int main() { return ::std::move(1); }    ]]
                  )],
   [AC_DEFINE([HAVE_STD__MOVE],[1],[Defined to 1 if std::move is available])])


AC_CHECK_TYPES([pid_t])
AC_CHECK_TYPES([unique_ptr],[],[],[[#include <memory>
                                    typedef std::unique_ptr<int> unique_ptr;]])
AC_LANG_POP

if test "x$gnome" = "xtrue"; then
    CPPFLAGS="$CPPFLAGS -DPS_GNOME"
    PKG_CHECK_MODULES([WNCK],[libwnck-3.0])
fi

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
