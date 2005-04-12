INCLUDES = -I$(top_srcdir)/src $(all_includes)
METASOURCES = AUTO

AM_CFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\"

protodir=$(pkglibdir)/plugins/proto

noinst_HEADERS =

proto_LTLIBRARIES = net.la
net_la_SOURCES = net.c
net_la_LDFLAGS = -module
