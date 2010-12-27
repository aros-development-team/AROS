#ifndef _ETASK_H
#define _ETASK_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal description of the ETask structure
    Lang: english
*/

#include <exec/interrupts.h>
#include <exec/tasks.h>

/* Known alert context types */
#define AT_NONE 0x00
#define AT_CPU  0x01

/* Alert data. Can have different contents, depending on what actually happened */
struct AlertContext
{
    union
    {
	struct ExceptionContext acpu;
    } u;
};

struct IntETask
{
    struct ETask	iet_ETask;
#ifdef DEBUG_ETASK
    STRPTR	 	iet_Me;
#endif
    APTR	 	iet_RT;			/* Structure for resource tracking	 */
    APTR	 	iet_Context;		/* Structure to store CPU registers	 */
    APTR         	iet_acpd;      		/* Structure to store shared clib's data */
    APTR	 	iet_startup;   		/* Structure to store startup code stuff */
    UQUAD	 	iet_CpuTime;
    UQUAD	 	iet_private1;
    ULONG	 	iet_AlertCode;		/* Alert code for crash handler		 */
    UBYTE		iet_AlertType;		/* Type of the alert context		 */
    APTR	 	iet_AlertLocation;	/* Alert location for crash handler	 */
    APTR		iet_AlertStack;		/* Frame pointer for stack backtrace	 */
    struct AlertContext iet_AlertData;		/* Extra data coming with the crash	 */
};

#define GetIntETask(task)   ((struct IntETask *)(((struct Task *) \
				(task))->tc_UnionETask.tc_ETask))
#define IntETask(etask)	    ((struct IntETask *)(etask))

#endif /* _ETASK_H */
