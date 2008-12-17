$!---------------vms_make.com for Freetype2 demos ------------------------------
$! make Freetype2 under OpenVMS
$!
$! In case of problems with the build you might want to contact me at
$! zinser@zinser.no-ip.info (preferred) or
$! zinser@sysdev.deutsche-boerse.com (Work)
$!
$!------------------------------------------------------------------------------
$!
$ on error then goto err_exit
$!
$! Just some general constants
$!
$ Make   = ""
$ true   = 1
$ false  = 0
$!
$! Setup variables holding "config" information
$!
$ name   = "FT2demos"
$ optfile =  name + ".opt"
$ ccopt    = "/name=(as_is,short)/float=ieee"
$ lopts    = ""
$!
$! Check for MMK/MMS
$!
$ If F$Search ("Sys$System:MMS.EXE") .nes. "" Then Make = "MMS"
$ If F$Type (MMK) .eqs. "STRING" Then Make = "MMK"
$!
$! Which command parameters were given
$!
$ gosub check_opts
$!
$! Create option file
$!
$ open/write optf 'optfile'
$ If f$getsyi("HW_MODEL") .gt. 1024
$ Then
$  write optf "[-.freetype2.lib]freetype2shr.exe/share"
$ else
$   write optf "[-.freetype2.lib]freetype.olb/lib"
$ endif
$ gosub check_create_vmslib
$ write optf "sys$share:decw$xlibshr.exe/share"
$ close optf
$!
$ gosub crea_mms
$ 'Make'
$ purge/nolog descrip.mms
$!
$ exit
$!
$ERR_LIB:
$ write sys$output "Error reading config file [-.freetype2]vmslib.dat"
$ goto err_exit
$FT2_ERR:
$ write sys$output "Could not locate Freetype 2 include files"
$ goto err_exit
$ERR_EXIT:
$ set message/facil/ident/sever/text
$ close/nolog optf
$ close/nolog out
$ close/nolog libdata
$ write sys$output "Exiting..."
$ exit 2
$!------------------------------------------------------------------------------
$!
$! If MMS/MMK are available dump out the descrip.mms if required
$!
$CREA_MMS:
$ write sys$output "Creating descrip.mms..."
$ create descrip.mms
$ open/append out descrip.mms
$ copy sys$input: out
$ deck
# This file is part of the FreeType project.
#
# DESCRIP.MMS: Make file for OpenVMS using MMS or MMK
# Created by Martin P.J. Zinser
#    (zinser@decus.de (preferred) or zinser@sysdev.deutsche-boerse.com (work))
$EOD
$ write out "CCOPT = ", ccopt
$ write out "LOPTS = ", lopts
$ copy sys$input: out
$ deck

.FIRST

        define freetype [-.freetype2.include.freetype]

CC = cc

# location of src for Test programs
SRCDIR = [.src]
GRAPHSRC = [.graph]
GRX11SRC = [.graph.x11]
OBJDIR = [.obj]

# include paths
INCLUDES = /include=([-.freetype2.include],[.graph])

GRAPHOBJ = $(OBJDIR)grblit.obj,  \
           $(OBJDIR)grobjs.obj,  \
           $(OBJDIR)grfont.obj,  \
           $(OBJDIR)grinit.obj,  \
           $(OBJDIR)grdevice.obj,\
           $(OBJDIR)grx11.obj,   \
           $(OBJDIR)gblender.obj, \
           $(OBJDIR)gblblit.obj

# C flags
CFLAGS = $(CCOPT)$(INCLUDES)/obj=$(OBJDIR)

ALL : ftchkwd.exe ftdump.exe ftlint.exe ftmemchk.exe ftmulti.exe ftview.exe \
      ftstring.exe fttimer.exe ftbench.exe testname.exe


ftbench.exe    : $(OBJDIR)ftbench.obj,$(OBJDIR)common.obj
        link $(LOPTS) $(OBJDIR)ftbench.obj,$(OBJDIR)common.obj,-
                     []ft2demos.opt/opt
ftchkwd.exe    : $(OBJDIR)ftchkwd.obj,$(OBJDIR)common.obj
        link $(LOPTS) $(OBJDIR)ftchkwd.obj,$(OBJDIR)common.obj,-
	             []ft2demos.opt/opt
