# $Id$
TOP=../..

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

all: setup $(LIBDIR)/libarosc.a

setup: make-dirs update-includes

make-dirs:
	@if [ ! -d $(INCDIR) ]; then $(MKDIR) $(INCDIR) ; else true ; fi
	@if [ ! -d $(INCDIR)/sys ]; then $(MKDIR) $(INCDIR)/sys ; else true ; fi
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

test: $(TESTDIR)/snprint

$(TESTDIR)/snprintf: snprintf.c $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) -DTEST $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

update-includes:
	@echo "Updating ANSI C includes"
	@cd include ; \
	$(MAKE) $(MFLAGS) TOP="$(TOP)/.." CURDIR="$(CURDIR)/include" all

clean:
	-$(RM) $(OBJDIR) *.err $(LIBDIR)/libc.a

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIBDIR)/libarosc.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o) \
	    $(foreach f,$(ASMFILES),$(OBJDIR)/$(f).o)
	$(AR) $@ $?
	$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D) ; else true ; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(APPCFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
NODEPS:=yes
endif
ifneq ($(TARGET),setup)
NODEPS:=yes
endif

ifndef NODEPS
include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)
endif
