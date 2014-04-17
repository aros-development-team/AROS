/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    File descriptors handling internals.
*/

#include LC_LIBDEFS_FILE

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
#include <dos/stdio.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include "__fdesc.h"
#include "__upath.h"

/* TODO: Add locking to make filedesc usage thread safe
   Using vfork()+exec*() filedescriptors may be shared between different
   tasks. Only one DOS file handle is used between shared file descriptors.
   DOS file handles are not thread safe so we should add it here to make it
   thread safe.
   Possible implementation should look carefully at performance impact.
*/

void __getfdarray(APTR *arrayptr, int *slotsptr)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    *arrayptr = PosixCBase->fd_array;
    *slotsptr = PosixCBase->fd_slots;
}

void __setfdarray(APTR array, int slots)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    PosixCBase->fd_array = array;
    PosixCBase->fd_slots = slots;
}

void __setfdarraybase(struct PosixCIntBase *PosixCBase2)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    PosixCBase->fd_array = PosixCBase2->fd_array;
    PosixCBase->fd_slots = PosixCBase2->fd_slots;
}

int __getfdslots(void)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    return PosixCBase->fd_slots;
}

fdesc *__getfdesc(register int fd)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    return ((PosixCBase->fd_slots>fd) && (fd>=0))?PosixCBase->fd_array[fd]:NULL;
}

void __setfdesc(register int fd, fdesc *desc)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    /* FIXME: Check if fd is in valid range... */
    PosixCBase->fd_array[fd] = desc;
}

int __getfirstfd(register int startfd)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    /* FIXME: Check if fd is in valid range... */
    for (
	;
	startfd < PosixCBase->fd_slots && PosixCBase->fd_array[startfd];
	startfd++
    );

    return startfd;
}

int __getfdslot(int wanted_fd)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (wanted_fd>=PosixCBase->fd_slots)
    {
        void *tmp;
        
        tmp = AllocPooled(PosixCBase->internalpool, (wanted_fd+1)*sizeof(fdesc *));
        
        if (!tmp) return -1;

        if (PosixCBase->fd_array)
        {
            size_t size = PosixCBase->fd_slots*sizeof(fdesc *);
            CopyMem(PosixCBase->fd_array, tmp, size);
            FreePooled(PosixCBase->internalpool, PosixCBase->fd_array, size);
        }

        PosixCBase->fd_array = tmp;
        PosixCBase->fd_slots = wanted_fd+1;
    }
    else if (wanted_fd < 0)
    {
        errno = EINVAL;
        return -1;
    }
    else if (PosixCBase->fd_array[wanted_fd])
    {
        close(wanted_fd);
    }
    
    return wanted_fd;
}

LONG __oflags2amode(int flags)
{
    LONG openmode = -1;

    /* filter out invalid modes */
    switch (flags & (O_CREAT|O_TRUNC|O_EXCL))
    {
    	case O_EXCL:
    	case O_EXCL|O_TRUNC:
            return -1;
    }

    /* Sorted in 'trumping' order. Ie if 
     * O_WRITE is on, that overrides O_READ.
     * Similarly, O_CREAT overrides O_WRITE.
     */
    if (flags & O_RDONLY)   openmode = MODE_OLDFILE;
    if (flags & O_WRONLY)   openmode = MODE_OLDFILE;
    if (flags & O_RDWR)     openmode = MODE_OLDFILE;
    if (flags & O_READ)     openmode = MODE_OLDFILE;
    if (flags & O_WRITE)    openmode = MODE_READWRITE;
    if (flags & O_CREAT)    openmode = MODE_NEWFILE;
    if (flags & O_APPEND)   /* Handled later */;
    if (flags & O_TRUNC)    /* Handled later */;
    if (flags & O_EXEC)     /* Ignored */;
    if (flags & O_NONBLOCK) /* Ignored */;

    return openmode;
}

