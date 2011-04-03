#
#	GiggleDisk
#
#	Coypright © 2011 Guido Mersmann <geit@gmx.de>
#	All rights reserved.
#
#

APPLICATION        = GiggleDisk
OBJDIR             = .objects/$(APPLICATION)
CATALOGS           = deutsch

CATALOGDIR         = Catalogs

OBJECTS        = $(OBJDIR)/main.o                \
				 $(OBJDIR)/cache.o               \
				 $(OBJDIR)/create.o              \
                 $(OBJDIR)/createpartitionlist.o \
				 $(OBJDIR)/giggledisk.o          \
				 $(OBJDIR)/header.o              \
				 $(OBJDIR)/mbr.o                 \
				 $(OBJDIR)/rdb.o                 \
				 $(OBJDIR)/readargs.o            \
				 $(OBJDIR)/mount.o               \
				 $(OBJDIR)/readwrite.o           \
				 $(OBJDIR)/partitionentry.o      \
				 $(OBJDIR)/wbstartup.o           \
				 $(OBJDIR)/library.o             \
				 $(OBJDIR)/requester.o           \
				 $(OBJDIR)/locale.o              \
				 $(OBJDIR)/process.o             \
				 $(OBJDIR)/sprintf.o             \
				 $(OBJDIR)/version.o             \
				 $(OBJDIR)/icon.o                \
				 $(OBJDIR)/functions.o           \

HEADERS        = header.h

### no need to edit below

NEWCATCOMP         = include/internal/catcomp.pl
BOOTDEVICE         = /sys/

### GLOBAL DEBUG

ifdef debug
DEBUG    = _D
CFLAGS   = -DDEBUG=1
endif

### GNUC setup

ifndef vbcc

OBJDIR    = .objects/gnuc_mosppc$(DEBUG)/$(APPLICATION)

CC        = ppc-morphos-gcc
LD        = ppc-morphos-ld
STRIP     = ppc-morphos-strip

CDEFS     = -D__AMIGADATE__=\"$(shell date "+%d.%m.%y")\" -DSYSTEM_PRIVATE -D__COPYRIGHTYEAR__=\"$(shell date "+%Y")\"
CFLAGS   += -pipe -noixemul -O3 -W -Wall -mstring -mmultiple -fomit-frame-pointer -Wcomments -Wpointer-arith -Iinclude/ -Iinclude/sdi/
LDFLAGS   = -fl libnix -noixemul -nostartfiles -Map=$(OBJDIR)/$@.map
LIBS      = -laboxstubs -labox -lmemblock -lc -ldebug
SARGS     = --strip-all --strip-unneeded --remove-section .comment -o $@ $(OBJDIR)/$@.db
LDARGS    = -o $(OBJDIR)/$@.db

else ## vbcc

### VBCC setup

CC        = vc
LD        = vc
STRIP     =

CDEFS     = 
CFLAGS   +=
LIBS      = -lamiga -lvcs
SARGS     =
LDARGS    = -o $@

### VBCC AOS68K setup

ifdef aos68k

OBJDIR    = .objects/vbcc_aos68k$(DEBUG)/$(APPLICATION)
CFLAGS   += +aos68ksmall -O2 -c99 -sc -Iinclude
LDFLAGS   = +aos68ksmall

else ## aos68k

### VBCC AOSPPC setup

ifdef aosppc

OBJDIR    = .objects/vbcc_aosppc$(DEBUG)/$(APPLICATION)
CFLAGS   += +aosppcsmall -O3 -sc -size -Iinclude -D__USE_INLINE__ 
LDFLAGS   = +aosppcsmall

else ## aosppc

### VBCC AROS X86 setup

ifdef arosx86

OBJDIR    = .objects/vbcc_arosx86$(DEBUG)/$(APPLICATION)
CFLAGS   += +arossmall -O3 -size -DNO_INLINE_STDARG -Iinclude
LDFLAGS   = +arossmall
LIBS      = -lmui -lamiga

else ## arosx86

### VBCC MOSPPC setup

OBJDIR    = .objects/vbcc_mosppc$(DEBUG)/$(APPLICATION)
CFLAGS   += +morphossmall -O2  -Iinclude 
LDFLAGS   = +morphossmall -final

endif ## arosx86
endif ## aosppc
endif ## aos68k
endif ## vbcc

###################################################################

all: $(OBJDIR) $(APPLICATION)_strings.h $(APPLICATION) catalogs

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(OBJDIR)/$(APPLICATION).o: $(APPLICATION).c

$(OBJDIR)/%.o: %.c $(APPLICATION)_strings.h $(HEADERS)
	@echo "    [COMPILING] $@"
	@$(CC)  $(CDEFS) $(MODEDEFS) $(CFLAGS) -c $< -o $@

$(APPLICATION): $(OBJECTS)
	@echo "    [LINKING]"
	@$(LD) $(LDFLAGS) $(OBJECTS) $(LDARGS) $(LIBS)
	@$(STRIP) $(SARGS)

clean:
	@echo "    [CLEAN]"
	@rm -f *.o *.elf *.dump *.db *.s *.map $(APPLICATION)_strings.h $(APPLICATION)
	@rm -r -f .objects
	@rm -f $(patsubst %,catalogs/%/$(APPLICATION).catalog,$(CATALOGS))

#  locale
#
$(APPLICATION)_strings.h: $(APPLICATION).cd
	@echo "    [CATCOMP] $@"
	@$(NEWCATCOMP) --no-code -cd $(APPLICATION).cd --cfile $@

.PHONY: catalogs
catalogs: $(patsubst %,catalogs/%/$(APPLICATION).catalog,$(CATALOGS))

%.catalog: %.ct $(APPLICATION).cd
	@echo "    [CATCOMP] $@"
	@$(NEWCATCOMP) -u -cd $(APPLICATION).cd -ct $< -o $@

#  Installer export
#
scexport:
	@echo "    [SCEXPORT] $(APPLICATION).cd"
	@c:SimpleCat QUIET $(APPLICATION).cs export files $(APPLICATION).cd english
	@for language in $(CATALOGS); \
	do\
		echo "    [SCEXPORT] catalogs/$$language/$(APPLICATION).ct";\
		mkdir -p catalogs/$$language;\
		c:SimpleCat QUIET $(APPLICATION).cs export files catalogs/$$language/$(APPLICATION).ct $$language;\
	done

#  Installer import
#
scimport:
	@echo "    [SCIMPORT] $(APPLICATION).cd"
	@c:SimpleCat QUIET import files $(APPLICATION).cd english to $(APPLICATION).cs
	@for language in $(CATALOGS); \
	do\
		echo "    [SCIMPORT] catalogs/$$language/$(APPLICATION).ct";\
		c:SimpleCat QUIET $(APPLICATION).cs import files catalogs/$$language/$(APPLICATION).ct $$language to $(APPLICATION).cs;\
	done

sdk:
