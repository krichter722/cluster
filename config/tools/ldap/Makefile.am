MAINTAINERCLEANFILES	= Makefile.in

sbin_PROGRAMS		= confdb2ldif

confdb2ldif_CPPFLAGS	= -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

confdb2ldif_CFLAGS	= $(confdb_CFLAGS)

confdb2ldif_LDFLAGS	= $(confdb_LIBS) -lcoroipcc
