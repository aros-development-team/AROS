# (C) 1997-1998 AROS - The Amia Research OS

FUNCTIONS = \
    arosinquirea
    
$(AROS): endtag.c functions.c
	@echo -n Compiling aros.library...
	@$(CC) -c functable.c aros_init.c endtag.c
	@$(CC) -c functions.c 2>functions.err
	@$(LINK) aros_init.o functions.o functable.o endtag.o -o $(AROS)
	@echo Done!

endtag.c:
	@echo -e "#include \"libdefs.h\"\n#include <libcore/libtail.c>" >$@

functions.c:
	@$(CPAK) >/dev/null $(shell for i in $(FUNCTIONS); do \
	if [ -f "$$i.c" ]; then echo $$i; \
	else echo $(TOP)/rom/aros/$$i; fi; done)

%.s: %.c
	@gcc -S -O0 -I../../include -I../../../../compiler/include $<

clean::
	@rm -f *.o $(AROS) functions.* endtag.c