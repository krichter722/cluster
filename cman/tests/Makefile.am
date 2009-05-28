MAINTAINERCLEANFILES	= Makefile.in

noinst_PROGRAMS		= client sysman sysmand #libtestc qwait user_service

EXTRA_DIST		= user_service.c

AM_CPPFLAGS		= -I$(top_srcdir)/cman/services/cman/lib

libcman_LDADD		= $(top_builddir)/cman/services/cman/lib/libcman.la

client_LDADD		= $(libcman_LDADD)
sysman_LDADD		= $(libcman_LDADD)
sysmand_LDADD		= $(libcman_LDADD)
