uninstall:
ifdef LIBDIRT
	${UNINSTALL} ${LIBDIRT} ${libdir}
endif
ifdef LIBSYMT
	${UNINSTALL} ${LIBSYMT} ${libdir}
endif
ifdef COROSYNCINCDIRT
	@echo WARNING: need to fix uninstall for corosync headers
endif
ifdef INCDIRT
	${UNINSTALL} ${INCDIRT} ${incdir}
endif
ifdef SBINDIRT
	${UNINSTALL} ${SBINDIRT} ${sbindir}
endif
ifdef SBINSYMT
	${UNINSTALL} ${SBINSYMT} ${sbindir}
endif
ifdef LCRSOT
	${UNINSTALL} ${LCRSOT} ${libexecdir}/lcrso
endif
ifdef INITDT
	${UNINSTALL} ${INITDT} ${initddir}
endif
ifdef UDEVT
	${UNINSTALL} ${UDEVT} ${DESTDIR}/etc/udev/rules.d
endif
ifdef DOCS
	${UNINSTALL} ${DOCS} ${docdir}
endif
ifdef LOGRORATED
	${UNINSTALL} ${LOGRORATED} ${logrotatedir}
endif
ifdef NOTIFYD
	${UNINSTALL} ${NOTIFYD} ${notifyddir}
endif
ifdef PKGCONF
	${UNINSTALL} ${PKGCONF} ${pkgconfigdir}
endif
