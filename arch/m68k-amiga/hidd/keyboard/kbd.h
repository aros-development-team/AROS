#ifndef HIDD_KBD_H
#define HIDD_KBD_H

/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the kbd HIDD.
    Lang: English.
*/

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#ifndef EXEC_INTRERRUPTS_H
#   include <exec/interrupts.h>
#endif

#include <dos/bptr.h>

/****************************************************************************************/

/***** Kbd HIDD *******************/

/* IDs */
#define IID_Hidd_HwKbd		"hidd.kbd.hw"
#define CLID_Hidd_HwKbd		"hidd.kbd.hw"

/* Methods */
enum
{
    moHidd_Kbd_HandleEvent
};

struct pHidd_Kbd_HandleEvent
{
    OOP_MethodID 		mID;
    ULONG 			event;
};

/* misc */

struct abdescr
{
    STRPTR 			interfaceid;
    OOP_AttrBase 		*attrbase;
};

struct kbd_staticdata
{
    struct SignalSemaphore sema; /* Protexting this whole struct */
    struct Interrupt kbint;
    struct Resource *ciares;
    struct timerequest *timerio;
    struct Library *TimerBase;

    OOP_Class *kbdclass;
    OOP_Object *kbdhidd;

    OOP_AttrBase hiddAB;
    OOP_AttrBase hiddKbdAB;

    OOP_MethodID hiddKbdBase;

    BPTR                cs_SegList;
    struct Library     *cs_OOPBase;
};

struct kbdbase
{
    struct Library library;
    struct kbd_staticdata ksd;
};

struct kbd_data
{
    VOID    (*kbd_callback)(APTR, UWORD);
    APTR    callbackdata;
    struct Library *TimerBase;
    UBYTE resetstate;
};

/****************************************************************************************/

BOOL obtainattrbases(struct abdescr *abd, struct Library *OOPBase);
VOID releaseattrbases(struct abdescr *abd, struct Library *OOPBase);

/****************************************************************************************/

#define XSD(cl) 	(&((struct kbdbase *)cl->UserData)->ksd)

#undef HiddAttrBase
#define HiddAttrBase	(XSD(cl)->hiddAB)

#undef HiddKbdAB
#define HiddKbdAB	(XSD(cl)->hiddKbdAB)

#undef HiddKbdBase
#define HiddKbdBase	(XSD(cl)->hiddKbdBase)

#define OOPBase		(XSD(cl)->cs_OOPBase)

/****************************************************************************************/

#endif /* HIDD_KBD_H */
