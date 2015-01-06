lib_LTLIBRARIES = libprocess.la

AM_LIBTOOLFLAGS = --tag=CC

libprocess_la_CPPFLAGS = \
	-iquote$(top_srcdir)/include \
	$(WNCK_CFLAGS)

libprocess_la_SOURCES = \
	$(top_srcdir)/include/ps/common.h \
	$(top_srcdir)/include/ps/process.h \
	$(top_srcdir)/include/ps/snapshot.h \
	$(top_srcdir)/include/ps/icon.h \
	$(top_srcdir)/include/ps/cocoa.h \
	cocoa.mm

libprocess_la_LDFLAGS = \
	-release @PACKAGE_VERSION@
