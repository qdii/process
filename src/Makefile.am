lib_LTLIBRARIES = libprocess.la

AM_LIBTOOLFLAGS = --tag=CC

libprocess_la_SOURCES = \
	common.h \
	process.h \
	snapshot.h

libprocess_la_LDFLAGS = \
	-framework AppKit \
	-framework Foundation

libprocess_la_CPPFLAGS =

if COCOA
libprocess_la_SOURCES += \
	cocoa.mm \
	cocoa.h
endif

if GNOME
libprocess_la_CPPFLAGS += \
	$(WNCK_CFLAGS)
endif
