# This file is part of the FreeType project.
#
# DESCRIP.MMS: Make file for OpenVMS using MMS or MMK
# Created by Martin P.J. Zinser
#    (zinser@decus.de (preferred) or zinser@sysdev.deutsche-boerse.com (work))


.FIRST

        define freetype [-.include.freetype]

CC = cc

# location of src for Test programs
SRCDIR = [.src]
GRAPHSRC = [.graph]
GRX11SRC = [.graph.x11]
OBJDIR = [.obj]

# include paths
INCLUDES = /include=([-.include],[.graph])

GRAPHOBJ = $(OBJDIR)grblit.obj,  \
           $(OBJDIR)grobjs.obj,  \
           $(OBJDIR)grfont.obj,  \
           $(OBJDIR)grinit.obj,  \
           $(OBJDIR)grdevice.obj,\
           $(OBJDIR)grx11.obj

# C flags
CFLAGS = $(CCOPT)$(INCLUDES)/obj=$(OBJDIR)

ALL : $(OBJDIR)test.opt ftlint.exe ftdump.exe \
      ftview.exe ftmulti.exe ftstring.exe fttimer.exe


$(OBJDIR)test.opt :
        open/write opt $(OBJDIR)test.opt
        write opt "[-.lib]freetype.olb/lib"
        write opt "sys$share:decw$xlibshr.exe/share"
        close opt
ftlint.exe    : $(OBJDIR)ftlint.obj
        link $(OBJDIR)ftlint.obj,test.opt/opt
ftdump.exe    : $(OBJDIR)ftdump.obj,$(OBJDIR)common.obj
        link $(OBJDIR)ftdump.obj,common.obj,test.opt/opt
ftview.exe    : $(OBJDIR)ftview.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(OBJDIR)ftview.obj,common.obj,$(GRAPHOBJ),test.opt/opt
ftmulti.exe   : $(OBJDIR)ftmulti.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(OBJDIR)ftmulti.obj,common.obj,$(GRAPHOBJ),test.opt/opt
ftstring.exe  : $(OBJDIR)ftstring.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(OBJDIR)ftstring.obj,common.obj,$(GRAPHOBJ),test.opt/opt
fttimer.exe   : $(OBJDIR)fttimer.obj
        link $(OBJDIR)fttimer.obj,test.opt/opt

$(OBJDIR)common.obj    : $(SRCDIR)common.c , $(SRCDIR)common.h
$(OBJDIR)ftlint.obj    : $(SRCDIR)ftlint.c
$(OBJDIR)ftdump.obj    : $(SRCDIR)ftdump.c
$(OBJDIR)ftview.obj    : $(SRCDIR)ftview.c
$(OBJDIR)grblit.obj    : $(GRAPHSRC)grblit.c
$(OBJDIR)grobjs.obj    : $(GRAPHSRC)grobjs.c
$(OBJDIR)grfont.obj    : $(GRAPHSRC)grfont.c
$(OBJDIR)grinit.obj    : $(GRAPHSRC)grinit.c
        set def $(GRAPHSRC)
        $(CC)$(CCOPT)/include=([.x11],[])/define=(DEVICE_X11)/obj=[-.obj] grinit.c
        set def [-]
$(OBJDIR)grx11.obj     : $(GRX11SRC)grx11.c
        set def $(GRX11SRC)
        $(CC)$(CCOPT)/obj=[--.obj]/include=([-]) grx11.c
        set def [--]
$(OBJDIR)grdevice.obj  : $(GRAPHSRC)grdevice.c
$(OBJDIR)ftmulti.obj   : $(SRCDIR)ftmulti.c
$(OBJDIR)ftstring.obj  : $(SRCDIR)ftstring.c
$(OBJDIR)fttimer.obj   : $(SRCDIR)fttimer.c

CLEAN :
       delete $(OBJDIR)*.obj;*,$(OBJDIR)test.opt;*
# EOF
