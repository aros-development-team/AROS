/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: kbd Hidd for standalone i386 AROS
    Lang: english
*/

#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <proto/exec.h>
#include <utility/utility.h>

#include "libbase.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef OOPBase
#define OOPBase (LIBBASE->ksd.cs_OOPBase)

static int i8042Kbd_InitAttrs(struct kbdbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd      , &LIBBASE->ksd.hiddAttrBase},
        {IID_Hidd_Kbd  , &LIBBASE->ksd.hiddKbdAB   },
        {IID_Hidd_Mouse, &LIBBASE->ksd.hiddMouseAB },
        {NULL          , NULL                      }
    };

    D(bug("[i8042] %s()\n", __func__));

    LIBBASE->ksd.cs_KernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->ksd.cs_KernelBase)
        return FALSE;

    LIBBASE->ksd.cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->ksd.cs_UtilityBase)
        return FALSE;
        
    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    LIBBASE->ksd.hwMethodBase = OOP_GetMethodID(IID_HW, 0);

    D(bug("[i8042] %s: Initialization done\n", __func__));
    
    return TRUE;
}

/****************************************************************************************/

static int i8042Kbd_ExpungeAttrs(struct kbdbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd      , &LIBBASE->ksd.hiddAttrBase},
        {IID_Hidd_Kbd  , &LIBBASE->ksd.hiddKbdAB   },
        {IID_Hidd_Mouse, &LIBBASE->ksd.hiddMouseAB },
        {NULL          , NULL                      }
    };
    
    D(bug("[i8042] %s()\n", __func__));

    OOP_ReleaseAttrBases(attrbases);

    if (!LIBBASE->ksd.cs_UtilityBase)
        CloseLibrary(LIBBASE->ksd.cs_UtilityBase);

    ReturnInt("i8042Kbd_ExpungeAttrs", int, TRUE);
}

/****************************************************************************************/

ADD2INITLIB(i8042Kbd_InitAttrs, 0)
ADD2EXPUNGELIB(i8042Kbd_ExpungeAttrs, 0)
