MAINTAINERCLEANFILES	= Makefile.in

dist_doc_DATA		= 99cluster.ldif example.ldif

AM_CFLAGS		= -fPIC $(corosync_CFLAGS)

AM_LDFLAGS		= -lldap

LCRSO			= config_ldap.lcrso

SOURCES			= configldap.c

EXTRA_DIST		= $(SOURCES)

include $(top_srcdir)/make/lcrso.mk