ftdump.exe    : $(OBJDIR)ftdump.obj,$(OBJDIR)common.obj
        link $(LOPTS) $(OBJDIR)ftdump.obj,common.obj,[]ft2demos.opt/opt
ftlint.exe    : $(OBJDIR)ftlint.obj
        link $(LOPTS) $(OBJDIR)ftlint.obj,[]ft2demos.opt/opt
ftmemchk.exe  : $(OBJDIR)ftmemchk.obj
        link $(LOPTS) $(OBJDIR)ftmemchk.obj,[]ft2demos.opt/opt
ftmulti.exe   : $(OBJDIR)ftmulti.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(LOPTS) $(OBJDIR)ftmulti.obj,common.obj,$(GRAPHOBJ),[]ft2demos.opt/opt
ftview.exe    : $(OBJDIR)ftview.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(LOPTS) $(OBJDIR)ftview.obj,common.obj,$(GRAPHOBJ),[]ft2demos.opt/opt
ftstring.exe  : $(OBJDIR)ftstring.obj,$(OBJDIR)common.obj,$(GRAPHOBJ)
        link $(LOPTS) $(OBJDIR)ftstring.obj,common.obj,$(GRAPHOBJ),[]ft2demos.opt/opt
fttimer.exe   : $(OBJDIR)fttimer.obj
        link $(LOPTS) $(OBJDIR)fttimer.obj,[]ft2demos.opt/opt
testname.exe  : $(OBJDIR)testname.obj
        link $(LOPTS) $(OBJDIR)testname.obj,[]ft2demos.opt/opt

$(OBJDIR)common.obj    : $(SRCDIR)common.c , $(SRCDIR)common.h
$(OBJDIR)ftbench.obj   : $(SRCDIR)ftbench.c
$(OBJDIR)ftchkwd.obj   : $(SRCDIR)ftchkwd.c
$(OBJDIR)ftlint.obj    : $(SRCDIR)ftlint.c
$(OBJDIR)ftmemchk.obj  : $(SRCDIR)ftmemchk.c
$(OBJDIR)ftdump.obj    : $(SRCDIR)ftdump.c
$(OBJDIR)testname.obj  : $(SRCDIR)testname.c
$(OBJDIR)ftview.obj    : $(SRCDIR)ftview.c
$(OBJDIR)grblit.obj    : $(GRAPHSRC)grblit.c
$(OBJDIR)grobjs.obj    : $(GRAPHSRC)grobjs.c
$(OBJDIR)grfont.obj    : $(GRAPHSRC)grfont.c
$(OBJDIR)gblender.obj  : $(GRAPHSRC)gblender.c
$(OBJDIR)gblblit.obj   : $(GRAPHSRC)gblblit.c
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
       delete $(OBJDIR)*.obj;*,[]ft2demos.opt;*
