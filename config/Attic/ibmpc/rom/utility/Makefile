# (C) 1997-1998 AROS - The Amia Research OS

FUNCTIONS = \
    addnamedobject allocatetagitems allocnamedobjecta amiga2date applytagchanges\
    attemptremnamedobject callhookpkt checkdate clonetagitems date2amiga\
    filtertagchanges filtertagitems findnamedobject findtagitem freenamedobject\
    freetagitems getnamespace gettagdata getuniqueid intfindnamedobj maptags\
    namedobjectname nexttagitem packbooltags packstructuretags refreshtagitemclones\
    releasenamedobject remnamedobject sdivmod32 smult32 smult64 stricmp strnicmp\
    taginarray tolower toupper udivmod32 umult32 umult64 unpackstructuretags

$(UTILITY): endtag.c functions.c
	@echo -n Compiling utility.library...
	@$(CC) -c functable.c utility_init.c endtag.c
	@$(CC) -c functions.c 2>functions.err
	@$(LINK) utility_init.o functions.o functable.o endtag.o -o $(UTILITY)
	@echo Done!

endtag.c:
	@echo -e "#include \"libdefs.h\"\n#include <libcore/libtail.c>" >$@

functions.c:
	@$(CPAK) >/dev/null $(shell for i in $(FUNCTIONS); do \
	if [ -f "$$i.c" ]; then echo $$i; \
	else echo $(TOP)/rom/utility/$$i; fi; done)

%.s: %.c
	@gcc -S -O0 -I../../include -I../../../../compiler/include $<

clean::
	@rm -f *.o $(UTILITY) functions.* endtag.c