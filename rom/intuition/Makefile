# $Id$

INSTALLPATH = ../../build
ABOXINCLUDEPATH = /gg/morphos/aboxinclude/

LDFLAGS =

LIBS =

LIB = /gg/morphos/lib/

CC = ppc-morphos-gcc

SHELL=/bin/sh

#commodore patents valid?
#PATENTS = -DNOCBMPATENTS
PATENTS =

VERSION = 50
CCPU = -O2 -mcpu=604e
CERROR = -Wall -Werror-implicit-function-declaration
CDEFINES = -D__MORPHOS__ -DPUBLIC_ONLY -DSKINS -DINTUITION_NOTIFY_SUPPORT $(PATENTS) -D_AROS -DSYSTEM_PRIVATE -U__varargs68k__ -DUSE_INLINE_STDARG -DTITLEHACK
CFLAGS = $(COPTS) $(CCPU) $(CERROR) $(CDEFINES) -mmultiple -noixemul -Icache-os -Icache-std -I$(ABOXINCLUDEPATH)os-include -I$(ABOXINCLUDEPATH)include -I/gg/morphos/include -I./ -Imorphos/


SHELL = /bin/sh

.SUFFIXES: .C .o

OBJS = \
	intuition_init.o\
	activategadget.o\
	activatewindow.o\
	addclass.o\
	addgadget.o\
	addglist.o\
	allocremember.o\
	alohaworkbench.o\
	autorequest.o\
	beginrefresh.o\
	changescreenbuffer.o\
	changewindowbox.o\
	cleardmrequest.o\
	clearmenustrip.o\
	clearpointer.o\
	closeworkbench.o\
	displayalert.o\
	donotify.o\
	doubleclick.o\
	endrefresh.o\
	endrequest.o\
	findclass.o\
	freeclass.o\
	freeintuimessage.o\
	freeremember.o\
	freescreenbuffer.o\
	freescreendrawinfo.o\
	freesysrequest.o\
	getdefaultpubscreen.o\
	getdefprefs.o\
	getprefs.o\
	getscreendata.o\
	getscreendrawinfo.o\
	helpcontrol.o\
	initrequester.o\
	itemaddress.o\
	lendmenus.o\
	lockibase.o\
	lockpubscreen.o\
	lockpubscreenlist.o\
	makeclass.o\
	makescreen.o\
	modifyidcmp.o\
	modifyprop.o\
	movescreen.o\
	movewindow.o\
	movewindowinfrontof.o\
	newmodifyprop.o\
	newobjecta.o\
	nextobject.o\
	nextpubscreen.o\
	offgadget.o\
	offmenu.o\
	ongadget.o\
	onmenu.o\
	openscreentaglist.o\
	pointinimage.o\
	pubscreenstatus.o\
	refreshgadgets.o\
	refreshglist.o\
	refreshwindowframe.o\
	remakedisplay.o\
	removeclass.o\
	removegadget.o\
	reportmouse.o\
	request.o\
	resetmenustrip.o\
	rethinkdisplay.o\
	screenposition.o\
	screentoback.o\
	screentofront.o\
	scrollwindowraster.o\
	setattrsa.o\
	setdefaultpubscreen.o\
	setdefaultscreenfont.o\
	setdmrequest.o\
	setedithook.o\
	setgadgetattrsa.o\
	setmenustrip.o\
	setmousequeue.o\
	setpointer.o\
	setprefs.o\
	setpubscreenmodes.o\
	setwindowpointera.o\
	setwindowtitles.o\
	showtitle.o\
	sizewindow.o\
	sysreqhandler.o\
	timeddisplayalert.o\
	unlockibase.o\
	unlockpubscreen.o\
	unlockpubscreenlist.o\
	wbenchtoback.o\
	wbenchtofront.o\
	windowlimits.o\
	windowtoback.o\
	windowtofront.o\
	zipwindow.o \
	getattr.o\
	drawimagestate.o\
	openwindowtaglist.o\
	allocintuimessage.o\
	lateintuiinit.o\
	eraseimage.o\
	viewaddress.o\
	dogadgetmethoda.o\
	disposeobject.o\
	drawborder.o\
	currenttime.o\
	allocscreenbuffer.o\
	changewindowshape.o\
	hidewindow.o\
	showwindow.o\
	queryoverscan.o\
	viewportaddress.o\
	printitext.o\
	sendintuimessage.o\
	removeglist.o\
	freeicdata.o\
	drawimage.o\
	gadgetmouse.o\
	buildsysrequest.o\
	buildeasyrequestargs.o\
	easyrequestargs.o\
	displaybeep.o\
	openwindow.o\
	intuition_misc.o\
	boopsigadgets.o\
	wbtasktalk.o\
	requesters.o\
	gadgets.o\
	propgadgets.o\
	propgclass.o\
	boolgadgets.o\
	strgadgets.o\
	buttongclass.o\
	fillrectclass.o\
	frameiclass.o\
	frbuttonclass.o\
	gadgetclass.o\
	groupgclass.o\
	imageclass.o\
	modelclass.o\
	rootclass.o\
	strgclass.o\
	sysiclass.o\
	icclass.o\
	itexticlass.o\
	barlabelclass.o\
	windowclasses.o\
	pointerclass.o\
	inputhandler_actions.o\
	inputhandler_support.o\
	menus.o\
	intuitextlength.o\
	obtaingirport.o\
	releasegirport.o\
	closewindow.o\
	openworkbench.o\
	misc.o\
	screendepth.o\
	closescreen.o\
	inputhandler.o\
	openscreen.o\
	iswindowvisible.o\
	functable.o\
	setiprefs.o\
	lockpubclass.o\
	unlockpubclass.o\
	dumpintuistate.o \
	menutask.o \

