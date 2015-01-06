check_PROGRAMS = tests dump_all_icons

tests_SOURCES = tests.cpp
tests_CPPFLAGS = -iquote $(top_srcdir) $(BOOST_CPPFLAGS) $(WNCK_CFLAGS) $(ICNS_CFLAGS) $(PNG_CFLAGS)
tests_LDFLAGS = $(BOOST_FILESYSTEM_LDFLAGS) $(BOOST_SYSTEM_LIBS)
tests_DEPENDENCIES = $(top_builddir)/src/libprocess.la
tests_LDADD = $(BOOST_FILESYSTEM_LIBS) $(BOOST_SYSTEM_LIBS) $(tests_DEPENDENCIES) $(WNCK_LIBS) $(ICNS_LIBS) $(PNG_LIBS)

dump_all_icons_SOURCES = dump_all_icons.cpp
dump_all_icons_CPPFLAGS = -iquote $(top_srcdir) $(BOOST_CPPFLAGS) $(WNCK_CFLAGS) $(PNG_CFLAGS)
dump_all_icons_LDFLAGS = $(BOOST_FILESYSTEM_LDFLAGS) $(BOOST_SYSTEM_LIBS) $(PNG_LIBS)
dump_all_icons_DEPENDENCIES = $(top_builddir)/src/libprocess.la
dump_all_icons_LDADD = $(BOOST_FILESYSTEM_LIBS) $(BOOST_SYSTEM_LIBS) $(dump_all_icons_DEPENDENCIES) $(WNCK_LIBS)

