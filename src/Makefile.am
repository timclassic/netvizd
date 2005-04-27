# our code needs to know about pkglibdir
AM_CFLAGS = -DPKGLIBDIR=\"$(pkglibdir)\"

bin_PROGRAMS = netvizd
netvizd_SOURCES = netvizd.c plugin.c nvconfig.c storage.c sensor.c io.c proto.c
noinst_HEADERS = netvizd.h plugin.h nvtypes.h nvconfig.h nvlist.h storage.h sensor.h io.h proto.h

# set the include path found by configure
INCLUDES= $(LTDLINCL) $(all_includes)


SUBDIRS = plugins

# the library search path.
netvizd_LDFLAGS = -export-dynamic $(all_libraries)
netvizd_LDADD = $(PTHREAD_LIBS) $(LIBLTDL) "-dlopen" self "-dlopen" plugins/config/file.la
netvizd_DEPENDENCIES = $(LIBLTDL)
