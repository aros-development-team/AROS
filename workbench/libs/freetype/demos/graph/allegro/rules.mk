#**************************************************************************
#*
#*  Allegro driver makefile
#*
#**************************************************************************

# test for the `ALLEGRO' environment variable. This is non-optimal.
#
ifdef ALLEGRO

# directory of Allegro driver
#
GR_ALLEG := $(GRAPH_)allegro
GR_ALLEG_ := $(GR_ALLEG)$(SEP)

# add Allegro driver to lib objects
#
GRAPH_OBJS += $(OBJ_)gralleg.$O

# add Allegro driver to list of devices
#
DEVICES += ALLEGRO

# our compilation rule
#
$(OBJ_)gralleg.$O : $(GR_ALLEG_)gralleg.c $(GR_ALLEG_)gralleg.h
	$(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) $I$(GR_ALLEG) $T$@ $<

# we need to link with Allegro
#
GRAPH_LINK += -lalleg

endif # test ALLEGRO