int __open(int wanted_fd, const char *pathname, int flags, int mode)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    BPTR fh = BNULL, lock = BNULL;
    fdesc *currdesc = NULL;
    fcb *cblock = NULL;
    struct FileInfoBlock *fib = NULL;
    LONG  openmode = __oflags2amode(flags);

    if (PosixCBase->doupath && pathname[0] == '\0')
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
    currdesc = __alloc_fdesc();
    if (!currdesc) { D(bug("__open: no memory [2]\n")); goto err; }
    currdesc->fdflags = 0;
    currdesc->fcb = cblock;

    wanted_fd = __getfdslot(wanted_fd);
    if (wanted_fd == -1) { D(bug("__open: no free fd\n")); goto err; }

    /*
     * In case of file system, test existance of file. Non-file system handlers (i.e CON:)
     * support opening a file while not supporting locking.
     */
    if(IsFileSystem(pathname) == DOSTRUE)
    {
        lock = Lock((char *)pathname, SHARED_LOCK);
        if (!lock)
        {
            if (IoErr() == ERROR_OBJECT_WRONG_TYPE)
            {
                /*
                   Needed for sfs file system which reports this error number on a
                   Lock aaa/bbb/ccc with bbb being a file instead of a directory.
                */
                errno = ENOTDIR;
                goto err;
            }

            if
            (
                (IoErr() != ERROR_OBJECT_NOT_FOUND) ||
                /* If the file doesn't exist and the flag O_CREAT is not set return an error */
                (IoErr() == ERROR_OBJECT_NOT_FOUND && !(flags & O_CREAT))
            )
            {
                errno = __stdc_ioerr2errno(IoErr());
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
               errno = __stdc_ioerr2errno(IoErr());
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

            /* FIXME: implement softlink handling */

            /* Check if it's a directory or a softlink.
               Softlinks are not handled yet, though */
            if (fib->fib_DirEntryType > 0)
            {
                /* A directory cannot be opened for writing */
                if (openmode != MODE_OLDFILE)
                {
                    errno = EISDIR;
                    goto err;
                }

                fh = lock;
                FreeDosObject(DOS_FIB, fib);
                currdesc->fcb->privflags |= _FCB_ISDIR;

                goto success;
            }
            FreeDosObject(DOS_FIB, fib);
            fib = NULL;
        }
    }

    /* the file exists and it's not a directory or the file doesn't exist */

    if (lock)
    {
	UnLock(lock);
	lock = BNULL;
    }

    if (!(fh = Open ((char *)pathname, openmode)) )
    {
	ULONG ioerr = IoErr();
	D(bug("__open: Open ioerr=%d\n", ioerr));
	errno = __stdc_ioerr2errno(ioerr);
        goto err;
    }

    /* Allow opening NIL: (/dev/null) regardless of additional flags/modes */
    if (!strcasecmp(pathname, "NIL:"))
        goto success;

    /* Handle O_TRUNC */
    if((flags & O_TRUNC) && (flags & (O_RDWR | O_WRONLY)))
    {
	if(SetFileSize(fh, 0, OFFSET_BEGINNING) != 0)
	{
	    ULONG ioerr = IoErr();
	    /* Ignore error if ACTION_SET_FILE_SIZE is not implemented */
	    if(ioerr != ERROR_NOT_IMPLEMENTED &&
	       ioerr != ERROR_ACTION_NOT_KNOWN)
	    {
		D(bug("__open: SetFileSize ioerr=%d\n", ioerr));
	        errno = __stdc_ioerr2errno(ioerr);
                goto err;
	    }
	}
    }

    /* Handle O_APPEND */
    if((flags & O_APPEND) && (flags & (O_RDWR | O_WRONLY)))
    {
        if(Seek(fh, 0, OFFSET_END) != 0) {
            errno = __stdc_ioerr2errno(IoErr());
            goto err;
        }
    }


success:
    currdesc->fcb->handle    = fh;
    currdesc->fcb->flags     = flags;
    currdesc->fcb->opencount = 1;

    __setfdesc(wanted_fd, currdesc);

    D(bug("__open: exiting fd=%d\n", wanted_fd));

    return wanted_fd;

err:
    if (fib) FreeDosObject(DOS_FIB, fib);
    FreeVec(cblock);
    if (currdesc) __free_fdesc(currdesc);
    if (fh && fh != lock) Close(fh);
    if (lock) UnLock(lock);

    D(bug("__open: exiting with error %d\n", errno ));

    return -1;
}

