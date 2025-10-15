#ifndef HIDD_MOUSE_H
#define HIDD_MOUSE_H

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
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

#define CLID_Hidd_Mouse "hidd.input.mouse"
#define CLID_HW_Mouse   "hw.input.mouse"

#define IID_Hidd_Mouse "hidd.input.mouse"

#define HiddMouseAB __abHidd_Mouse

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddMouseAB;
#endif

/* Attrs */

enum {

   aoHidd_Mouse_State,
   aoHidd_Mouse_RelativeCoords,
   aoHidd_Mouse_Extended,

   num_Hidd_Mouse_Attrs
};

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

#endif /* HIDD_MOUSE_H */
