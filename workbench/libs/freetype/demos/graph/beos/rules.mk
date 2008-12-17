#**************************************************************************
#*
#*  BeOS specific rules file, used to compile the BeOS graphics driver
#*  to the graphics subsystem
#*
#**************************************************************************

ifeq ($(PLATFORM),beos)

  # directory of the BeOS graphics driver
  #
  GR_BEOS := $(GRAPH)/beos

  # add the BeOS driver object file to the graphics library `graph.a'
  #
  GRAPH_OBJS += $(OBJ_DIR_2)/grbeos.$(SO)

  DEVICES         += BEOS
  DEVICE_INCLUDES += $(GR_BEOS)

  # the rule used to compile the graphics driver
  #
  $(OBJ_DIR_2)/grbeos.$(SO): $(GR_BEOS)/grbeos.cpp $(GR_BEOS)/grbeos.h
	  $(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) \
                $I$(subst /,$(COMPILER_SEP),$(GR_BEOS)) \
                $(X11_INCLUDE:%=$I%) \
                $T$(subst /,$(COMPILER_SEP),$@ $<)

  # Now update GRAPH_LINK according to the compiler used on BeOS
  GRAPH_LINK += -lbe -lstdc++.r4

endif

# EOF
