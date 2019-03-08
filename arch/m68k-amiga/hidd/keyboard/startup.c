/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include "kbd.h"

#undef XSD
#define XSD(cl) 	ksd

static int AmigaKbd_Init(struct kbdbase *LIBBASE)
{
    struct kbd_staticdata *ksd = &LIBBASE->ksd;
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd	, &HiddAttrBase   },
        {IID_Hidd_Kbd	, &HiddKbdAB   },
        {NULL	    	, NULL      	    }
    };
    OOP_Object *kbd;
    OOP_Object *drv = NULL;

    EnterFunc(bug("AmigaKbd_Init\n"));

    kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    if (kbd) {
	if (OOP_ObtainAttrBases(attrbases))
	{
	    HiddKbdBase = OOP_GetMethodID(IID_Hidd_Kbd, 0);
	    drv = HIDD_Kbd_AddHardwareDriver(kbd, LIBBASE->ksd.kbdclass, NULL);
	}
	OOP_DisposeObject(kbd);
    }

    if (!drv)
	return FALSE;

    LIBBASE->library.lib_OpenCnt = 1;

    ReturnInt("AmigaKbd_Init", int, TRUE);
}

static int AmigaKbd_Expunge(struct kbdbase *LIBBASE)
{
    struct kbd_staticdata *ksd = &LIBBASE->ksd;
    struct OOP_ABDescr attrbases[] =
    {
        {IID_Hidd_Kbd	, &LIBBASE->ksd.hiddKbdAB   },
        {NULL	    	, NULL      	    }
    };
    
    EnterFunc(bug("AmigaKbd_Expunge\n"));

    OOP_ReleaseAttrBases(attrbases);
    
    ReturnInt("AmigaKbd_Expunge", int, TRUE);
}

ADD2INITLIB(AmigaKbd_Init, 0)
ADD2EXPUNGELIB(AmigaKbd_Expunge, 0)
