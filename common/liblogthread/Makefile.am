MAINTAINERCLEANFILES	= Makefile.in

libversion		= 3:0:0

include_HEADERS		= liblogthread.h

pkgconfigdir		= $(libdir)/pkgconfig

pkgconfig_DATA		= liblogthread.pc

lib_LTLIBRARIES		= liblogthread.la

liblogthread_la_LDFLAGS	= -lpthread \
			  -version-info $(libversion)
