# $Id$
TOP=../..

USER_DEFINES = -D__AROS
USER_INCLUDES = -I$(TOP)/compiler/clib/include

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/compiler/alib
EXEDIR=$(BINDIR)/test

LIB = $(LIBDIR)/libamiga.a
STARTUP = $(LIBDIR)/startup.o

STARTUP = $(LIBDIR)/startup.o
LIBS = -L$(LIBDIR)  -lamiga -larossupport -larosc 
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a $(STARTUP) \
	$(LIBDIR)/libarosc.a 

#LIBS = -L$(LIBDIR) -larosc -lamiga -larossupport -larosc
#DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
#	 $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a

FILES = alib_util \
	allocnamedobject \
	callhook \
	coercemethod \
	createextio \
	createport \
	createstdio \
	createtask \
	deleteextio \
	deleteport \
	deletestdio \
	deletetask \
	dogadgetmethod \
	domethod \
	dosupermethod \
	easyrequest \
	fastrand \
	hookentry \
	liballocpooled \
	libcreatepool \
	libdeletepool \
	libfreepooled \
	newlist \
	newobject \
	openscreentags \
	openwindowtags \
	rangerand \
	setattrs \
	setsuperattrs

OBJS = $(foreach f,$(FILES),$(OBJDIR)/$(f).o)
DEPS = $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

all: setup $(LIB) $(STARTUP)

setup :
	@for dir in $(OBJDIR) $(LIBDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done

clean:
	-$(RM) $(OBJDIR) *.err $(LIB) $(STARTUP)

test: $(EXEDIR)/betest $(EXEDIR)/wstest $(EXEDIR)/rstest

TEST_ILDFLAGS = -DTEST $(ILDFLAGS)

$(EXEDIR)/rstest: readstruct.c $(LIBDIR)/startup.o $(DEPLIBS)
	@$(ECHO) "Linking $@..."
	@$(CC) $(TEST_ILDFLAGS) $(STARTUP) $< -o $@ $(LIBS) 2>&1|tee $(@F)-x.err
	@if test ! -s $(@F)-x.err; then rm $(@F)-x.err ; else true ; fi

$(EXEDIR)/wstest: writestruct.c $(LIBDIR)/startup.o $(DEPLIBS)
	@$(ECHO) "Linking $@..."
	@$(CC) $(TEST_ILDFLAGS) $(STARTUP) $< -o $@ $(LIBS) 2>&1|tee $(@F)-x.err
	@if test ! -s $(@F)-x.err; then rm $(@F)-x.err ; else true ; fi

$(EXEDIR)/betest: betest.c $(LIBDIR)/startup.o $(DEPLIBS)
	@$(ECHO) "Linking $@..."
	@$(CC) $(TEST_ILDFLAGS) $(STARTUP) $< -o $@ $(LIBS) 2>&1|tee $(@F)-x.err
	@if test ! -s $(@F)-x.err; then rm $(@F)-x.err ; else true ; fi

$(OBJDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIBDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(CFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIB): $(OBJS)
	@$(ECHO) "Creating library $@..."
	@$(AR) $@ $(OBJS)
	@$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(ECHO) "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/  -- $(CFLAGS) -- $< > $@

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
