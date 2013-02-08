#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <proto/oop.h>

#include "mouse.h"

#undef HiddAttrBase
#define HiddAttrBase (LIBBASE->msd.hiddAttrBase)

static int init_mouse(struct mousebase *LIBBASE)
{
    struct TagItem tags[] =
    {
        {aHidd_Name        , (IPTR)"PSMouse"        },
        {aHidd_HardwareName, (IPTR)"PS/2 mouse port"},
        {TAG_DONE          , 0                      }
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
