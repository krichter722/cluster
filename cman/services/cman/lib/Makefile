TARGET= libcman

SOMAJOR=4
SOMINOR=0

include ../../../../make/defines.mk
include $(OBJDIR)/make/libs.mk
include $(OBJDIR)/make/cobj.mk
include $(OBJDIR)/make/clean.mk
include $(OBJDIR)/make/install.mk
include $(OBJDIR)/make/uninstall.mk

CFLAGS += -fPIC
CFLAGS += -I${ccsincdir} -I${cmanincdir}
CFLAGS += -I${incdir}
CFLAGS += -I../include -I../../quorum/include

LDFLAGS += -L${corosynclibdir} -lcoroutil -lcfg 
LDFLAGS += -L${ccslibdir} -lccs
LDFLAGS += -L../../quorum/lib/ -Wl,-Bstatic -lcmanquorum -Wl,-Bdynamic
