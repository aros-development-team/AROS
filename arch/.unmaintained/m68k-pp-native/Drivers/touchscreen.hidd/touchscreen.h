#ifndef _TOUCHSCREEN_H
#define _TOUCHSCREEN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the touchscreen HIDD.
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

#include <exec/types.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <hidd/irq.h>
#include <hidd/mouse.h>

/***** Touchscreen HIDD *******************/

/* IDs */
#define IID_Hidd_DBmouse	"hidd.touchscreen.db"
#define CLID_Hidd_DBmouse	"hidd.touchscreen.db"

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
    
    struct Library	*oopbase;
    struct Library	*utilitybase;
    struct ExecBase	*sysbase;

    OOP_AttrBase	hiddMouseAB;

    OOP_Class		*mouseclass;

    OOP_Object		*mousehidd;
};

/* Object data */

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR                           callbackdata;

    UWORD                          buttonstate;

    char                           *mouse_name;
    OOP_Object                     *irqhidd;
    HIDDT_IRQ_Handler              *irq;
    struct pHidd_Mouse_Event        me;
    UBYTE                           state; //see below
    UBYTE                           idlectr;
    UBYTE                           lastx;
    UBYTE                           lasty;
    struct Interrupt                VBlank;
};


enum {
	STATE_IDLE = 0,
	STATE_PEN_DOWN
};

/****************************************************************************************/


OOP_Class *_init_mouseclass  ( struct mouse_staticdata * );
VOID _free_mouseclass  ( struct mouse_staticdata * );

#define TSD(cl)         ((struct mouse_staticdata *)cl->UserData)

#define OOPBase         ((struct Library *)TSD(cl)->oopbase)
#define UtilityBase     ((struct Library *)TSD(cl)->utilitybase)
#define SysBase         (TSD(cl)->sysbase)


void touchscreen_int(HIDDT_IRQ_Handler * irq, HIDDT_IRQ_HwInfo *hw);

AROS_UFP4(ULONG, tsVBlank,
     AROS_UFPA(ULONG, dummy, A0),
     AROS_UFPA(void *, _data, A1),
     AROS_UFPA(ULONG, dummy2, A5),
     AROS_UFPA(struct ExecBase *, SysBase, A6));

#endif /* _TOUCHSCREEN_H */
