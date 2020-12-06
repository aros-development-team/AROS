/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>

#include "i8042_intern.h"

#undef HiddAttrBase
#undef HWBase
#undef OOPBase
#define HiddAttrBase (LIBBASE->ksd.hiddAttrBase)
#define HWBase       (LIBBASE->ksd.hwMethodBase)
#define OOPBase      (LIBBASE->ksd.cs_OOPBase)

static int i8042_Init(struct kbdbase *LIBBASE)
{
    OOP_Object *kbd;
    OOP_Object *ms;

    D(bug("[i8042] %s()\n", __func__));
    kbd = OOP_NewObject(NULL, CLID_HW_Kbd, NULL);
    D(bug("[i8042] %s: %s @ %p\n", __func__, CLID_HW_Kbd, kbd));
    if (!kbd)
    {
        /* This can be triggered by old base kickstart */
        D(bug("[i8042] %s: unable to create Keyboard class instance\n", __func__));
        return FALSE;
    }
    D(bug("[i8042] %s: registering Keyboard hardware driver ..\n", __func__));
    if (!HW_AddDriver(kbd, LIBBASE->ksd.kbdclass, NULL))
    {
        D(bug("[i8042] %s: no Keyboard controller detected\n", __func__));
        return FALSE;
    }
    LIBBASE->library.lib_OpenCnt = 1;

    /* Mouse can be missing, it's not a failure */
    ms = OOP_NewObject(NULL, CLID_HW_Mouse, NULL);
    D(bug("[i8042] %s: %s @ %p\n", __func__, CLID_HW_Mouse, ms));
    if (ms)
    {
        D(bug("[i8042] %s: registering Mouse hardware driver ..\n", __func__));
        if (HW_AddDriver(ms, LIBBASE->ksd.mouseclass, NULL))
        {
            D(bug("[i8042] %s: Mouse driver installed\n", __func__));
            LIBBASE->library.lib_OpenCnt++;
        }
    }
    else
    {
        D(
            bug("[i8042] %s: unable to create Mouse class instance\n", __func__);
            bug("[i8042] %s: # starting with Keyboard only\n", __func__);
        )
    }
    D(bug("[i8042] %s: finished\n", __func__));

    return TRUE;
}

ADD2INITLIB(i8042_Init, 10);
