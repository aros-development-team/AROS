# (C) 1997-1998 AROS - The Amia Research OS

FUNCTIONS = \
    expansion_init \
    addbootnode addconfigdev adddosnode allocboardmem allocconfigdev \
    allocexpansionmem configboard configchain findconfigdev freeboardmem \
    freeconfigdev freeexpansionmem getcurrentbinding makedosnode \
    obtainconfigbinding readexpansionbyte readexpansionrom releaseconfigbinding \
    remconfigdev setcurrentbinding writeexpansionbyte \
    expansion_endtag

$(EXPANSION): functions.c
	@echo -n Compiling expansion.library...
	@$(CC) -c expansion_functable.c
	@$(CC) -c functions.c 2>functions.err
	@$(LINK) functions.o expansion_functable.o -o $(EXPANSION)
	@echo Done!

functions.c:
	@$(CPAK) >/dev/null $(shell for i in $(FUNCTIONS); do \
	if [ -f "$$i.c" ]; then echo $$i; \
	else echo $(TOP)/rom/expansion/$$i; fi; done)

%.s: %.c
	@gcc -S -O1 -I../../include -I../../../../compiler/include $<

clean::
	@rm -f *.o $(EXPANSION) functions.*