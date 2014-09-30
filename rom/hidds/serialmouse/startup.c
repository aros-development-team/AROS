/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <proto/oop.h>

#include "mouse.h"

#undef HiddAttrBase
#undef HWBase
#undef OOPBase
#define HiddAttrBase (LIBBASE->msd.hiddAttrBase)
#define HWBase       (LIBBASE->msd.hwMethodBase)
#define OOPBase      (LIBBASE->msd.oopBase)

static int init_mouse(struct mousebase *LIBBASE)
{
    /* FIXME: implement proper detection. See TODO notice in mouseclass.c */
    struct TagItem tags[] =
    {
        {aHidd_Name        , (IPTR)"SerMouse"    },
        {aHidd_HardwareName, (IPTR)"Serial mouse"},
        {TAG_DONE          , 0                   }
    };
    OOP_Object *ms = OOP_NewObject(NULL, CLID_HW_Mouse, NULL);

    if (!ms)
        return FALSE;

    if (!HW_AddDriver(ms, LIBBASE->msd.mouseclass, tags))
        return FALSE;

    LIBBASE->library.lib_OpenCnt = 1;
    return TRUE;
}

ADD2INITLIB(init_mouse, 10);
