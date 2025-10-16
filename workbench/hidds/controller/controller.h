#ifndef HIDD_CONTROLLER_INTERN_H
#define HIDD_CONTROLLER_INTERN_H

#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>
#include <hidd/input.h>

struct ControllerInput_Data
{
    ULONG   unused;
};

struct ControllerInput_DriverData
{
    OOP_Object          *drv;
    InputIrqCallBack_t  inputcb;
    APTR                inputcbd;
    UWORD               flags;
};

#define vHidd_Controller_Extended 0x8000 /* Private flag */

struct controller_staticdata
{
    OOP_AttrBase           driverdataAB;
    OOP_AttrBase           hiddInputAB;
    OOP_AttrBase           hwInputAB;
    OOP_AttrBase           hiddControllerAB;
    OOP_AttrBase           hwAttrBase;
    OOP_MethodID           hwMethodBase;
    OOP_Class             *controllerClass;
    OOP_Class             *dataClass;
    OOP_Class             *hwClass;
    OOP_Object            *hwObject;

    struct Library *cs_SysBase;
    struct Library *cs_OOPBase;
    struct Library *cs_UtilityBase;
};

struct controllerbase
{
    struct Library          LibNode;
    struct controller_staticdata csd;
};

#define CSD(cl) (&((struct controllerbase *)cl->UserData)->csd)

#undef HiddInputAB
#undef HWInputAB
#undef HiddControllerAB
#undef HWAttrBase
#undef HWBase
#define HiddInputAB (CSD(cl)->hiddInputAB)
#define HWInputAB (CSD(cl)->hwInputAB)
#define HiddControllerAB (CSD(cl)->hiddControllerAB)
#define HWAttrBase  (CSD(cl)->hwAttrBase)
#define HWBase      (CSD(cl)->hwMethodBase)

/* Private interface of our private ControllerInput_DriverData class */
#define IID_DriverData "hidd.controller.driverdata"

#define aHidd_DriverData_ClassPtr (CSD(cl)->driverdataAB + 0)

#endif
