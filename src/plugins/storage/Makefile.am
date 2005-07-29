INCLUDES = -I$(top_srcdir)/src $(all_includes)
METASOURCES = AUTO

AM_CFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\"

storagedir=$(pkglibdir)/plugins/storage

noinst_HEADERS =

storage_LTLIBRARIES = pgsql.la
pgsql_la_SOURCES = pgsql.c
pgsql_la_CPPFLAGS = $(PQINCPATH)
pgsql_la_LDFLAGS = -module $(PQLIBPATH)
