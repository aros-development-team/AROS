#ifndef _EXEC_UTIL_H
#define _EXEC_UTIL_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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

/*
    Prototypes
*/
APTR AllocTaskMem (struct Task * task, ULONG size, ULONG flags);
void FreeTaskMem (struct Task * task, APTR mem);


#endif /* _EXEC_UTIL_H */
