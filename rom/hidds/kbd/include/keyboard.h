#ifndef HIDD_KEYBOARD_H
#define HIDD_KEYBOARD_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the keyboard hidd.
    Lang: English.
*/

#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#define CLID_Hidd_Kbd "hidd.kbd"
#define IID_Hidd_Kbd "hidd.kbd"

#define HiddKbdAB __abHidd_Kbd

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddKbdAB;
#endif

enum {
   aoHidd_Kbd_IrqHandler,
   aoHidd_Kbd_IrqHandlerData,
   
   num_Hidd_Kbd_Attrs
};

#define aHidd_Kbd_IrqHandler		(aoHidd_Kbd_IrqHandler     + HiddKbdAB)
#define aHidd_Kbd_IrqHandlerData	(aoHidd_Kbd_IrqHandlerData + HiddKbdAB)

#define IS_HIDDKBD_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddKbdAB, num_Hidd_Kbd_Attrs)

enum
{
    moHidd_Kbd_AddHardwareDriver = 0,
    moHidd_Kbd_RemHardwareDriver,

    NUM_Kbd_METHODS
};

struct pHidd_Kbd_AddHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Class	    *driverClass;
    struct TagItem  *tags;
};

struct pHidd_Kbd_RemHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Object	    *driverObject;
};

#if !defined(HiddKbdBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddKbdBase HIDD_Kbd_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_Kbd_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID KbdMethodBase;

    if (!KbdMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OOPBASE(obj);

        KbdMethodBase = OOP_GetMethodID(IID_Hidd_Kbd, 0);
    }

    return KbdMethodBase;
}
#endif

#define HIDD_Kbd_AddHardwareDriver(obj, driverClass, tags) \
    ({OOP_Object *__obj = obj;\
      HIDD_Kbd_AddHardwareDriver_(HiddKbdBase, __obj, driverClass, tags); })

static inline OOP_Object *HIDD_Kbd_AddHardwareDriver_(OOP_MethodID KbdMethodBase, OOP_Object *obj, OOP_Class *driverClass, struct TagItem *tags)
{
    struct pHidd_Kbd_AddHardwareDriver p;

    p.mID = KbdMethodBase + moHidd_Kbd_AddHardwareDriver;
    p.driverClass = driverClass;
    p.tags = tags;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) &p);
}

#define HIDD_Kbd_RemHardwareDriver(obj, driverObject) \
    ({OOP_Object *__obj = obj;\
      HIDD_Kbd_RemHardwareDriver_(HiddKbdBase, __obj, driverObject); })

static inline OOP_Object *HIDD_Kbd_RemHardwareDriver_(OOP_MethodID KbdMethodBase, OOP_Object *obj, OOP_Object *driverObject)
{
    struct pHidd_Kbd_RemHardwareDriver p;

    p.mID = KbdMethodBase + moHidd_Kbd_RemHardwareDriver;
    p.driverObject = driverObject;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) &p);
}

#endif /* HIDD_KEYBOARD_H */
