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

/*
 * Private attribute of our private driverNode class.
 * We need only one attribute, so we borrow IID_Hidd_Mouse interface
 * in order not to pollute attribute space and avoid hassle with obtaining
 * one more attribute base.
 */
#define aHidd_Mouse_ClassPtr HiddMouseAB + num_Hidd_Mouse_Attrs + 0

struct mouse_staticdata
{
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
