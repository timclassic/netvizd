INCLUDES = -I$(top_srcdir)/src $(all_includes)
METASOURCES = AUTO

AM_CFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\"

configdir=$(pkglibdir)/plugins/config

noinst_HEADERS = file.h
BUILT_SOURCES = file_parser.h
CLEANFILES = file_lexer.c file_parser.c file_parser.h
AM_YFLAGS = -d
#AM_LFLAGS = -d
config_LTLIBRARIES = file.la
file_la_SOURCES = file.c file_lexer.l file_parser.y file_parser.h
file_la_LDFLAGS = -module

dist_sysconf_DATA = netvizd.conf
