MAINTAINERCLEANFILES	= Makefile.in

sbin_PROGRAMS		= cmannotifyd

notifyscript		= cman_notify

sbin_SCRIPTS		= $(notifyscript)

EXTRA_DIST		= $(notifyscript).in

cmannotifyd_SOURCES	= main.c

cmannotifyd_CPPFLAGS	= -D_GNU_SOURCE \
			  -I$(top_srcdir)/common/liblogthread/ \
			  -I$(top_srcdir)/config/libs/libccsconfdb/ \
			  -I$(top_srcdir)/cman/services/cman/lib/

cmannotifyd_LDADD	= $(top_builddir)/common/liblogthread/liblogthread.la \
			  $(top_builddir)/config/libs/libccsconfdb/libccs.la \
			  $(top_builddir)/cman/services/cman/lib/libcman.la

$(notifyscript): $(notifyscript).in
	cat $^ | sed \
		-e 's#_NOTIFYDDIR_#${NOTIFYDDIR}#g' \
		-e 's#_LOGDIR_#${LOGDIR}#g' \
	> $@
	chmod a+x $@

install-exec-local:
	$(INSTALL) -d $(DESTDIR)/$(NOTIFYDDIR)

uninstall-local:
	rmdir $(DESTDIR)/$(NOTIFYDDIR) || :;

clean-generic:
	rm -f $(notifyscript)
