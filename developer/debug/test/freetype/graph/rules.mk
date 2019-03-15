#**************************************************************************
#*
#*  FreeType demo utilities sub-Makefile
#*
#*  This Makefile is to be included by `../Makefile'.  Its
#*  purpose is to compile MiGS (the Minimalist Graphics Subsystem).
#*
#*  It is written for GNU Make.  Other make utilities are not
#*  supported!
#*
#**************************************************************************


GRAPH_INCLUDES := $(subst /,$(COMPILER_SEP),$(TOP_DIR_2)/graph)
GRAPH_LIB      := $(OBJ_DIR_2)/graph.$(SA)

GRAPH := $(TOP_DIR_2)/graph

GRAPH_H := $(GRAPH)/gblany.h    \
           $(GRAPH)/gblbgra.h   \
           $(GRAPH)/gblblit.h   \
           $(GRAPH)/gblcolor.h  \
           $(GRAPH)/gblhbgr.h   \
           $(GRAPH)/gblhrgb.h   \
           $(GRAPH)/gblvbgr.h   \
           $(GRAPH)/gblvrgb.h   \
           $(GRAPH)/gblender.h  \
           $(GRAPH)/graph.h     \
           $(GRAPH)/grblit.h    \
           $(GRAPH)/grconfig.h  \
           $(GRAPH)/grdevice.h  \
           $(GRAPH)/grevents.h  \
           $(GRAPH)/grfont.h    \
           $(GRAPH)/grobjs.h    \
           $(GRAPH)/grswizzle.h \
           $(GRAPH)/grtypes.h


GRAPH_OBJS := $(OBJ_DIR_2)/gblblit.$(O)   \
              $(OBJ_DIR_2)/gblender.$(O)  \
              $(OBJ_DIR_2)/grblit.$(O)    \
              $(OBJ_DIR_2)/grdevice.$(O)  \
              $(OBJ_DIR_2)/grfill.$(O)    \
              $(OBJ_DIR_2)/grfont.$(O)    \
              $(OBJ_DIR_2)/grinit.$(O)    \
              $(OBJ_DIR_2)/grobjs.$(O)    \
              $(OBJ_DIR_2)/grswizzle.$(O)



# Default value for COMPILE_GRAPH_LIB;
# this value can be modified by the system-specific graphics drivers.
#
ifneq ($(LIBTOOL),)
  COMPILE_GRAPH_LIB = $(LIBTOOL) --mode=link $(CCraw) -static \
                                 -o $(subst /,$(COMPILER_SEP),$@ $(GRAPH_OBJS))
else
  COMPILE_GRAPH_LIB = ar -r $(subst /,$(COMPILER_SEP),$@ $(GRAPH_OBJS))
endif


# Add the rules used to detect and compile graphics driver depending
# on the current platform.
#
include $(wildcard $(TOP_DIR_2)/graph/*/rules.mk)


#########################################################################
#
# Build the `graph' library from its objects.  This should be changed
# in the future in order to support more systems.  Probably something
# like a `config/<system>' hierarchy with a system-specific rules file
# to indicate how to make a library file, but for now, I'll stick to
# unix, Win32, and OS/2-gcc.
#
#
$(GRAPH_LIB): $(GRAPH_OBJS)
	$(COMPILE_GRAPH_LIB)


# pattern rule for normal sources
#
$(OBJ_DIR_2)/%.$(O): $(GRAPH)/%.c $(GRAPH_H)
ifneq ($(LIBTOOL),)
	$(LIBTOOL) --mode=compile $(CC) -static $(CFLAGS) \
                   $(GRAPH_INCLUDES:%=$I%) $T$@ $<
else
	$(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) $T$@ $<
endif


# a special rule is used for 'grinit.o' as it needs the definition
# of some macros like "-DDEVICE_X11" or "-DDEVICE_OS2_PM"
#
$(OBJ_DIR_2)/grinit.$(O): $(GRAPH)/grinit.c $(GRAPH_H)
ifneq ($(LIBTOOL),)
	$(LIBTOOL) --mode=compile $(CC) -static \
                   $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) \
                   $(DEVICES:%=$DDEVICE_%) $T$(subst /,$(COMPILER_SEP),$@ $<)
else
	$(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) \
              $(DEVICES:%=$DDEVICE_%) $T$(subst /,$(COMPILER_SEP),$@ $<)
endif


# EOF
