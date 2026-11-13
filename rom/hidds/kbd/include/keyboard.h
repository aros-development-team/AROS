#ifndef HIDD_KEYBOARD_H
#define HIDD_KEYBOARD_H

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the keyboard hidd.
    Lang: English.
*/

#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#define CLID_Hidd_Kbd "hidd.input.kbd"
#define CLID_HW_Kbd "hw.input.kbd"
#define IID_Hidd_Kbd "hidd.input.kbd"

#define HiddKbdAB __abHidd_Kbd

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddKbdAB;
#endif

/* Parameter values for the IRQ handler */

struct pHidd_Kbd_Event
{
    union {
        struct {
            UWORD flags;
            UWORD code;
        };
        ULONG kbdevt;
    };
};

/*
 * Keyboard Event Flags ...
 */

// Qualifier keys sent with KEYTOGGLE set, set their state based on the keys UP/DOWN state.
#define KBD_KEYTOGGLE (1 << 7)

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

#endif /* HIDD_KEYBOARD_H */
