#ifndef _ETASK_H
#define _ETASK_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Internal description of the ETask structure
    Lang: english
*/
#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif

struct IntETask
{
    struct ETask iet_Task;
    APTR	 iet_RT;	/* Structure for resource tracking */
    APTR	 iet_Context;	/* Structure to store CPU registers */
    APTR	 iet_FPU;	/* Structure to store FPU data */
    APTR	 iet_CR3;	/* Reserved for VMM and PM */
};

#define GetIntETask(task) ((struct IntETask *)(((struct Task *) \
			    (task))->tc_UnionETask.tc_ETask))

#endif /* _ETASK_H */
