INCLUDES = -I$(top_srcdir)/src $(all_includes)
METASOURCES = AUTO

AM_CFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\"

sensordir=$(pkglibdir)/plugins/sensor

noinst_HEADERS =

sensor_LTLIBRARIES = rrd.la
rrd_la_SOURCES = rrd.c
rrd_la_LDFLAGS = -module
