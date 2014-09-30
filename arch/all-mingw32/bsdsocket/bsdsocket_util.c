/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <sys/errno.h>
#include <string.h>

#include "bsdsocket_intern.h"
#include "bsdsocket_util.h"

void SetError(int error, struct TaskBase *libPtr)
{
    switch (libPtr->errnoSize)
    {
    case 8:
	*(UQUAD *)libPtr->errnoPtr = (UQUAD)error;
	break;

    case 4:
	*(ULONG *)libPtr->errnoPtr = (ULONG)error;
	break;

    case 2:
	*(UWORD *)libPtr->errnoPtr = (UWORD)error;
	break;

    case 1:
	*(UBYTE *)libPtr->errnoPtr = (UBYTE)error;
	break;

    default:
	D(bug("[SetErrno] Bogus errno size %u for TaskBase 0x%p\n", libPtr->errnoSize, libPtr));
	break;
    }
}

ULONG SetDTableSize(ULONG size, struct TaskBase *taskBase)
{
    struct Socket **old = taskBase->dTable;
    struct Socket **table;
    ULONG oldsize = taskBase->dTableSize * sizeof(struct Socket *);
    ULONG newsize = size * sizeof(struct Socket *);
    

    /* FIXME: just a temporary measure */
    if (size < taskBase->dTableSize)
	return EMFILE;

    table = AllocPooled(taskBase->pool, newsize);
    
    if (!table)
	return ENOMEM;

    memset(table, 0, newsize);

    if (old)
	CopyMem(old, table, oldsize);

    taskBase->dTable = table;
    taskBase->dTableSize = size;

    if (old)
	FreePooled(taskBase->pool, old, oldsize);

    return 0;
}

int GetFreeFD(struct TaskBase *taskBase)
{
    int i;
    
    for (i = 0; i < taskBase->dTableSize; i++)
    {
	if (!taskBase->dTable[i])
	    return i;
    }

    SetError(EMFILE, taskBase);
    return -1;
}

struct Socket *GetSocket(int s, struct TaskBase *taskBase)
{
    struct Socket *sd;

    if (s >= taskBase->dTableSize)
    {
	SetError(ENOTSOCK, taskBase);
	return NULL;
    }

    sd = taskBase->dTable[s];
    if (!sd)
	SetError(ENOTSOCK, taskBase);

    return sd;
}

struct WSsockaddr *MakeSockAddr(const struct sockaddr *src, int len, struct TaskBase *taskBase)
{
    struct WSsockaddr *sa = AllocPooled(taskBase->pool, len);

    if (sa)
    {
	CopyMem(src, sa, len);
	sa->sa_family = src->sa_family;
    }
    else
	SetError(ENOMEM, taskBase);

    return sa;
}
