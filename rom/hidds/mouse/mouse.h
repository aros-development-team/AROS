#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

struct mouse_data
{
    struct MinNode node;
    void (*callback)(APTR data, struct pHidd_Mouse_ExtEvent *ev);
    APTR callbackdata;
};

struct driverNode
{
    OOP_Object     *drv;
    struct MinList *callbacks;
    UWORD           flags;
};

#define vHidd_Mouse_Extended 0x8000 /* Private flag */

struct mouse_staticdata
{
    OOP_AttrBase           driverdataAB;
    OOP_AttrBase           hiddMouseAB;
    OOP_AttrBase           hwAttrBase;
    OOP_MethodID           hwMethodBase;
    OOP_Class             *mouseClass;
    OOP_Class             *dataClass;
    OOP_Class             *hwClass;
    OOP_Object            *hwObject;

    struct MinList         callbacks;

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

#undef HiddMouseAB
#undef HWAttrBase
#undef HWBase
#define HiddMouseAB (CSD(cl)->hiddMouseAB)
#define HWAttrBase  (CSD(cl)->hwAttrBase)
#define HWBase      (CSD(cl)->hwMethodBase)

/* Private interface of our private driverNode class */
#define IID_DriverData "hidd.mouse.driverdata"

#define aHidd_DriverData_ClassPtr (CSD(cl)->driverdataAB + 0)
