#ifndef EXEC_SEMAPHORES_H
#define EXEC_SEMAPHORES_H

/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Semaphore handling
    Lang: english
*/

#ifndef EXEC_LISTS_H
#    include <exec/lists.h>
#endif

#ifndef EXEC_NODES_H
#    include <exec/nodes.h>
#endif

#ifndef EXEC_PORTS_H
#    include <exec/ports.h>
#endif

#ifndef EXEC_TASKS_H
#    include <exec/tasks.h>
#endif


                           /* Signal Semaphores */

/* Private structure for use in ObtainSemaphore */
struct SemaphoreRequest
{
    struct MinNode sr_Link;
    struct Task  * sr_Waiter;
};

struct SignalSemaphore
{
    struct Node             ss_Link;
    WORD                    ss_NestCount;
    struct MinList          ss_WaitQueue;
    struct SemaphoreRequest ss_MultipleLink;
    struct Task           * ss_Owner;
    WORD                    ss_QueueCount;
};

/* For use in Procure()/Vacate() */
struct SemaphoreMessage
{
    struct Message           ssm_Message;
    struct SignalSemaphore * ssm_Semaphore;
};

#define SM_EXCLUSIVE (0L)
#define SM_SHARED    (1L)

#endif /* EXEC_SEMAPHORES_H */
