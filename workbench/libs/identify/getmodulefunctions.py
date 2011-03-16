#!/bin/env python
# -*- coding: iso-8859-15 -*-

# creates header for identify.library/ID_Function

import re
import sys
import os

# pattern for function names
pat = re.compile(r'(\w+)\s*\(')

# LVO index of 1st function
lvolibrary = 5
lvousb = 5
lvomui = 6 # mcc, mcp, mui
lvodevice = 7
lvoresource = 1
lvogadget = 5
lvodatatype = 6
lvohidd = 5

# List contains only modules with a functionlist block in *.conf
# Format: (module name, path to *.conf, 1st LVO index, varname)
modules = (
    ["hid.hidd", "workbench/devs/USB/classes/HID/hid.conf", lvohidd, None],
    ["storage.hidd", "workbench/devs/USB/classes/MassStorage/storage.conf", lvohidd, None],
    ["aros.library", "rom/aros/aros.conf", lvolibrary, None],
    ["battclock.resource", "rom/battclock/battclock.conf", lvoresource, None],
#    ["bootmenu.resource", "rom/bootmenu/bootmenu.conf", lvoresource, None],
    ["cybergraphics.library", "workbench/libs/cgfx/cybergraphics.conf", lvolibrary, None],
    ["dbus.library", "rom/dbus/dbus.conf", lvolibrary, None],
    ["ata.device", "rom/devs/ata/ata.conf", lvodevice, None],
    ["console.device", "rom/devs/console/console.conf", lvodevice, None],
    ["input.device", "rom/devs/input/input.conf", lvodevice, None],
    ["dos.library", "rom/dos/dos.conf", lvolibrary, None],
    ["exec.library", "rom/exec/exec.conf", lvolibrary, None],
    ["expansion.library", "rom/expansion/expansion.conf", lvolibrary, None],
    ["graphics.library", "rom/graphics/graphics.conf", lvolibrary, None],
    ["layers.library", "rom/hyperlayers/layers.conf", lvolibrary, None],
    ["intuition.library", "rom/intuition/intuition.conf", lvolibrary, None],
    ["keymap.library", "rom/keymap/keymap.conf", lvolibrary, None],
    ["mathffp.library", "workbench/libs/mathffp/mathffp.conf", lvolibrary, None],
    ["mathieeesingbas.library", "workbench/libs/mathieeesingbas/mathieeesingbas.conf", lvolibrary, None],
    ["misc.resource", "rom/misc/misc.conf", lvoresource, None],
    ["oop.library", "rom/oop/oop.conf", lvolibrary, None],
    ["openfirmware.resource", "rom/openfirmware/openfirmware.conf", lvoresource, None],
    ["timer.device", "rom/timer/timer.conf", lvodevice, None],
    ["asixeth.class", "rom/usb/classes/asixeth/asixeth.conf", lvousb, None],
    ["usbaudio.class", "rom/usb/classes/audio/usbaudio.conf", lvousb, None],
    ["bluetooth.class", "rom/usb/classes/bluetooth/bluetooth.conf", lvousb, None],
    ["bootkeyboard.class", "rom/usb/classes/bootkeyboard/bootkeyboard.conf", lvousb, None],
    ["bootmouse.class", "rom/usb/classes/bootmouse/bootmouse.conf", lvousb, None],
    ["camdusbmidi.class", "rom/usb/classes/camdmidi/camdusbmidi.conf", lvousb, None],
    ["cdcacm.class", "rom/usb/classes/cdcacm/cdcacm.conf", lvousb, None],
    ["dm9601eth.class", "rom/usb/classes/davicometh/dm9601eth.conf", lvousb, None],
    ["dfu.class", "rom/usb/classes/dfu/dfu.conf", lvousb, None],
    ["egalaxtouch.class", "rom/usb/classes/egalaxtouch/egalaxtouch.conf", lvousb, None],
    ["ethwrap.class", "rom/usb/classes/ethwrap/ethwrap.conf", lvousb, None],
    ["hid.class", "rom/usb/classes/hid/hid.conf", lvousb, None],
    ["hub.class", "rom/usb/classes/hub/hub.conf", lvousb, None],
    ["massstorage.class", "rom/usb/classes/massstorage/massstorage.conf", lvousb, None],
    ["moschipeth.class", "rom/usb/classes/moschipeth/moschipeth.conf", lvousb, None],
    ["palmpda.class", "rom/usb/classes/palmpda/palmpda.conf", lvousb, None],
    ["pegasus.class", "rom/usb/classes/pegasuseth/pegasus.conf", lvousb, None],
    ["printer.class", "rom/usb/classes/printer/printer.conf", lvousb, None],
    ["ptp.class", "rom/usb/classes/ptp/ptp.conf", lvousb, None],
    ["rawwrap.class", "rom/usb/classes/rawwrap/rawwrap.conf", lvousb, None],
    ["serialcp210x.class", "rom/usb/classes/serialcp210x/serialcp210x.conf", lvousb, None],
    ["serialpl2303.class", "rom/usb/classes/serialpl2303/serialpl2303.conf", lvousb, None],
    ["simplemidi.class", "rom/usb/classes/simplemidi/simplemidi.conf", lvousb, None],
    ["stir4200.class", "rom/usb/classes/stir4200/stir4200.conf", lvousb, None],
    ["usbclass.library", "rom/usb/classes/usbclass.conf", lvolibrary, None],
    ["poseidon.library", "rom/usb/poseidon/poseidon.conf", lvolibrary, None],
    ["usbromstartup.resource", "rom/usb/poseidon/usbromstartup.conf", lvoresource, None],
    ["utility.library", "rom/utility/utility.conf", lvolibrary, None],
    ["workbench.library", "workbench/libs/workbench/workbench.conf", lvolibrary, None],
    ["png.datatype", "workbench/classes/datatypes/png/png.conf", lvodatatype, None],
    ["colorwheel.gadget", "workbench/classes/gadgets/colorwheel/colorwheel.conf", lvogadget, None],
    ["ramdrive.device", "workbench/devs/ramdrive.conf", lvodevice, None],
    ["amigaguide.library", "workbench/libs/amigaguide/amigaguide.conf", lvolibrary, None],
    ["asl.library", "workbench/libs/asl/asl.conf", lvolibrary, None],
#    ["bsdsocket.library", "workbench/libs/bsdsocket/bsdsocket.conf", lvolibrary, None],
    ["bullet.library", "workbench/libs/bullet/bullet.conf", lvolibrary, None],
    ["camd.library", "workbench/libs/camd/camd.conf", lvolibrary, None],
    ["cgxvideo.library", "workbench/libs/cgxvideo/cgxvideo.conf", lvolibrary, None],
    ["commodities.library", "workbench/libs/commodities/commodities.conf", lvolibrary, None],
    ["coolimages.library", "workbench/libs/coolimages/coolimages.conf", lvolibrary, None],
    ["datatypes.library", "workbench/libs/datatypes/datatypes.conf", lvolibrary, None],
    ["desktop.library", "workbench/libs/desktop/desktop.conf", lvolibrary, None],
    ["diskfont.library", "workbench/libs/diskfont/diskfont.conf", lvolibrary, None],
    ["freetype2.library", "workbench/libs/freetype/src/freetype2.conf", lvolibrary, None],
    ["gadtools.library", "workbench/libs/gadtools/gadtools.conf", lvolibrary, None],
    ["icon.library", "workbench/libs/icon/icon.conf", lvolibrary, None],
    ["identify.library", "workbench/libs/identify/identify.conf", lvolibrary, None],
    ["iffparse.library", "workbench/libs/iffparse/iffparse.conf", lvolibrary, None],
    ["locale.library", "workbench/libs/locale/locale.conf", lvolibrary, None],
    ["lowlevel.library", "workbench/libs/lowlevel/lowlevel.conf", lvolibrary, None],
    ["mathieeedoubbas.library", "workbench/libs/mathieeedoubbas/mathieeedoubbas.conf", lvolibrary, None],
    ["mathieeedoubtrans.library", "workbench/libs/mathieeedoubtrans/mathieeedoubtrans.conf", lvolibrary, None],
    ["mathieeesingtrans.library", "workbench/libs/mathieeesingtrans/mathieeesingtrans.conf", lvolibrary, None],
    ["mathtrans.library", "workbench/libs/mathtrans/mathtrans.conf", lvolibrary, None],
    ["muimaster.library", "workbench/libs/muimaster/muimaster.conf", lvolibrary, None],
    ["muiscreen.library", "workbench/libs/muiscreen/muiscreen.conf", lvolibrary, None],
    ["nonvolatile.library", "workbench/libs/nonvolatile/nonvolatile.conf", lvolibrary, None],
    ["nvdisk.library", "workbench/libs/nonvolatile/nvdisk/nvdisk.conf", lvolibrary, None],
    ["partition.library", "workbench/libs/partition/partition.conf", lvolibrary, None],
    ["popupmenu.library", "workbench/libs/popupmenu/popupmenu.conf", lvolibrary, None],
    ["prometheus.library", "workbench/libs/prometheus/prometheus.conf", lvolibrary, None],
    ["realtime.library", "workbench/libs/realtime/realtime.conf", lvolibrary, None],
    ["reqtools.library", "workbench/libs/reqtools/reqtools.conf", lvolibrary, None],
    ["rexxsupport.library", "workbench/libs/rexxsupport/rexxsupport.conf", lvolibrary, None],
    ["rexxsyslib.library", "workbench/libs/rexxsyslib/rexxsyslib.conf", lvolibrary, None],
    ["thread.library", "workbench/libs/thread/thread.conf", lvolibrary, None],
    ["uuid.library", "workbench/libs/uuid/uuid.conf", lvolibrary, None],
    ["MUI.MiamiPanel", "workbench/network/stacks/AROSTCP/MUI.MiamiPanel/MUI.MiamiPanel.conf", lvolibrary, None],
    ["Garshnelib.library", "workbench/tools/commodities/gblanker/Libraries/Garshnelib/Garshnelib.conf", lvolibrary, None]
)

