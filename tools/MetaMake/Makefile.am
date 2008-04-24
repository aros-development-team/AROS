AUTOMAKE_OPTIONS = ansi2knr
mmake_objdir = @mmake_objdir@
MKDIR_P = @MKDIR_P@
bin_PROGRAMS = mmake
mmake_SOURCES = mmake.c mem.c list.c var.c dirnode.c dep.c project.c cache.c io.c
am_mmake_OBJECTS = $(mmake_objdir)/mmake.o $(mmake_objdir)/mem.o $(mmake_objdir)/list.o $(mmake_objdir)/var.o $(mmake_objdir)/dirnode.o $(mmake_objdir)/dep.o $(mmake_objdir)/project.o $(mmake_objdir)/cache.o $(mmake_objdir)/io.o
EXTRA_DIST = mmake.txt README.GeekGadgets Makefile.amiga config.h.amiga TODO
DEPDIR = $(mmake_objdir)/.deps

$(mmake_objdir)/mmake.o: mmake.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/mmake.o -MD -MP -MF $(DEPDIR)/mmake.Tpo -c -o $(mmake_objdir)/mmake.o mmake.c
	mv -f $(DEPDIR)/mmake.Tpo $(DEPDIR)/mmake.Po

$(mmake_objdir)/mem.o: mem.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/mem.o -MD -MP -MF $(DEPDIR)/mem.Tpo -c -o $(mmake_objdir)/mem.o mem.c
	mv -f $(DEPDIR)/mem.Tpo $(DEPDIR)/mem.Po

$(mmake_objdir)/list.o: list.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/list.o -MD -MP -MF $(DEPDIR)/list.Tpo -c -o $(mmake_objdir)/list.o list.c
	mv -f $(DEPDIR)/list.Tpo $(DEPDIR)/list.Po

$(mmake_objdir)/var.o: var.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/var.o -MD -MP -MF $(DEPDIR)/var.Tpo -c -o $(mmake_objdir)/var.o var.c
	mv -f $(DEPDIR)/var.Tpo $(DEPDIR)/var.Po

$(mmake_objdir)/dirnode.o: dirnode.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/dirnode.o -MD -MP -MF $(DEPDIR)/dirnode.Tpo -c -o $(mmake_objdir)/dirnode.o dirnode.c
	mv -f $(DEPDIR)/dirnode.Tpo $(DEPDIR)/dirnode.Po

$(mmake_objdir)/dep.o: dep.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/dep.o -MD -MP -MF $(DEPDIR)/dep.Tpo -c -o $(mmake_objdir)/dep.o dep.c
	mv -f $(DEPDIR)/dep.Tpo $(DEPDIR)/dep.Po

$(mmake_objdir)/project.o: project.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/project.o -MD -MP -MF $(DEPDIR)/project.Tpo -c -o $(mmake_objdir)/project.o project.c
	mv -f $(DEPDIR)/project.Tpo $(DEPDIR)/project.Po

$(mmake_objdir)/cache.o: cache.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/cache.o -MD -MP -MF $(DEPDIR)/cache.Tpo -c -o $(mmake_objdir)/cache.o cache.c
	mv -f $(DEPDIR)/cache.Tpo $(DEPDIR)/cache.Po

$(mmake_objdir)/io.o: io.c
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $(mmake_objdir)/io.o -MD -MP -MF $(DEPDIR)/io.Tpo -c -o $(mmake_objdir)/io.o io.c
	mv -f $(DEPDIR)/io.Tpo $(DEPDIR)/io.Po

