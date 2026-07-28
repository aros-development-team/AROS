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

/*
 * Input events which arrive before any consumer has registered are copied
 * into a small pending buffer and replayed to the first consumer that
 * attaches. Without this, everything a hardware driver delivers between
 * its installation and the first consumer registration is lost - e.g. the
 * Amiga keyboard's power-up key stream (keys held during boot) arrives as
 * soon as the driver starts handshaking, before keyboard.device has
 * opened, which broke the "hold SPACE for the boot menu" check.
 *
 * ihd_evsize is the per-event byte size the owning subsystem declared
 * with aHW_Input_EventSize (0 = buffering disabled).
 *
 * NOTE: ihd_consumers must remain the first member: the default input
 * processing hook and InputHW_FlushPendingEvents() recover the
 * InputHWData from the consumer-list pointer used as callback data.
 */
#define IHD_PENDING_MAX     16  /* events held before the first consumer */
#define IHD_PENDING_EVSIZE  8   /* maximum per-event copy, in bytes */

struct InputHWData
{
    struct MinList              ihd_consumers;
    UWORD                       ihd_evsize;
    UWORD                       ihd_pendingcnt;
    UBYTE                       ihd_pending[IHD_PENDING_MAX][IHD_PENDING_EVSIZE];
};

void InputHW_FlushPendingEvents(struct MinList *cbList, InputIrqCallBack_t callback,
    APTR callbackdata);

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
