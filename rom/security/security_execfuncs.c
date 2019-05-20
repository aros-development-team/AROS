/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>

#include <libraries/security.h>
#include <libraries/mufs.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_monitor.h"
#include "security_server.h"

static void AddTaskNode(struct SecurityBase *secBase, struct secTaskNode *node, struct secTaskNode *parent);
static void RemTaskNode(struct SecurityBase *secBase, struct secTaskNode *node);

/*
 *      Add a Task Node to a Session
 *
 *      Make sure you have access to the list (via ObtainSemaphore(Shared))!!
 */

static void AddTaskNode(struct SecurityBase *secBase, struct secTaskNode *node, struct secTaskNode *parent)
{
    if (parent)
    {
        if (parent->Session)
            AddHead((struct List *)&parent->Session->SessionMembers, (struct Node *)&node->SessionNode);
        AddHead((struct List *)&parent->Children, (struct Node *)&node->Siblings);
        node->Session = parent->Session;
        parent->ChildrenCount++;
    }
    AddHead((struct List *)&secBase->TaskOwnerList[(IPTR)node->Task%TASKHASHVALUE],
                      (struct Node *)&node->ListNode);
    node->Parent = parent;
    CallMonitors(secBase, secTrgB_OwnerChange, secNOBODY_UID, (node->Owner? node->Owner->uid:secNOBODY_UID), 0);
}

/*
 *      Remove a Task Node from a Task Level
 *
 *      Make sure you have access to the list (via ObtainSemaphore(Shared))!!
 */

static void RemTaskNode(struct SecurityBase *secBase, struct secTaskNode *node)
{
    if (node->Session)
        Remove((struct Node *)&node->SessionNode);
    Remove((struct Node *)&node->ListNode);
    CallMonitors(secBase, secTrgB_OwnerChange, (node->Owner? node->Owner->uid:secNOBODY_UID), secNOBODY_UID, 0);
}

/*
 *      Replacement for the exec.library AddTask() function
 */
AROS_LH3(APTR, SecurityAddTask,
	AROS_LHA(struct Task *,     task,      A1),
	AROS_LHA(APTR,              initialPC, A2),
	AROS_LHA(APTR,              finalPC,   A3),
	struct ExecBase *, SysBase, 47, Exec)
{
    AROS_LIBFUNC_INIT

    struct Task *server = FindTask(SERVERNAME);
    struct SecurityBase *secBase = server->tc_UserData;
    struct secTaskNode *node;
    struct secTaskNode *parent;

    D(bug("[security.library] %s()\n", __func__));
    D(bug("[security.library] secBase @ %p (server @ %p)\n", secBase, server));

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((parent = FindTaskNode(secBase, FindTask(NULL))))
    {
        if ( (node = AllocTaskNode(task, parent->DefProtection, parent)) )
        {
            AddTaskNode(secBase, node, parent);
        }
    }
    else
        CreateOrphanTask(secBase, task, DEFPROTECTION);
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    return(secBase->OLDAddTask(task, initialPC, finalPC, SysBase));

    AROS_LIBFUNC_EXIT
}


	/*
	 *		Replacement for the exec.library RemTask() function
	 */
AROS_LH1(void, SecurityRemTask,
        AROS_LHA(struct Task *, task, A1),
        struct ExecBase *, SysBase, 48, Exec)
{
    AROS_LIBFUNC_INIT

    struct Task *server = FindTask(SERVERNAME);
    struct SecurityBase *secBase = server->tc_UserData;
    struct secTaskNode *node;

    D(bug("[security.library] Task %p is entering RemTask\n", FindTask(NULL)));
    D(bug("[security.library] secBase @ %p (server @ %p)\n", secBase, server));

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindTaskNode(secBase, task? task : FindTask(NULL))) != NULL)
    {
        RemTaskNode(secBase, node);
        /* Wait for children to quit */
        if (node->ChildrenCount)
        {
            D(bug("[security.library] Adding task %lx to zombie list\n", task? task:FindTask(NULL)));
            AddHead((struct List *)&secBase->Zombies, (struct Node *)&node->SessionNode);
            /* Home-made polling. One could do something nicer I guess... */
            do
            {
                ReleaseSemaphore(&secBase->TaskOwnerSem);
                Delay(5);
                ObtainSemaphore(&secBase->TaskOwnerSem);
            }
            while (node->ChildrenCount);
            D(bug("[security.library] Removing task %lx from zombie list\n", task? task:FindTask(NULL)));
            Remove((struct Node *)&node->SessionNode);
        }
        if (node->Parent)
        {
            Remove((struct Node *)&node->Siblings);
            node->Parent->ChildrenCount--;
        }
        FreeTaskNode(node);
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);
    secBase->OLDRemTask(task, SysBase);

    AROS_LIBFUNC_EXIT
}
