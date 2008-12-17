#**************************************************************************
#*
#*  X11-specific rules files, used to compile the X11 graphics driver
#*  when supported by the current platform
#*
#**************************************************************************


#########################################################################
#
# Try to detect an X11 setup.
#
# We try to detect the following directories (in that order) in the current
# path:
#
#   X11   (usually a symlink to the current release)
#   X11R6
#   X11R5
#
# If no success, we directly check the directories
#
#   /usr
#   /usr/X11R6
#   /usr/local/X11R6
#
# whether they contain `include/X11/Xlib.h'.  Note that the Makefile
# silently assumes that they will also contain `lib/X11/libX11.(a|so)'.
#
# If the variable X11_PATH is set (to specify unusual locations of X11), no
# other directory is searched.  More than one directory must be separated
# with spaces.  Example:
#
#   make X11_PATH="/usr/openwin /usr/local/X11R6"
#
FT_PATH := $(subst ;, ,$(subst :, ,$(subst $(SEP),/,$(PATH))))

ifndef X11_PATH
  ifneq ($(findstring X11/bin,$(FT_PATH)),)
    xversion := X11
  else
    ifneq ($(findstring X11R6/bin,$(FT_PATH)),)
      xversion := X11R6
    else
      ifneq ($(findstring X11R5/bin,$(FT_PATH)),)
        xversion := X11R5
      endif
    endif
  endif

  ifdef xversion
    X11_PATH := $(filter %$(xversion)/bin,$(FT_PATH))
    X11_PATH := $(X11_PATH:%/bin=%)
  else
    X11_DIRS := /usr /usr/X11R6 /usr/local/X11R6
    X11_XLIB := include/X11/Xlib.h
    X11_PATH := $(foreach dir,$(X11_DIRS),$(wildcard $(dir)/$(X11_XLIB)))
    X11_PATH := $(X11_PATH:%/$(X11_XLIB)=%)
  endif
endif


##########################################################################
#
# Update some variables to compile the X11 graphics module.  Note that
# X11 is available on Unix, or on OS/2.  However, it only compiles with
# gcc on the latter platform, which is why it is safe to use the flags
# `-L' and `-l' in GRAPH_LINK.
#
ifneq ($(X11_PATH),)

  X11_INCLUDE := $(subst /,$(COMPILER_SEP),$(X11_PATH:%=%/include))
  X11_LIB     := $(subst /,$(COMPILER_SEP),$(X11_PATH:%=%/lib))

  # The GRAPH_LINK variable is expanded each time an executable is linked
  # against the graphics library.
  #
  ifeq ($(PLATFORM),unix)
    GRAPH_LINK += $(X11_LIB:%=-R%)
  endif
  GRAPH_LINK += $(X11_LIB:%=-L%) -lX11

  # Solaris needs a -lsocket in GRAPH_LINK.
  #
  UNAME := $(shell uname)
  ifneq ($(findstring $(UNAME),SunOS Solaris),)
    GRAPH_LINK += -lsocket
  endif


  # Add the X11 driver object file to the graphics library.
  #
  GRAPH_OBJS += $(OBJ_DIR_2)/grx11.$(SO)

  GR_X11 := $(GRAPH)/x11

  DEVICES += X11

  # the rule used to compile the X11 driver
  #
  $(OBJ_DIR_2)/grx11.$(SO): $(GR_X11)/grx11.c $(GR_X11)/grx11.h
	  $(CC) $(CFLAGS) $(GRAPH_INCLUDES:%=$I%) \
                $I$(subst /,$(COMPILER_SEP),$(GR_X11)) \
                $(X11_INCLUDE:%=$I%) \
                $T$(subst /,$(COMPILER_SEP),$@ $<)
endif

# EOF
