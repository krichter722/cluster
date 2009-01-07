TARGET=libgroup

MAKESTATICLIB = 1

LIBDIRT=$(TARGET).a
INCDIRT=$(TARGET).h

include ../../make/defines.mk
include $(OBJDIR)/make/libs.mk
include $(OBJDIR)/make/cobj.mk
include $(OBJDIR)/make/clean.mk
include $(OBJDIR)/make/install.mk
include $(OBJDIR)/make/uninstall.mk

CFLAGS += -fPIC
CFLAGS += -I$(S) -I$(S)/../daemon
CFLAGS += -I${incdir}
