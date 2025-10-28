#ifndef I8042_INTERN_H
#define I8042_INTERN_H

#include <dos/bptr.h>
#include <exec/libraries.h>
#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/input.h>
#include <hidd/mouse.h>

#define USE_FAST_PUSHEVENT 1

/***** Common static data *******************/

struct i8042_staticdata
{
    OOP_Object              *kbdhw;
    OOP_Class               *kbdclass;
    OOP_Object              *kbdhidd;
#if USE_FAST_PUSHEVENT
    OOP_MethodFunc          kbdPushEvent;
    OOP_Class               *kbdPushEvent_Class;
#endif

    OOP_Object              *mousehw;
    OOP_Class               *mouseclass;
    OOP_Object              *mousehidd;
#if USE_FAST_PUSHEVENT
    OOP_MethodFunc          mousePushEvent;
    OOP_Class               *mousePushEvent_Class;
#endif
    
    OOP_AttrBase            hiddAttrBase;
    OOP_AttrBase            hiddInputAB;
    OOP_AttrBase            hiddMouseAB;
    OOP_MethodID            hwMethodBase;
    OOP_MethodID            hwInputMethodBase;

    BPTR                    cs_SegList;
    APTR                    cs_KernelBase;
    struct Library          *cs_OOPBase;
    struct Library          *cs_UtilityBase;
    struct Interrupt        cs_ResetInt;
    UWORD                   cs_Flags;
    UBYTE                   cs_pad;
    UBYTE                   cs_intbits;
};

#define PS2B_DISABLEKEYB    0
#define PS2F_DISABLEKEYB    (1 << PS2B_DISABLEKEYB)
#define PS2B_DISABLEMOUSE   1
#define PS2F_DISABLEMOUSE   (1 << PS2B_DISABLEMOUSE)

struct i8042base
{
    struct Library          library;    
    struct i8042_staticdata csd;
};

struct i8042_hw_common
{
    struct i8042base        *base;
    struct IORequest        *ioTimer;
    OOP_Object              *self;
};

/****************************************************************************************/

#define XSD(cl) (&((struct i8042base *)cl->UserData)->csd)

#undef HiddAttrBase
#undef HiddInputAB
#undef HiddMouseAB
#undef HWBase
#define HiddAttrBase    (XSD(cl)->hiddAttrBase)
#define HiddInputAB     (XSD(cl)->hiddInputAB)
#define HiddMouseAB     (XSD(cl)->hiddMouseAB)
#define HWBase          (XSD(cl)->hwMethodBase)
#define HWInputBase     (XSD(cl)->hwInputMethodBase)

#define KernelBase      (XSD(cl)->cs_KernelBase)
#define OOPBase         (XSD(cl)->cs_OOPBase)
#define UtilityBase     (XSD(cl)->cs_UtilityBase)

#if USE_FAST_PUSHEVENT
static inline void KBDPUSHEVENT(OOP_Class *cl, OOP_Object *o, OOP_Object *driver, InputIrqData_t evt)
{
    struct pHW_Input_PushEvent pushevt_p;

    pushevt_p.mID = HWInputBase + moHW_Input_PushEvent;
    pushevt_p.driver   = driver;
    pushevt_p.iedata   = evt;
    XSD(cl)->kbdPushEvent(XSD(cl)->kbdPushEvent_Class, o, &pushevt_p.mID);
}

static inline void MOUSEPUSHEVENT(OOP_Class *cl, OOP_Object *o, OOP_Object *driver, InputIrqData_t evt)
{
    struct pHW_Input_PushEvent pushevt_p;

    pushevt_p.mID = HWInputBase + moHW_Input_PushEvent;
    pushevt_p.driver   = driver;
    pushevt_p.iedata   = evt;
    XSD(cl)->mousePushEvent(XSD(cl)->mousePushEvent_Class, o, &pushevt_p.mID);
}
#else
#define MOUSEPUSHEVENT(cl, obj, driver, evt) HW_Input_PushEvent(obj, driver, evt)
#define KBDPUSHEVENT(cl, obj, driver, evt) HW_Input_PushEvent(obj, driver, evt)
#endif

#endif
