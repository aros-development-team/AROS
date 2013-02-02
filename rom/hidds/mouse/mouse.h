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
    struct MinNode  node;
    OOP_Object     *drv;
    struct MinList *callbacks;
    UWORD           flags;
};

#define vHidd_Mouse_Extended 0x8000 /* Private flag */

struct mouse_staticdata
{
    OOP_AttrBase           hiddMouseAB;
    OOP_Class             *mouseClass;

    struct MinList         callbacks;
    struct MinList         drivers;
    struct SignalSemaphore drivers_sem;

    struct Library *cs_SysBase;
    struct Library *cs_OOPBase;
    BPTR cs_SegList;
};

struct mousebase
{
    struct Library          LibNode;
    struct mouse_staticdata csd;
};

#define CSD(cl) (&((struct mousebase *)cl->UserData)->csd)

#define SysBase (CSD(cl)->cs_SysBase)
#define OOPBase (CSD(cl)->cs_OOPBase)
