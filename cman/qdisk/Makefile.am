MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -D_GNU_SOURCE \
			  -I$(top_srcdir)/common/liblogthread/ \
			  -I$(top_srcdir)/config/libs/libccsconfdb/ \
			  -I$(top_srcdir)/cman/services/cman/lib/

AM_LDFLAGS		= -lpthread -lz -lrt

sbin_PROGRAMS		= qdiskd mkqdisk

noinst_HEADERS		= disk.h iostate.h platform.h scandisk.h score.h

shared_SOURCES		= disk.c disk_util.c proc.c scandisk.c iostate.c

qdiskd_SOURCES		= main.c score.c bitmap.c daemon_init.c \
			  $(shared_SOURCES)

mkqdisk_SOURCES		= mkqdisk.c \
			  $(shared_SOURCES)

shared_LDADD		= $(top_builddir)/common/liblogthread/liblogthread.la

qdiskd_LDADD		= $(top_builddir)/config/libs/libccsconfdb/libccs.la \
			  $(top_builddir)/cman/services/cman/lib/libcman.la \
			  $(shared_LDADD)

mkqdisk_LDADD		= $(shared_LDADD)
