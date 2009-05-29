MAINTAINERCLEANFILES	= Makefile.in

AM_CFLAGS		= -fPIC \
			  $(xml_CFLAGS) $(corosync_CFLAGS)

AM_LDFLAGS		= $(xml_LIBS)

LCRSO			= config_xml.lcrso

SOURCES			= config.c

EXTRA_DIST		= $(SOURCES)

include $(top_srcdir)/make/lcrso.mk
