#ifndef HIDD_KEYBOARD_H
#define HIDD_KEYBOARD_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the keyboard hidd.
    Lang: English.
*/

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
    STRPTR	    driverId;
    struct TagItem  *tags;
};

struct pHidd_Kbd_RemHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Object	    *driverObject;
};

OOP_Object *HIDD_Kbd_AddHardwareDriver(OOP_Object *obj, OOP_Class *driverClass, STRPTR driverId, struct TagItem *tags);
void HIDD_Kbd_RemHardwareDriver(OOP_Object *obj, OOP_Object *driver);

#endif /* HIDD_KEYBOARD_H */
