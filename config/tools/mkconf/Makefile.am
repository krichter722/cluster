MAINTAINERCLEANFILES	= Makefile.in

sbin_PROGRAMS		= cman-mkconf

cman_mkconf_SOURCES	= mkconf.c

cman_mkconf_CPPFLAGS	= -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
			  -I$(top_srcdir)/config/libs/libccsconfdb/

cman_mkconf_CFLAGS	= $(confdb_CFLAGS) $(quorum_CFLAGS) $(cfg_CFLAGS)

cman_mkconf_LDFLAGS	= $(confdb_LIBS) $(quorum_LIBS) $(cfg_LIBS)

cman_mkconf_LDADD	= $(top_builddir)/config/libs/libccsconfdb/libccs.la
