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

/*
 * This macro resets crash status of the task:
 * - iet_AlertCode serves as an actual indicator of crash status.
 *   If we enter Alert() with iet_AlertCode already set, this is
 *   considered a nested alert and is directed to supervisor-mode routine.
 * - iet_AlertType specifies type of alert context (if any). We make sure
 *   it is clear so as next time Alert() will not display old information.
 * - iet_AlertLocation can also be set only once. It is either set explicitly
 *   before calling Alert(), or it is set by Alert() routine itself. So we clear
 *   it in order for Alert() to be able to remember it if task ever alerts again.
 * - iet_AlertStack is always used in pair with iet_AlertLocation, so there's no
 *   need to clear it.
 */
#define ResetETask(etask)		\
    (etask)->iet_AlertCode = 0;		\
    (etask)->iet_AlertType = AT_NONE;	\
    (etask)->iet_AlertLocation = NULL;

#endif /* _ETASK_H */