fdesc *__alloc_fdesc(void)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    fdesc * desc;
    
    desc = AllocPooled(PosixCBase->internalpool, sizeof(fdesc));
    
    D(bug("Allocated fdesc %x from %x pool\n", desc, PosixCBase->internalpool));
    
    return desc;
}

void __free_fdesc(fdesc *desc)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    D(bug("Freeing fdesc %x from %x pool\n", desc, PosixCBase->internalpool));
    FreePooled(PosixCBase->internalpool, desc, sizeof(fdesc));
}


/* FIXME: perhaps this has to be handled in a different way...  */
int __init_stdfiles(struct PosixCIntBase *PosixCBase)
{
    struct Process *me;
    fcb *infcb = NULL, *outfcb = NULL, *errfcb = NULL;
    fdesc *indesc=NULL, *outdesc=NULL, *errdesc=NULL;
    BPTR infh, outfh;
    int res = __getfdslot(2);

    if
    (
        res == -1                           ||
	!(infcb  = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR)) ||
	!(indesc  = __alloc_fdesc()) ||
	!(outfcb = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR)) ||
	!(outdesc = __alloc_fdesc()) ||
	!(errfcb = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR)) ||
	!(errdesc = __alloc_fdesc())
    )
    {
        FreeVec(infcb);
        if(indesc)
            __free_fdesc(indesc);
        FreeVec(outfcb);
        if(outdesc)
            __free_fdesc(outdesc);
        FreeVec(errfcb);
        if(errdesc)
            __free_fdesc(errdesc);
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return 0;
    }

    me = (struct Process *)FindTask (NULL);

    infh = Input();
    infcb->handle = OpenFromLock(DupLockFromFH(infh));
    infcb->flags  = O_RDONLY;
    infcb->opencount = 1;
    /* Remove (remaining) command line args on first read */
    infcb->privflags = _FCB_FLUSHONREAD;
    /* Use original fh if it can't be duplicated */
    if (infcb->handle == BNULL)
    {
        infcb->handle = infh;
        infcb->privflags |= _FCB_DONTCLOSE_FH;
    }
    indesc->fcb = infcb;
    indesc->fdflags = 0;
    D(bug("[__init_stdfiles]Input(): %p, infcb->handle: %p\n",
          BADDR(Input()), BADDR(infcb->handle)
    ));

    outfh = Output();
    outfcb->handle = OpenFromLock(DupLockFromFH(outfh));
    outfcb->flags = O_WRONLY | O_APPEND;
    outfcb->opencount = 1;
    /* Use original fh if it can't be duplicated */
    if (outfcb->handle == BNULL)
    {
        outfcb->handle = outfh;
        outfcb->privflags |= _FCB_DONTCLOSE_FH;
    }
    outdesc->fcb = outfcb;
    outdesc->fdflags = 0;
    D(bug("[__init_stdfiles]Output(): %p, outfcb->handle: %p\n",
          BADDR(Output()), BADDR(outfcb->handle)
    ));

    /* Normally stderr is expected to be unbuffered for POSIX.
       We only do this if we can duplicate the handle otherwise
       we obey the buffering of the error stream as originally set.
    */
    if (me->pr_CES != BNULL)
    {
        errfcb->handle = OpenFromLock(DupLockFromFH(me->pr_CES));
        if (errfcb->handle != BNULL)
            SetVBuf(errfcb->handle, NULL, BUF_NONE, -1);
        else /* File handle could not be duplicated; use original */
        {
            errfcb->handle = me->pr_CES;
            errfcb->privflags |= _FCB_DONTCLOSE_FH;
        }
    }
    else
    {
        errfcb->handle = OpenFromLock(DupLockFromFH(Output()));
        if (errfcb->handle != BNULL)
            SetVBuf((BPTR) errfcb->handle, NULL, BUF_NONE, -1);
        else /* File handle could not be duplicated; use original */
        {
            errfcb->handle = outfcb->handle;
            errfcb->privflags = _FCB_DONTCLOSE_FH;
        }
    }
    errfcb->flags = O_WRONLY | O_APPEND;
    errfcb->opencount = 1;
    errdesc->fcb = errfcb;
    errdesc->fdflags = 0;
    D(bug("[__init_stdfiles]me->pr_CES: %p, errfcb->handle: %p\n",
          BADDR(me->pr_CES), BADDR(errfcb->handle)
    ));

    PosixCBase->fd_array[STDIN_FILENO]  = indesc;
    PosixCBase->fd_array[STDOUT_FILENO] = outdesc;
    PosixCBase->fd_array[STDERR_FILENO] = errdesc;

    return 1;
}

