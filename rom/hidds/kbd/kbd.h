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
    OOP_AttrBase        hwAB;
    OOP_MethodID        hwMB;
    OOP_Class           *kbdClass;
    OOP_Class           *hwClass;
    OOP_Object          *hwObj;

    struct MinList      callbacks;

    BPTR                cs_SegList;
    struct Library     *cs_OOPBase;
    struct Library     *cs_UtilityBase;
};

struct kbdbase
{
    struct Library              LibNode;
    struct kbd_staticdata       csd;
};

#define CSD(cl) (&((struct kbdbase *)cl->UserData)->csd)

#undef HiddKbdAB
#undef HWAttrBase
#undef HWBase
#define HiddKbdAB  (CSD(cl)->hiddKbdAB)
#define HWAttrBase (CSD(cl)->hwAB)
#define HWBase     (CSD(cl)->hwMB)
