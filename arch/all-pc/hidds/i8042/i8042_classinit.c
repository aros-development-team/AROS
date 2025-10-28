/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <proto/oop.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>

#include "i8042_intern.h"

#undef HiddAttrBase
#define HiddAttrBase    (LIBBASE->csd.hiddAttrBase)
#undef HWBase
#define HWBase          (LIBBASE->csd.hwMethodBase)
#undef HWInputBase
#define HWInputBase     (LIBBASE->csd.hwInputMethodBase)
#undef OOPBase
#define OOPBase         (LIBBASE->csd.cs_OOPBase)

static int i8042_Init(struct i8042base * LIBBASE)
{
    ULONG initcnt = LIBBASE->library.lib_OpenCnt;

    D(bug("[i8042] %s()\n", __func__));

    LIBBASE->csd.kbdhw = OOP_NewObject(NULL, CLID_HW_Kbd, NULL);
    D(bug("[i8042] %s: %s @ %p\n", __func__, CLID_HW_Kbd, LIBBASE->csd.kbdhw));
    if (!(LIBBASE->csd.cs_Flags & PS2F_DISABLEKEYB) && LIBBASE->csd.kbdhw) {
#if USE_FAST_PUSHEVENT
        LIBBASE->csd.kbdPushEvent = OOP_GetMethod(LIBBASE->csd.kbdhw, HWInputBase + moHW_Input_PushEvent, &LIBBASE->csd.kbdPushEvent_Class);
        D(bug("[i8042] %s: Fast Keyboard PushEvent @ 0x%p\n", __func__, LIBBASE->csd.kbdPushEvent));
#endif
        D(bug("[i8042] %s: registering Keyboard hardware driver ..\n", __func__));
        if (HW_AddDriver(LIBBASE->csd.kbdhw, LIBBASE->csd.kbdclass, NULL)) {
            D(bug("[i8042] %s: Keyboard driver installed\n", __func__));
            LIBBASE->library.lib_OpenCnt++;
        }
    } else {
        D(
            bug("[i8042] %s: unable to create Keyboard class instance\n", __func__);
        )
    }

    /* Mouse can be missing, it's not a failure */
    LIBBASE->csd.mousehw = OOP_NewObject(NULL, CLID_HW_Mouse, NULL);
    D(bug("[i8042] %s: %s @ %p\n", __func__, CLID_HW_Mouse, LIBBASE->csd.mousehw));
    if (!(LIBBASE->csd.cs_Flags & PS2F_DISABLEMOUSE) && LIBBASE->csd.mousehw) {
#if USE_FAST_PUSHEVENT
        LIBBASE->csd.mousePushEvent = OOP_GetMethod(LIBBASE->csd.mousehw, HWInputBase + moHW_Input_PushEvent, &LIBBASE->csd.mousePushEvent_Class);
        D(bug("[i8042] %s: Fast Mouse PushEvent @ 0x%p\n", __func__, LIBBASE->csd.mousePushEvent));
#endif
        D(bug("[i8042] %s: registering Mouse hardware driver ..\n", __func__));
        if (HW_AddDriver(LIBBASE->csd.mousehw, LIBBASE->csd.mouseclass, NULL)) {
            D(bug("[i8042] %s: Mouse driver installed\n", __func__));
            LIBBASE->library.lib_OpenCnt++;
        }
    } else {
        D(
            bug("[i8042] %s: unable to create Mouse class instance\n", __func__);
        )
    }
    D(bug("[i8042] %s: finished\n", __func__));

    return (initcnt < LIBBASE->library.lib_OpenCnt) ? TRUE : FALSE;
}

ADD2INITLIB(i8042_Init, 10);
