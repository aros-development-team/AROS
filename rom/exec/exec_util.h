#ifndef _EXEC_UTIL_H
#define _EXEC_UTIL_H

/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Utility functions for exec.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef EXEC_TASKS_H
struct Task;
#endif
#ifndef EXEC_LISTS_H
struct List;
#endif
#ifndef ETASK_H
struct IntETask;
#endif

/*
    Prototypes
*/
APTR AllocTaskMem (struct Task * task, ULONG size, ULONG flags);
void FreeTaskMem (struct Task * task, APTR mem);

struct Task *FindTaskByID(ULONG id);
struct IntETask *FindETask(struct List *, ULONG id);

#endif /* _EXEC_UTIL_H */
