#ifndef HIDD_MOUSE_H
#define HIDD_MOUSE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the mouse hidd.
    Lang: English.
*/

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#define IID_Hidd_Mouse "hidd.mouse"

#define HiddMouseAB __abHidd_Mouse

extern OOP_AttrBase HiddMouseAB;

/* Attrs */

enum {

   aoHidd_Mouse_IrqHandler,
   aoHidd_Mouse_IrqHandlerData,
   aoHidd_Mouse_State,
   aoHidd_Mouse_RelativeCoords,
   
   num_Hidd_Mouse_Attrs
   
   
};


#define aHidd_Mouse_IrqHandler		(aoHidd_Mouse_IrqHandler     + HiddMouseAB)
#define aHidd_Mouse_IrqHandlerData	(aoHidd_Mouse_IrqHandlerData + HiddMouseAB)
#define aHidd_Mouse_State		(aoHidd_Mouse_State          + HiddMouseAB)  
#define aHidd_Mouse_RelativeCoords  	(aoHidd_Mouse_RelativeCoords + HiddMouseAB)

#define IS_HIDDMOUSE_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddMouseAB, num_Hidd_Mouse_Attrs)

/* Parameter values for the IRQ handler */

struct pHidd_Mouse_Event
{
    UWORD button;
    WORD x;
    WORD y;
    UWORD type; /* See below */
};

enum {
   vHidd_Mouse_Press,
   vHidd_Mouse_Release,
   vHidd_Mouse_Motion,
   vHidd_Mouse_WheelMotion
};

enum {
   vHidd_Mouse_NoButton,
   vHidd_Mouse_Button1,
   vHidd_Mouse_Button2,
   vHidd_Mouse_Button3
};

#endif /* HIDD_MOUSE_H */
