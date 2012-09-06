include ../../make/defines.mk

TARGET1 = fence_check

SBINDIRT = $(TARGET1)

all: $(TARGET1)

include $(OBJDIR)/make/clean.mk
include $(OBJDIR)/make/install.mk
include $(OBJDIR)/make/uninstall.mk

${TARGET1}: $(S)/${TARGET1}.in
	cat $(S)/$(TARGET1).in | sed \
		-e 's#@SBINDIR@#${sbindir}#g' \
		-e 's#@LOGDIR@#${logdir}#g' \
		-e 's#@VERSION@#${RELEASE_VERSION}#g' \
	> $(TARGET1)

clean: generalclean
