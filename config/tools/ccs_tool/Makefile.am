MAINTAINERCLEANFILES	= Makefile.in

sbin_PROGRAMS		= ccs_tool

noinst_HEADERS		= editconf.h

ccs_tool_SOURCES	= ccs_tool.c editconf.c

ccs_tool_CPPFLAGS	= -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
			  -I$(top_srcdir)/config/libs/libccsconfdb

ccs_tool_CFLAGS		= $(xml_CFLAGS)

ccs_tool_LDFLAGS	= $(xml_LIBS)

ccs_tool_LDADD		= $(top_builddir)/config/libs/libccsconfdb/libccs.la

install-exec-hook:
	(cd $(DESTDIR)/$(sbindir) && $(LN_S) ccs_tool ccs_test)

uninstall-hook:
	(cd $(DESTDIR)/$(sbindir) && rm -f ccs_test)
