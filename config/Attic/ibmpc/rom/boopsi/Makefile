# (C) 1997-1998 AROS - The Amia Research OS

FUNCTIONS = \
    addclass disposeobject donotify findclass freeclass freeicdata getattr \
    makeclass newobjecta nextobject removeclass setattrsa
    
$(BOOPSI): endtag.c functions.c
	@echo -n Compiling boopsi.library...
	@$(CC) -c functable.c boopsi_init.c endtag.c rootclass.c icclass.c
	@$(CC) -c functions.c 2>functions.err
	@$(LINK) boopsi_init.o rootclass.o icclass.o functions.o functable.o endtag.o -o $(BOOPSI)
	@echo Done!

endtag.c:
	@echo -e "#include \"libdefs.h\"\n#include <libcore/libtail.c>" >$@

functions.c:
	@$(CPAK) >/dev/null $(shell for i in $(FUNCTIONS); do \
	if [ -f "$$i.c" ]; then echo $$i; \
	else echo $(TOP)/rom/boopsi/$$i; fi; done)

%.s: %.c
	@gcc -S -O0 -I../../include -I../../../../compiler/include $<

clean::
	@rm -f *.o $(BOOPSI) functions.* endtag.c