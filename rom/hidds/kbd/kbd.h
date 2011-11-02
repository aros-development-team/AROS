#include <exec/lists.h>
#include <dos/bptr.h>

struct kbd_data
{
    struct MinNode node;
    void (*callback)(APTR data, UWORD keyCode);
    APTR callbackdata;
};

struct kbd_staticdata
{
    OOP_AttrBase	hiddKbdAB;
    OOP_Class		*kbdClass;

    struct MinList	callbacks;

    BPTR                cs_SegList;
    struct Library     *cs_OOPBase;

    /* Some useful HIDD methods */
    OOP_MethodID        cs_mHidd_Kbd_AddHardwareDriver;
    OOP_MethodID        cs_mHidd_Kbd_RemHardwareDriver;
};

struct kbdbase
{
    struct Library 		LibNode;
    struct kbd_staticdata	csd;
};

#define CSD(cl) (&((struct kbdbase *)cl->UserData)->csd)
