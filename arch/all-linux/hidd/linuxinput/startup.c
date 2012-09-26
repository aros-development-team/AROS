#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/mouse.h>
#include <hidd/keyboard.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

static int LinuxInput_Startup(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *ms, *kbd = NULL;

    D(bug("[LinuxMouse] LinuxMouse_Startup()\n"));

    ms = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);
    if (ms) {
        HIDD_Mouse_AddHardwareDriver(ms, LIBBASE->lsd.mouseclass, NULL);
        OOP_DisposeObject(ms);
    }

    kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    if (kbd) {
        HIDD_Mouse_AddHardwareDriver(kbd, LIBBASE->lsd.kbdclass, NULL);
        OOP_DisposeObject(kbd);
    }


    /* We use ourselves, and noone else */
    LIBBASE->library.lib_OpenCnt = 1;

    return TRUE;
}

ADD2INITLIB(LinuxInput_Startup, 10);
