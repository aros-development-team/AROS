/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.
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
#define HiddAttrBase (LIBBASE->csd.hiddAttrBase)
#define HWBase       (LIBBASE->csd.hwMethodBase)
#define OOPBase      (LIBBASE->csd.cs_OOPBase)

static int i8042_Init(struct i8042base * LIBBASE)
{
    OOP_Object *kbd;
    OOP_Object *ms;
    ULONG initcnt = LIBBASE->library.lib_OpenCnt;

    D(bug("[i8042] %s()\n", __func__));

    kbd = OOP_NewObject(NULL, CLID_HW_Kbd, NULL);
    D(bug("[i8042] %s: %s @ %p\n", __func__, CLID_HW_Kbd, kbd));
    if (!(LIBBASE->csd.cs_Flags & PS2F_DISABLEKEYB) && kbd)
    {
        D(bug("[i8042] %s: registering Keyboard hardware driver ..\n", __func__));
        if (HW_AddDriver(kbd, LIBBASE->csd.kbdclass, NULL))
        {
            D(bug("[i8042] %s: Keyboard driver installed\n", __func__));
            LIBBASE->library.lib_OpenCnt++;
        }
    }
    else
    {
        D(
            bug("[i8042] %s: unable to create Keyboard class instance\n", __func__);
        )
    }

    /* Mouse can be missing, it's not a failure */
    ms = OOP_NewObject(NULL, CLID_HW_Mouse, NULL);
    D(bug("[i8042] %s: %s @ %p\n", __func__, CLID_HW_Mouse, ms));
    if (!(LIBBASE->csd.cs_Flags & PS2F_DISABLEMOUSE) && ms)
    {
        D(bug("[i8042] %s: registering Mouse hardware driver ..\n", __func__));
        if (HW_AddDriver(ms, LIBBASE->csd.mouseclass, NULL))
        {
            D(bug("[i8042] %s: Mouse driver installed\n", __func__));
            LIBBASE->library.lib_OpenCnt++;
        }
    }
    else
    {
        D(
            bug("[i8042] %s: unable to create Mouse class instance\n", __func__);
        )
    }
    D(bug("[i8042] %s: finished\n", __func__));

    return (initcnt < LIBBASE->library.lib_OpenCnt) ? TRUE : FALSE;
}

ADD2INITLIB(i8042_Init, 10);
