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
    OOP_AttrBase        hiddKbdAB;
    OOP_Class           *kbdClass;

    struct MinList      callbacks;

    BPTR                cs_SegList;
    struct Library     *cs_OOPBase;
};

struct kbdbase
{
    struct Library              LibNode;
    struct kbd_staticdata       csd;
};

#define CSD(cl) (&((struct kbdbase *)cl->UserData)->csd)
