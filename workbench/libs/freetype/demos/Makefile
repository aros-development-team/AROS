all: exes


####################################################################
#
# The `space' variable is used to avoid trailing spaces in defining
# the `T' variable later.
#
empty :=
space := $(empty) $(empty)


####################################################################
#
# TOP_DIR is the directory where the main FreeType source is found,
# as well as the `config.mk' file.
#
# TOP_DIR_2 is the directory is the top of the demonstration
# programs directory.
#
# OBJ_DIR gives the objects directory of the FreeType library.
#
TOP_DIR   ?= ../freetype2
TOP_DIR_2 ?= .
OBJ_DIR   ?= $(TOP_DIR)/objs


######################################################################
#
# CONFIG_MK points to the current `config.mk' to use.  It is defined
# by default as $(TOP_DIR)/config.mk.
#
ifndef CONFIG_MK
  PROJECT   := freetype
  CONFIG_MK := $(TOP_DIR)/config.mk
endif


######################################################################
#
# MODULES_CFG points to the current `modules.cfg' to use.  It is defined
# by default as $(TOP_DIR)/modules.cfg.
#
MODULES_CFG ?= $(TOP_DIR)/modules.cfg

ifeq ($(wildcard $(MODULES_CFG)),)
  no_modules_cfg := 1
endif


####################################################################
#
# Check that we have a working `config.mk' in the above directory.
# If not, issue a warning message, then stop there.
#
ifeq ($(wildcard $(CONFIG_MK)),)
  no_config_mk := 1
endif

ifdef no_config_mk

  exes:
	  @echo Please compile the library before the demo programs!
  clean distclean:
	  @echo "I need a path to FreeType 2's \`config.mk' to do that!"
	  @echo "Set the \`TOP_DIR' variable to the correct value."

