# $Id$
TOP=../..

USER_INCLUDES=-Iinclude

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/compiler/clib
INCDIR=$(BINDIR)/include

LIB = $(LIBDIR)/libarosc.a

STARTUP = $(LIBDIR)/startup.o
LIBS = -L$(LIBDIR)  -lamiga -larossupport -larosc 
DEPLIBS = $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a $(STARTUP) \
	$(LIBDIR)/libarosc.a 

#LIBS = -L$(LIBDIR) -larosc -lamiga -larossupport -larosc
#DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
#	 $(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a

FILES = \
	__assert \
	__getfilenode4fd \
	__ioerr2errno \
	__stdio \
	__vcformat \
	__vcscan \
	abort \
	abs \
	asctime \
	atoi \
	atol \
	bcopy \
	bsearch \
	bzero \
	calloc \
	clearerr \
	ctime \
	ctype \
	close \
	creat \
	errno \
	exit \
	fclose \
	feof \
	ferror \
	fflush \
	fgetc \
	fgets \
	fopen \
	fprintf \
	fscanf \
	fputc \
	fputs \
	fread \
	free \
	fseek \
	fwrite \
	gmtime \
	labs \
	localtime \
	malloc \
	memchr \
	memcmp \
	memcpy \
	memmove \
	memset \
	open \
	printf \
	puts \
	qsort \
	rand \
	read \
	realloc \
	scanf \
	snprintf \
	sprintf \
	sscanf \
	strcasecmp \
	strcat \
	strchr \
	strcmp \
	strcpy \
	strcspn \
	strdup \
	strerror \
	strftime \
	strlen \
	strncasecmp \
	strncat \
	strncmp \
	strncpy \
	strpbrk \
	strrchr \
	strspn \
	strstr \
	strtok \
	strtol \
	strtoul \
	time \
	ungetc \
	unlink \
	vfprintf \
	vfscanf \
	vprintf \
	vscanf \
	vsnprintf \
	vsprintf \
	vsscanf \
	write

ASMFILES = setjmp longjmp

OBJS = $(foreach f,$(FILES) $(ASMFILES),$(OBJDIR)/$(f).o)
DEPS = $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

all: setup $(LIB)

setup: make-dirs update-includes

make-dirs:
	@for dir in $(INCDIR)/sys $(OBJDIR) ; do \
	    if [ ! -d $$dir ]; then $(MKDIR) $$dir ; else true ; fi ; \
	done

test: $(TESTDIR)/snprint

$(TESTDIR)/snprintf: snprintf.c $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) -DTEST $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

update-includes:
	@echo "Updating ANSI C includes"
	@cd include ; \
	$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/include" all

clean:
	-$(RM) $(OBJDIR) *.err $(LIB)

$(OBJDIR)/%.o: %.c
	@$(ECHO) "Compiling $(CURDIR)/$<..."
	@$(SYS_CC) $(APPCFLAGS) -c $< -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIB): $(OBJS)
	@$(ECHO) "Creating library $@..."
	@$(AR) $@ $(OBJS)
	@$(RANLIB) $@

#	 $(AR) $@ $?
#	 $(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	@$(ECHO) "Makedepend $(CURDIR)/$<..."
	@$(MKDEPEND) -f- -p$(@D)/  -- $(APPCFLAGS) -- $< > $@

#	 @if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
#	 $(MKDEPEND) -f- -p$(@D)/ -- $(APPCFLAGS) -- $^ > $@

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
