INCLUDES = -I$(top_srcdir)/src $(all_includes)
METASOURCES = AUTO

AM_CFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\"

authdir=$(pkglibdir)/plugins/auth

noinst_HEADERS =

auth_LTLIBRARIES = permit.la
permit_la_SOURCES = permit.c
permit_la_LDFLAGS = -module
