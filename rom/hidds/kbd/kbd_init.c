/*
    Copyright (C) 2004-2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "kbd.h"

#undef HWBase
#define HWBase (LIBBASE->csd.hwMB)

static int KbdHidd_InitClass(struct kbdbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_HW,            &LIBBASE->csd.hwAB          },
        {IID_Hidd_Input,    &LIBBASE->csd.hiddInputAB   },
        {NULL,              NULL                        }
    };
    struct Library *OOPBase = LIBBASE->csd.cs_OOPBase;

    D(bug("[KBD] base class initialization\n"));
    
    LIBBASE->csd.cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->csd.cs_UtilityBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    LIBBASE->csd.hwMB = OOP_GetMethodID(IID_HW, 0);

    if (LIBBASE->csd.hiddInputAB && LIBBASE->csd.hwAB)
    {
        OOP_Object *root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
        if (HW_AddDriver(root, LIBBASE->csd.hwClass, NULL))
        {
            D(bug("[KBD] Keyboard Subsystem Registered\n"));
            return TRUE;
        }
    }

    return FALSE;
}

static int KbdHidd_ExpungeClass(struct kbdbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_HW,            &LIBBASE->csd.hwAB          },
        {IID_Hidd_Input,    &LIBBASE->csd.hiddInputAB   },
        {NULL,              NULL                        }
    };
    struct Library *OOPBase = LIBBASE->csd.cs_OOPBase;

    D(bug("[KBD] Base Class destruction\n"));

    OOP_ReleaseAttrBases(attrbases);

    if (LIBBASE->csd.cs_UtilityBase)
        CloseLibrary(LIBBASE->csd.cs_UtilityBase);
        
    return TRUE;
}

ADD2INITLIB(KbdHidd_InitClass, 0)
ADD2EXPUNGELIB(KbdHidd_ExpungeClass, 0)
