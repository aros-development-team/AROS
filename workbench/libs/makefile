TOP=..

include $(TOP)/make.cfg

all: setup $(SLIBDIR)/dummy.library

setup :
	@if [ ! -d $(SLIBDIR) ]; then $(MKDIR) $(SLIBDIR) ; fi

clean:
	$(RM) $(SLIBDIR)/dummy.library

$(SLIBDIR)/dummy.library: dummylib.c
	$(CC) $(CFLAGS) -I $(TOP) $< -c -o $@

$(SLIBDIR)/%.d: %.c
	@$(RM) $@
	@touch $@
	$(MKDEPEND) -f$@ -p$(SLIBDIR)/ -- $(CFLAGS) -- $^

include $(SLIBDIR)/dummylib.d

