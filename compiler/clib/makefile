# $Id$
TOP=../../..

USER_INCLUDES=-Iinclude

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/clib
INCDIR=$(BINDIR)/include
LIBS = -L$(LIBDIR) -larosc -lamiga -laros -larosc
DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
	$(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a

FILES = \
	__assert \
	__getfilenode4fd \
	__ioerr2errno \
	__stdio \
	__vcformat \
	abort \
	abs \
	atoi \
	atol \
	bcopy \
	bsearch \
	bzero \
	calloc \
	clearerr \
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
	fputc \
	fputs \
	fread \
	free \
	fwrite \
	labs \
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
	snprintf \
	sprintf \
	strcasecmp \
	strcat \
	strchr \
	strcmp \
	strcpy \
	strcspn \
	strdup \
	strerror \
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
	vfprintf \
	vprintf \
	vsnprintf \
	vsprintf \
	write

ifneq ($(FLAVOUR),native)
ASMFILES = setjmp longjmp
endif

all: setup update-includes $(LIBDIR)/libarosc.a

setup:
	@if [ ! -d $(INCDIR) ]; then $(MKDIR) $(INCDIR) ; else true ; fi
	@if [ ! -d $(INCDIR)/sys ]; then $(MKDIR) $(INCDIR)/sys ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

test: $(TESTDIR)/snprintf

$(TESTDIR)/snprintf: snprintf.c $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) -DTEST $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

update-includes:
	@echo "Updating ANSI C includes"
	@cd include ; \
	$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/include" all

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libc.a

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIBDIR)/libarosc.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o) \
	    $(foreach f,$(ASMFILES),$(OBJDIR)/$(f).o)
	$(AR) $@ $?
	$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@$(TOUCH) $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

