#include <hidd/keyboard.h>
#include <proto/oop.h>

#include "kbd.h"

#undef OOPBase
#define OOPBase	(OOP_OOPBASE(obj))

OOP_Object *HIDD_Kbd_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass, struct TagItem *tags)
{
    struct kbd_staticdata *csd = CSD(OOP_OCLASS(obj));
    struct pHidd_Kbd_AddHardwareDriver p, *msg = &p;

    if (!csd->cs_mHidd_Kbd_AddHardwareDriver)
        csd->cs_mHidd_Kbd_AddHardwareDriver = OOP_GetMethodID(IID_Hidd_Kbd, moHidd_Kbd_AddHardwareDriver);

    p.mID = csd->cs_mHidd_Kbd_AddHardwareDriver;
    p.driverClass = driverClass;
    p.tags = tags;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) msg);
}

void HIDD_Kbd_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver)
{
    struct kbd_staticdata *csd = CSD(OOP_OCLASS(obj));
    struct pHidd_Kbd_RemHardwareDriver p, *msg = &p;

    if (!csd->cs_mHidd_Kbd_RemHardwareDriver)
        csd->cs_mHidd_Kbd_RemHardwareDriver = OOP_GetMethodID(IID_Hidd_Kbd, moHidd_Kbd_RemHardwareDriver);

    p.mID = csd->cs_mHidd_Kbd_RemHardwareDriver;
    p.driverObject = driver;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}
