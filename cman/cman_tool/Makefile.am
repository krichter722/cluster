MAINTAINERCLEANFILES	= Makefile.in

sbin_PROGRAMS		= cman_tool

noinst_HEADERS		= cman_tool.h

cman_tool_SOURCES	= main.c join.c

cman_tool_CPPFLAGS	= -I$(top_srcdir)/cman/services/cman/lib/ \
			  -I$(top_srcdir)/config/libs/libccsconfdb/

cman_tool_CFLAGS	= $(votequorum_CFLAGS)

cman_tool_LDFLAGS	= $(votequorum_LIBS)

cman_tool_LDADD		= $(top_builddir)/cman/services/cman/lib/libcman.la \
			  $(top_builddir)/config/libs/libccsconfdb/libccs.la
