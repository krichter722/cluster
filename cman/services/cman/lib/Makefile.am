MAINTAINERCLEANFILES	= Makefile.in

libversion		= 4:0:0

include_HEADERS		= libcman.h

pkgconfigdir		= $(libdir)/pkgconfig

pkgconfig_DATA		= libcman.pc

lib_LTLIBRARIES		= libcman.la

libcman_la_CPPFLAGS	= -I$(top_srcdir)/config/libs/libccsconfdb \
			  -I$(top_srcdir)/cman/services/cman/include

libcman_la_CFLAGS	= $(coroipcc_CFLAGS) $(cfg_CFLAGS) $(votequorum_CFLAGS)

libcman_la_LDFLAGS	= $(coroipcc_LIBS) $(cfg_LIBS) $(votequorum_LIBS) \
			  -version-info $(libversion)

libcman_la_LIBADD	= $(top_builddir)/config/libs/libccsconfdb/libccs.la
