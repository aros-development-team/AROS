# $Id$
TOP=../../..

include $(TOP)/config/make.cfg

OBJDIR=$(GENDIR)/workbench/locale/countries
LOCALEDIR=$(BINDIR)/Locale
COUNTDIR=$(LOCALEDIR)/Countries
DEPLIBS=$(LIBDIR)/libamiga.a $(LIBDIR)/libarosc.a
LIBS=-L$(LIBDIR) -lamiga -larosc

COUNTRIES=australia germany great_britain nederland

COUNTIFF=$(foreach f, $(COUNTRIES), $(COUNTDIR)/$(f).country)

all: setup $(COUNTIFF)

setup:
	@if [ ! -d $(OBJDIR) ]; then $(MKDIR) $(OBJDIR); else true; fi
	@if [ ! -d $(LOCALEDIR) ]; then $(MKDIR) $(LOCALEDIR); else true; fi
	@if [ ! -d $(COUNTDIR) ]; then $(MKDIR) $(COUNTDIR); else true; fi

clean:
	$(RM)   $(OBJDIR) *.err

$(COUNTIFF): makecountry
	./makecountry $(COUNTDIR)/ $(foreach f, $@, $(notdir $(basename $(f))))

makecountry: $(OBJDIR)/makecountry.o $(foreach f, $(COUNTRIES), $(OBJDIR)/$(f).o)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 2>&1|tee $*.err
	@if test ! -s $*.err; then rm $*.err ; else true; fi

$(OBJDIR)/%.d: %.c
	@if [ ! -d $(@D) ]; then $(MKDIR) $(@D); else true; fi
	$(MKDEPEND) -f- -p$(@D)/ -- $(CFLAGS) -- $^ > $@

ifneq ($(TARGET),clean)
include $(foreach f,makecountry $(COUNTRIES),$(OBJDIR)/$(f).d)
endif
