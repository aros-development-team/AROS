#ifndef _MOUSE_H
#define _MOUSE_H

/*
    (C) 2000 AROS - The Amiga Research OS
    $Id: 

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

/***** PCI bus HIDD *******************/

/* IDs */
#define IID_Hidd_PCmouse	"hidd.bus.mouse"
#define CLID_Hidd_PCmouse	"hidd.bus.mouse"

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

/* 256 byte long ring buffer used to read data with timeout defined */

#define RingSize 256

struct Ring
{
    char ring[RingSize];
    int top, ptr;
};

/* Object data */

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
	
    UWORD buttonstate;

    char *mouse_name;

/* Driver specific data */

    union
    {
        struct
        {
            OOP_Object	*usbhidd;
        } usb;
        struct
        {
            OOP_Object	*irqhidd;
        } ps2;
        struct
        {
            OOP_Object	*serial;
            OOP_Object	*unit;

            struct Ring *rx;    /* Ring structure for mouse init */
        } com;
    } u;
};

OOP_Class *_init_mouseclass  ( struct mouse_staticdata * );
VOID _free_mouseclass  ( struct mouse_staticdata * );

#define MSD(cl)         ((struct mouse_staticdata *)cl->UserData)

#define OOPBase         ((struct Library *)MSD(cl)->oopbase)
#define UtilityBase     ((struct Library *)MSD(cl)->utilitybase)
#define SysBase         (MSD(cl)->sysbase)

#endif /* _MOUSE_H */
