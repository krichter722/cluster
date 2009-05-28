MAINTAINERCLEANFILES	= Makefile.in

dist_doc_DATA		= gfs2.txt \
			  journaling.txt \
			  min-gfs.txt \
			  usage.txt \
			  COPYING.applications \
			  COPYING.libraries \
			  COPYRIGHT \
			  README.licence

EXTRA_DIST		= cman_notify_template.sh \
			  cluster.logrotate.in

logrotate_TARGET	= cluster

$(logrotate_TARGET): $(logrotate_TARGET).logrotate.in
	cat $(srcdir)/$^ | sed \
		-e 's#_LOGDIR_#${LOGDIR}#g' \
	> $@

all-local: $(logrotate_TARGET)

clean-local:
	rm -f $(logrotate_TARGET)

install-exec-local:
	$(INSTALL) -d $(DESTDIR)/$(LOGROTATEDIR)
	$(INSTALL) -m 644 cluster $(DESTDIR)/$(LOGROTATEDIR)

uninstall-local:
	cd $(DESTDIR)/$(LOGROTATEDIR) && \
		rm -f $(logrotate_TARGET)
	rmdir $(DESTDIR)/$(LOGROTATEDIR) || :;
