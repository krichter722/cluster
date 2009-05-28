LCRSO_OBJS		= $(SOURCES:%.c=%.o)

$(LCRSO): $(LCRSO_OBJS)
	$(CC) $(AM_LDFLAGS) $(LDFLAGS) -shared -Wl,-soname=$@ $^ -o $@

%.o: %.c
	$(CC)	$(AM_CPPFLAGS) $(AM_CFLAGS) \
		$(CFLAGS) $(CPPFLAGS) \
		$(INCLUDES) \
		-c -o $@ $<

all-local: $(LCRSO_OBJS) $(LCRSO)

install-exec-local:
	$(INSTALL) -d $(DESTDIR)/$(LCRSODIR)
	$(INSTALL) -m 755 $(LCRSO) $(DESTDIR)/$(LCRSODIR)

uninstall-local:
	cd $(DESTDIR)/$(LCRSODIR) && \
		rm -f $(LCRSO)

clean-local:
	rm -f *.o *.a *.lcrso
