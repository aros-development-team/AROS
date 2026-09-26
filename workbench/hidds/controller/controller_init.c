/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: controller.hidd library initialisation
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/controller.h>
#include <devices/timer.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/timer.h>

#include "controller_intern.h"

#define SysBase     ((struct ExecBase *)(LIBBASE->csd.cs_SysBase))
#define OOPBase     LIBBASE->csd.cs_OOPBase
#define TimerBase   LIBBASE->csd.cs_TimerBase
#undef HWBase
#define HWBase      LIBBASE->csd.hwMethodBase

static int ControllerHidd_InitClass(struct controllerbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd,                  &LIBBASE->csd.hiddAB                },
        {IID_HW,                    &LIBBASE->csd.hwAttrBase            },
        {IID_Hidd_Input,            &LIBBASE->csd.hiddInputAB           },
        {IID_HW_Input,              &LIBBASE->csd.hwInputAB             },
        {IID_Hidd_Controller,       &LIBBASE->csd.hiddControllerAB      },
        {IID_HW_Controller,         &LIBBASE->csd.hwControllerAB        },
        {NULL,                      NULL                                }
    };
    OOP_Object *root;

    D(bug("[Controller] %s()\n", __func__));

    LIBBASE->csd.cs_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->csd.cs_UtilityBase)
        return FALSE;

    if (!OOP_ObtainAttrBases(attrbases))
        return FALSE;

    LIBBASE->csd.hwMethodBase                = OOP_GetMethodID(IID_HW, 0);
    LIBBASE->csd.hwInputMethodBase           = OOP_GetMethodID(IID_HW_Input, 0);
    LIBBASE->csd.hiddControllerMethodBase    = OOP_GetMethodID(IID_Hidd_Controller, 0);
    LIBBASE->csd.hwControllerMethodBase      = OOP_GetMethodID(IID_HW_Controller, 0);

    /*
     * timer.device for ReadEClock() based timestamps. Only the device base is
     * needed, no reply port: the request is never sent.
     */
    LIBBASE->csd.cs_TimerBase = NULL;
    LIBBASE->csd.cs_EClockFreq = 0;
    if (OpenDevice("timer.device", UNIT_ECLOCK, &LIBBASE->csd.cs_TimerReq.tr_node, 0) == 0)
    {
        struct EClockVal ev;

        LIBBASE->csd.cs_TimerBase = LIBBASE->csd.cs_TimerReq.tr_node.io_Device;
        LIBBASE->csd.cs_EClockFreq = ReadEClock(&ev);
        D(bug("[Controller] EClock frequency %u\n", LIBBASE->csd.cs_EClockFreq));
    }

    root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (root && HW_AddDriver(root, LIBBASE->csd.hwClass, NULL))
    {
        D(bug("[Controller] subsystem registered, hw object 0x%p\n", LIBBASE->csd.hwObject));
        return TRUE;
    }

    return FALSE;
}

static int ControllerHidd_ExpungeClass(struct controllerbase *LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd,                  &LIBBASE->csd.hiddAB                },
        {IID_HW,                    &LIBBASE->csd.hwAttrBase            },
        {IID_Hidd_Input,            &LIBBASE->csd.hiddInputAB           },
        {IID_HW_Input,              &LIBBASE->csd.hwInputAB             },
        {IID_Hidd_Controller,       &LIBBASE->csd.hiddControllerAB      },
        {IID_HW_Controller,         &LIBBASE->csd.hwControllerAB        },
        {NULL,                      NULL                                }
    };

    D(bug("[Controller] %s()\n", __func__));

    if (LIBBASE->csd.cs_TimerBase)
    {
        CloseDevice(&LIBBASE->csd.cs_TimerReq.tr_node);
        LIBBASE->csd.cs_TimerBase = NULL;
    }

    OOP_ReleaseAttrBases(attrbases);

    if (LIBBASE->csd.cs_UtilityBase)
        CloseLibrary(LIBBASE->csd.cs_UtilityBase);

    return TRUE;
}

ADD2INITLIB(ControllerHidd_InitClass, 0)
ADD2EXPUNGELIB(ControllerHidd_ExpungeClass, 0)
