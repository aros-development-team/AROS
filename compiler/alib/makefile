# $Id$
TOP=../../..

SPECIAL_CFLAGS = -D__AROS

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/alib
EXEDIR=$(BINDIR)/test
LIBS = -L$(LIBDIR) -larosc -lamiga -laros -larosc
DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
	$(LIBDIR)/libamiga.a $(LIBDIR)/libaros.a

FILES = alib_util \
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
	fastrand \
	freestruct \
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
	readbyte \
	readdouble \
	readfloat \
	readlong \
	readstring \
	readstruct \
	readword \
	setattrs \
	setsuperattrs \
	writebyte \
	writedouble \
	writefloat \
	writelong \
	writestring \
	writestruct \
	writeword

all: setup $(LIBDIR)/libamiga.a $(LIBDIR)/startup.o

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	$(RM) $(OBJDIR) *.err $(LIBDIR)/libamiga.a

test: $(EXEDIR)/betest $(EXEDIR)/wstest $(EXEDIR)/rstest

$(EXEDIR)/rstest: readstruct.c $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) -DTEST $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(EXEDIR)/wstest: writestruct.c $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) -DTEST $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(EXEDIR)/betest: betest.c $(LIBDIR)/startup.o $(DEPLIBS)
	$(CC) $(ILDFLAGS) $(LIBDIR)/startup.o $< -o $@ $(LIBS)

$(OBJDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true ; fi

$(LIBDIR)/%.o: %.c
	$(SYS_CC) $(CFLAGS) -c $< -o $@

$(LIBDIR)/libamiga.a: $(foreach f,$(FILES),$(OBJDIR)/$(f).o)
	$(AR) $@ $?
	$(RANLIB) $@

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(OBJDIR)/ -- $(CFLAGS) -- $^

include $(foreach f,$(FILES),$(OBJDIR)/$(f).d) $(OBJDIR)/startup.d
