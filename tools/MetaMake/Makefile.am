AUTOMAKE_OPTIONS = ansi2knr
bin_PROGRAMS = mmake
mmake_FILES = mmake mem list var dirnode dep project cache io
mmake_SOURCES = $(addsuffix .c,$(mmake_FILES))
mmake_OBJS = $(addsuffix .o,$(mmake_FILES))
am_mmake_OBJECTS = $(addprefix $(mmake_objdir)/,$(mmake_OBJS))
EXTRA_DIST = mmake.txt README.GeekGadgets Makefile.amiga config.h.amiga TODO
DEPDIR = $(mmake_objdir)/.deps

$(mmake_objdir)/%.o: %.c
	$(MKDIR_P) "$(DEPDIR)"
	$(MKDIR_P) "$(mmake_objdir)"
	$(MKDIR_P) "$(DEPDIR)"
	$(COMPILE) -MT $@ -MD -MP -MF $(DEPDIR)/$*.Tpo -c -o $@ $<
	mv -f $(DEPDIR)/$*.Tpo $(DEPDIR)/$*.Po
