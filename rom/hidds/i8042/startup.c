#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <proto/oop.h>

#include "libbase.h"

#undef HiddAttrBase
#undef HWBase
#undef OOPBase
#define HiddAttrBase (LIBBASE->ksd.hiddAttrBase)
#define HWBase       (LIBBASE->ksd.hwMethodBase)
#define OOPBase      (LIBBASE->ksd.cs_OOPBase)

static int init_kbd(struct kbdbase *LIBBASE)
{
    struct TagItem kbd_tags[] =
    {
        {aHidd_Name        , (IPTR)"ATKbd"                          },
        {aHidd_HardwareName, (IPTR)"IBM AT-compatible keyboard port"},
        {TAG_DONE          , 0                                      }
    };
    struct TagItem mouse_tags[] =
    {
        {aHidd_Name        , (IPTR)"PSMouse"        },
        {aHidd_HardwareName, (IPTR)"PS/2 mouse port"},
        {TAG_DONE          , 0                      }
    };
    OOP_Object *kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    OOP_Object *ms = OOP_NewObject(NULL, CLID_HW_Mouse, NULL);

    if ((!kbd) || (!ms))
    {
        /* This can be triggered by old base kickstart */
        return FALSE;
    }

    if (HW_AddDriver(kbd, LIBBASE->ksd.kbdclass, kbd_tags))
        LIBBASE->library.lib_OpenCnt++;

    if (HW_AddDriver(ms, LIBBASE->ksd.mouseclass, mouse_tags))        
        LIBBASE->library.lib_OpenCnt++;

    return TRUE;
}

ADD2INITLIB(init_kbd, 10);
