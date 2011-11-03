#ifndef _MOUSE_H
#define _MOUSE_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the mouse native HIDD.
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

#ifndef EXEC_INTERRUPTS_H
#   include <exec/interrupts.h>
#endif

#include <dos/bptr.h>

#include <hidd/mouse.h>

/* defines for buttonstate */

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MIDDLE_BUTTON   4

/***** Mouse HIDD *******************/

/* IDs */
#define IID_Hidd_Amigamouse "hidd.amiga.mouse"
#define CLID_Hidd_Amigamouse "hidd.amiga.mouse"

/* Methods */
enum
{
    moHidd_Mouse_HandleEvent
};
    
struct pHidd_Mouse_HandleEvent
{
    OOP_MethodID mID;
    ULONG event;
};
	    
VOID Hidd_Mouse_HandleEvent(OOP_Object *o, ULONG event);
	    
/* misc */

struct mouse_staticdata
{
    struct SignalSemaphore      sema; /* Protexting this whole struct */
	struct Interrupt mouseint;
	struct PotgoBase *potgo;
	UWORD potgobits;
     
    OOP_AttrBase	hiddMouseAB;

    OOP_Class		*mouseclass;

    OOP_Object		*mousehidd;

    OOP_MethodID         hiddMouseBase;

    struct Library *cs_SysBase;
    struct Library *cs_OOPBase;
    BPTR cs_SegList;
};

struct mousebase
{
    struct Library library;
    
    struct mouse_staticdata msd;
};

/* Object data */

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
    struct pHidd_Mouse_Event event;
    UWORD buttons;
    UWORD joydat;
    UBYTE port;
};

#define MSD(cl)         (&((struct mousebase *)cl->UserData)->msd)

#endif /* _MOUSE_H */

