# $Id$
TOP=../..

# Use OS's own includes over the ones from AROS
ifeq ("$(SHARED_EXEC"),"yes")
SPECIAL_CFLAGS=$(SHARED_CFLAGS) -I/usr/include
else
SPECIAL_CFLAGS=-I/usr/include
endif

include $(TOP)/config/make.cfg

OBJDIR=$(OSGENDIR)/exec

# ***** Native ***************
ifeq ($(FLAVOUR),native)

LIB=$(LIBDIR)/libexec.a

# Main function for exec.strap. Need this apart as an anchor for the link:
INIT_FILE = execstrap_init

# Other support files:
FILES =

FUNCTIONS = \
	initcode initstruct makelibrary makefunctions findresident	    \
	initresident setintvector addintserver remintserver allocmem	    \
	freemem availmem allocentry freeentry typeofmem addmemlist	    \
	addmemhandler remmemhandler insert addhead addtail remove remhead   \
	remtail enqueue findname setsignal allocsignal freesignal addport   \
	remport waitport findport createmsgport deletemsgport putmsg getmsg \
	replymsg doio sendio checkio waitio abortio createiorequest	    \
	deleteiorequest addresource remresource openresource addlibrary     \
	remlibrary oldopenlibrary closelibrary setfunction sumlibrary	    \
	openlibrary taggedopenlibrary adddevice remdevice opendevice	    \
	closedevice findtask rawdofmt allocvec freevec initsemaphore	    \
	attemptsemaphore findsemaphore addsemaphore remsemaphore	    \
	obtainsemaphoreshared attemptsemaphoreshared createpool deletepool  \
	allocpooled freepooled

# Functions that are written in asm and need to be linked into exec.strap.
# These are in config/$(KERNEL).
ASMFUNCTIONS = forbid disable getcc cacheclearu cachepredma cachepostdma

# ***** Not Native ***************
else

OBJDIR=$(OSGENDIR)/exec
ifeq ("$(SHARED_EXEC)","yes")
LIB=$(LIBDIR)/libexec.so
RMLIB=$(LIBDIR)/libexec.a
AR=$(SHARED_AR)
RANLIB=true
else
LIB=$(LIBDIR)/libexec.a
RMLIB=$(LIBDIR)/libexec.so
endif

FILES = init execfunctions

FUNCTIONS = \
	allocmem freemem addhead addtask remtask wait signal allocsignal    \
	freesignal alert freeentry remove remhead enqueue addmemhandler     \
	setexcept setsignal settaskpri findtask findname getmsg putmsg	    \
	replymsg addtail waitport allocate addport remport findport	    \
	addmemlist createmsgport deletemsgport createiorequest deallocate   \
	availmem deleteiorequest allocvec freevec allocabs allocentry	    \
	createpool deletepool allocpooled remmemhandler initstruct	    \
	initsemaphore attemptsemaphore obtainsemaphore			    \
	obtainsemaphoreshared attemptsemaphoreshared releasesemaphore	    \
	procure vacate addlibrary remlibrary makefunctions makelibrary	    \
	sumlibrary openlibrary closelibrary setfunction oldopenlibrary	    \
	copymemquick copymem freepooled remsemaphore addsemaphore	    \
	releasesemaphorelist obtainsemaphorelist typeofmem rawdofmt insert  \
	remtail findsemaphore addresource remresource openresource adddevice\
	remdevice opendevice closedevice doio sendio checkio abortio waitio \
	initresident cause addintserver remintserver setintvector alloctrap \
	childfree childorphan childstatus childwait coldreboot debug	    \
	findresident freetrap initcode sumkickdata obtainquickvector

# ***** END ***************
endif

DEPS = $(foreach f,$(INIT_FILE) $(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)

.PHONY: all clean setup

# ***** Native ***************
ifeq ($(FLAVOUR),native)

LIBS = -L$(LIBDIR) -lexec -larossupport -larosc
OBJS = $(foreach f,$(FILES) $(FUNCTIONS) $(ASMFUNCTIONS),$(OBJDIR)/$(f).o)
INITFUNC = $(OBJDIR)/$(INIT_FILE).o

all: setup $(LIB) $(OSMODDIR)/exec.strap

$(OSMODDIR)/exec.strap: $(LIB) $(INITFUNC) $(LIBDIR)/libarossupport.a $(LIBDIR)/libarosc.a
	@echo "Building $(notdir $@) ..."
	@$(CC) $(ILDFLAGS) -Xlinker -M $(INITFUNC) $(LIBS) -o $@ 2>&1|tee execstrap.map
	@if test ! -s execstrap.map; then rm execstrap.map ; else true ; fi
	@strip $@

# To study the assembler output for evaluation (can be helpful in tracking
# down bugs):
.PHONY: asm
asm: $(foreach f,$(FUNCTIONS) $(INIT_FILE) $(FILES),$(f).s) all

%.s: %.c
	@echo "Generating $@ ..."
	@$(CC) -S $(CFLAGS) $< -c -o $@

# Collect all (asm-)functions in a linklib for fast linking:
$(LIB): $(OBJS)
	@echo "Adding functions to $(notdir $@) ..."
	@$(AR) $@ $?
	@$(RANLIB) $@

# ***** Not Native ***************
else

OBJS = $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)

all: setup $(LIB)

$(LIB): $(OBJS)
	@$(ECHO) "Creating library $@..."
	@$(AR) $@ $(OBJS)
	@$(RANLIB) $@
	@$(RM) $(RMLIB)

# ***** END ***************
endif

setup :
	@for dir in $(OBJDIR) $(OSMODDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done
	@echo "CFLAGS=$(CFLAGS)"

clean:
	$(RM) $(OBJDIR) *.err $(LIB)

$(OBJDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(CC) $(CFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

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
