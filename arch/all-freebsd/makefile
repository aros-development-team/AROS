TOP=../..

USER_INCLUDES=-I$(TOP)/graphics -I$(TOP)/intuition

include $(TOP)/config/make.cfg

FILES = graphics_driver intuition_driver

all: setup \
	$(foreach f,$(FILES),$(OSGENDIR)/$(f).o)

setup :
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi

clean:
	$(RM) $(OSGENDIR) *.err

config:
	@echo "Configuring for FreeBSD..."

intuition_driver.c:
	$(TOP)/scripts/makelinks ../linux/ . intuition_driver.c

graphics_driver.c:
	$(TOP)/scripts/makelinks ../linux/ . graphics_driver.c

$(OSGENDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(GUI_CCFLAGS) $< -c -o $@ 2>&1|tee $*.err
	if test ! -s $*.err; then rm $*.err ; else true ; fi

$(OSGENDIR)/%.d: %.c
	@if [ ! -d $(OSGENDIR) ]; then $(MKDIR) $(OSGENDIR) ; else true ; fi
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -p$(OSGENDIR)/

include $(foreach f,$(FILES),$(OSGENDIR)/$(f).d)

# DO NOT DELETE
