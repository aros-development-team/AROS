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

/* 488 byte long ring buffer used to read data with timeout defined */

#define RingSize 488

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

/* Mouse types */
#define P_MS            0               /* Microsoft */
#define P_MSC           1               /* Mouse Systems Corp */
#define P_MM            2               /* MMseries */
#define P_LOGI          3               /* Logitech */
#define P_BM            4               /* BusMouse ??? */
#define P_LOGIMAN       5               /* MouseMan / TrackMan */
#define P_PS2           6               /* PS/2 mouse */
#define P_MMHIT         7               /* MM_HitTab */
#define P_GLIDEPOINT    8               /* ALPS serial GlidePoint */
#define P_IMSERIAL      9               /* Microsoft serial IntelliMouse */
#define P_THINKING      10              /* Kensington serial ThinkingMouse */
#define P_IMPS2         11              /* Microsoft PS/2 IntelliMouse */
#define P_THINKINGPS2   12              /* Kensington PS/2 ThinkingMouse */
#define P_MMANPLUSPS2   13              /* Logitech PS/2 MouseMan+ */
#define P_GLIDEPOINTPS2 14              /* ALPS PS/2 GlidePoint */
#define P_NETPS2        15              /* Genius PS/2 NetMouse */
#define P_NETSCROLLPS2  16              /* Genius PS/2 NetScroll */
#define P_SYSMOUSE      17              /* SysMouse */
#define P_AUTO          18              /* automatic */
#define P_ACECAD        19              /* ACECAD protocol */

OOP_Class *_init_mouseclass  ( struct mouse_staticdata * );
VOID _free_mouseclass  ( struct mouse_staticdata * );

#define MSD(cl)         ((struct mouse_staticdata *)cl->UserData)

#define OOPBase         ((struct Library *)MSD(cl)->oopbase)
#define UtilityBase     ((struct Library *)MSD(cl)->utilitybase)
#define SysBase         (MSD(cl)->sysbase)

#endif /* _MOUSE_H */
