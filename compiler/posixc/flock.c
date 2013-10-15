/*
    Copyright � 2008-2013, The AROS Development Team. All rights reserved.
    $Id$

    4.4BSD function flock().
*/

#define DEBUG 0
#include <proto/exec.h>
#include <exec/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <errno.h>

#include "__fdesc.h"
#include "__posixc_intbase.h"

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
	Apply or remove an advisory lock on open file descriptor fd. Operation
	argument can be one of the following constants:
	
	LOCK_SH - Place a shared lock on the file specified by fd. More that
	          one process can hold a shared lock on a given file at a
	          time.
	
	LOCK_EX - Place an exclusive lock on the file specified by fd. Only
	          one process can hold an exclusive lock on a given file at
	          a time.
	
	LOCK_UN - Remove an existing lock from the file specified by fd.

	LOCK_EX operation blocks if there is a lock already placed on the
	file. LOCK_SH blocks if there is an exclusive lock already placed
	on the file. If you want to do a non-blocking request, OR the
	operation specifier with LOCK_NB constant. In this case flock() will
	return -1 instead of blocking and set errno to EWOULDBLOCK.
	
	Advisory locks created with flock() are shared among duplicated file
	descriptors.
	
    INPUTS
	fd - File descriptor of the file you want to place or remove lock from.
	operation - Lock operation to be performed.

    RESULT
	0 on success, -1 on error. In case of error a global errno variable
	is set.

    NOTES
	Locks placed with flock() are only advisory, they place no
	restrictions to any file or file descriptor operations.

    EXAMPLE

    BUGS
	It's currently possible to remove lock placed by another process.

    SEE ALSO

    INTERNALS
	Since advisory locks semantics is equal to exec.library semaphores
	semantics, semaphores are used to implement locks. For a given file
	a semaphore named FLOCK(path) is created where path is a full path to
	the file. Locks held by a given process are stored in
        PosixCBase->file_locks and released during process exit.

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
            errno = __stdc_ioerr2errno(IoErr());
            return -1;
        }

        if(NameFromFH(fdesc->fcb->handle, (STRPTR) ((IPTR) buffer + 6), buffersize - 7))
            break;
        else if(IoErr() != ERROR_LINE_TOO_LONG)
        {
            errno = __stdc_ioerr2errno(IoErr());
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
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct FlockNode *node;
    node = AllocMem(sizeof(struct FlockNode), MEMF_ANY | MEMF_CLEAR);
    if(!node)
	return -1;
    node->sem = sem;
    AddHead((struct List *)PosixCBase->file_locks, (struct Node*) node);
    return 0;
}

void RemoveFromList(struct SignalSemaphore *sem)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct FlockNode *varNode;
    struct Node *tmpNode;
    
    ForeachNodeSafe(PosixCBase->file_locks, varNode, tmpNode)
    {
	if(varNode->sem == sem)
	{
	    Remove((struct Node*) varNode);
	    FreeMem(varNode, sizeof(struct FlockNode));
	    break;
	}
    }
}

/* __init_flocks is called during library init.
   PosixCBase->file_locks will be initialized here and then copied
   in all libbases for each open of the library.
   This means that a global file_locks list is used, this is needed as flocks
   are used for locking between different processes.
*/
int __init_flocks(struct PosixCIntBase *PosixCBase)
{
    PosixCBase->file_locks = AllocMem(sizeof(struct MinList), MEMF_PUBLIC);
    NEWLIST(PosixCBase->file_locks);

    D(bug("[flock] Initialized lock list at 0x%p\n", PosixCBase->file_locks));

    return 1;
}

/* This function is called once before root libbase would be freed.
   This function will be called when no other program has arosc.library open
   so no protection should be needed.
*/
void __unlock_flocks(struct PosixCIntBase *PosixCBase)
{
    struct FlockNode *lock;
    struct SignalSemaphore *sem;

    D(bug("[flock] Freeing lock list at 0x%p\n", PosixCBase->file_locks));

    while ((lock = (struct FlockNode *) REMHEAD(PosixCBase->file_locks)))
    {
        sem = lock->sem;
        ReleaseSemaphore(sem);
        FreeMem(lock, sizeof(struct FlockNode));

        if (sem->ss_Owner != NULL || sem->ss_QueueCount != -1)
            bug("[flock] Freeing locked semaphore!");

        D(bug("[flock] removing semaphore %s\n", sem->ss_Link.ln_Name));
        RemSemaphore(sem);
        FreeVec(sem->ss_Link.ln_Name);
        FreeVec(sem);
    }
    FreeMem(PosixCBase->file_locks, sizeof(struct MinList));
    PosixCBase->file_locks = NULL;
}

ADD2INITLIB(__init_flocks, 1);
ADD2EXPUNGELIB(__unlock_flocks, 1);
