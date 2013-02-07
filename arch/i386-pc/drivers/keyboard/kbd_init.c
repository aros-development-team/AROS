/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: kbd Hidd for standalone i386 AROS
    Lang: english
*/

#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <proto/exec.h>
#include <utility/utility.h>

#include "kbd.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0 
#include <aros/debug.h>

#undef OOPBase
#define OOPBase (LIBBASE->ksd.cs_OOPBase)

static int PCKbd_InitAttrs(struct kbdbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd    , &LIBBASE->ksd.hiddAttrBase},
        {IID_Hidd_Kbd, &LIBBASE->ksd.hiddKbdAB   },
        {NULL        , NULL                      }
    };

    LIBBASE->ksd.cs_KernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->ksd.cs_KernelBase)
        return FALSE;

    LIBBASE->ksd.cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!LIBBASE->ksd.cs_UtilityBase)
        return FALSE;
        
    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    LIBBASE->ksd.hwMethodBase = OOP_GetMethodID(IID_HW, 0);
    return TRUE;
}

/****************************************************************************************/

static int PCKbd_ExpungeAttrs(struct kbdbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd    , &LIBBASE->ksd.hiddAttrBase},
        {IID_Hidd_Kbd, &LIBBASE->ksd.hiddKbdAB   },
        {NULL        , NULL                      }
    };
    
    EnterFunc(bug("PCKbd_ExpungeAttrs\n"));

    OOP_ReleaseAttrBases(attrbases);

    if (!LIBBASE->ksd.cs_UtilityBase)
        CloseLibrary(LIBBASE->ksd.cs_UtilityBase);

    ReturnInt("PCKbd_ExpungeAttrs", int, TRUE);
}

/****************************************************************************************/

ADD2INITLIB(PCKbd_InitAttrs, 0)
ADD2EXPUNGELIB(PCKbd_ExpungeAttrs, 0)
