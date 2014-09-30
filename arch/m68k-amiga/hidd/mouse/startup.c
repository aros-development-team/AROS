/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <proto/oop.h>

#define HiddMouseBase (LIBBASE->msd.hiddMouseBase)

#include <hidd/mouse.h>

#include LC_LIBDEFS_FILE

static int init_mouse(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *ms;
    OOP_Object *drv = NULL;
    struct Library *OOPBase = GM_OOPBASE_FIELD(LIBBASE);

    HiddMouseBase = OOP_GetMethodID(IID_Hidd_Mouse, 0);
    
    ms = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);
    if (ms) {
        drv = HIDD_Mouse_AddHardwareDriver(ms, LIBBASE->msd.mouseclass, NULL);
	OOP_DisposeObject(ms);
    }

    if (!drv)
	return FALSE;

    LIBBASE->library.lib_OpenCnt = 1;
    return TRUE;
}

ADD2INITLIB(init_mouse, 40);
