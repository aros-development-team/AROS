# $Id$
TOP=../../..

include $(TOP)/config/make.cfg

#CC=$(PURIFY) $(SYS_CC)
EXEDIR=$(BINDIR)/Demos

OBJDIR = $(GENDIR)/demos/2view
LIBS = -L$(LIBDIR) -lamiga -larossupport -larosc
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a $(LIBDIR)/startup.o \
	$(LIBDIR)/libarosc.a

FILES = 2View Misc read

SRCS = $(foreach f,$(FILES),$(f).c)
OBJS = $(foreach f,$(FILES),$(OBJDIR)/$(f).o)
EXE = $(EXEDIR)/2view

all: setup $(EXE)

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	-$(RM) $(EXES)

$(EXE): $(OBJS) $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $(OBJS) -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(APPCFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)
endif
