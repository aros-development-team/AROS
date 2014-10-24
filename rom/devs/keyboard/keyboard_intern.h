#ifndef  KEYBOARD_INTERN_H
#define  KEYBOARD_INTERN_H

/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/devices.h>

#include <oop/oop.h>

#define KB_MAXKEYS     256
#define KB_MATRIXSIZE  (KB_MAXKEYS/(sizeof(UBYTE)*8))

#define KB_BUFFERSIZE  128

struct KeyboardBase
{
    struct Device      kb_device;
    struct Library    *kb_KbdHiddBase;

    struct MinList          kb_PendingQueue;  /* IOrequests (KBD_READEVENT) not done quick */
    struct SignalSemaphore  kb_QueueLock;
    struct MinList	    kb_ResetHandlerList;

    struct Interrupt  kb_Interrupt;     /* Interrupt to invoke in case of keypress (or
					   releases) and there are pending requests */

    UWORD   kb_nHandlers;      		/* Number of reset handlers added */
    UWORD  *kb_keyBuffer;
    UWORD   kb_writePos;
    BOOL    kb_ResetPhase;	        /* True if reset has begun */
    UBYTE  *kb_Matrix;
    
    OOP_Object	   *kb_Hidd;	        /* Hidd object to use */
    struct Library *kb_OOPBase;
    
    OOP_AttrBase    HiddKbdAB_;

    /* FIXME:
     * m68k lowlevel.library stores only io_Device field after keyboard.device
     * has been opened and when it is time to close it, lowlevel reuses old
     * iorequest (which was used for other purposes originally), sets io_Device
     * and calls CloseDevice(). Which means we can't assume io_Unit is valid.
     * we leak memory if above happens but it is much better than freeing
     * random memory
     */
    struct MinList kb_kbunits;
};


typedef struct KBUnit
{
	struct MinNode node;
    UWORD  kbu_readPos;		/* Position in the key buffer */
    UWORD  kbu_Qualifiers;      /* Known qualifiers at this moment */
    UBYTE  kbu_flags;           /* For unit flags definitions, see below */
} KBUnit;

#define KBUB_PENDING 0		/* Unit has pending request for keyevents */

#define KBUF_PENDING 0x01

#undef HiddKbdAB
#define HiddKbdAB KBBase->HiddKbdAB_

#endif /* KEYBOARD_INTERN_H */

