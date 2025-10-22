#include <exec/lists.h>
#include <dos/bptr.h>

#include <hidd/input.h>

struct InputHWInstData
{
    struct MinNode              ihid_node;
    OOP_Object                  *ihid_hwObj;                // Subsystem object this device instance belongs to.
    InputIrqCallBack_t          ihid_callback;
    APTR                        ihid_private;
};

struct InputHWData
{
    struct MinList              ihd_consumers;
};

struct InputClassStaticData
{
    OOP_AttrBase                icsd_hiddInputAB;
    OOP_AttrBase                icsd_hwInputAB;
    OOP_AttrBase                icsd_hwAB;
    OOP_MethodID                icsd_hwMB;
    OOP_Class                   *icsd_inputClass;
    OOP_Class                   *icsd_hwClass;

    struct MinList              icsd_producers;

    struct Library              *icsd_OOPBase;
    struct Library              *icsd_UtilityBase;
};

struct InputClassBase
{
    struct Library              LibNode;
    struct InputClassStaticData icsd;
};

#define __ICSD(cl) (&((struct InputClassBase *)cl->UserData)->icsd)

#undef HiddInputAB
#undef HWInputAB
#undef HWAttrBase
#undef HWBase
#define HiddInputAB (__ICSD(cl)->icsd_hiddInputAB)
#define HWInputAB   (__ICSD(cl)->icsd_hwInputAB)
#define HWAttrBase  (__ICSD(cl)->icsd_hwAB)
#define HWBase      (__ICSD(cl)->icsd_hwMB)
