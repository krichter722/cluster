TARGET= config_cmanpre.lcrso

LCRSOT=$(TARGET)

all: depends ${TARGET}

include ../../make/defines.mk
include $(OBJDIR)/make/cobj.mk
include $(OBJDIR)/make/clean.mk
include $(OBJDIR)/make/install.mk
include $(OBJDIR)/make/uninstall.mk

CFLAGS += -fPIC
CFLAGS += -I${openaisincdir} -I${corosyncincdir}
CFLAGS += -I${incdir}

LDFLAGS += -L${libdir}

OBJS=	cman-preconfig.o

${TARGET}: ${OBJS}
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDFLAGS)

depends:
	$(MAKE) -C ${cmanlibdir} all

clean: generalclean 

-include $(OBJS:.o=.d)
