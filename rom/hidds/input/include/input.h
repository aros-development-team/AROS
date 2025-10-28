#ifndef HIDD_INPUT_H
#define HIDD_INPUT_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the input hidd.
    Lang: English.
*/

#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

typedef void *InputIrqData_t;

#include <interface/HW_Input.h>

#define CLID_Hidd_Input "hidd.input"
#define CLID_HW_Input "hw.input"

#define IID_Hidd_Input "hidd.input"

#define HiddInputAB __abHidd_Input
#define HWInputAB   __abHW_Input

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddInputAB;
extern OOP_AttrBase HWInputAB;
#endif

typedef VOID (*InputIrqCallBack_t)(APTR, InputIrqData_t);

enum {
   aoHidd_Input_Subsystem,
   aoHidd_Input_IrqHandler,
   aoHidd_Input_IrqHandlerData,
   
   num_Hidd_Input_Attrs
};

#define aHidd_Input_Subsystem		(aoHidd_Input_Subsystem      + HiddInputAB)
#define aHidd_Input_IrqHandler		(aoHidd_Input_IrqHandler     + HiddInputAB)
#define aHidd_Input_IrqHandlerData	(aoHidd_Input_IrqHandlerData + HiddInputAB)

#define IS_HIDDINPUT_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddInputAB, num_Hidd_Input_Attrs)

enum {
   aoHW_Input_ConsumerList,
   
   num_HW_Input_Attrs
};

#define aHW_Input_ConsumerList		(aoHW_Input_ConsumerList        + HWInputAB)

#define IS_HWINPUT_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HWInputAB, num_HW_Input_Attrs)

/*
 * The following flags are OR'd by hardware drivers with the keycode to pass to the IrqHandler(s)
 */

// Qualifier keys sent with KEYTOGGLE set, set their state based on the keys UP/DOWN state.
#define KBD_KEYTOGGLE (1 << 7)

/*
 * The following methods are legacy and deprecated. Do not use them.
 * Use HW_AddDriver() and HW_RemoveDriver() methods on CLID_HW_Input
 * object instead.
 */
enum
{
    moHidd_Input_AddHardwareDriver = 0,
    moHidd_Input_RemHardwareDriver,

    NUM_Input_METHODS
};

struct pHidd_Input_AddHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Class	    *driverClass;
    struct TagItem  *tags;
};

struct pHidd_Input_RemHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Object	    *driverObject;
};

#if !defined(HiddInputBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddInputBase HIDD_Input_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_Input_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID InputMethodBase;

    if (!InputMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OOPBASE(obj);

        InputMethodBase = OOP_GetMethodID(IID_Hidd_Input, 0);
    }

    return InputMethodBase;
}
#endif

#define HIDD_Input_AddHardwareDriver(obj, driverClass, tags) \
    ({OOP_Object *__obj = obj;\
      HIDD_Input_AddHardwareDriver_(HiddInputBase, __obj, driverClass, tags); })

static inline OOP_Object *HIDD_Input_AddHardwareDriver_(OOP_MethodID InputMethodBase, OOP_Object *obj, OOP_Class *driverClass, struct TagItem *tags)
{
    struct pHidd_Input_AddHardwareDriver p;

    p.mID = InputMethodBase + moHidd_Input_AddHardwareDriver;
    p.driverClass = driverClass;
    p.tags = tags;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) &p);
}

#define HIDD_Input_RemHardwareDriver(obj, driverObject) \
    ({OOP_Object *__obj = obj;\
      HIDD_Input_RemHardwareDriver_(HiddInputBase, __obj, driverObject); })

static inline OOP_Object *HIDD_Input_RemHardwareDriver_(OOP_MethodID InputMethodBase, OOP_Object *obj, OOP_Object *driverObject)
{
    struct pHidd_Input_RemHardwareDriver p;

    p.mID = InputMethodBase + moHidd_Input_RemHardwareDriver;
    p.driverObject = driverObject;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) &p);
}

#endif /* HIDD_INPUT_H */
