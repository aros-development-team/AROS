# $Id$
TOP=../..

USER_DEFINES = -D__AROS
USER_INCLUDES = -I$(TOP)/compiler/clib/include

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/alib
EXEDIR=$(BINDIR)/test
LIBS = -L$(LIBDIR) -larosc -lamiga -larossupport -larosc
DEPLIBS = $(LIBDIR)/startup.o $(LIBDIR)/libarosc.a \
	$(LIBDIR)/libamiga.a $(LIBDIR)/libarossupport.a

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

all: setup $(LIBDIR)/libamiga.a $(LIBDIR)/startup.o

setup :
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR) ; else true ; fi

clean:
	-$(RM) $(OBJDIR) *.err $(LIBDIR)/libamiga.a

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
