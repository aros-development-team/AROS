/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: virtualpad.hidd initialisation
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/controller.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "virtualpad_intern.h"

#define SysBase ((struct ExecBase *)(LIBBASE->vsd.cs_SysBase))
#define OOPBase LIBBASE->vsd.cs_OOPBase

static int VirtualPad_InitClass(struct virtualpadbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd,                 &LIBBASE->vsd.hiddAB            },
        { IID_Hidd_Controller,      &LIBBASE->vsd.hiddControllerAB  },
        { IID_Hidd_VirtualPad,      &LIBBASE->vsd.hiddVirtualPadAB  },
        { NULL,                     NULL                            }
    };

    D(bug("[VirtualPad] %s()\n", __func__));

    LIBBASE->vsd.cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->vsd.cs_UtilityBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    LIBBASE->vsd.hiddControllerMB    = OOP_GetMethodID(IID_Hidd_Controller, 0);
    LIBBASE->vsd.hiddVirtualPadMB    = OOP_GetMethodID(IID_Hidd_VirtualPad, 0);

    return TRUE;
}

static int VirtualPad_ExpungeClass(struct virtualpadbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd,                 &LIBBASE->vsd.hiddAB            },
        { IID_Hidd_Controller,      &LIBBASE->vsd.hiddControllerAB  },
        { IID_Hidd_VirtualPad,      &LIBBASE->vsd.hiddVirtualPadAB  },
        { NULL,                     NULL                            }
    };

    OOP_ReleaseAttrBases(attrbases);
    if (LIBBASE->vsd.cs_UtilityBase)
        CloseLibrary(LIBBASE->vsd.cs_UtilityBase);
    return TRUE;
}

/*
 * controller.hidd must be loaded before genmodule creates our class (the
 * superclass CLID_Hidd_Controller has to exist), so it is opened through the
 * LIBS set which runs first.
 */
ADD2LIBS("controller.hidd", 0, struct Library *, ControllerHiddBase)

ADD2INITLIB(VirtualPad_InitClass, 0)
ADD2EXPUNGELIB(VirtualPad_ExpungeClass, 0)
