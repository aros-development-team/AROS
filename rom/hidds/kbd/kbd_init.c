/*
    Copyright (C) 2004-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "kbd.h"

#undef HWBase
#define HWBase (LIBBASE->csd.hwMB)

static int KBD_InitClass(struct kbdbase *LIBBASE)
{
    struct Library *OOPBase = LIBBASE->csd.cs_OOPBase;

    D(bug("[KBD] base class initialization\n"));
    
    LIBBASE->csd.cs_UtilityBase = OpenLibrary("utility.library", 0);
    if (!LIBBASE->csd.cs_UtilityBase)
        return FALSE;

    LIBBASE->csd.hiddKbdAB = OOP_ObtainAttrBase(IID_Hidd_Kbd);
    LIBBASE->csd.hwAB = OOP_ObtainAttrBase(IID_HW);
    LIBBASE->csd.hwMB = OOP_GetMethodID(IID_HW, 0);

    if (LIBBASE->csd.hiddKbdAB && LIBBASE->csd.hwAB)
    {
        OOP_Object *root = OOP_NewObject(NULL, CLID_HW_Root, NULL);

        NewList((struct List *)&LIBBASE->csd.callbacks);

        if (HW_AddDriver(root, LIBBASE->csd.hwClass, NULL))
        {
            D(bug("[KBD] Everything OK\n"));
            return TRUE;
        }
    }

    return FALSE;
}

static int KBD_ExpungeClass(struct kbdbase *LIBBASE)
{
    struct Library *OOPBase = LIBBASE->csd.cs_OOPBase;

    D(bug("[KBD] Base Class destruction\n"));

    if (LIBBASE->csd.hiddKbdAB)
        OOP_ReleaseAttrBase(IID_Hidd_Kbd);
    if (LIBBASE->csd.cs_UtilityBase)
        CloseLibrary(LIBBASE->csd.cs_UtilityBase);
        
    return TRUE;
}

ADD2INITLIB(KBD_InitClass, 0)
ADD2EXPUNGELIB(KBD_ExpungeClass, 0)
