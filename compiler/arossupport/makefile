# $Id$
TOP=../..

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/arossupport

FILES = calcchecksum kprintf nastyfreemem \
	purify removeslist debugmem \
	_kprintf hexdump \
	freestruct \
	readbyte \
	readdouble \
	readfloat \
	readlong \
	readstring \
	readstruct \
	readword \
	writebyte \
	writedouble \
	writefloat \
	writelong \
	writestring \
	writestruct \
	writeword \
	strrncasecmp \
	rawputchars

INCLUDE_FILES = $(wildcard include/*.h)
DEST_INCLUDE_FILES = $(subst include,$(INCDIR)/aros,$(INCLUDE_FILES))

all: setup $(LIBDIR)/libarossupport.a

setup : make-dirs os-include

os-include: $(DEST_INCLUDE_FILES)

$(INCDIR)/aros/%.h: include/%.h
	$(CP) $< $@

make-dirs :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(INCDIR) ]; then $(MKDIR) $(INCDIR) ; else true ; fi
	@echo "CFLAGS=$(CFLAGS)"

clean:
	-$(RM) $(OBJDIR) *.err $(LIBDIR)/libarossupport.a

$(OBJDIR)/%.o: %.c
	@echo "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err; else true ; fi

$(OBJDIR)/kprintf.o: kprintf.c
	@echo "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS) -I$(TOP)/rom/exec $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err; else true ; fi

$(OBJDIR)/rawputchars.o: rawputchars.c
	@echo "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS) -I$(TOP)/rom/exec $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err; else true ; fi

$(OBJDIR)/_kprintf.o: _kprintf.c
	@echo "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS) -I$(TOP)/rom/exec $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err; else true ; fi

$(OBJDIR)/rt_stubs.o: rt_stubs.s
	@echo "Assembling $(CURDIR)/$<..."
	@$(SYS_AS) $(AFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err; else true ; fi

$(LIBDIR)/libarossupport.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o)
	@echo "Updating $@..."
	@$(AR) $@ $?
	@$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@echo "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/ -- $(APPCFLAGS) -I$(TOP)/rom/exec -- $^ > $@

ifeq ($(TARGET),clean)
NODEPS:=yes
endif
ifeq ($(TARGET),setup)
NODEPS:=yes
endif

ifndef NODEPS
include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)
endif
