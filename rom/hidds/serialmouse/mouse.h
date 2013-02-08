#ifndef _MOUSE_H
#define _MOUSE_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the mouse native HIDD.
    Lang: English.
*/

#include <dos/bptr.h>
#include <exec/libraries.h>
#include <oop/oop.h>
#include <exec/semaphores.h>
#include <hidd/mouse.h>

/* defines for buttonstate */

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MIDDLE_BUTTON   4

/***** Mouse HIDD *******************/

struct mouse_staticdata
{
    OOP_AttrBase        hiddAttrBase;
    OOP_AttrBase        hiddMouseAB;
    OOP_MethodID        hwMethodBase;

    OOP_Class           *mouseclass;
    OOP_Object          *mousehidd;
    
    struct Library      *oopBase;
    struct Library      *utilityBase;
    BPTR                segList;
};

struct mousebase
{
    struct Library library;
    
    struct mouse_staticdata msd;
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

    OOP_Object                  *serial;
    OOP_Object                  *unit;
    struct Library              *shidd;
    UBYTE                       mouse_data[5];
    UBYTE                       mouse_collected_bytes;
    UBYTE                       mouse_protocol;
    UBYTE                       mouse_inth_state;

    struct pHidd_Mouse_Event    event;
    struct Ring                 *rx;    /* Ring structure for mouse init */
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

/****************************************************************************************/

#ifdef inb
#undef inb
#endif
static inline unsigned char inb(unsigned short port)
{
    unsigned char  _v;

    __asm__ __volatile__
    ("inb %w1,%0"
     : "=a" (_v)
     : "Nd" (port)
    );

    return _v;
}

#ifdef outb
#undef outb
#endif
static inline void outb(unsigned char value, unsigned short port)
{
    __asm__ __volatile__
    ("outb %b0,%w1"
     :
     : "a" (value), "Nd" (port)
    );
}

/****************************************************************************************/

#define MSD(cl)         (&((struct mousebase *)cl->UserData)->msd)

#undef HiddAttrBase
#undef HiddMouseAB
#undef HWBase
#define HiddAttrBase (MSD(cl)->hiddAttrBase)
#define HiddMouseAB  (MSD(cl)->hiddMouseAB)
#define HWBase       (MSD(cl)->hwMethodBase)

#define OOPBase     (MSD(cl)->oopBase)
#define UtilityBase (MSD(cl)->utilityBase)

#endif /* _MOUSE_H */
