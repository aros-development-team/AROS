# $Id$
TOP=../..

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/compiler/arossupport

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

DEPS = $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

LIB = $(LIBDIR)/libarossupport.a

INCLUDE_FILES = $(wildcard include/*.h)
DEST_INCLUDE_FILES = $(subst include,$(INCDIR)/aros,$(INCLUDE_FILES))

all: setup $(LIB)

setup : make-dirs os-include

os-include: $(DEST_INCLUDE_FILES)

$(INCDIR)/aros/%.h: include/%.h
	$(CP) $< $@

make-dirs :
	@for dir in $(OBJDIR) $(INCDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done
	@echo "CFLAGS=$(CFLAGS)"

clean:
	-$(RM) $(OBJDIR) *.err $(LIBDIR)/libarossupport.a

$(OBJDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(CFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

CFLAGS2 = $(CFLAGS) -I$(TOP)/rom/exec

$(OBJDIR)/kprintf.o: kprintf.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS2) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/rawputchars.o: rawputchars.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS2) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/_kprintf.o: _kprintf.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS2) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/rt_stubs.o: rt_stubs.s
	@$(ECHO) "Assembling $(CURDIR)/$<..."
	@$(SYS_AS) $(AFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

OBJS = $(foreach f,$(FILES),$(OBJDIR)/$(f).o)

$(LIB) : $(OBJS)
	@$(ECHO) "Creating library $@..."
	@$(AR) $@ $(OBJS)
	@$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(ECHO) "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/  -- $(CFLAGS2) -- $< > $@

# Also check if the makefile itself must be remade
all : makefile

# Remake makefile
makefile : makefile.src $(TOP)/scripts/genmf.gawk \
	    $(TOP)/config/make.tmpl
	@$(ECHO) "Regenerating makefile..."
	@$(MV) $@ $@.bak
	@$(AWK) -f $(TOP)/scripts/genmf.gawk --assign TOP="$(TOP)" \
	    $< > $@

# Include these files but only if the current target is neither
# setup nor clean.
ifneq ("$(TARGET)","clean")
ifneq ("$(TARGET)","setup")
include $(DEPS)
endif
endif
