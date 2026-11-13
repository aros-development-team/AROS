/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_server.h"
#include "security_segment.h"
#include "security_monitor.h"
#include "security_memory.h"
#include "security_plugins.h"
#include "security_crypto.h"
#include "security_enforce.h"
#include "security_packetio.h"
#include "security_userinfo.h"
#include "security_groupinfo.h"
#include "security_login.h"
#include "security_support.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, secEnumChildren,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, parent, A0),
        AROS_LHA(struct Task **, children, A1),
        AROS_LHA(LONG, size, D0),

/*  LOCATION */
        struct SecurityBase *, secBase, 36, Security)

/*
    FUNCTION
        Enumerate the children of a task.

    INPUTS
        parent   - the task, NULL for the current one.
        children - array to fill (may be NULL to query the count).
        size     - number of entries in the array.

    RESULT
        The number of children, negative if the array was too small (the
        absolute value is the required size).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTaskNode *node;
    struct MinNode *n;
    LONG result = 0;
    int i = 0;

    ObtainSemaphoreShared(&secBase->TaskOwnerSem);
    if ((node = FindTaskNode(secBase, parent ? parent : FindTask(NULL))) && node->ChildrenCount)
    {
        if ((node->ChildrenCount > size) || (children == NULL))
            result = -(LONG)node->ChildrenCount;
        else
        {
            result = node->ChildrenCount;
            ForeachNode(&node->Children, n)
            {
                if (i >= result)
                    break;
                children[i++] = TASKNODE_FROM_SIBLINGS(n)->Task;
            }
        }
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    return result;

    AROS_LIBFUNC_EXIT
} /* secEnumChildren */
