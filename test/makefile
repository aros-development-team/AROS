# $Id$
TOP=..

SPECIAL_CFLAGS = -D__AROS -Dmain=submain

include $(TOP)/config/make.cfg

INCLUDES = -I$(BINDIR)/include -I$(TOP)/amiga/include -I$(TOP)/libs

#CC=$(PURIFY) $(SYS_CC)

TESTDIR = $(BINDIR)/test
OBJDIR = $(GENDIR)/test

LIBS = -L$(LIBDIR) $(GENDIR)/filesys/emul_handler.o -lAmigaOS -larossupport \
	$(X11LDFLAGS) -lX11

DEPLIBS = $(LIBDIR)/libarosc.a \
	$(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a

FILES = tasktest signaltest exceptiontest tasktest2 messagetest \
	semaphoretest initstructtest devicetest filetest

EXES = $(foreach f,$(FILES),$(TESTDIR)/$(f))

all: setup $(EXES)

setup :
	@if [ ! -d $(TESTDIR) ]; then $(MKDIR) $(TESTDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	$(RM) $(TESTDIR) $(OBJDIR)

$(TESTDIR)/%: $(OBJDIR)/%.o $(DEPLIBS)
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	$(MKDEPEND) -f- -p$(OBJDIR)/ -- $(CFLAGS) -- $^ > $@

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

