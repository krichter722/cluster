ifdef outoftreebuild

%.o: $(S)/%.c
	cd $(S) > /dev/null 2>&1 && \
	src=$(shell basename $<) && \
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(REENT_CFLAGS) -c -o $(O)/$@ $$src && \
	cd - >/dev/null 2>&1;

# used by dlm/libdlm
%_lt.o: $(S)/%.c
	cd $(S) > /dev/null 2>&1 && \
	src=$(shell basename $<) && \
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $(O)/$@ $$src && \
	cd - >/dev/null 2>&1;

# used by rgmanager/src/daemons
%-noccs.o: $(S)/%.c
	cd $(S) > /dev/null 2>&1 && \
	src=$(shell basename $<) && \
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(NOCCS_CFLAGS) -c -o $(O)/$@ $$src && \
	cd - >/dev/null 2>&1;

else
%.o: $(S)/%.c
	src=$(shell basename $<) && \
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(REENT_CFLAGS) -c -o $@ $$src

# used by dlm/libdlm
%_lt.o: $(S)/%.c
	src=$(shell basename $<) && \
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $$src

# used by rgmanager/src/daemons
%-noccs.o: $(S)/%.c
	src=$(shell basename $<) && \
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(NOCCS_CFLAGS) -c -o $@ $$src

endif
