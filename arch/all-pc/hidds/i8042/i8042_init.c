/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Desc: kbd Hidd for standalone i386 AROS
*/

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <proto/exec.h>
#include <utility/utility.h>

#include "i8042_intern.h"

#undef OOPBase
#define OOPBase     (LIBBASE->csd.cs_OOPBase)
#undef HWBase
#define HWBase      (LIBBASE->csd.hwMethodBase)
#undef HWInputBase
#define HWInputBase (LIBBASE->csd.hwInputMethodBase)

static int i8042Kbd_InitAttrs(struct i8042base * LIBBASE)
{
    struct OOP_ABDescr attrbases[] = {
        {IID_Hidd,          &LIBBASE->csd.hiddAttrBase  },
        {IID_Hidd_Input,    &LIBBASE->csd.hiddInputAB   },
        {IID_Hidd_Mouse,    &LIBBASE->csd.hiddMouseAB   },
        {NULL,              NULL                        }
    };

    D(bug("[i8042] %s()\n", __func__));

    LIBBASE->csd.cs_KernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->csd.cs_KernelBase)
        return FALSE;

    LIBBASE->csd.cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->csd.cs_UtilityBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    HWBase = OOP_GetMethodID(IID_HW, 0);
    HWInputBase = OOP_GetMethodID(IID_HW_Input, 0);

    D(bug("[i8042] %s: Initialization done\n", __func__));

    return TRUE;
}

/****************************************************************************************/

static int i8042Kbd_ExpungeAttrs(struct i8042base * LIBBASE)
{
    struct OOP_ABDescr attrbases[] = {
        {IID_Hidd,          &LIBBASE->csd.hiddAttrBase  },
        {IID_Hidd_Input,    &LIBBASE->csd.hiddInputAB   },
        {IID_Hidd_Mouse,    &LIBBASE->csd.hiddMouseAB   },
        {NULL,              NULL                        }
    };

    D(bug("[i8042] %s()\n", __func__));

    OOP_ReleaseAttrBases(attrbases);

    if (!LIBBASE->csd.cs_UtilityBase)
        CloseLibrary(LIBBASE->csd.cs_UtilityBase);

    ReturnInt("i8042Kbd_ExpungeAttrs", int, TRUE);
}

/****************************************************************************************/

ADD2INITLIB(i8042Kbd_InitAttrs, 0)
ADD2EXPUNGELIB(i8042Kbd_ExpungeAttrs, 0)
