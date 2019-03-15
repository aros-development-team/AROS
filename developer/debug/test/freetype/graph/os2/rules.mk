#**************************************************************************
#*
#*  OS/2 specific rules file, used to compile the OS/2 graphics driver
#*  to the graphics subsystem
#*
#**************************************************************************

ifeq ($(PLATFORM),os2)

  GR_OS2  := $(GRAPH)/os2

  # the GRAPH_LINK is expanded each time an executable is linked with the
  # graphics library.
  #
  GRAPH_LINK += $(subst /,$(COMPILER_SEP),$(GR_OS2)/gros2pm.def)

  # add the OS/2 driver object file to the graphics library `graph.a'
  #
  GRAPH_OBJS += $(OBJ_DIR_2)/gros2pm.$O

  DEVICES += OS2_PM

  # the rule used to compile the graphics driver
  #
  $(OBJ_DIR_2)/gros2pm.$O: $(GR_OS2)/gros2pm.c $(GR_OS2)/gros2pm.h
	  $(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) \
                $I$(subst /,$(COMPILER_SEP),$(GR_OS2)) \
                $T$(subst /,$(COMPILER_SEP),$@ $<)

endif

# EOF
