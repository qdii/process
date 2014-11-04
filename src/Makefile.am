lib_LTLIBRARIES = libprocess.la

AM_LIBTOOLFLAGS = --tag=CC

libprocess_la_SOURCES = \
	common.h \
	process.h \
	snapshot.h

libprocess_la_LDFLAGS = \
	-framework AppKit \
	-framework Foundation

if COCOA
libprocess_la_SOURCES += \
	cocoa.m	\
	cocoa.h
endif