# EOF
$ eod
$ close out
$ return
$!------------------------------------------------------------------------------
$!
$! Check commandline options and set symbols accordingly
$!
$ CHECK_OPTS:
$ i = 1
$ OPT_LOOP:
$ if i .lt. 9
$ then
$   cparm = f$edit(p'i',"upcase")
$   if cparm .eqs. "DEBUG"
$   then
$     ccopt = ccopt + "/noopt/deb"
$     lopts = lopts + "/deb"
$   endif
$!   if cparm .eqs. "link $(LOPTS)" then link only = true
$   if f$locate("LOPTS",cparm) .lt. f$length(cparm)
$   then
$     start = f$locate("=",cparm) + 1
$     len   = f$length(cparm) - start
$     lopts = lopts + f$extract(start,len,cparm)
$   endif
$   if f$locate("CCOPT",cparm) .lt. f$length(cparm)
$   then
$     start = f$locate("=",cparm) + 1
$     len   = f$length(cparm) - start
$     ccopt = ccopt + f$extract(start,len,cparm)
$   endif
$   i = i + 1
$   goto opt_loop
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! Take care of driver file with information about external libraries
$!
$! Version history
$! 0.01 20040220 First version to receive a number
$! 0.02 20040229 Echo current procedure name; use general error exit handler
$!               Remove xpm hack -> Replaced by more general dnsrl handling
$! ---> Attention slightly changed version to take into account special
$!      Situation for Freetype2 demos
$CHECK_CREATE_VMSLIB:
$!
$ if f$search("[-.freetype2]VMSLIB.DAT") .eqs. ""
$ then
$   write sys$output "Freetype2 driver file [-.freetype2]vmslib.dat not found."
$   write sys$output "Either Ft2demos have been installed in the wrong location"
$   write sys$output "or Freetype2 has not yet been configured."
$   write sys$output "Exiting..."
$   goto err_exit
$ endif
$!
$! Init symbols used to hold CPP definitions and include path
$!
$ libdefs = ""
$ libincs = ""
$!
$! Open data file with location of libraries
$!
$ open/read/end=end_lib/err=err_lib libdata [-.freetype2]VMSLIB.DAT
$LIB_LOOP:
$ read/end=end_lib libdata libline
$ libline = f$edit(libline, "UNCOMMENT,COLLAPSE")
$ if libline .eqs. "" then goto LIB_LOOP ! Comment line
$ libname = f$edit(f$element(0,"#",libline),"UPCASE")
$ write sys$output "Processing ''libname' setup ..."
$ libloc  = f$element(1,"#",libline)
$ libsrc  = f$element(2,"#",libline)
$ testinc = f$element(3,"#",libline)
$ cppdef  = f$element(4,"#",libline)
$ old_cpp = f$locate("=1",cppdef)
$ if old_cpp.lt.f$length(cppdef) then cppdef = f$extract(0,old_cpp,cppdef)
$ if f$search("''libloc'").eqs. ""
$ then
$   write sys$output "Can not find library ''libloc' - Skipping ''libname'"
$   goto LIB_LOOP
$ endif
$ libsrc_elem = 0
$ libsrc_found = false
$LIBSRC_LOOP:
$ libsrcdir = f$element(libsrc_elem,",",libsrc)
$ if (libsrcdir .eqs. ",") then goto END_LIBSRC
$ if f$search("''libsrcdir'''testinc'") .nes. "" then libsrc_found = true
$ libsrc_elem = libsrc_elem + 1
$ goto LIBSRC_LOOP
$END_LIBSRC:
$ if .not. libsrc_found
$ then
$   write sys$output "Can not find includes at ''libsrc' - Skipping ''libname'"
$   goto LIB_LOOP
$ endif
$ if (cppdef .nes. "") then libdefs = libdefs +  cppdef + ","
$ libincs = libincs + "," + libsrc
$ lqual = "/lib"
$ libtype = f$edit(f$parse(libloc,,,"TYPE"),"UPCASE")
$ if f$locate("EXE",libtype) .lt. f$length(libtype) then lqual = "/share"
$ write optf libloc , lqual
$ if (f$trnlnm("topt") .nes. "") then write topt libloc , lqual
$!
$! Nasty hack to get the freetype includes to work
$!
$ ft2def = false
$ if ((libname .eqs. "FREETYPE") .and. -
      (f$locate("FREETYPE2",cppdef) .lt. f$length(cppdef)))
$ then
$   if ((f$search("freetype:freetype.h") .nes. "") .and. -
        (f$search("freetype:[internal]ftobjs.h") .nes. ""))
$   then
$     write sys$output "Will use local definition of freetype logical"
$   else
$     ft2elem = 0
$FT2_LOOP:
$     ft2srcdir = f$element(ft2elem,",",libsrc)
$     if f$search("''ft2srcdir'''testinc'") .nes. ""
$     then
$        if f$search("''ft2srcdir'internal.dir") .nes. ""
$        then
$          ft2dev  = f$parse("''ft2srcdir'",,,"device","no_conceal")
$          ft2dir  = f$parse("''ft2srcdir'",,,"directory","no_conceal")
$          ft2conc = f$locate("][",ft2dir)
$          ft2len  = f$length(ft2dir)
$          if ft2conc .lt. ft2len
$          then
$             ft2dir = f$extract(0,ft2conc,ft2dir) + -
                       f$extract(ft2conc+2,ft2len-2,ft2dir)
$          endif
$          ft2dir = ft2dir - "]" + ".]"
$          define freetype 'ft2dev''ft2dir','ft2srcdir'
$          ft2def = true
$        else
$          goto ft2_err
$        endif
$     else
$       ft2elem = ft2elem + 1
$       goto ft2_loop
$     endif
$   endif
$ endif
$ goto LIB_LOOP
$END_LIB:
$ close libdata
$ return
