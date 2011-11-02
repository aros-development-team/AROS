#include <hidd/mouse.h>
#include <proto/oop.h>

#include "mouse.h"

#undef OOPBase
#define OOPBase	(OOP_OOPBASE(obj))

OOP_Object *HIDD_Mouse_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass, struct TagItem *tags)
{
    struct mouse_staticdata *csd = CSD(OOP_OCLASS(obj));
    struct pHidd_Mouse_AddHardwareDriver p, *msg = &p;

    if (!csd->cs_mHidd_Mouse_AddHardwareDriver)
        csd->cs_mHidd_Mouse_AddHardwareDriver = OOP_GetMethodID(IID_Hidd_Mouse, moHidd_Mouse_AddHardwareDriver);

    p.mID = csd->cs_mHidd_Mouse_AddHardwareDriver;
    p.driverClass = driverClass;
    p.tags = tags;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) msg);
}

void HIDD_Mouse_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver)
{
    struct mouse_staticdata *csd = CSD(OOP_OCLASS(obj));
    struct pHidd_Mouse_RemHardwareDriver p, *msg = &p;

    if (!csd->cs_mHidd_Mouse_RemHardwareDriver)
        csd->cs_mHidd_Mouse_RemHardwareDriver = OOP_GetMethodID(IID_Hidd_Mouse, moHidd_Mouse_RemHardwareDriver);

    p.mID = csd->cs_mHidd_Mouse_RemHardwareDriver;
    p.driverObject = driver;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}
