#**************************************************************************
#*
#*  Win32 specific rules file, used to compile the Win32 graphics driver
#*  to the graphics subsystem
#*
#**************************************************************************

ifeq ($(PLATFORM),win32)

  # directory of the Win32 graphics driver
  #
  GR_WIN32  := $(GRAPH)/win32

  # add the Win32 driver object file to the graphics library `graph.lib'
  #
  GRAPH_OBJS += $(OBJ_DIR_2)/grwin32.$O

  DEVICES += WIN32

  # the rule used to compile the graphics driver
  #
  $(OBJ_DIR_2)/grwin32.$O: $(GR_WIN32)/grwin32.c $(GR_WIN32)/grwin32.h
	  $(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) \
                $I$(subst /,$(COMPILER_SEP),$(GR_WIN32)) \
                $T$(subst /,$(COMPILER_SEP),$@ $<)

  # now update COMPILE_GRAPH_LIB according to the compiler used on Win32
  #
  ifeq ($(firstword $(CC)),gcc)   # test for GCC
    GRAPH_LINK += -luser32 -lgdi32
  endif

  ifeq ($(findstring $(CC),cl icl), $(CC))    # test for Visual C++ & Intel C++
    COMPILE_GRAPH_LIB = lib /nologo /out:$(subst /,$(COMPILER_SEP),$(GRAPH_LIB) $(GRAPH_OBJS))
    LINK              = cl /nologo /MD /Fe$(subst /,$(COMPILER_SEP),$@ $< $(FTLIB))
    GRAPH_LINK       += user32.lib gdi32.lib
  endif

  ifeq ($(CC),lcc)   # test for LCC-Win32
    COMPILE_GRAPH_LIB = lcclib /out:$(subst /,$(COMPILER_SEP),$(GRAPH_LIB) $(GRAPH_OBJS))
    LINK              = lcclnk -o $(subst /,$(COMPILER_SEP),$@ $< $(FTLIB))
    GRAPH_LINK       += user32.lib gdi32.lib
  endif

  ifeq ($(CC),bcc32) # test for Borland C++
    COMPILE_GRAPH_LIB = tlib /u $(subst /,$(COMPILER_SEP),$(GRAPH_LIB) $(GRAPH_OBJS:%=+%))
    LINK              = bcc32 -e$(subst /,$(COMPILER_SEP),$@ $< $(FTLIB))
  endif
endif

# EOF
