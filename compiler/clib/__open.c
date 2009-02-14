/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    File descriptors handling internals.
*/

#include "__arosc_privdata.h"

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define DEBUG 0

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include "__errno.h"
#include "__open.h"
#include "__upath.h"


fdesc *__getfdesc(register int fd)
{
    return ((__numslots>fd) && (fd>=0))?__fd_array[fd]:NULL;
}

void __setfdesc(register int fd, fdesc *desc)
{
    /* FIXME: Check if fd is in valid range... */
    __fd_array[fd] = desc;
}

int __getfirstfd(register int startfd)
{
    /* FIXME: Check if fd is in valid range... */
    for (
	;
	startfd < __numslots && __fd_array[startfd];
	startfd++
    );

    return startfd;
}

int __getfdslot(int wanted_fd)
{
    if (wanted_fd>=__numslots)
    {
        void *tmp;
        
        tmp = malloc((wanted_fd+1)*sizeof(fdesc *));
        
        if (!tmp) return -1;

        if (__fd_array)
        {
            CopyMem(__fd_array, tmp, __numslots*sizeof(fdesc *));
            free(__fd_array);
        }

        __fd_array = tmp;

        bzero(__fd_array + __numslots, (wanted_fd - __numslots + 1) * sizeof(fdesc *));
        __numslots = wanted_fd+1;
    }
    else if (wanted_fd < 0)
    {
        errno = EINVAL;
        return -1;
    }
    else if (__fd_array[wanted_fd])
    {
        close(wanted_fd);
    }
    
    return wanted_fd;
}

LONG __oflags2amode(int flags)
{
    LONG openmode = 0;

    /* filter out invalid modes */
    switch (flags & (O_CREAT|O_TRUNC|O_EXCL))
    {
    	case O_EXCL:
    	case O_EXCL|O_TRUNC:
            return -1;
    }

    if (flags & O_WRITE)    openmode |= FMF_WRITE;
    if (flags & O_READ)     openmode |= FMF_READ;
    if (flags & O_EXEC)     openmode |= FMF_EXECUTE;
    if (flags & O_CREAT)    openmode |= FMF_CREATE;
    if (flags & O_NONBLOCK) openmode |= FMF_NONBLOCK;
    if (flags & O_APPEND)   openmode |= FMF_APPEND;

    return openmode;
}

