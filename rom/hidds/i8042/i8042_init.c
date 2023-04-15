/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: kbd Hidd for standalone i386 AROS
*/

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <proto/exec.h>
#include <utility/utility.h>

#include "i8042_intern.h"

#undef OOPBase
#define OOPBase (LIBBASE->csd.cs_OOPBase)

static int i8042Kbd_InitAttrs(struct i8042base * LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd      , &LIBBASE->csd.hiddAttrBase},
        {IID_Hidd_Kbd  , &LIBBASE->csd.hiddKbdAB   },
        {IID_Hidd_Mouse, &LIBBASE->csd.hiddMouseAB },
        {NULL          , NULL                      }
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

    LIBBASE->csd.hwMethodBase = OOP_GetMethodID(IID_HW, 0);

    D(bug("[i8042] %s: Initialization done\n", __func__));
    
    return TRUE;
}

/****************************************************************************************/

static int i8042Kbd_ExpungeAttrs(struct i8042base * LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd      , &LIBBASE->csd.hiddAttrBase},
        {IID_Hidd_Kbd  , &LIBBASE->csd.hiddKbdAB   },
        {IID_Hidd_Mouse, &LIBBASE->csd.hiddMouseAB },
        {NULL          , NULL                      }
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
