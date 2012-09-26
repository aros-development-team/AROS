#ifndef LINUXMOUSE_INTERN_H
#define LINUXMOUSE_INTERN_H

/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux /dev/input/eventX hidd for AROS
    Lang: English.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

extern OOP_AttrBase HiddKbdAB;
extern OOP_AttrBase HiddMouseAB;

/***** Linux Kbd HIDD *******************/

/* IDs */
#define IID_Hidd_LinuxKbd   "hidd.kbd.linux"
#define CLID_Hidd_LinuxKbd  "hidd.kbd.linux"

/* Methods */
enum
{
    moHidd_LinuxKbd_HandleEvent
};

struct pHidd_LinuxKbd_HandleEvent
{
    OOP_MethodID mID;
    UBYTE scanCode;
};

VOID HIDD_LinuxKbd_HandleEvent(OOP_Object *o, UBYTE scanCode);

/* Data */
struct LinuxKbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;
};

/***** Linux Mouse HIDD *******************/

/* IDs */
#define IID_Hidd_LinuxMouse     "hidd.mouse.linux"
#define CLID_Hidd_LinuxMouse    "hidd.mouse.linux"


/* Methods */
enum
{
    moHidd_LinuxMouse_HandleEvent
};

struct pHidd_LinuxMouse_HandleEvent
{
    OOP_MethodID mID;
    struct pHidd_Mouse_Event *mouseEvent;
};

VOID HIDD_LinuxMouse_HandleEvent(OOP_Object *o, struct pHidd_Mouse_Event *mouseEvent);

/* Data */
struct LinuxMouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};

/*** Shared data ***/
#define CAP_NONE        (0)
#define CAP_KEYBOARD    (1<<0)
#define CAP_MOUSE       (1<<1)

struct EventHandler
{
    struct Node     node;
    int             eventdev;
    ULONG           capabilities;
    struct Task    *inputtask;
    OOP_Object      *mousehidd;
    OOP_Object      *kbdhidd;
    OOP_Object      *unixio;
};

struct LinuxInput_staticdata
{
    struct SignalSemaphore sema;

    struct List eventhandlers;

    OOP_Class *mouseclass;
    OOP_Object *mousehidd;
    OOP_Class *kbdclass;
    OOP_Object *kbdhidd;
    OOP_Object *unixio;
};

struct LinuxInput_base
{
    struct Library library;
    struct LinuxInput_staticdata lsd;
};

#define LSD(cl) (&((struct LinuxInput_base *)cl->UserData)->lsd)

VOID Update_EventHandlers(struct LinuxInput_staticdata *lsd);

#endif /* LINUXMOUSE_INTERN_H */
