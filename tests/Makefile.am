check_PROGRAMS = list_all_processes dump_all_icons

list_all_processes_SOURCES = list_all_processes.cpp
list_all_processes_CPPFLAGS = -iquote $(top_srcdir) $(BOOST_CPPFLAGS)
list_all_processes_LDFLAGS = $(BOOST_FILESYSTEM_LDFLAGS) $(BOOST_SYSTEM_LIBS)
list_all_processes_DEPENDENCIES = $(top_builddir)/src/libprocess.la
list_all_processes_LDADD = $(BOOST_FILESYSTEM_LIBS) $(BOOST_SYSTEM_LIBS) $(list_all_processes_DEPENDENCIES)

if GNOME
list_all_processes_CPPFLAGS += \
	$(WNCK_CFLAGS)
list_all_processes_LDADD += \
	$(WNCK_LIBS)
endif

dump_all_icons_SOURCES = dump_all_icons.cpp
dump_all_icons_CPPFLAGS = -iquote $(top_srcdir) $(BOOST_CPPFLAGS)
dump_all_icons_LDFLAGS = $(BOOST_FILESYSTEM_LDFLAGS) $(BOOST_SYSTEM_LIBS)
dump_all_icons_DEPENDENCIES = $(top_builddir)/src/libprocess.la
dump_all_icons_LDADD = $(BOOST_FILESYSTEM_LIBS) $(BOOST_SYSTEM_LIBS) $(dump_all_icons_DEPENDENCIES)

if GNOME
dump_all_icons_CPPFLAGS += \
	$(WNCK_CFLAGS)
dump_all_icons_LDADD += \
	$(WNCK_LIBS)
endif
