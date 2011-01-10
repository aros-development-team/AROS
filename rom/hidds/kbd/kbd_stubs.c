#include <hidd/keyboard.h>
#include <proto/oop.h>
#include <oop/static_mid.h>

#undef OOPBase
#define OOPBase	(OOP_OOPBASE(obj))

OOP_Object *HIDD_Kbd_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass, struct TagItem *tags)
{
    STATIC_MID;
    struct pHidd_Kbd_AddHardwareDriver p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_Kbd, moHidd_Kbd_AddHardwareDriver);

    p.mID = static_mid;
    p.driverClass = driverClass;
    p.tags = tags;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) msg);
}

void HIDD_Kbd_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver)
{
    STATIC_MID;
    struct pHidd_Kbd_RemHardwareDriver p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_Kbd, moHidd_Kbd_RemHardwareDriver);

    p.mID = static_mid;
    p.driverObject = driver;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}
