/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/controller.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "controller.h"

/* Class initialization and destruction */
#define SysBase     ((struct ExecBase *)(LIBBASE->csd.cs_SysBase))
#define OOPBase     LIBBASE->csd.cs_OOPBase
#undef HWBase
#define HWBase      LIBBASE->csd.hwMethodBase

static int ControllerHidd_InitClass(struct controllerbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_HW,            &LIBBASE->csd.hwAttrBase    },
        {IID_Hidd_Input,    &LIBBASE->csd.hiddInputAB   },
        {IID_HW_Input,      &LIBBASE->csd.hwInputAB     },
        {IID_Hidd_Controller,    &LIBBASE->csd.hiddControllerAB   },
        {IID_DriverData,    &LIBBASE->csd.driverdataAB  },
        {NULL,              NULL                        }
    };
    D(bug("[Controller] base class initialization\n"));

    LIBBASE->csd.cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->csd.cs_UtilityBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    LIBBASE->csd.hwMethodBase   = OOP_GetMethodID(IID_HW, 0);

    OOP_Object *root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (HW_AddDriver(root, LIBBASE->csd.hwClass, NULL))
    {
        D(bug("[Controller] Controller Subsystem Registered\n"));
        return TRUE;
    }

    return FALSE;
}

static int ControllerHidd_ExpungeClass(struct controllerbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_HW,            &LIBBASE->csd.hwAttrBase    },
        {IID_Hidd_Input,    &LIBBASE->csd.hiddInputAB   },
        {IID_HW_Input,      &LIBBASE->csd.hwInputAB     },
        {IID_Hidd_Controller,    &LIBBASE->csd.hiddControllerAB   },
        {IID_DriverData,    &LIBBASE->csd.driverdataAB  },
        {NULL,              NULL                        }
    };

    D(bug("[Controller] Base Class destruction\n"));

    OOP_ReleaseAttrBases(attrbases);

    if (LIBBASE->csd.cs_UtilityBase)
        CloseLibrary(LIBBASE->csd.cs_UtilityBase);

    return TRUE;
}

ADD2INITLIB(ControllerHidd_InitClass, 0)
ADD2EXPUNGELIB(ControllerHidd_ExpungeClass, 0)
