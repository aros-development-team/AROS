#include <exec/lists.h>
#include <exec/semaphores.h>

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
    OOP_AttrBase	   hiddMouseAB;
    OOP_Class		  *mouseClass;

    struct MinList	   callbacks;
    struct MinList	   drivers;
    struct SignalSemaphore drivers_sem;
};

struct mousebase
{
    struct Library 	    LibNode;
    struct mouse_staticdata csd;
};

#define CSD(cl) (&((struct mousebase *)cl->UserData)->csd)
