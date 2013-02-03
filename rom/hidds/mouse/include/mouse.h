#ifndef HIDD_MOUSE_H
#define HIDD_MOUSE_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the mouse hidd.
    Lang: English.
*/

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef PROTO_OOP_H
#   include <proto/oop.h>
#endif

#define CLID_Hidd_Mouse "hidd.mouse"
#define CLID_HW_Mouse   "hw.mouse"

#define IID_Hidd_Mouse "hidd.mouse"

#define HiddMouseAB __abHidd_Mouse

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddMouseAB;
#endif

/* Attrs */

enum {

   aoHidd_Mouse_IrqHandler,
   aoHidd_Mouse_IrqHandlerData,
   aoHidd_Mouse_State,
   aoHidd_Mouse_RelativeCoords,
   aoHidd_Mouse_Extended,

   num_Hidd_Mouse_Attrs
};


#define aHidd_Mouse_IrqHandler          (aoHidd_Mouse_IrqHandler     + HiddMouseAB)
#define aHidd_Mouse_IrqHandlerData      (aoHidd_Mouse_IrqHandlerData + HiddMouseAB)
#define aHidd_Mouse_State               (aoHidd_Mouse_State          + HiddMouseAB)  
#define aHidd_Mouse_RelativeCoords      (aoHidd_Mouse_RelativeCoords + HiddMouseAB)
#define aHidd_Mouse_Extended            (aoHidd_Mouse_Extended       + HiddMouseAB)

#define IS_HIDDMOUSE_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddMouseAB, num_Hidd_Mouse_Attrs)

/* Parameter values for the IRQ handler */

struct pHidd_Mouse_Event
{
    UWORD button;
    WORD x;
    WORD y;
    UWORD type; /* See below */
};

struct pHidd_Mouse_ExtEvent
{
    UWORD button;
    WORD  x;
    WORD  y;
    UWORD type;
    UWORD flags; /* See below */
};

/* Event types */
enum {
   vHidd_Mouse_Press,
   vHidd_Mouse_Release,
   vHidd_Mouse_Motion,
   vHidd_Mouse_WheelMotion
};

/* Button codes */
enum {
   vHidd_Mouse_NoButton,
   vHidd_Mouse_Button1,
   vHidd_Mouse_Button2,
   vHidd_Mouse_Button3
};

/* Flags */
#define vHidd_Mouse_Relative 0x0001

enum
{
    moHidd_Mouse_AddHardwareDriver = 0,
    moHidd_Mouse_RemHardwareDriver,

    NUM_Mouse_METHODS
};

struct pHidd_Mouse_AddHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Class       *driverClass;
    struct TagItem  *tags;
};

struct pHidd_Mouse_RemHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Object      *driverObject;
};

#if !defined(HiddMouseBase) && !defined(__OOP_NOMETHODBASES__)
#define HiddMouseBase HIDD_Mouse_GetMethodBase(__obj)

static inline OOP_MethodID HIDD_Mouse_GetMethodBase(OOP_Object *obj)
{
    static OOP_MethodID MouseMethodBase;

    if (!MouseMethodBase)
    {
        struct Library *OOPBase = (struct Library *)OOP_OOPBASE(obj);

        MouseMethodBase = OOP_GetMethodID(IID_Hidd_Mouse, 0);
    }

    return MouseMethodBase;
}
#endif

#define HIDD_Mouse_AddHardwareDriver(obj, driverClass, tags) \
    ({OOP_Object *__obj = obj;\
     HIDD_Mouse_AddHardwareDriver_(HiddMouseBase, __obj, driverClass, tags); })

static inline OOP_Object *HIDD_Mouse_AddHardwareDriver_(OOP_MethodID MouseMethodBase, OOP_Object *obj, OOP_Class *driverClass, struct TagItem *tags)
{
    struct pHidd_Mouse_AddHardwareDriver p;

    p.mID = MouseMethodBase + moHidd_Mouse_AddHardwareDriver;
    p.driverClass = driverClass;
    p.tags = tags;

    return (OOP_Object *)OOP_DoMethod(obj, (OOP_Msg) &p);
}

#define HIDD_Mouse_RemHardwareDriver(obj, driverObject) \
    ({OOP_Object *__obj = obj; \
     HIDD_Mouse_RemHardwareDriver_(HiddMouseBase, __obj, driverObject); })

static inline void HIDD_Mouse_RemHardwareDriver_(OOP_MethodID MouseMethodBase, OOP_Object *obj, OOP_Object *driver)
{
    struct pHidd_Mouse_RemHardwareDriver p;

    p.mID = MouseMethodBase + moHidd_Mouse_RemHardwareDriver;
    p.driverObject = driver;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
#endif /* HIDD_MOUSE_H */
