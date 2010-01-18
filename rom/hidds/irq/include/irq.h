#ifndef HIDD_IRQ_H
#define HIDD_IRQ_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for the IRQ HIDD system.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include <utility/utility.h>

#define CLID_Hidd_IRQ       "hidd.bus.irq"
#define IID_Hidd_IRQ        "hidd.bus.irq"

/**** irq definitions ****************************************************/

enum
{
    /* Methods for a serial hidd */

    moHidd_IRQ_AddHandler = 0,       
    moHidd_IRQ_RemHandler,
    moHidd_IRQ_CauseIRQ,
    moHidd_IRQ_NumMethods
};

typedef struct {
    struct Node h_Node;     /* Handler's node */
    APTR        h_Data;     /* Handler data */
    VOID     (* h_Code)();  /* Handler code */
} HIDDT_IRQ_Handler;

typedef struct {
    struct ExecBase *sysBase;
    ULONG           Error;		/* Error code if defined */
} HIDDT_IRQ_HwInfo;

/* IRQ id's */
typedef enum {
    /*
	The ID's defined here are not real IRQ numbers! They are internal codes.
    */
    vHidd_IRQ_Timer = -14,    /* Timer IRQ, also known as VBlank IRQ */
    vHidd_IRQ_Keyboard,     /* Keyboard */
    vHidd_IRQ_Serial1,      /* Serial ports 1 and 3 */
    vHidd_IRQ_Serial2,      /* Serial ports 2 and 4 */
    vHidd_IRQ_Audio,        /* IRQ from audio card */
    vHidd_IRQ_Floppy,       /* Floppy drive */
    vHidd_IRQ_Parallel1,    /* Parallel port 1 */
    vHidd_IRQ_Parallel2,    /* Parallel port 2 */
    vHidd_IRQ_RTC,          /* Real Time Clock, 1024Hz interrupt */
    vHidd_IRQ_FPU,          /* FPU math error */
    vHidd_IRQ_HDD1,         /* IDE port 1 */
    vHidd_IRQ_HDD2,         /* IDE port 2 */
    vHidd_IRQ_Ether,        /* Ethernet card IRQ */
    vHidd_IRQ_Mouse,        /* PS/2 mouse IRQ */

//    vHidd_IRQ_NumIRQ        /* Number of IRQ's defined */
    
} HIDDT_IRQ_Id;

/* messages for IRQ hidd */

struct pHidd_IRQ_AddHandler
{
    OOP_MethodID        mID;
    HIDDT_IRQ_Handler   *handlerinfo;
    HIDDT_IRQ_Id        id;
};

struct pHidd_IRQ_RemHandler
{
    OOP_MethodID        mID;
    HIDDT_IRQ_Handler   *handlerinfo;
};

struct pHidd_IRQ_CauseIRQ
{
    OOP_MethodID        mID;
    HIDDT_IRQ_Id        id;
    HIDDT_IRQ_HwInfo    *hardwareinfo;
};

/* Predeclarations of stubs in libhiddirqstubs */

BOOL HIDD_IRQ_AddHandler    (OOP_Object *, HIDDT_IRQ_Handler *, HIDDT_IRQ_Id);
VOID HIDD_IRQ_RemHandler    (OOP_Object *, HIDDT_IRQ_Handler *);
VOID HIDD_IRQ_CauseIRQ      (OOP_Object *, HIDDT_IRQ_Id, HIDDT_IRQ_HwInfo *);

#endif /* HIDD_IRQ_H */

