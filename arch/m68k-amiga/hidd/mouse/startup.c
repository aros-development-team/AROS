/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include "mouse.h"

#undef MSD
#define MSD(cl) 	msd

static int AmigaMouse_Init(struct mousebase *LIBBASE)
{
    struct mouse_staticdata *msd = &LIBBASE->msd;
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd, &HiddAttrBase },
        { IID_Hidd_Mouse, &HiddMouseAB },
        { NULL	    	, NULL      	    }
    };
    OOP_Object *ms;
    OOP_Object *drv = NULL;

    EnterFunc(bug("AmigaMouse_Init\n"));

    ms = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);
    if (ms) {
        if (OOP_ObtainAttrBases(attrbases))
        {
            HiddMouseBase = OOP_GetMethodID(IID_Hidd_Mouse, 0);
            drv = HIDD_Mouse_AddHardwareDriver(ms, LIBBASE->msd.mouseclass, NULL);
        }
        OOP_DisposeObject(ms);
    }

    if (!drv)
        return FALSE;

    LIBBASE->library.lib_OpenCnt = 1;

    ReturnInt("AmigaMouse_Init", int, TRUE);
}

static int AmigaMouse_Expunge(struct mousebase *LIBBASE)
{
    struct mouse_staticdata *msd = &LIBBASE->msd;
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd, &HiddAttrBase },
        { IID_Hidd_Mouse, &HiddMouseAB },
        { NULL	    	, NULL      	    }
    };

    EnterFunc(bug("AmigaMouse_Expunge\n"));

    OOP_ReleaseAttrBases(attrbases);

    ReturnInt("AmigaMouse_Expunge", int, TRUE);
}

ADD2INITLIB( AmigaMouse_Init, 0)
ADD2EXPUNGELIB(AmigaMouse_Expunge, 0)
