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
AC_CHECK_HEADERS([winnt.h])
AC_CHECK_HEADERS([ole2.h])
AC_CHECK_HEADERS([winuser.h])
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
AC_CHECK_HEADERS([pwd.h])
AC_CHECK_HEADERS([sys/sysctl.h])
AC_CHECK_HEADERS([sys/proc_info.h])
AC_CHECK_HEADERS([libproc.h])
AC_CHECK_HEADERS([gdk-pixbuf/gdk-pixbuf.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([sys/sysctl.h])
AC_CHECK_FUNCS([kill])
AC_CHECK_FUNCS([execve])
AC_CHECK_FUNCS([fork])
AC_CHECK_FUNCS([sleep])
AC_CHECK_FUNCS([signal])
AC_CHECK_FUNCS([ShellExecute])
AC_CHECK_FUNCS([EnumProcesses])
AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM(
                    [[#include <utility>]],
                    [[int main() { return ::std::move(1); }    ]]
                  )],
   [AC_DEFINE([HAVE_STD__MOVE],[1],[Defined to 1 if std::move is available])])


AC_CHECK_TYPES([pid_t])
AC_CHECK_TYPES([unique_ptr],[],[],[[#include <memory>
                                    typedef std::unique_ptr<int> unique_ptr;]])
PKG_CHECK_MODULES([ICNS],
                  [libicns],
                  [AC_DEFINE([HAVE_LIBICNS],[1],[Defined to 1 if libicns is available])],
                  [AC_MSG_WARN([Could not detect libicns, which is necessary to enumerate windows on linux])])
PKG_CHECK_MODULES([WNCK],
                  [libwnck-3.0],
                  [AC_DEFINE([HAVE_LIBWNCK],[1],[Defined to 1 if libwnck is available])],
                  [AC_MSG_WARN([Could not detect libwnck, which is necessary to enumerate windows on linux])])
AX_CHECK_DEFINE([sys/sysctl.h],[KERN_ARGMAX],[CPPFLAGS="-DDEFINED_KERN_ARGMAX=1 $CPPFLAGS"])
AX_CHECK_DEFINE([sys/sysctl.h],[KERN_PROCARGS2],[CPPFLAGS="-DDEFINED_KERN_PROCARGS2=1 $CPPFLAGS"])
AC_LANG_POP

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
