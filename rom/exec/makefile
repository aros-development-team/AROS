# $Id$
TOP=../..

# Use OS's own includes over the ones from AROS
SPECIAL_CFLAGS=-I/usr/include

include $(TOP)/config/make.cfg

# ***** Native ***************
ifeq ($(FLAVOUR),native)

OBJDIR=$(OSGENDIR)/exec
LIB=$(LIBDIR)/libexec.a

# Main function for exec.strap. Need this apart as an anchor for the link:
INITFUNC = execstrap_init

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

.PHONY: all clean setup

# ***** Native ***************
ifeq ($(FLAVOUR),native)

LIBS = -L$(LIBDIR) -lexec -laros -larosc
OBJS = $(foreach f,$(FILES) $(FUNCTIONS) $(ASMFUNCTIONS),$(OBJDIR)/$(f).o)

all: setup $(LIB) $(OSMODDIR)/exec.strap

$(OSMODDIR)/exec.strap: $(LIB) $(LIBDIR)/libaros.a $(LIBDIR)/libarosc.a
	@echo "Compiling $@..."
	@$(CC) $(ILDFLAGS) -Xlinker -M \
	 $(OBJDIR)/$(INITFUNC).o \
	 $(LIBS) -o $@ 2>&1|tee execstrap.map
	@if test ! -s execstrap.map; then rm execstrap.map ; else true ; fi
	@strip $@

# To study the assembler output for evaluation (can be helpful in tracking
# down bugs):
.PHONY: asm
asm: $(foreach f,$(FUNCTIONS),$(f).s) all

%.s: %.c
	@echo "Generating $@..."
	$(CC) -S $(CFLAGS) $< -c -o $@

# Collect all (asm-)functions in a linklib for fast linking:
$(LIB): $(OBJS)
	@echo "Adding functions to $@..."
	@$(AR) $@ $?
	@$(RANLIB) $@

# ***** Not Native ***************
else

OBJS = $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).o)

all: setup $(LIB)

# ***** END ***************
endif

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@if [ ! -d $(OSMODDIR) ]; then $(MKDIR) $(OSMODDIR) ; else true ; fi
	@echo "CFLAGS=$(CFLAGS)"

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libexec.a

$(LIB): $(OBJS)
	@echo "Recreating $@..."
	@$(AR) $@ $(OBJS)
	@$(RANLIB) $@
	@$(RM) $(RMLIB)

$(OBJDIR)/%.o: %.c
	@echo "Compiling $(CURDIR)/$<"
	@$(CC) $(SHARED_CFLAGS) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@echo "Makedepend $(CURDIR)/$<"
	@$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,$(FILES) $(FUNCTIONS),$(OBJDIR)/$(f).d)
endif
