/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: A simpler mutual exclusion device than semaphores.
	  With added complexity...
*/

#ifndef AROS_MUTEX_H
#define AROS_MUTEX_H

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef EXEC_TASKS_H
#include <exec/tasks.h>
#endif

/*
 *  A simple Mutex, with a list of tasks waiting on it.
 */

struct Mutex
{
    struct Task	    *m_Locker;
    struct MinList  m_Waiters;
};

/*
 *  A waiting task adds a MutexRequest onto the m_Waiters list.
 */
struct MutexWaiter
{
    struct MinNode  m_Node;
    struct Task	    *m_Waiter;
    
    unsigned long   m_Condition;
    BOOL	    m_IsCondition;
    ULONG	    m_MilliToWait;
};

#endif /* AROS_MUTEX_H */
