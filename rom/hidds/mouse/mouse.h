#ifndef HIDD_MOUSE_INTERN_H
#define HIDD_MOUSE_INTERN_H

#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>
#include <hidd/input.h>

struct MouseInput_Data
{
    ULONG   unused;
};

struct MouseInput_DriverData
{
    OOP_Object          *drv;
    InputIrqCallBack_t  inputcb;
    APTR                inputcbd;
    UWORD               flags;
};

#define vHidd_Mouse_Extended 0x8000 /* Private flag */

struct mouse_staticdata
{
    OOP_AttrBase           driverdataAB;
    OOP_AttrBase           hiddInputAB;
    OOP_AttrBase           hwInputAB;
    OOP_AttrBase           hiddMouseAB;
    OOP_AttrBase           hwAttrBase;
    OOP_MethodID           hwMethodBase;
    OOP_Class             *mouseClass;
    OOP_Class             *dataClass;
    OOP_Class             *hwClass;
    OOP_Object            *hwObject;

    struct Library *cs_SysBase;
    struct Library *cs_OOPBase;
    struct Library *cs_UtilityBase;
};

struct mousebase
{
    struct Library          LibNode;
    struct mouse_staticdata csd;
};

#define CSD(cl) (&((struct mousebase *)cl->UserData)->csd)

#undef HiddInputAB
#undef HWInputAB
#undef HiddMouseAB
#undef HWAttrBase
#undef HWBase
#define HiddInputAB (CSD(cl)->hiddInputAB)
#define HWInputAB (CSD(cl)->hwInputAB)
#define HiddMouseAB (CSD(cl)->hiddMouseAB)
#define HWAttrBase  (CSD(cl)->hwAttrBase)
#define HWBase      (CSD(cl)->hwMethodBase)

/* Private interface of our private MouseInput_DriverData class */
#define IID_DriverData "hidd.mouse.driverdata"

#define aHidd_DriverData_ClassPtr (CSD(cl)->driverdataAB + 0)

#endif
