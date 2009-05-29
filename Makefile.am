EXTRA_DIST		= autogen.sh make/lcrso.mk

AUTOMAKE_OPTIONS	= foreign

MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 configure depcomp \
			  config.guess config.sub missing install-sh \
			  autoheader automake autoconf libtool libtoolize \
			  ltmain.sh compile make/config.h.in make/config.h.in~

noinst_HEADERS		= make/copyright.cf

ACLOCAL_AMFLAGS		= -I m4

SUBDIRS			= common config cman group doc

if BUILD_BINDINGS
SUBDIRS			+= bindings
endif

install-exec-local:
			$(INSTALL) -d $(DESTDIR)/$(LOGDIR)
			$(INSTALL) -d $(DESTDIR)/$(CLUSTERVARRUN)
			$(INSTALL) -d $(DESTDIR)/$(CLUSTERVARLIB)

uninstall-local:
			rmdir $(DESTDIR)/$(LOGDIR) || :;
			rmdir $(DESTDIR)/$(CLUSTERVARRUN) || :;
			rmdir $(DESTDIR)/$(CLUSTERVARLIB) || :;

maintainer-clean-local:
	rm -rf m4
