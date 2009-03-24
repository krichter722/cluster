include make/defines.mk

REALSUBDIRS = common config/libs/libccsconfdb cman/services config cman group doc \
	      bindings contrib

SUBDIRS = $(filter-out \
	  $(if ${without_common},common) \
	  $(if ${without_config},config/libs/libccsconfdb) \
	  $(if ${without_config},config) \
	  $(if ${without_cman},cman/services) \
	  $(if ${without_cman},cman) \
	  $(if ${without_group},group) \
	  $(if ${without_bindings},bindings) \
	  , $(REALSUBDIRS))

all: ${SUBDIRS}

${SUBDIRS}:
	[ -n "${without_$@}" ] || ${MAKE} -C $@ all

# Dependencies

common:
cman/services: config/libs/libccsconfdb
config: cman/services
cman: common config
group: cman
bindings: cman
contrib: 

oldconfig:
	@if [ -f $(OBJDIR)/.configure.sh ]; then \
		sh $(OBJDIR)/.configure.sh; \
	else \
		echo "Unable to find old configuration data"; \
	fi

install:
	set -e && for i in ${SUBDIRS}; do ${MAKE} -C $$i $@; done
	install -d ${notifyddir}
	install -d ${logdir}
	install -d ${DESTDIR}/var/lib/cluster
	install -d ${DESTDIR}/var/run/cluster

uninstall:
	set -e && for i in ${SUBDIRS}; do ${MAKE} -C $$i $@; done
	rmdir ${notifyddir} || :;
	rmdir ${logdir} || :;
	rmdir ${DESTDIR}/var/lib/cluster || :;
	rmdir ${DESTDIR}/var/run/cluster || :;

clean:
	set -e && for i in ${REALSUBDIRS}; do \
		contrib_code=1 \
		legacy_code=1 \
		${MAKE} -C $$i $@;\
	done

distclean: clean
	rm -f make/defines.mk
	rm -f .configure.sh
	rm -f *tar.gz
	rm -rf build

.PHONY: ${REALSUBDIRS}
