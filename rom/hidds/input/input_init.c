/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <oop/oop.h>
#include <hidd/hidd.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "input.h"

#define OOPBase LIBBASE->icsd.icsd_OOPBase
#undef HWBase
#define HWBase  LIBBASE->icsd.icsd_hwMB

static int Input_InitClass(struct InputClassBase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Input,    &LIBBASE->icsd.icsd_hiddInputAB },
        {IID_HW,            &LIBBASE->icsd.icsd_hwAB        },
        {IID_HW_Input,      &LIBBASE->icsd.icsd_hwInputAB   },
        {NULL,              NULL                            }
    };

    D(bug("[InputHidd] base class initialization\n"));

    NEWLIST(&LIBBASE->icsd.icsd_producers);

    LIBBASE->icsd.icsd_UtilityBase  = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->icsd.icsd_UtilityBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    LIBBASE->icsd.icsd_hwMB         = OOP_GetMethodID(IID_HW, 0);

    D(bug("[InputHidd] class initialized\n"));
    return TRUE;
}

static int Input_ExpungeClass(struct InputClassBase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Input,    &LIBBASE->icsd.icsd_hiddInputAB },
        {IID_HW,            &LIBBASE->icsd.icsd_hwAB        },
        {IID_HW_Input,      &LIBBASE->icsd.icsd_hwInputAB   },
        {NULL,              NULL                            }
    };

    D(bug("[InputHidd] class destruction\n"));

    OOP_ReleaseAttrBases(attrbases);

    if (LIBBASE->icsd.icsd_UtilityBase)
        CloseLibrary(LIBBASE->icsd.icsd_UtilityBase);

    return TRUE;
}

ADD2INITLIB(Input_InitClass, 0)
ADD2EXPUNGELIB(Input_ExpungeClass, 0)