static int __copy_fdarray(fdesc **__src_fd_array, int fd_slots)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int i;
    
    for(i = fd_slots - 1; i >= 0; i--)
    {
        if(__src_fd_array[i])
        {
            if(__getfdslot(i) != i)
                return 0;
            
            if((PosixCBase->fd_array[i] = __alloc_fdesc()) == NULL)
                return 0;

            PosixCBase->fd_array[i]->fdflags = __src_fd_array[i]->fdflags;
            PosixCBase->fd_array[i]->fcb = __src_fd_array[i]->fcb;
            PosixCBase->fd_array[i]->fcb->opencount++;
        }
    }
    
    return 1;
}

int __init_fd(struct PosixCIntBase *PosixCBase)
{
    struct PosixCIntBase *pPosixCBase =
        (struct PosixCIntBase *)__GM_GetBaseParent(PosixCBase);

    D(bug("Found parent PosixCBase %p with flags 0x%x\n",
          pPosixCBase, pPosixCBase ? pPosixCBase->flags : 0
    ));

    if (pPosixCBase && (pPosixCBase->flags & (VFORK_PARENT | EXEC_PARENT)))
        return __copy_fdarray(pPosixCBase->fd_array, pPosixCBase->fd_slots);
    else
        return __init_stdfiles(PosixCBase);
}

void __exit_fd(struct PosixCIntBase *PosixCBase)
{
    int i = PosixCBase->fd_slots;
    while (i)
    {
	if (PosixCBase->fd_array[--i])
	    close(i);
    }
}

void __close_on_exec_fdescs(void)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int i;
    fdesc *fd;

    for (i = PosixCBase->fd_slots - 1; i >= 0; i--)
    {
        if ((fd = __getfdesc(i)) != NULL)
        {
            D(bug("__close_fdesc_on_exec: checking fd %d\n", i));
            if (fd->fdflags & FD_CLOEXEC)
            {
                D(bug("__close_fdesc_on_exec: closing fd %d\n", i));
                close(i);
            }
        }
    }
}

#include <stdio.h>

void __updatestdio(void)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct Process *me;
    fcb *fcb = NULL;

    me = (struct Process *)FindTask(NULL);

    fflush(((struct PosixCBase *)PosixCBase)->_stdin);
    fflush(((struct PosixCBase *)PosixCBase)->_stdout);
    fflush(((struct PosixCBase *)PosixCBase)->_stderr);

    fcb = PosixCBase->fd_array[STDIN_FILENO]->fcb;
    if (!(fcb->privflags & _FCB_DONTCLOSE_FH))
        Close(fcb->handle);
    else
        fcb->privflags &= ~_FCB_DONTCLOSE_FH;
    fcb->handle = OpenFromLock(DupLockFromFH(Input()));

    fcb = PosixCBase->fd_array[STDOUT_FILENO]->fcb;
    if (!(fcb->privflags & _FCB_DONTCLOSE_FH))
        Close(fcb->handle);
    else
        fcb->privflags &= ~_FCB_DONTCLOSE_FH;
    fcb->handle = OpenFromLock(DupLockFromFH(Output()));

    fcb = PosixCBase->fd_array[STDERR_FILENO]->fcb;
    if (!(fcb->privflags & _FCB_DONTCLOSE_FH))
        Close(fcb->handle);
    if (me->pr_CES != BNULL)
    {
        fcb->handle = OpenFromLock(DupLockFromFH(me->pr_CES));
        fcb->privflags &= ~_FCB_DONTCLOSE_FH;
    }
    else
    {
        fcb->handle = PosixCBase->fd_array[STDOUT_FILENO]->fcb->handle;
        fcb->privflags |= _FCB_DONTCLOSE_FH;
    }
}

ADD2OPENLIB(__init_fd, 2);
ADD2CLOSELIB(__exit_fd, 2);

