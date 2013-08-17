/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    File descriptors handling internals.
*/

#include LC_LIBDEFS_FILE

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
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include "__fdesc.h"
#include "__upath.h"

/* TODO: Add locking to make filedesc usage thread safe
   Using vfork()+exec*() i filedescriptors may be shared between different
   tasks. Only one DOS file handle is used between shared file descriptors.
   DOS file handles are not thread safe so we should add it here to make it
   thread safe.
   Possible implementation should carefully at performance impact.
*/

void __getfdarray(APTR *arrayptr, int *slotsptr)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    *arrayptr = aroscbase->acb_fd_array;
    *slotsptr = aroscbase->acb_numslots;
}

void __setfdarray(APTR array, int slots)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    aroscbase->acb_fd_array = array;
    aroscbase->acb_numslots = slots;
}

void __setfdarraybase(struct aroscbase *aroscbase2)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    aroscbase->acb_fd_array = aroscbase2->acb_fd_array;
    aroscbase->acb_numslots = aroscbase2->acb_numslots;
}

int __getfdslots(void)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    return aroscbase->acb_numslots;
}

fdesc *__getfdesc(register int fd)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    return ((aroscbase->acb_numslots>fd) && (fd>=0))?aroscbase->acb_fd_array[fd]:NULL;
}

void __setfdesc(register int fd, fdesc *desc)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    /* FIXME: Check if fd is in valid range... */
    aroscbase->acb_fd_array[fd] = desc;
}

int __getfirstfd(register int startfd)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    /* FIXME: Check if fd is in valid range... */
    for (
	;
	startfd < aroscbase->acb_numslots && aroscbase->acb_fd_array[startfd];
	startfd++
    );

    return startfd;
}

int __getfdslot(int wanted_fd)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    if (wanted_fd>=aroscbase->acb_numslots)
    {
        void *tmp;
        
        tmp = AllocPooled(aroscbase->acb_internalpool, (wanted_fd+1)*sizeof(fdesc *));
        
        if (!tmp) return -1;

        if (aroscbase->acb_fd_array)
        {
            size_t size = aroscbase->acb_numslots*sizeof(fdesc *);
            CopyMem(aroscbase->acb_fd_array, tmp, size);
            FreePooled(aroscbase->acb_internalpool, aroscbase->acb_fd_array, size);
        }

        aroscbase->acb_fd_array = tmp;

        memset(
            aroscbase->acb_fd_array + aroscbase->acb_numslots,
            0,
            (wanted_fd - aroscbase->acb_numslots + 1) * sizeof(fdesc *)
        );
        aroscbase->acb_numslots = wanted_fd+1;
    }
    else if (wanted_fd < 0)
    {
        errno = EINVAL;
        return -1;
    }
    else if (aroscbase->acb_fd_array[wanted_fd])
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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    BPTR fh = BNULL, lock = BNULL;
    fdesc *currdesc = NULL;
    fcb *cblock = NULL;
    struct FileInfoBlock *fib = NULL;
    LONG  openmode = __oflags2amode(flags);

    if (aroscbase->acb_doupath && pathname[0] == '\0')
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
                errno = __arosc_ioerr2errno(IoErr());
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
               errno = __arosc_ioerr2errno(IoErr());
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
	errno = __arosc_ioerr2errno(ioerr);
        goto err;
    }
   
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
	        errno = __arosc_ioerr2errno(ioerr);
                goto err;
	    }
	}
    }

    /* Handle O_APPEND */
    if((flags & O_APPEND) && (flags & (O_RDWR | O_WRONLY)))
    {
        if(Seek(fh, 0, OFFSET_END) != 0) {
            errno = __arosc_ioerr2errno(IoErr());
            goto err;
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
    FreeVec(cblock);
    if (currdesc) __free_fdesc(currdesc);
    if (fh && fh != lock) Close(fh);
    if (lock) UnLock(lock);

    D(bug("__open: exiting with error %d\n", errno ));

    return -1;
}

fdesc *__alloc_fdesc(void)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    fdesc * desc;
    
    desc = AllocPooled(aroscbase->acb_internalpool, sizeof(fdesc));
    
    D(bug("Allocated fdesc %x from %x pool\n", desc, aroscbase->acb_internalpool));
    
    return desc;
}