LIBOBJS = lib.o \
	libend.o \
	libfunctable.o \
	libfunctions.o

all:	morphosfuncs intuition.library.elf


%.o: %.c
	$(CC) $(CFLAGS) -c $*.c

morphosfuncs:
	make -C morphos

intuition.library.elf: $(OBJS) libend.o
	ppc-morphos-ld -L$(LIB) $(OBJS) libmorphosfuncs.a -lstring -lmemblock -labox -lsyscall libend.o -o intuition.library.elf.db
	ppc-morphos-strip -s --remove-section .comment intuition.library.elf.db -o intuition.library.elf

dump:
	ppc-morphos-objdump --syms --reloc --disassemble-all intuition.library.elf.db >intuition.library.elf.dump

dumpstate: dumpstate.c
	gcc -o dumpstate -O -noixemul dumpstate.c

freezedemon: freezedemon.c
	gcc -o freezedemon -O -noixemul freezedemon.c

clean:
	rm -f intuition.library.elf libmorphosfuncs.a *.o *~ morphos/*.o

# $(INSTALLPATH)/modules/intuition.library.elf
# this dependency doesn't work on a clean system..sigh
#

install: all
	-cp intuition.library.elf.db $(INSTALLPATH)/modules/intuition.library.elf

#	-cp intuition.library.elf ../../extmodules/
#	 -cp intuition.library.elf sys:morphos/extmodules/

bump:
	bumprev2 VERSION $(VERSION) FILE $(SRC)intuition_rev TAG intuition.library 

GLOBAL =   intuition_intern.h morphos/intuition_internmos.h
#GLOBAL  =
ADEBUG	=	aros/debug.h
SKIN = morphos/intuition_customize.h

intuition_init.o: intuition_init.c libdefs.h $(GLOBAL) $(SKIN) strgadgets.h inputhandler.h menutask.h intuition_rev.h
activategadget.o: activategadget.c $(GLOBAL) inputhandler_actions.h inputhandler.h gadgets.h
activatewindow.o: activatewindow.c $(GLOBAL) inputhandler.h inputhandler_actions.h inputhandler_support.h
addclass.o: addclass.c $(GLOBAL) maybe_boopsi.h
addgadget.o: addgadget.c $(GLOBAL)
addglist.o: addglist.c $(GLOBAL) boopsigadgets.h
allocremember.o: allocremember.c $(GLOBAL)
alohaworkbench.o: alohaworkbench.c $(GLOBAL)
autorequest.o: autorequest.c requesters.h $(GLOBAL)
beginrefresh.o: beginrefresh.c $(GLOBAL)
changescreenbuffer.o: changescreenbuffer.c $(GLOBAL)
changewindowbox.o: changewindowbox.c $(GLOBAL) inputhandler_actions.h
cleardmrequest.o: cleardmrequest.c $(GLOBAL)
clearmenustrip.o: clearmenustrip.c $(GLOBAL)
clearpointer.o: clearpointer.c $(GLOBAL)
closeworkbench.o: closeworkbench.c $(GLOBAL) inputhandler.h inputhandler_actions.h
displayalert.o: displayalert.c $(GLOBAL)
donotify.o: donotify.c $(GLOBAL) intuition_private.h icclass.h
doubleclick.o: doubleclick.c $(GLOBAL)
endrefresh.o: endrefresh.c $(GLOBAL)
endrequest.o: endrequest.c $(GLOBAL) inputhandler_actions.h
findclass.o: findclass.c $(GLOBAL)
freeclass.o: freeclass.c $(GLOBAL)
freeintuimessage.o: freeintuimessage.c $(GLOBAL) $(ADEBUG)
freeremember.o: freeremember.c $(GLOBAL)
freescreenbuffer.o: freescreenbuffer.c $(GLOBAL)
freescreendrawinfo.o: freescreendrawinfo.c $(GLOBAL)
freesysrequest.o: freesysrequest.c requesters.h $(GLOBAL) $(SKIN)
getdefaultpubscreen.o: getdefaultpubscreen.c $(GLOBAL)
getdefprefs.o: getdefprefs.c $(GLOBAL)
getprefs.o: getprefs.c $(GLOBAL)
getscreendata.o: getscreendata.c $(GLOBAL)
getscreendrawinfo.o: getscreendrawinfo.c $(GLOBAL) $(ADEBUG)
helpcontrol.o: helpcontrol.c $(GLOBAL) $(ADEBUG)
initrequester.o: initrequester.c $(GLOBAL)
itemaddress.o: itemaddress.c $(GLOBAL)
lendmenus.o: lendmenus.c $(GLOBAL) $(ADEBUG)
lockibase.o: lockibase.c $(GLOBAL)
lockpubscreen.o: lockpubscreen.c $(GLOBAL) $(ADEBUG)
lockpubscreenlist.o: lockpubscreenlist.c $(GLOBAL)
makeclass.o: makeclass.c $(GLOBAL) intuition_private.h
makescreen.o: makescreen.c $(GLOBAL)
modifyidcmp.o: modifyidcmp.c $(GLOBAL) $(SKIN)
modifyprop.o: modifyprop.c $(GLOBAL)
movescreen.o: movescreen.c $(GLOBAL)
movewindow.o: movewindow.c $(GLOBAL)
movewindowinfrontof.o: movewindowinfrontof.c $(GLOBAL) inputhandler_actions.h
newmodifyprop.o: newmodifyprop.c $(GLOBAL) propgadgets.h
newobjecta.o: newobjecta.c $(GLOBAL) intuition_private.h maybe_boopsi.h
nextobject.o: nextobject.c $(GLOBAL) maybe_boopsi.h
nextpubscreen.o: nextpubscreen.c $(GLOBAL)
offgadget.o: offgadget.c $(GLOBAL)
offmenu.o: offmenu.c $(GLOBAL)
ongadget.o: ongadget.c $(GLOBAL)
onmenu.o: onmenu.c $(GLOBAL)
openscreentaglist.o: openscreentaglist.c $(GLOBAL)
pointinimage.o: pointinimage.c $(GLOBAL)
pubscreenstatus.o: pubscreenstatus.c $(GLOBAL)
refreshgadgets.o: refreshgadgets.c $(GLOBAL)
refreshglist.o: refreshglist.c $(GLOBAL) boolgadgets.h boopsigadgets.h propgadgets.h strgadgets.h
refreshwindowframe.o: refreshwindowframe.c $(GLOBAL) $(SKIN) morphos/mosmisc.h morphos/renderwindowframe.h
remakedisplay.o: remakedisplay.c $(GLOBAL)
removeclass.o: removeclass.c $(GLOBAL) maybe_boopsi.h
removegadget.o: removegadget.c $(GLOBAL)
reportmouse.o: reportmouse.c $(GLOBAL)
request.o: request.c $(GLOBAL) inputhandler_actions.h
resetmenustrip.o: resetmenustrip.c $(GLOBAL)
rethinkdisplay.o: rethinkdisplay.c $(GLOBAL) inputhandler.h
screenposition.o: screenposition.c $(GLOBAL)
screentoback.o: screentoback.c $(GLOBAL)
screentofront.o: screentofront.c $(GLOBAL)
scrollwindowraster.o: scrollwindowraster.c $(GLOBAL) inputhandler_actions.h inputhandler_support.h
setattrsa.o: setattrsa.c $(GLOBAL) maybe_boopsi.h
setdefaultpubscreen.o: setdefaultpubscreen.c $(GLOBAL)
setdefaultscreenfont.o: setdefaultscreenfont.c $(GLOBAL) $(ADEBUG)
setdmrequest.o: setdmrequest.c $(GLOBAL) inputhandler_actions.h
setedithook.o: setedithook.c $(GLOBAL)
setgadgetattrsa.o: setgadgetattrsa.c $(GLOBAL)
setmenustrip.o: setmenustrip.c $(GLOBAL) menus.h
setmousequeue.o: setmousequeue.c $(GLOBAL)
setpointer.o: setpointer.c $(GLOBAL)
setprefs.o: setprefs.c $(GLOBAL)
setpubscreenmodes.o: setpubscreenmodes.c $(GLOBAL)
setwindowpointera.o: setwindowpointera.c $(GLOBAL)
setwindowtitles.o: setwindowtitles.c $(GLOBAL)
showtitle.o: showtitle.c $(GLOBAL) inputhandler_actions.h
sizewindow.o: sizewindow.c $(GLOBAL)
sysreqhandler.o: sysreqhandler.c $(GLOBAL) requesters.h
timeddisplayalert.o: timeddisplayalert.c $(GLOBAL) morphos/displayalert.h
unlockibase.o: unlockibase.c $(GLOBAL)
unlockpubscreen.o: unlockpubscreen.c $(GLOBAL)
unlockpubscreenlist.o: unlockpubscreenlist.c $(GLOBAL)
wbenchtoback.o: wbenchtoback.c $(GLOBAL)
wbenchtofront.o: wbenchtofront.c $(GLOBAL)
windowlimits.o: windowlimits.c $(GLOBAL)
windowtoback.o: windowtoback.c $(GLOBAL)
windowtofront.o: windowtofront.c $(GLOBAL) inputhandler_actions.h inputhandler_support.h
zipwindow.o : zipwindow.c $(GLOBAL) inputhandler_actions.h
getattr.o: getattr.c $(GLOBAL) maybe_boopsi.h
drawimagestate.o: drawimagestate.c $(GLOBAL)
openwindowtaglist.o: openwindowtaglist.c $(GLOBAL) $(ADEBUG)
allocintuimessage.o: allocintuimessage.c $(GLOBAL) $(ADEBUG)
lateintuiinit.o: lateintuiinit.c $(GLOBAL)
eraseimage.o: eraseimage.c $(GLOBAL)
viewaddress.o: viewaddress.c $(GLOBAL)
dogadgetmethoda.o: dogadgetmethoda.c $(GLOBAL) gadgets.h
disposeobject.o: disposeobject.c $(GLOBAL) maybe_boopsi.h
drawborder.o: drawborder.c $(GLOBAL)
currenttime.o: currenttime.c $(GLOBAL)
allocscreenbuffer.o: allocscreenbuffer.c $(GLOBAL)
changewindowshape.o: changewindowshape.c $(GLOBAL) $(ADEBUG) inputhandler_actions.h
hidewindow.o: hidewindow.c $(GLOBAL) intuition_private.h inputhandler_actions.h
showwindow.o: showwindow.c $(GLOBAL) intuition_private.h inputhandler_actions.h
queryoverscan.o: queryoverscan.c $(GLOBAL) $(ADEBUG)
viewportaddress.o: viewportaddress.c $(GLOBAL)
printitext.o: printitext.c $(GLOBAL)
sendintuimessage.o: sendintuimessage.c icclass.h $(GLOBAL) $(ADEBUG)
removeglist.o: removeglist.c $(GLOBAL) inputhandler.h
freeicdata.o: freeicdata.c $(GLOBAL) icclass.h maybe_boopsi.h
drawimage.o: drawimage.c $(GLOBAL)
gadgetmouse.o: gadgetmouse.c $(GLOBAL) $(ADEBUG) inputhandler.h inputhandler_support.h
buildsysrequest.o: buildsysrequest.c $(GLOBAL) maybe_boopsi.h requesters.h $(SKIN)
buildeasyrequestargs.o: buildeasyrequestargs.c $(GLOBAL) maybe_boopsi.h requesters.h $(SKIN)
easyrequestargs.o: easyrequestargs.c $(GLOBAL) requesters.h
displaybeep.o: displaybeep.c $(GLOBAL)
openwindow.o: openwindow.c $(GLOBAL) $(ADEBUG) $(SKIN) inputhandler.h inputhandler_actions.h boopsigadgets.h
intuition_misc.o: intuition_misc.c intuition_preferences.h $(GLOBAL) boopsigadgets.h $(SKIN)
boopsigadgets.o: boopsigadgets.c $(GLOBAL)
wbtasktalk.o: wbtasktalk.c $(GLOBAL)
requesters.o: requesters.c $(GLOBAL) maybe_boopsi.h
gadgets.o: gadgets.c $(GLOBAL) gadgets.h
propgadgets.o: propgadgets.c $(GLOBAL) propgadgets.h gadgets.h $(SKIN) morphos/intuition_extend.h
propgclass.o: propgclass.c $(GLOBAL) maybe_boopsi.h propgadgets.h
boolgadgets.o: boolgadgets.c $(GLOBAL) gadgets.h
strgadgets.o: strgadgets.c $(GLOBAL) strgadgets.h
buttongclass.o: buttongclass.c $(GLOBAL) maybe_boopsi.h gadgets.h
fillrectclass.o: fillrectclass.c $(GLOBAL) maybe_boopsi.h
frameiclass.o: frameiclass.c $(GLOBAL) maybe_boopsi.h gadgets.h
frbuttonclass.o: frbuttonclass.c $(GLOBAL) maybe_boopsi.h gadgets.h
gadgetclass.o: gadgetclass.c $(GLOBAL) maybe_boopsi.h gadgets.h icclass.h intuition_private.h
groupgclass.o: groupgclass.c $(GLOBAL) $(ADEBUG) maybe_boopsi.h
imageclass.o: imageclass.c $(GLOBAL) maybe_boopsi.h
modelclass.o: modelclass.c intuition_private.h intuition_pdefs.h $(GLOBAL)
rootclass.o: rootclass.c $(GLOBAL)
strgclass.o: strgclass.c $(GLOBAL) maybe_boopsi.h strgadgets.h
sysiclass.o: sysiclass.c $(GLOBAL) $(SKIN) morphos/sysiclass.h maybe_boopsi.h gadgets.h morphos/renderwindowframe.h
icclass.o: icclass.c intuition_private.h intuition_pdefs.h $(GLOBAL) icclass.h
itexticlass.o: itexticlass.c $(GLOBAL) maybe_boopsi.h gadgets.h
barlabelclass.o: barlabelclass.c $(GLOBAL) maybe_boopsi.h
windowclasses.o: windowclasses.c gadgets.h $(GLOBAL) inputhandler.h inputhandler_support.h $(SKIN)
pointerclass.o: pointerclass.c $(GLOBAL)
inputhandler_actions.o: inputhandler_actions.c inputhandler.h boopsigadgets.h boolgadgets.h propgadgets.h strgadgets.h gadgets.h $(GLOBAL) maybe_boopsi.h inputhandler_support.h inputhandler_actions.h menus.h
inputhandler_support.o: inputhandler_support.c inputhandler.h $(SKIN) boopsigadgets.h boolgadgets.h propgadgets.h strgadgets.h gadgets.h $(GLOBAL) intuition_private.h maybe_boopsi.h inputhandler_support.h menus.h
menus.o: menus.c inputhandler.h $(GLOBAL) maybe_boopsi.h menus.h
intuitextlength.o: intuitextlength.c $(GLOBAL)
obtaingirport.o: obtaingirport.c $(GLOBAL)
releasegirport.o: releasegirport.c $(GLOBAL)
closewindow.o: closewindow.c $(GLOBAL) inputhandler.h inputhandler_actions.h $(SKIN)
menutask.o: menutask.c inputhandler.h $(GLOBAL) maybe_boopsi.h menus.h menutask.h $(SKIN) morphos/smallmenu.h
menutask_new.o: menutask_new.c inputhandler.h $(GLOBAL) maybe_boopsi.h menus.h menutask_new.h $(SKIN) morphos/smallmenu.h morphos/smallmenu_menusupport.h
openworkbench.o: openworkbench.c $(GLOBAL)
misc.o: misc.c $(GLOBAL)
screendepth.o: screendepth.c $(GLOBAL) inputhandler_actions.h
closescreen.o: closescreen.c $(GLOBAL) morphos/intuition_extend.h $(SKIN)
inputhandler.o: inputhandler.c $(SKIN) inputhandler.h boopsigadgets.h boolgadgets.h propgadgets.h strgadgets.h gadgets.h $(GLOBAL) intuition_private.h maybe_boopsi.h inputhandler_support.h inputhandler_actions.h menus.h morphos/intuition_customizesupport.h
openscreen.o: openscreen.c $(GLOBAL) $(ADEBUG) $(SKIN) morphos/intuition_extend.h
iswindowvisible.o: iswindowvisible.c $(GLOBAL)
functable.o: functable.c libdefs.h
setiprefs.o: setiprefs.c $(GLOBAL) $(SKIN)
lockpubclass.o: lockpubclass.c $(GLOBAL)
unlockpubclass.o: unlockpubclass.c $(GLOBAL)
dumpintuistate.o: dumpintuistate.c $(GLOBAL)