else

  ####################################################################
  #
  # Good, now include `config.mk' in order to know how to build
  # object files from sources, as well as other things (compiler
  # flags).
  #
  include $(CONFIG_MK)

  ifndef no_modules_cfg
    include $(MODULES_CFG)
  endif

  have_makefile := $(strip $(wildcard Makefile))

  ifeq ($(PLATFORM),unix)
    ifdef DEVEL_DIR
      PLATFORM := unixdev
    endif
  endif


  ####################################################################
  #
  # Define a few important variables now.
  #
  ifeq ($(PLATFORM),unix)
    # without absolute paths libtool fails
    TOP_DIR   := $(shell cd $(TOP_DIR); pwd)
    TOP_DIR_2 := $(shell cd $(TOP_DIR_2); pwd)
    ifneq ($(have_makefile),)
      BIN_DIR_2 ?= $(TOP_DIR_2)/bin
      OBJ_DIR_2 ?= $(TOP_DIR_2)/obj
    else
      BIN_DIR_2 ?= .
      OBJ_DIR_2 ?= .
    endif
  else
    ifneq ($(have_makefile),)
      BIN_DIR_2 ?= bin
      OBJ_DIR_2 ?= obj
    else
      BIN_DIR_2 ?= .
      OBJ_DIR_2 ?= .
    endif
  endif

  GRAPH_DIR := $(TOP_DIR_2)/graph

  ifeq ($(TOP_DIR),..)
    SRC_DIR := src
  else
    SRC_DIR := $(TOP_DIR_2)/src
  endif

  FT_INCLUDES := $(OBJ_BUILD) $(BUILD_DIR) $(TOP_DIR)/include $(SRC_DIR)

  COMPILE = $(CC) $(CFLAGS) \
                  $(INCLUDES:%=$I%) \
                  $DFT_CONFIG_MODULES_H="<ftmodule.h>"

  FTLIB := $(LIB_DIR)/$(LIBRARY).$A

  # `-lm' is required to compile on some Unix systems.
  #
  ifeq ($(PLATFORM),unix)
    MATH := -lm
  endif

  ifeq ($(PLATFORM),unixdev)
    MATH := -lm
  endif

  # The default variables used to link the executables.  These can
  # be redefined for platform-specific stuff.
  #
  # The first token of LINK_ITEMS must be the executable.
  #
  LINK_ITEMS = $T$(subst /,$(COMPILER_SEP),$@ $<)

  ifeq ($(PLATFORM),unix)
    CC        = $(CCraw)
    LINK_CMD  = $(subst /,$(SEP),$(OBJ_BUILD)/libtool) \
                --mode=link $(CC) \
                $(subst /,$(COMPILER_SEP),$(LDFLAGS))
    LINK_LIBS = $(subst /,$(COMPILER_SEP),$(FTLIB) $(EFENCE))
  else
    LINK_CMD = $(CC) $(subst /,$(COMPILER_SEP),$(LDFLAGS))
    ifeq ($(PLATFORM),unixdev)
      LINK_LIBS = $(subst /,$(COMPILER_SEP),$(FTLIB) $(EFENCE)) -lm
    else
      LINK_LIBS = $(subst /,$(COMPILER_SEP),$(FTLIB) $(EFENCE))
    endif
  endif

  LINK_COMMON = $(LINK_CMD) \
                $(LINK_ITEMS) $(subst /,$(COMPILER_SEP),$(COMMON_OBJ)) \
                $(LINK_LIBS)
  LINK_GRAPH  = $(LINK_COMMON) $(subst /,$(COMPILER_SEP),$(GRAPH_LIB)) \
                $(GRAPH_LINK) $(MATH)
  LINK_NEW    = $(LINK_CMD) \
                $(LINK_ITEMS) $(subst /,$(COMPILER_SEP),$(COMMON_OBJ) \
                                        $(FTCOMMON_OBJ)) \
                $(LINK_LIBS) $(subst /,$(COMPILER_SEP),$(GRAPH_LIB)) \
                $(GRAPH_LINK) $(MATH)

  .PHONY: exes clean distclean


  ###################################################################
  #
  # Include the rules needed to compile the graphics sub-system.
  # This will also select which graphics driver to compile to the
  # sub-system.
  #
  include $(GRAPH_DIR)/rules.mk


  ####################################################################
  #
  # Detect DOS-like platforms, currently DOS, Win 3.1, Win32 & OS/2.
  #
  ifneq ($(findstring $(PLATFORM),os2 win16 win32 dos),)
    DOSLIKE := 1
  endif


  ###################################################################
  #
  # Clean-up rules.  Because the `del' command on DOS-like platforms
  # cannot take a long list of arguments, we simply erase the directory
  # contents.
  #
  ifdef DOSLIKE

    clean_demo:
	    -del obj\*.$(SO) 2> nul
	    -del $(subst /,\,$(TOP_DIR_2)/src/*.bak) 2> nul

    distclean_demo: clean_demo
	    -del obj\*.lib 2> nul
	    -del bin\*.exe 2> nul

  else

    clean_demo:
	    -$(DELETE) $(subst /,$(SEP),$(OBJ_DIR_2)/*.$(SO))
	    -$(DELETE) $(subst /,$(SEP),$(SRC_DIR)/*.bak graph/*.bak)
	    -$(DELETE) $(subst /,$(SEP),$(SRC_DIR)/*~ graph/*~)

    distclean_demo: clean_demo
	    -$(DELETE) $(subst /,$(SEP),$(EXES:%=$(BIN_DIR_2)/%$E))
	    -$(DELETE) $(subst /,$(SEP),$(GRAPH_LIB))
    ifeq ($(PLATFORM),unix)
	      -$(DELETE) $(BIN_DIR_2)/.libs/*
	      -$(DELDIR) $(BIN_DIR_2)/.libs
    endif

  endif

  clean:     clean_demo
  distclean: distclean_demo


  ####################################################################
  #
  # Compute the executable suffix to use, and put it in `E'.
  # It is ".exe" on DOS-ish platforms, and nothing otherwise.
  #
  ifdef DOSLIKE
    E := .exe
  else
    E :=
  endif


  ###################################################################
  #
  # The list of demonstration programs to build.
  #
  EXES := ftlint ftmemchk ftdump testname fttimer ftbench ftchkwd ftpatchk

  # Comment out the next line if you don't have a graphics subsystem.
  EXES += ftview ftmulti ftstring ftgamma ftgrid ftdiff

  # ftvalid requires ftgxval.c and ftotval.c
  #
  ifneq ($(findstring ftgxval.c,$(BASE_EXTENSIONS)),)
    ifneq ($(findstring ftotval.c,$(BASE_EXTENSIONS)),)
      EXES += ftvalid
    endif
  endif

  # Only uncomment the following lines if the truetype driver was
  # compiled with TT_CONFIG_OPTION_BYTECODE_INTERPRETER defined.
  #
  #  ifneq ($(findstring $(PLATFORM),os2 unix win32),)
  #    EXES += ttdebug
  #  endif

  exes: $(EXES:%=$(BIN_DIR_2)/%$E)


  INCLUDES := $(subst /,$(COMPILER_SEP),$(FT_INCLUDES))


  ####################################################################
  #
  # Rules for compiling object files for text-only demos.
  #
  COMMON_OBJ := $(OBJ_DIR_2)/common.$(SO)
  $(COMMON_OBJ): $(SRC_DIR)/common.c
  ifdef DOSLIKE
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $DEXPAND_WILDCARDS
  else
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)
  endif

  FTCOMMON_OBJ := $(OBJ_DIR_2)/ftcommon.$(SO)
  $(FTCOMMON_OBJ): $(SRC_DIR)/ftcommon.c $(SRC_DIR)/ftcommon.h
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<)


  $(OBJ_DIR_2)/%.$(SO): $(SRC_DIR)/%.c $(FTLIB)
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/ftlint.$(SO): $(SRC_DIR)/ftlint.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/ftbench.$(SO): $(SRC_DIR)/ftbench.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $(EXTRAFLAGS)

  $(OBJ_DIR_2)/ftpatchk.$(SO): $(SRC_DIR)/ftpatchk.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $(EXTRAFLAGS)

  $(OBJ_DIR_2)/ftchkwd.$(SO): $(SRC_DIR)/ftchkwd.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $(EXTRAFLAGS)

  $(OBJ_DIR_2)/compos.$(SO): $(SRC_DIR)/compos.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/ftmemchk.$(SO): $(SRC_DIR)/ftmemchk.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/fttimer.$(SO): $(SRC_DIR)/fttimer.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/fttry.$(SO): $(SRC_DIR)/fttry.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/testname.$(SO): $(SRC_DIR)/testname.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


  # We simplify the dependencies on the graphics library by using
  # $(GRAPH_LIB) directly.

  $(OBJ_DIR_2)/ftgamma.$(SO): $(SRC_DIR)/ftgamma.c \
                              $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/ftdiff.$(SO): $(SRC_DIR)/ftdiff.c \
                             $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/ftmulti.$(SO): $(SRC_DIR)/ftmulti.c \
                              $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<)

  $(OBJ_DIR_2)/ftstring.$(SO): $(SRC_DIR)/ftstring.c \
                               $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<)


# $(OBJ_DIR_2)/ftsbit.$(SO): $(SRC_DIR)/ftsbit.c $(GRAPH_LIB)
#	 $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


  ####################################################################
  #
  # Special rule to compile the `ftdump' program as it includes
  # internal header files.
  #
  $(OBJ_DIR_2)/ftdump.$(SO): $(SRC_DIR)/ftdump.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $DFT2_BUILD_LIBRARY


  ####################################################################
  #
  # Special rule to compile the `t1dump' program as it includes
  # the Type1 source path.
  #
  $(OBJ_DIR_2)/t1dump.$(SO): $(SRC_DIR)/t1dump.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<)


  ####################################################################
  #
  # Special rule to compile the `ftvalid' program as it includes
  # internal header files.
  #
  $(OBJ_DIR_2)/ftvalid.$(SO): $(SRC_DIR)/ftvalid.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) $DFT2_BUILD_LIBRARY


  ####################################################################
  #
  # Special rule to compile the `ftview' program as it includes
  # internal header files.
  #
  $(OBJ_DIR_2)/ftview.$(SO): $(SRC_DIR)/ftview.c \
                             $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<) $DFT2_BUILD_LIBRARY

  ####################################################################
  #
  # Special rule to compile the `ftgrid' program as it includes
  # internal header files.
  #
  $(OBJ_DIR_2)/ftgrid.$(SO): $(SRC_DIR)/ftgrid.c \
                             $(GRAPH_LIB)
	  $(COMPILE) $(GRAPH_INCLUDES:%=$I%) \
                     $T$(subst /,$(COMPILER_SEP),$@ $<) $DFT2_BUILD_LIBRARY


  ####################################################################
  #
  # Special rule to compile the `ttdebug' program as it includes
  # the TrueType source path and needs extra flags for correct keyboard
  # handling on Unix.

  # POSIX TERMIOS: Do not define if you use OLD U*ix like 4.2BSD.
  #
  # detect a Unix system
  #
  ifeq ($(PLATFORM),unix)
    EXTRAFLAGS = $DUNIX $DHAVE_POSIX_TERMIOS
  endif

  $(OBJ_DIR_2)/ttdebug.$(SO): $(SRC_DIR)/ttdebug.c
	  $(COMPILE) $T$(subst /,$(COMPILER_SEP),$@ $<) \
                     $I$(subst /,$(COMPILER_SEP),$(TOP_DIR)/src/truetype) \
                     $(EXTRAFLAGS)


  ####################################################################
  #
  # Rules used to link the executables.  Note that they could be
  # overridden by system-specific things.
  #
  $(BIN_DIR_2)/ftlint$E: $(OBJ_DIR_2)/ftlint.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/ftbench$E: $(OBJ_DIR_2)/ftbench.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/ftpatchk$E: $(OBJ_DIR_2)/ftpatchk.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/ftchkwd$E: $(OBJ_DIR_2)/ftchkwd.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/ftmemchk$E: $(OBJ_DIR_2)/ftmemchk.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/compos$E: $(OBJ_DIR_2)/compos.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/ftvalid$E: $(OBJ_DIR_2)/ftvalid.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/ftdump$E: $(OBJ_DIR_2)/ftdump.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/fttimer$E: $(OBJ_DIR_2)/fttimer.$(SO) $(FTLIB) $(COMMON_OBJ)
	  $(LINK_COMMON)

  $(BIN_DIR_2)/fttry$E: $(OBJ_DIR_2)/fttry.$(SO) $(FTLIB)
	  $(LINK)

  $(BIN_DIR_2)/ftsbit$E: $(OBJ_DIR_2)/ftsbit.$(SO) $(FTLIB)
	  $(LINK)

  $(BIN_DIR_2)/t1dump$E: $(OBJ_DIR_2)/t1dump.$(SO) $(FTLIB)
	  $(LINK)

  $(BIN_DIR_2)/ttdebug$E: $(OBJ_DIR_2)/ttdebug.$(SO) $(FTLIB)
	  $(LINK)

  $(BIN_DIR_2)/testname$E: $(OBJ_DIR_2)/testname.$(SO) $(FTLIB)
	  $(LINK)


  $(BIN_DIR_2)/ftview$E: $(OBJ_DIR_2)/ftview.$(SO) $(FTLIB) \
                         $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

  $(BIN_DIR_2)/ftgrid$E: $(OBJ_DIR_2)/ftgrid.$(SO) $(FTLIB) \
                         $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

  $(BIN_DIR_2)/ftgamma$E: $(OBJ_DIR_2)/ftgamma.$(SO) $(FTLIB) \
                          $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

  $(BIN_DIR_2)/ftdiff$E: $(OBJ_DIR_2)/ftdiff.$(SO) $(FTLIB) \
                         $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)

  $(BIN_DIR_2)/ftmulti$E: $(OBJ_DIR_2)/ftmulti.$(SO) $(FTLIB) \
                          $(GRAPH_LIB) $(COMMON_OBJ)
	  $(LINK_GRAPH)

  $(BIN_DIR_2)/ftstring$E: $(OBJ_DIR_2)/ftstring.$(SO) $(FTLIB) \
                           $(GRAPH_LIB) $(COMMON_OBJ) $(FTCOMMON_OBJ)
	  $(LINK_NEW)


endif


# This target builds the tarballs.
#
# Not to be run by a normal user -- there are no attempts to make it
# generic.

# we check for `dist', not `distclean'
ifneq ($(findstring distx,$(MAKECMDGOALS)x),)
  FT_H := ../freetype2/include/freetype/freetype.h

  major := $(shell sed -n 's/.*FREETYPE_MAJOR.*\([0-9]\+\)/\1/p' < $(FT_H))
  minor := $(shell sed -n 's/.*FREETYPE_MINOR.*\([0-9]\+\)/\1/p' < $(FT_H))
  patch := $(shell sed -n 's/.*FREETYPE_PATCH.*\([0-9]\+\)/\1/p' < $(FT_H))

  version    := $(major).$(minor).$(patch)
  winversion := $(major)$(minor)$(patch)
endif

dist:
	-rm -rf tmp
	rm -f ft2demos-$(version).tar.gz
	rm -f ft2demos-$(version).tar.bz2
	rm -f ftdmo$(winversion).zip

	for d in `find . -wholename '*/CVS' -prune \
	                 -o -type f \
	                 -o -print` ; do \
	  mkdir -p tmp/$$d ; \
	done ;

	currdir=`pwd` ; \
	for f in `find . -wholename '*/CVS' -prune \
	                 -o -name .cvsignore \
	                 -o -type d \
	                 -o -print` ; do \
	  ln -s $$currdir/$$f tmp/$$f ; \
	done

	cd tmp ; \
	$(MAKE) distclean

	mv tmp ft2demos-$(version)

	tar cfh - ft2demos-$(version) \
	| gzip -c > ft2demos-$(version).tar.gz
	tar cfh - ft2demos-$(version) \
	| bzip2 -c > ft2demos-$(version).tar.bz2

	@# Use CR/LF for zip files.
	zip -lr ftdmo$(winversion).zip ft2demos-$(version)

	rm -fr ft2demos-$(version)

# EOF
