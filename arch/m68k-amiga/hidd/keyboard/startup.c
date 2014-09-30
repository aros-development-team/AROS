/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <proto/oop.h>

#define HiddKbdBase (LIBBASE->ksd.hiddKbdBase)

#include <hidd/keyboard.h>

#include LC_LIBDEFS_FILE

static int init_kbd(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *kbd;
    OOP_Object *drv = NULL;
    struct Library *OOPBase = LIBBASE->ksd.cs_OOPBase;

    HiddKbdBase = OOP_GetMethodID(IID_Hidd_Kbd, 0);
    
    kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    if (kbd) {
        drv = HIDD_Kbd_AddHardwareDriver(kbd, LIBBASE->ksd.kbdclass, NULL);
	OOP_DisposeObject(kbd);
    }

    if (!drv)
	return FALSE;

    LIBBASE->library.lib_OpenCnt = 1;
    return TRUE;
}

ADD2INITLIB(init_kbd, 40);
