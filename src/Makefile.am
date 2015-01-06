lib_LTLIBRARIES = libprocess.la

AM_LIBTOOLFLAGS = --tag=CC

libprocess_la_CPPFLAGS = \
	-iquote$(top_srcdir)/include \
	$(WNCK_CFLAGS)

libprocess_la_SOURCES = \
	common.h \
	process.h \
	snapshot.h \
	icon.h \
	cocoa.h \
	cocoa.mm

libprocess_la_LDFLAGS = \
	-release @PACKAGE_VERSION@
