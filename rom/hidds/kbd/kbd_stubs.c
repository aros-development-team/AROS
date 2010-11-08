#include <hidd/keyboard.h>
#include <proto/oop.h>

#ifdef AROS_CREATE_ROM
#define	STATIC_MID  OOP_MethodID mid = 0
#else
#define	STATIC_MID  static OOP_MethodID mid
#endif

#undef OOPBase
#define OOPBase	(OOP_OOPBASE(obj))

OOP_Object *HIDD_Kbd_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass, struct TagItem *tags)
{
    STATIC_MID;
    struct pHidd_Kbd_AddHardwareDriver p, *msg = &p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_Kbd, moHidd_Kbd_AddHardwareDriver);

    p.mID = mid;
    p.driverClass = driverClass;
    p.tags = tags;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) msg);
}

void HIDD_Kbd_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver)
{
    STATIC_MID;
    struct pHidd_Kbd_RemHardwareDriver p, *msg = &p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_Kbd, moHidd_Kbd_RemHardwareDriver);

    p.mID = mid;
    p.driverObject = driver;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}
