#**************************************************************************
#*
#*  BeOS specific rules file, used to compile the BeOS graphics driver
#*  to the graphics subsystem
#*
#**************************************************************************

ifeq ($(PLATFORM),beos)

# directory of the BeOS graphics driver
#
GR_BEOS  := $(GRAPH_)beos
GR_BEOS_ := $(GR_BEOS)$(SEP)

# Add the BeOS driver object file to the graphics library "graph.a"
#
GRAPH_OBJS += $(OBJ_)grbeos.$(SO)

DEVICES         += BEOS
DEVICE_INCLUDES += $(GR_BEOS)

# the rule used to compile the graphics driver
#
  $(OBJ_)grbeos.$(SO): $(GR_BEOS_)grbeos.cpp $(GR_BEOS_)grbeos.h
	  $(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) $I$(GR_BEOS) \
                $(X11_INCLUDE:%=$I%) $T$@ $<

# Now update GRAPH_LINK according to the compiler used on BeOS
GRAPH_LINK        = $(COMMON_LINK) $(GRAPH_LIB) -lbe -lstdc++.r4

endif


