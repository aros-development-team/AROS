#ifndef _ETASK_H
#define _ETASK_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal description of the ETask structure
    Lang: english
*/

#include <exec/interrupts.h>
#include <exec/tasks.h>

#include "mmextra.h"
#include "mungwallextra.h"

/* Known alert context types */
#define AT_NONE     0x00
#define AT_CPU      0x01
#define AT_MUNGWALL 0x02
#define AT_MEMORY   0x03

/* Alert data. Can have different contents, depending on what actually happened */
struct AlertContext
{
    union
    {
	struct ExceptionContext acpu;
	struct MungwallContext	amw;
	struct MMContext	amm;
    } u;
};

struct IntETask
{
    struct ETask	iet_ETask;
    APTR	 	iet_RT;			/* Structure for resource tracking	 */
    UQUAD	 	iet_CpuTime;
    UQUAD	 	iet_private1;
    ULONG	 	iet_AlertCode;		/* Alert code for crash handler		 */
    UBYTE		iet_AlertType;		/* Type of the alert context		 */
    UBYTE		iet_AlertFlags;		/* See below				 */
    APTR	 	iet_AlertLocation;	/* Alert location for crash handler	 */
    APTR		iet_AlertStack;		/* Frame pointer for stack backtrace	 */
    struct AlertContext iet_AlertData;		/* Extra data coming with the crash	 */
#ifdef DEBUG_ETASK
    STRPTR	 	iet_Me;
#endif
};

#define GetIntETask(task)   ((struct IntETask *)(((struct Task *) \
				(task))->tc_UnionETask.tc_ETask))
#define IntETask(etask)	    ((struct IntETask *)(etask))

/* iet_AlertFlags */
#define AF_Alert    0x01	/* The task is in alert state	  */
#define AF_Location 0x02	/* iet_AlertLocation is filled in */

/*
 * This function resets crash status of the task:
 * - AF_Alert flag serves as an actual indicator of crash status.
 *   If we enter Alert() with this flag already set, this is
 *   considered a nested alert and is directed to supervisor-mode routine.
 * - AF_Location flag can also be set only once. It is either set explicitly
 *   before calling Alert(), or it is set by Alert() routine itself. So we clear
 *   it in order for Alert() to be able to remember it if task ever alerts again.
 * - iet_AlertType specifies type of alert context (if any). We make sure
 *   it is clear so as next time Alert() will not display old information.
 */
static inline void ResetETask(struct IntETask *etask)
{
    etask->iet_AlertType  = AT_NONE;
    etask->iet_AlertFlags = 0;
}

#endif /* _ETASK_H */
