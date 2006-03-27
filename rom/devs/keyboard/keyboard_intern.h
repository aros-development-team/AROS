#ifndef  KEYBOARD_INTERN_H
#define  KEYBOARD_INTERN_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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
    struct ExecBase   *kb_sysBase;
    struct Library    *kb_LowLevelBase;

    APTR               kb_seglist;
    
    struct MinList          kb_PendingQueue;  /* IOrequests (KBD_READEVENT) not done quick */
    struct SignalSemaphore  kb_QueueLock;
    struct MinList	    kb_ResetHandlerList;

    struct Interrupt  kb_Interrupt;     /* Interrupt to invoke in case of keypress (or
					   releases) and there are pending requests */
    APTR    kb_kbIrqHandle;             /* Handle from AddKBInt() */

    UWORD   kb_nHandlers;      		/* Number of reset handlers added */
    UWORD  *kb_keyBuffer;
    UWORD   kb_writePos;
    BOOL    kb_ResetPhase;	        /* True if reset has begun */
    UBYTE  *kb_Matrix;
    
    OOP_Object	   *kb_Hidd;	        /* Hidd object to use */
    struct Library *kb_OOPBase;
    
    OOP_AttrBase    HiddKbdAB_;
};


typedef struct KBUnit
{
    UWORD  kbu_readPos;		/* Position in the key buffer */
    UWORD  kbu_Qualifiers;      /* Known qualifiers at this moment */
    UWORD  kbu_LastCode;	/* Previous rawkey code */
    UBYTE  kbu_LastQuals;	/* Lower half of previous qualifiers */
    UWORD  kbu_LastLastCode;	/* The rawkey code two keys ago... */
    UBYTE  kbu_LastLastQuals;	/* ...and the lower half of qualifiers
				   associated with that code */

    /* Note: The xxxQuals is UBYTE for now. They may be UWORDS at a later
             implementation but as we want to be binary compatible,
	     we just store UBYTEs in the InputEvent. This goes for
	     deadkey handling too, but as it is a fact that keycodes will
	     be UWORDS here in the future, it's so now. Regarding the
	     qualifiers, it may be enough with 8. */

    UBYTE  kbu_flags;           /* For unit flags definitions, see below */
} KBUnit;

#define KBUB_PENDING 0		/* Unit has pending request for keyevents */

#define KBUF_PENDING 0x01

#define HiddKbdAB KBBase->HiddKbdAB_

#endif /* KEYBOARD_INTERN_H */

