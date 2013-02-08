#include <aros/symbolsets.h>
#include <hidd/mouse.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

static int init_mouse(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *ms;
    OOP_Object *drv = NULL;
    
    ms = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);
    if (ms) {
        drv = HIDD_Mouse_AddHardwareDriver(ms, LIBBASE->msd.mouseclass, NULL);
        OOP_DisposeObject(ms);
    }

    if (!drv)
        return FALSE;

    LIBBASE->library.lib_OpenCnt = 1;
    return TRUE;
}

ADD2INITLIB(init_mouse, 10);
