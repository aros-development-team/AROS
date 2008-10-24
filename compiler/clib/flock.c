/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    4.4BSD function flock().
*/

#define DEBUG 0

#include <proto/exec.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <aros/debug.h>
#include <aros/startup.h>
#include <aros/symbolsets.h>

#include <errno.h>

#include "__open.h"
#include "__errno.h"
#include "__arosc_privdata.h"

struct FlockNode
{
    struct Node node;
    struct SignalSemaphore *sem;
};

LONG AddToList(struct SignalSemaphore *sem);
void RemoveFromList(struct SignalSemaphore *sem);

/*****************************************************************************

    NAME */
#include <sys/file.h>

	int flock (

/*  SYNOPSIS */
	int fd,
	int operation)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    fdesc *fdesc = __getfdesc(fd);
    char *buffer;
    int buffersize = 256;
    struct SignalSemaphore *sem;

    D(bug("flock(%d, %d)\n", fd, operation));
    if (!fdesc)
    {
	errno = EBADF;
	return -1;
    }

    /* Sanity check */
    if(
	(operation & ~LOCK_NB) != LOCK_SH &&
	(operation & ~LOCK_NB) != LOCK_EX &&
	(operation & ~LOCK_NB) != LOCK_UN
    )
    {
	errno = EINVAL;
	return -1;
    }
	
    /* Get the full path of the flocked filesystem object */
    do
    {
        if(!(buffer = AllocVec(buffersize, MEMF_ANY)))
        {
            errno = IoErr2errno(IoErr());
            return -1;
        }

        if(NameFromLock(fdesc->fcb->fh, (STRPTR) ((IPTR) buffer + 6), buffersize - 7))
            break;
        else if(IoErr() != ERROR_LINE_TOO_LONG)
        {
            errno = IoErr2errno(IoErr());
            FreeVec(buffer);
            return -1;
        }
        FreeVec(buffer);
        buffersize *= 2;
    }
    while(TRUE);
    
    CopyMem("FLOCK(", buffer, strlen("FLOCK("));
    buffer[strlen(buffer) + 1] = '\0';
    buffer[strlen(buffer)] = ')';

    D(bug("[flock] Semaphore name: %s\n", buffer));

    /* Find semaphore named FLOCK(path), add a new one if not found any */
    Forbid();
    sem = FindSemaphore((STRPTR) buffer);
    if(!sem)
    {
	D(bug("[flock] Semaphore %s not found, creating a new one\n", buffer));
	sem = (struct SignalSemaphore*) 
	    AllocVec(sizeof(struct SignalSemaphore), MEMF_PUBLIC|MEMF_CLEAR);
	if(!sem)
	{
	    errno = ENOMEM;
	    Permit();
	    return -1;
	}
	sem->ss_Link.ln_Name = buffer;
	AddSemaphore(sem);
    }
    else
    {
	D(bug("[flock] Semaphore %s found, freeing buffer\n", buffer));
	FreeVec(buffer);
    }
    
    if(operation & LOCK_UN)
    {
	D(bug("[flock] Releasing semaphore %s\n", sem->ss_Link.ln_Name));
	ReleaseSemaphore(sem);
	RemoveFromList(sem);
	if(sem->ss_Owner == NULL && sem->ss_QueueCount == -1)
	{
	    D(bug("[flock] All locks unlocked, removing semaphore %s\n", sem->ss_Link.ln_Name));
	    /* All locks for this file were unlocked, we don't need semaphore
	     * anymore */
	    RemSemaphore(sem);
	    FreeVec(sem->ss_Link.ln_Name);
	    FreeVec(sem);
	    Permit();
	    return 0;
	}
    }
    Permit();
    
    switch(operation & ~LOCK_NB)
    {
	case LOCK_SH:
	    D(bug("[flock] Obtaining shared lock\n"));
	    if(operation & LOCK_NB)
	    {
		if(!AttemptSemaphoreShared(sem))
		{
		    errno = EWOULDBLOCK;
		    return -1;
		}
	    }
	    else
		ObtainSemaphoreShared(sem);
	    D(bug("[flock] Shared lock obtained\n"));
	    AddToList(sem);
	    break;
	case LOCK_EX:
	    D(bug("[flock] Obtaining exclusive lock\n"));
	    if(operation & LOCK_NB)
	    {
		if(!AttemptSemaphore(sem))
		{
		    errno = EWOULDBLOCK;
		    return -1;
		}
	    }
	    else
		ObtainSemaphore(sem);
	    D(bug("[flock] Exclusive lock obtained\n"));
	    AddToList(sem);
	    break;
    }
    return 0;
}

LONG AddToList(struct SignalSemaphore *sem)
{
    struct FlockNode *node;
    node = AllocMem(sizeof(struct FlockNode), MEMF_ANY | MEMF_CLEAR);
    if(!node)
	return -1;
    node->sem = sem;
    AddHead((struct List *)&__flocks_list, (struct Node*) node);
    return 0;
}

void RemoveFromList(struct SignalSemaphore *sem)
{
    struct FlockNode *varNode;
    struct Node *tmpNode;
    
    ForeachNodeSafe(&__flocks_list, varNode, tmpNode)
    {
	if(varNode->sem == sem)
	{
	    Remove((struct Node*) varNode);
	    FreeMem(varNode, sizeof(struct FlockNode));
	    break;
	}
    }
}

int __init_flocks(void)
{
    NEWLIST((struct List *)&__flocks_list);

    return 1;
}

void __unlock_flocks(void)
{
    {
	struct FlockNode *lock;
	struct SignalSemaphore *sem;

	Forbid();
	while ((lock = (struct FlockNode *) REMHEAD(
	    (struct List *) &__flocks_list)))
	{
	    sem = lock->sem;
    	    ReleaseSemaphore(sem);
	    FreeMem(lock, sizeof(struct FlockNode));

    	    if(sem->ss_Owner == NULL && sem->ss_QueueCount == -1)
    	    {
    	        D(bug("[flock] All locks unlocked, removing semaphore %s\n", sem->ss_Link.ln_Name));
    	        /* All locks for this file were unlocked, we don't need semaphore
                 * anymore */
                RemSemaphore(sem);
                FreeVec(sem->ss_Link.ln_Name);
                FreeVec(sem);
            }
	}
	Permit();
    }
}

ADD2INIT(__init_flocks, 1);
ADD2EXIT(__unlock_flocks, 1);