#ifndef HIDD_KBD_H
#define HIDD_KBD_H

/*
    (C) 1999 AROS - The Amiga Research OS
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
    MethodID mID;
    ULONG event;
};

VOID Hidd_Kbd_HandleEvent(Object *o, ULONG event);

/* misc */

struct abdescr
{
    STRPTR interfaceid;
    AttrBase *attrbase;
};

struct kbd_staticdata
{
    struct SignalSemaphore sema; /* Protexting this whole struct */

    struct Library *oopbase;
    struct Library *utilitybase;
    struct ExecBase *sysbase;

    Class *kbdclass;
    
    Object *kbdhidd;
};

BOOL obtainattrbases(struct abdescr *abd, struct Library *OOPBase);
VOID releaseattrbases(struct abdescr *abd, struct Library *OOPBase);

Class *init_kbdclass  ( struct kbd_staticdata * );

VOID free_kbdclass  ( struct kbd_staticdata * );

#define XSD(cl) ((struct kbd_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)XSD(cl)->oopbase)
#define UtilityBase	((struct Library *)XSD(cl)->utilitybase)
#define SysBase		(XSD(cl)->sysbase)

#endif /* HIDD_KBD_H */
