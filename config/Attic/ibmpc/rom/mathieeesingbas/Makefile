# (C) 1997-1998 AROS - The Amia Research OS

FUNCTIONS = \
    ieeespabs ieeespadd ieeespceil ieeespcmp ieeespdiv ieeespfix ieeespfloor\
    ieeespflt ieeespmul ieeespneg ieeespsub ieeesptst
    
$(MATHIEEESINGBAS): endtag.c functions.c
	@echo -n Compiling mathieeesingbas.library...
	@$(CC) -c functable.c mathieeesingbas_init.c endtag.c
	@$(CC) -c functions.c 2>functions.err
	@$(LINK) mathieeesingbas_init.o functions.o functable.o endtag.o -o $(MATHIEEESINGBAS)
	@echo Done!

endtag.c:
	@echo -e "#include \"libdefs.h\"\n#include <libcore/libtail.c>" >$@

functions.c:
	@$(CPAK) >/dev/null $(shell for i in $(FUNCTIONS); do \
	if [ -f "$$i.c" ]; then echo $$i; \
	else echo $(TOP)/rom/mathieeesingbas/$$i; fi; done)

%.s: %.c
	@gcc -S -O0 -I../../include -I../../../../compiler/include $<

clean::
	@rm -f *.o $(MATHIEEESINGBAS) functions.* endtag.c