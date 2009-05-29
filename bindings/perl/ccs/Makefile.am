MAINTAINERCLEANFILES	= Makefile.in

PERL_TARGET 		= CCS.pm META.yml

PERL_MAKEFILE		= Makefile.perl

PERL_MAKEFILEPL		= Makefile.PL

LINK_SRCS		= CCS.xs typemap test.pl

EXTRA_DIST		= $(PERL_MAKEFILE_PL) $(LINK_SRCS) \
			  CCS.pm.in META.yml.in

AM_CPPFLAGS		= -I$(top_srcdir)/config/libs/libccsconfdb/

AM_LDFLAGS		= -L$(top_builddir)/config/libs/libccsconfdb/.libs -lccs

all-local: linkcode $(PERL_MAKEFILE) $(PERL_TARGET)
	${MAKE} -f $(PERL_MAKEFILE) LD_RUN_PATH="";

%.pm: $(srcdir)/%.pm.in
	cat $< | \
	sed \
		-e 's/_VERSION_/${PACKAGE_VERSION}/g' \
	> $@

%.yml: $(srcdir)/%.yml.in
	cat $< | \
	sed \
		-e 's/_VERSION_/${PACKAGE_VERSION}/g' \
	> $@

$(PERL_MAKEFILE): linkcode $(PERL_TARGET) Makefile
	perl $(PERL_MAKEFILEPL) \
		FIRST_MAKEFILE=$@ \
		INC='$(AM_CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)' \
		LIBS='$(AM_LDFLAGS) $(LDFLAGS)' \
		INSTALLDIRS=vendor

linkcode:
	if [ ! -e $(PERL_MAKEFILEPL) ]; then \
		for i in $(PERL_MAKEFILEPL) $(LINK_SRCS); do \
			$(LN_S) $(srcdir)/$$i $$i; \
		done; \
		touch linkcode; \
	fi

clean-local:
	-${MAKE} -f $(PERL_MAKEFILE) clean
	rm -f $(PERL_TARGET) *.old
	if [ -e linkcode ]; then \
		rm -f $(PERL_MAKEFILEPL) $(LINK_SRCS) linkcode; \
	fi

install-exec-local:
	${MAKE} -f $(PERL_MAKEFILE) install ;

uninstall-local:
	echo "MakeMaker does not support uninstall."
