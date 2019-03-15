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
  GR_ALLEG := $(GRAPH)/allegro

  # add Allegro driver to lib objects
  #
  GRAPH_OBJS += $(OBJ_DIR_2)/gralleg.$O

  # add Allegro driver to list of devices
  #
  DEVICES += ALLEGRO

  # our compilation rule
  #
  $(OBJ_DIR_2)/gralleg.$O : $(GR_ALLEG)/gralleg.c $(GR_ALLEG)/gralleg.h
	  $(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) \
                $I$(subst /,$(COMPILER_SEP),$(GR_ALLEG)) \
                $T$(subst /,$(COMPILER_SEP),$@ $<)

  # we need to link with Allegro
  #
  GRAPH_LINK += -lalleg

endif # test ALLEGRO

# EOF