def getmodulefunctions(module):
    modname = module[0]
    path = os.path.join(basepath, module[1])
    mode = 0
    functions = []
    
    varname = modname.replace('.', '_').lower() + '_functions'
    module[3] = varname

    file = open(path)
    
    # handle init LVO
    for r in range(0, module[2]):
        functions.append('-- init --')

    for line in file:
        if line[:20] == '##begin functionlist':
            mode = 1
        elif line[:18] == '##end functionlist':
            break
        elif mode == 1:
            if line[0:5] == '.skip':
                skip = int(line[6:])
                for r in range(0, skip):
                    functions.append('-- skipped --')
            elif line[0:1] == ".":
                None
            else:
                match = pat.search(line)
                if match:
                    name = match.group(1)
                    functions.append(name)
                else:
                    functions.append('-- empty --')

    file.close()
    return functions

def printmodulefunctions(varname, functionlist):
    lvo = 0
    print 'CONST_STRPTR %s[] = {' % varname
    for entry in functionlist:
        print '    "%s",    // %s' % (entry, lvo)
        lvo = lvo + 1
    print '    NULL'
    print '};\n'


basepath = sys.argv[1]

print '/*'
print '    Copyright © 2010, The AROS Development Team. All rights reserved.'
print '    ****** This file is automatically generated. DO NOT EDIT! *******'
print '*/\n'
print '#include <exec/types.h>\n'

for module in modules:
    result = getmodulefunctions(module)
    printmodulefunctions(module[3], result)

print 'struct Module {'
print '    CONST_STRPTR name;'
print '    CONST_STRPTR *functions;'
print '};\n'

print "struct Module modules[] = {"
for module in modules:
    print '    {"%s", %s},' % (module[0], module[3])
print '    {NULL}'
print '};\n'
