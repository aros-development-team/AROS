#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "mouse.h"

/* Class initialization and destruction */
#define SysBase LIBBASE->csd.cs_SysBase
#define OOPBase LIBBASE->csd.cs_OOPBase
#undef HWBase
#define HWBase LIBBASE->csd.hwMethodBase

static int Mouse_InitClass(struct mousebase *LIBBASE)
{
    D(bug("[Mouse] base class initialization\n"));

    LIBBASE->csd.cs_UtilityBase = OpenLibrary("utility.library", 36);
    if (!LIBBASE->csd.cs_UtilityBase)
        return FALSE;

    LIBBASE->csd.hwAttrBase  = OOP_ObtainAttrBase(IID_HW);
    LIBBASE->csd.hiddMouseAB = OOP_ObtainAttrBase(IID_Hidd_Mouse);
    LIBBASE->csd.hwMethodBase = OOP_GetMethodID(IID_HW, 0);

    if (LIBBASE->csd.hwAttrBase && LIBBASE->csd.hiddMouseAB)
    {
        OOP_Object *root = OOP_NewObject(NULL, CLID_HW_Root, NULL);

        NewList((struct List *)&LIBBASE->csd.callbacks);

        if (HW_AddDriver(root, LIBBASE->csd.hwClass, NULL))
        {
            D(bug("[Mouse] Everything OK\n"));
            return TRUE;
        }
    }

    return FALSE;
}

static int Mouse_ExpungeClass(struct mousebase *LIBBASE)
{
    D(bug("[Mouse] Base Class destruction\n"));

    if (LIBBASE->csd.hiddMouseAB)
        OOP_ReleaseAttrBase(IID_Hidd_Mouse);
    if (LIBBASE->csd.hwAttrBase)
        OOP_ReleaseAttrBase(IID_HW);
    
    if (LIBBASE->csd.cs_UtilityBase)
        CloseLibrary(LIBBASE->csd.cs_UtilityBase);

    return TRUE;
}

ADD2INITLIB(Mouse_InitClass, 0)
ADD2EXPUNGELIB(Mouse_ExpungeClass, 0)
