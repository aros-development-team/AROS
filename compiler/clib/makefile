# $Id$
TOP=..

USER_INCLUDES=-Iinclude

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/clib
LIBS = -L$(LIBDIR) -larosc -lamiga -laros -larosc
DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
	$(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a

FILES = \
	__stdio \
	__vcformat \
	abs \
	atoi \
	atol \
	bcopy \
	bsearch \
	bzero \
	calloc \
	ctype \
	errno \
	feof \
	fflush \
	fgetc \
	fprintf \
	fputc \
	fputs \
	free \
	labs \
	malloc \
	memchr \
	memcmp \
	memcpy \
	memmove \
	memset \
	printf \
	puts \
	qsort \
	rand \
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
	vsprintf

all: setup $(OBJDIR)/links $(LIBDIR)/libarosc.a

setup:
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

test: $(TESTDIR)/snprintf

$(TESTDIR)/snprintf: snprintf.c $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) -DTEST $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/links:
	$(TOP)/scripts/makelinks include $(BINDIR)/include "*.h"
	touch $(OBJDIR)/links

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libc.a

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIBDIR)/libarosc.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o)
	$(AR) $@ $?
	$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d)

