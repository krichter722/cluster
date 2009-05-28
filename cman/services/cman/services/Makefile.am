MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -I$(top_srcdir)/cman/services/cman/include

AM_CFLAGS		= -fPIC \
			  $(cfg_CFLAGS)

LCRSO			= service_cmanbits.lcrso

SOURCES			= cman.c

EXTRA_DIST		= $(SOURCES)

include $(top_srcdir)/make/lcrso.mk
