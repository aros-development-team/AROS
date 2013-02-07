#include <aros/symbolsets.h>
#include <hidd/keyboard.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

static int init_kbd(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *kbd;
    OOP_Object *drv = NULL;

    kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    if (kbd) {
        drv = HIDD_Kbd_AddHardwareDriver(kbd, LIBBASE->ksd.kbdclass, NULL);
        OOP_DisposeObject(kbd);
    }

    if (!drv)
        return FALSE;

    LIBBASE->library.lib_OpenCnt = 1;
    return TRUE;
}

ADD2INITLIB(init_kbd, 10);
