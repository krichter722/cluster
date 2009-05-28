MAINTAINERCLEANFILES	= Makefile.in

libversion		= 3:0:0

include_HEADERS		= ccs.h

noinst_HEADERS		= ccs_internal.h

pkgconfigdir		= $(libdir)/pkgconfig

pkgconfig_DATA		= libccs.pc

lib_LTLIBRARIES		= libccs.la

libccs_la_SOURCES	= libccs.c xpathlite.c fullxpath.c extras.c

libccs_la_CPPFLAGS	= -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
			  -I$(top_srcdir)/common/liblogthread/

libccs_la_CFLAGS	= $(confdb_CFLAGS) \
			  $(xml_CFLAGS)

libccs_la_LDFLAGS	= $(xml_LIBS) $(confdb_LIBS) \
			  -version-info $(libversion)
