/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"

extern APTR BSDSocket_FuncTable[];

static AROS_UFH2(LONG, TaskKeyCompare,
                 AROS_UFHA(const struct AVLNode *, td, A0),
                 AROS_UFHA(AVLKey, key, A1))
{
    AROS_USERFUNC_INIT

    AVLKey id = ((struct TaskNode *)td)->task;

    if (id == key)
	return 0;
    else if (id < key)
	return -1;
    else
	return 1;

    AROS_USERFUNC_EXIT
}

static AROS_UFH2(LONG, TaskNodeCompare,
		 AROS_UFHA(const struct AVLNode *, td1, A0),
		 AROS_UFHA(const struct AVLNode *, td2, A1))
{
    AROS_USERFUNC_INIT

    IPTR t1 = (IPTR)((struct TaskNode *)td1)->task;
    IPTR t2 = (IPTR)((struct TaskNode *)td2)->task;

    if (t1 == t2)
	return 0;
    else if (t1 < t2)
	return -1;
    else
	return 1;

    AROS_USERFUNC_EXIT
}

AROS_LH1(struct TaskBase *, BSDSocket_OpenLib,
	 AROS_LHA (ULONG, version, D0),
	 struct bsdsocketBase *, SocketBase, 1, BSDSocket)
{
    AROS_LIBFUNC_INIT

    struct TaskBase *tb;
    struct Task *task = FindTask(NULL);
    struct TaskNode *tn = (struct TaskNode *)AVL_FindNode(SocketBase->tasks, task, TaskKeyCompare);

    D(bug("[OpenLib] Task 0x%p\n", task));
    if (tn) {
	tb = tn->self;
	D(bug("[OpenLib] Found TaskBase 0x%p\n", tb));
    } else {
	APTR pool = CreatePool(MEMF_ANY, 2048, 1024);

	D(bug("[OpenLib] Created pool 0x%p\n", pool));
	if (!pool)
	    return NULL;

	tb = (struct TaskBase *)MakeLibrary(BSDSocket_FuncTable, NULL, NULL, sizeof(struct TaskBase), NULL);
	D(bug("[OpenLib] Created TaskBase 0x%p\n", tb));
	if (!tb)
	{
	    DeletePool(pool);
	    return NULL;
	}
	
	tb->lib.lib_Node.ln_Name = SocketBase->lib.lib_Node.ln_Name;
	tb->lib.lib_Node.ln_Type = NT_LIBRARY;
	tb->lib.lib_Node.ln_Pri  = SocketBase->lib.lib_Node.ln_Pri;
	tb->lib.lib_Flags = LIBF_CHANGED;
	tb->lib.lib_Version  = SocketBase->lib.lib_Version;
	tb->lib.lib_Revision = SocketBase->lib.lib_Revision;
	tb->lib.lib_IdString = SocketBase->lib.lib_IdString;

	SumLibrary(&tb->lib);

	tb->n.task = task;
	tb->n.self = tb;
	tb->glob = SocketBase;
	tb->pool = pool;
	tb->errnoPtr = &tb->errnoVal;
	tb->errnoSize = sizeof(tb->errnoVal);
	tb->sigintr = SIGBREAKF_CTRL_C;

	SetDTableSize(FD_SETSIZE, tb);

	AVL_AddNode(&SocketBase->tasks, &tb->n.node, TaskNodeCompare);
    }

    if (tb)
    {
	tb->lib.lib_OpenCnt++;
	SocketBase->lib.lib_OpenCnt++;
	D(bug("[socket] Task open count %u, global open count %u\n", tb->lib.lib_OpenCnt, SocketBase->lib.lib_OpenCnt));
    }

    return tb;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, BSDSocket_CloseLib,
	 struct TaskBase *, tb, 2, BSDSocket)
{
    AROS_LIBFUNC_INIT

    struct bsdsocketBase *SocketBase = tb->glob;

    D(bug("[CloseLib] TaskBase 0x%p\n", tb));

    tb->lib.lib_OpenCnt--;
    SocketBase->lib.lib_OpenCnt--;
    D(bug("[CloseLib] Task open count %u, global open count %u\n", tb->lib.lib_OpenCnt, SocketBase->lib.lib_OpenCnt));

    if (!tb->lib.lib_OpenCnt)
    {
	APTR addr;
	int i;

	D(bug("[CloseLib] dTableSize is %u\n", tb->dTableSize));
	for (i = 0; i < tb->dTableSize; i++)
	    IntCloseSocket(i, tb);

	AVL_RemNodeByAddress(&SocketBase->tasks, &tb->n.node);

	DeletePool(tb->pool);

	addr = (APTR)tb - tb->lib.lib_NegSize;
	FreeMem(addr, tb->lib.lib_NegSize + tb->lib.lib_PosSize);
    }

    if (SocketBase->lib.lib_OpenCnt)
	return NULL;

    if (SocketBase->lib.lib_Flags & LIBF_DELEXP)
	return AROS_LC1(BPTR, BSDSocket_ExpungeLib,
			AROS_LCA(struct bsdsocketBase *, SocketBase, D0),
			struct bsdsocketBase *, SocketBase, 3, BSDSocket);
    else
	return NULL;

    AROS_LIBFUNC_EXIT
}