int __open(int wanted_fd, const char *pathname, int flags, int mode)
{
    BPTR fh = NULL, lock = NULL;
    fdesc *currdesc = NULL;
    fcb *cblock = NULL;
    struct FileInfoBlock *fib = NULL;
    LONG  openmode = __oflags2amode(flags);

    if (__doupath && pathname[0] == '\0')
    {
        /* On *nix "" is not really a valid file name.  */
        errno = ENOENT;
        return -1;
    }

    pathname = __path_u2a(pathname);
    if (!pathname) return -1;
    
    D(bug("__open: entering, wanted fd = %d, path = %s, flags = %d, mode = %d\n", wanted_fd, pathname, flags, mode));

    if (openmode == -1)
    {
        errno = EINVAL;
        D(bug( "__open: exiting with error EINVAL\n"));
        return -1;
    }

    cblock = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR);
    if (!cblock) { D(bug("__open: no memory [1]\n")); goto err; }
    currdesc = malloc(sizeof(fdesc));
    if (!currdesc) { D(bug("__open: no memory [2]\n")); goto err; }
    currdesc->fdflags = 0;
    currdesc->fcb = cblock;

    wanted_fd = __getfdslot(wanted_fd);
    if (wanted_fd == -1) { D(bug("__open: no free fd\n")); goto err; }

    lock = Lock((char *)pathname, SHARED_LOCK);
    if (!lock)
    {
        if
        (
            (IoErr() != ERROR_OBJECT_NOT_FOUND) ||
            /* If the file doesn't exist and the flag O_CREAT is not set return an error */
            (IoErr() == ERROR_OBJECT_NOT_FOUND && !(flags & O_CREAT))
        )
        {
            errno = IoErr2errno(IoErr());
            goto err;
        }
    }
    else
    {
        /* If the file exists, but O_EXCL is set, then return an error */
        if (flags & O_EXCL)
    	{
            errno = EEXIST;
            goto err;
        }

        fib = AllocDosObject(DOS_FIB, NULL);
        if (!fib)
        {
           errno = IoErr2errno(IoErr());
           goto err;
        }
        
        if (!Examine(lock, fib))
        {
            /*
                The filesystem in which the file resides doesn't support
                the EXAMINE action. It might be broken or might also not
                be a filesystem at all. So let's assume the file is not a
                diretory.
           */
           fib->fib_DirEntryType = 0;
        }

#       warning implement softlink handling

        /* Check if it's a directory or a softlink.
           Softlinks are not handled yet, though */
        if (fib->fib_DirEntryType > 0)
        {
            /* A directory cannot be opened for writing */
            if (openmode & FMF_WRITE)
            {
                errno = EISDIR;
                goto err;
            }
            
            fh = lock;
            FreeDosObject(DOS_FIB, fib);
            currdesc->fcb->isdir = 1;
            
            goto success;
        }
        FreeDosObject(DOS_FIB, fib);
        fib = NULL;
    }

    /* the file exists and it's not a directory or the file doesn't exist */

    if (lock)
    {
	UnLock(lock);
	lock = NULL;
    }

    if (openmode & (FMF_APPEND | FMF_WRITE))
	openmode |= FMF_READ; /* force filesystem ACTION_FINDUPDATE */

    if (!(fh = Open ((char *)pathname, openmode)) )
    {
	ULONG ioerr = IoErr();
	D(bug("[clib] Open ioerr=%d\n", ioerr));
	errno = IoErr2errno(ioerr);
        goto err;
    }
    
    if((flags & O_TRUNC) && (flags & (O_RDWR | O_WRONLY)))
    {
	if(SetFileSize(fh, 0, OFFSET_BEGINNING) != 0)
	{
	    /* Ignore error if FSA_SET_FILE_SIZE is not implemented */
	    if(IoErr() != ERROR_NOT_IMPLEMENTED)
	    {
	        errno = IoErr2errno(IoErr());
                goto err;	    
	    }
	}
    }

success:
    currdesc->fcb->fh        = fh;
    currdesc->fcb->flags     = flags;
    currdesc->fcb->opencount = 1;

    __setfdesc(wanted_fd, currdesc);

    D(bug("__open: exiting fd=%d\n", wanted_fd));

    return wanted_fd;

err:
    if (fib) FreeDosObject(DOS_FIB, fib);
    if (cblock) FreeVec(cblock);
    if (currdesc) free(currdesc);
    if (fh && fh != lock) Close(fh);
    if (lock) UnLock(lock);

    D(bug("__open: exiting with error %d\n", errno ));

    return -1;
}


struct __reg_fdarray {
    struct MinNode node;
    struct Task *task;
    fdesc **fdarray;
    int numslots;
};

/* Some local variables for register_init_fdarray */
static int __fdinit = 0;
static struct SignalSemaphore __fdsem;
static struct MinList __fdreglist;
    
int __init_vars(void)
{
    InitSemaphore(&__fdsem);
    NEWLIST(&__fdreglist);
    
    return TRUE;
}

int __register_init_fdarray(fdesc **__fdarray, int numslots)
{
    /* arosc privdata should not be used inside this function,
     * this function is called before aroscbase is initialized
     */
    struct __reg_fdarray *regnode = AllocVec(sizeof(struct __reg_fdarray), MEMF_ANY|MEMF_CLEAR);

    if (regnode == NULL)
        return 0;

    regnode->task = FindTask(NULL);
    regnode->fdarray = __fdarray;
    regnode->numslots = numslots;
    
    D(bug("Allocated regnode: %p, fdarray: %p, numslots: %d\n",
          regnode, regnode->fdarray, regnode->numslots
    ));
    
    ObtainSemaphore(&__fdsem);
    AddHead((struct List *)&__fdreglist, (struct Node *)regnode);
    ReleaseSemaphore(&__fdsem);
    
    return 1;
}


