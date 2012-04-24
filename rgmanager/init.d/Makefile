TARGET=rgmanager cpglockd cpglockd.init.defaults

INITDT=rgmanager cpglockd

all: $(TARGET)

include ../../make/defines.mk
include $(OBJDIR)/make/clean.mk
include $(OBJDIR)/make/install.mk
include $(OBJDIR)/make/uninstall.mk

%: $(S)/%.in
	cat $^ | sed \
		-e 's#@SBINDIR@#${sbindir}#g' \
		-e 's#@INITDDIR@#${initddir}#g' \
	> $@

clean: generalclean

check:
