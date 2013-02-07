#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <proto/oop.h>

#include "kbd.h"

#undef HiddAtteBase
#undef HWBase
#undef OOPBase
#define HiddAttrBase (LIBBASE->ksd.hiddAttrBase)
#define HWBase       (LIBBASE->ksd.hwMethodBase)
#define OOPBase      (LIBBASE->ksd.cs_OOPBase)

static int init_kbd(struct kbdbase *LIBBASE)
{
    struct TagItem tags[] =
    {
        {aHidd_Name        , (IPTR)"ATKbd"                          },
        {aHidd_HardwareName, (IPTR)"IBM AT-compatible keyboard port"},
        {TAG_DONE          , 0                                      }
    };
    OOP_Object *kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);

    if (!kbd)
    {
        /* This can be triggered by old base kickstart */
        return FALSE;
    }

    if (!HW_AddDriver(kbd, LIBBASE->ksd.kbdclass, tags))
        return FALSE;

    LIBBASE->library.lib_OpenCnt = 1;
    return TRUE;
}

ADD2INITLIB(init_kbd, 10);