#warning perhaps this has to be handled in a different way...
int __init_stdfiles(void)
{
    struct Process *me;
    fcb *infcb = NULL, *outfcb = NULL, *errfcb = NULL;
    fdesc *indesc=NULL, *outdesc=NULL, *errdesc=NULL;
    int res = __getfdslot(2);

    if
    (
        res == -1                           ||
	!(infcb  = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR)) ||
	!(indesc  = malloc(sizeof(fdesc))) ||
	!(outfcb = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR)) ||
	!(outdesc = malloc(sizeof(fdesc))) ||
	!(errfcb = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR)) ||
	!(errdesc = malloc(sizeof(fdesc)))
    )
    {
        if(infcb)
            FreeVec(infcb);
        if(indesc)
            free(indesc);
        if(outfcb)
            FreeVec(outfcb);
        if(outdesc)
            free(outdesc);
        if(errfcb)
            FreeVec(errfcb);
        if(errdesc)
            free(errdesc);
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return 0;
    }

    indesc->fdflags = 0;
    outdesc->fdflags = 0;
    errdesc->fdflags = 0;

    indesc->fcb = infcb;
    outdesc->fcb = outfcb;
    errdesc->fcb = errfcb;

    me = (struct Process *)FindTask (NULL);
    indesc->fcb->fh  = Input();
    outdesc->fcb->fh = Output();
    errdesc->fcb->fh = me->pr_CES ? me->pr_CES : me->pr_COS;

    indesc->fcb->flags  = O_RDONLY;
    outdesc->fcb->flags = O_WRONLY | O_APPEND;
    errdesc->fcb->flags = O_WRONLY | O_APPEND;

    indesc->fcb->opencount = outdesc->fcb->opencount = errdesc->fcb->opencount = 1;
    indesc->fcb->privflags = outdesc->fcb->privflags = errdesc->fcb->privflags = _FCB_DONTCLOSE_FH;

    __fd_array[STDIN_FILENO]  = indesc;
    __fd_array[STDOUT_FILENO] = outdesc;
    __fd_array[STDERR_FILENO] = errdesc;

    return 1;
}

static int __copy_fdarray(fdesc **__src_fd_array, int numslots)
{
    int i;
    
    for(i = numslots - 1; i >= 0; i--)
    {
        if(__src_fd_array[i])
        {
            if(__getfdslot(i) != i)
                return 0;
            
            if((__fd_array[i] = malloc(sizeof(fdesc))) == NULL)
                return 0;
            
            __fd_array[i]->fdflags = 0;
            __fd_array[i]->fcb = __src_fd_array[i]->fcb;
            __fd_array[i]->fcb->opencount++;
        }
    }
    
    return 1;
}

int __init_fd(void)
{
    struct __reg_fdarray *regnodeit, *regnode = NULL;
    struct Task *self = FindTask(NULL);

    if (!__fdinit)
    {
        __init_vars();
        __fdinit = 1;
    }
    
    ObtainSemaphore(&__fdsem);
    ForeachNode(&__fdreglist, regnodeit)
    {
        if (regnodeit->task == self)
        {
            regnode = regnodeit;
    
            D(bug("Found regnode: %p, fdarray: %p, numslots: %d\n",
                  regnode, regnode->fdarray, regnode->numslots
            ));
            Remove((struct Node *)regnode);
            break;
        }
    }
    ReleaseSemaphore(&__fdsem);
    
    if (regnode == NULL)
        return __init_stdfiles();
    else
    {
        int ok = __copy_fdarray(regnode->fdarray, regnode->numslots);
        
        FreeVec(regnode);
        
        return ok;
    }
}

void __exit_fd(void)
{
    int i = __numslots;
    while (i)
    {
	if (__fd_array[--i])
	    close(i);
    }
}

#include <stdio.h>

void __updatestdio(void)
{
    struct Process *me;

    me = (struct Process *)FindTask(NULL);

    fflush(stdin);
    fflush(stdout);
    fflush(stderr);

    __fd_array[STDIN_FILENO]->fcb->fh  = Input();
    __fd_array[STDOUT_FILENO]->fcb->fh = Output();
    __fd_array[STDERR_FILENO]->fcb->fh = me->pr_CES ? me->pr_CES : me->pr_COS;

    __fd_array[STDIN_FILENO]->fcb->privflags =
        __fd_array[STDOUT_FILENO]->fcb->privflags =
        __fd_array[STDERR_FILENO]->fcb->privflags = _FCB_DONTCLOSE_FH;
}

ADD2INIT(__init_fd, 2);
ADD2EXIT(__exit_fd, 2);
