#ifndef HIDD_KEYBOARD_H
#define HIDD_KEYBOARD_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the keyboard hidd.
    Lang: English.
*/

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif


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


/* Parameter values for the IRQ handler */

enum {
   vHidd_Kbd_Press,
   vHidd_Kbd_Release
};


#define aHidd_Kbd_IrqHandler		(aoHidd_Kbd_IrqHandler     + HiddKbdAB)
#define aHidd_Kbd_IrqHandlerData	(aoHidd_Kbd_IrqHandlerData + HiddKbdAB)

#define IS_HIDDKBD_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddKbdAB, num_Hidd_Kbd_Attrs)

#endif /* HIDD_KEYBOARD_H */
