/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifndef  GAMEPORT_INTERN_H
#define  GAMEPORT_INTERN_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/devices.h>


/* Must always be a multiple of 3 since one event consists of code, x and y */

#define  GP_NUMELEMENTS 	(100 * 3)
#define  GP_BUFFERSIZE  	(sizeof (WORD) * GP_NUMELEMENTS) 

#define  GP_NUNITS        	2 /* Number of units supported by gameport.device */

#define  GP_MAXUNIT 		1 /* Highest possible gameport unit */

struct GameportBase
{
    struct Device      		gp_device;
    struct ExecBase   		*gp_sysBase;
    struct Library    		*gp_LowLevelBase;

    BPTR               		gp_seglist;
    
    struct MinList          	gp_PendingQueue; 	/* IOrequests (GPD_READEVENT)
						    	   not done quick */
    struct SignalSemaphore  	gp_QueueLock;
    struct SignalSemaphore  	gp_Lock;

    struct Interrupt  		gp_Interrupt;     	/* Interrupt to invoke in case of
					   		   keypress (or releases) and there
					   		   are pending requests */
    struct Interrupt  		gp_VBlank;        	/* Gameport VBlank server */

    WORD  			*gp_eventBuffer;
    UWORD   			gp_writePos;

    ULONG   			gp_nTicks;          	/* Bookkeeping of frames */
    
    OOP_Object	   		*gp_Hidd;		/* Hidd object to use */
    struct Library 		*gp_OOPBase;
    
    UBYTE   			gp_cTypes[GP_NUNITS];

    BOOL    	    	    	gp_RelativeMouse;
    OOP_AttrBase                HiddMouseAB_;
};

#define HiddMouseAB (GPBase->HiddMouseAB_)

typedef struct GPUnit
{
    UWORD  			gpu_readPos;		/* Position in the key buffer */
    UWORD  			gpu_Qualifiers;      	/* Known qualifiers at this moment */

    UWORD  			gpu_unitNum;
    UBYTE  			gpu_flags;           	/* For unit flags definitions, see below */

    WORD  			gpu_lastX;
    WORD  			gpu_lastY;

    struct GamePortTrigger 	gpu_trigger;
} GPUnit;

#define GBUB_PENDING 		0			/* Unit has pending request for gameport
				   			   events */
#define GBUF_PENDING 		0x01


#define expunge() \
AROS_LC0(BPTR, expunge, struct GameportBase *, GPBase, 3, Gameport)


#ifdef SysBase
#undef SysBase
#endif
#define SysBase GPBase->gp_sysBase

#ifdef OOPBase
#undef OOPBase
#endif
#define OOPBase GPBase->gp_OOPBase

#endif /* GAMEPORT_INTERN_H */
