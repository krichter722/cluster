MAINTAINERCLEANFILES	= Makefile.in

AM_CFLAGS		= -fPIC \
			  $(cfg_CFLAGS)

LCRSO			= config_cmanpre.lcrso

SOURCES			= cman-preconfig.c

EXTRA_DIST		= $(SOURCES)

noinst_HEADERS		= cman.h nodelist.h

include $(top_srcdir)/make/lcrso.mk
