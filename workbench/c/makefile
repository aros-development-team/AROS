TOP=..

SPECIAL_CFLAGS = -D__AROS

include $(TOP)/make.cfg

#CC=$(PURIFY) $(SYS_CC)

OBJDIR = $(GENDIR)/exe
LIBS = -L$(LIBDIR) -lamiga -laros
DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a

FILES = \
	newshell \
	type \
	echo \
	cd \
	avail \
	list \
	run \
	execute \
	path \
	shell \
	dir \
	wait \
	assign \
	delete \
	makedir \
	mount \
	version \
	copy

EXES = $(foreach f,$(FILES),$(EXEDIR)/$(f))

all: setup $(EXES)

setup :
	@if [ ! -d $(EXEDIR) ]; then $(MKDIR) $(EXEDIR) ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi

clean:
	$(RM) $(EXES)

$(EXEDIR)/%: $(OBJDIR)/%.o $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