void __free_fdesc(fdesc *desc)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    D(bug("Freeing fdesc %x from %x pool\n", desc, aroscbase->acb_internalpool));
    FreePooled(aroscbase->acb_internalpool, desc, sizeof(fdesc));
}


/* FIXME: perhaps this has to be handled in a different way...  */
int __init_stdfiles(struct aroscbase *aroscbase)
{
    struct Process *me;
    fcb *infcb = NULL, *outfcb = NULL, *errfcb = NULL;
    fdesc *indesc=NULL, *outdesc=NULL, *errdesc=NULL;
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
    indesc->fcb->privflags = _FCB_DONTCLOSE_FH | _FCB_FLUSHONREAD;
    outdesc->fcb->privflags = errdesc->fcb->privflags = _FCB_DONTCLOSE_FH;

    aroscbase->acb_fd_array[STDIN_FILENO]  = indesc;
    aroscbase->acb_fd_array[STDOUT_FILENO] = outdesc;
    aroscbase->acb_fd_array[STDERR_FILENO] = errdesc;

    return 1;
}

static int __copy_fdarray(fdesc **__src_fd_array, int numslots)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    int i;
    
    for(i = numslots - 1; i >= 0; i--)
    {
        if(__src_fd_array[i])
        {
            if(__getfdslot(i) != i)
                return 0;
            
            if((aroscbase->acb_fd_array[i] = __alloc_fdesc()) == NULL)
                return 0;

            aroscbase->acb_fd_array[i]->fdflags = __src_fd_array[i]->fdflags;
            aroscbase->acb_fd_array[i]->fcb = __src_fd_array[i]->fcb;
            aroscbase->acb_fd_array[i]->fcb->opencount++;
        }
    }
    
    return 1;
}

int __init_fd(struct aroscbase *aroscbase)
{
    struct aroscbase *paroscbase = __GM_GetBaseParent(aroscbase);

    D(bug("Found parent aroscbase %p with flags 0x%x\n",
          paroscbase, paroscbase ? paroscbase->acb_flags : 0
    ));

    if (paroscbase && (paroscbase->acb_flags & (VFORK_PARENT | EXEC_PARENT)))
        return __copy_fdarray(paroscbase->acb_fd_array, paroscbase->acb_numslots);
    else
        return __init_stdfiles(aroscbase);
}

void __exit_fd(struct aroscbase *aroscbase)
{
    int i = aroscbase->acb_numslots;
    while (i)
    {
	if (aroscbase->acb_fd_array[--i])
	    close(i);
    }
}

void __close_on_exec_fdescs(void)
{
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();

    int i;
    fdesc *fd;

    for (i = aroscbase->acb_numslots - 1; i >= 0; i--)
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
    struct aroscbase *aroscbase = __aros_getbase_aroscbase();
    struct Process *me;

    me = (struct Process *)FindTask(NULL);

    fflush(stdin);
    fflush(stdout);
    fflush(stderr);

    aroscbase->acb_fd_array[STDIN_FILENO]->fcb->fh  = Input();
    aroscbase->acb_fd_array[STDOUT_FILENO]->fcb->fh = Output();
    aroscbase->acb_fd_array[STDERR_FILENO]->fcb->fh = me->pr_CES ? me->pr_CES : me->pr_COS;

    aroscbase->acb_fd_array[STDIN_FILENO]->fcb->privflags =
        aroscbase->acb_fd_array[STDOUT_FILENO]->fcb->privflags =
        aroscbase->acb_fd_array[STDERR_FILENO]->fcb->privflags = _FCB_DONTCLOSE_FH;
}

ADD2OPENLIB(__init_fd, 2);
ADD2CLOSELIB(__exit_fd, 2);

