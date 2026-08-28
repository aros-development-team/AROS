/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    File descriptor handling internals.
*/

#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <dos/stdio.h>
#include <aros/symbolsets.h>
#include <libraries/fd.h>
#include <proto/fd.h>

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "__fdesc.h"
#include "__dos64.h"
#include "__upath.h"

#ifdef errno
#undef errno
#endif
#define errno (*(&((struct PosixCBase *)PosixCBase)->StdCBase->_errno))

static int __fdlib_available(struct PosixCIntBase *PosixCBase)
{
    if (PosixCBase->PosixCFDBase == NULL)
        PosixCBase->PosixCFDBase = OpenLibrary("fd.library", 0);

    return PosixCBase->PosixCFDBase != NULL;
}

static void __set_errno(struct PosixCIntBase *PosixCBase, int enval)
{
    if (((struct PosixCBase *)PosixCBase)->StdCBase) {
        errno = enval;
    }
}

/* A pthread worker Task receives its own posixc.library base but the POSIX
   descriptor table belongs to the Process that created it.  __fd_owner()
   resolves the base that actually holds the descriptor array/slots/pool;
   it is the base itself for an ordinary Process or CLI child.  All descriptor
   table accesses go through FD_ARRAY()/FD_SLOTS()/FD_POOL() so a worker sees
   its creator's table. */
static struct PosixCIntBase *__fd_owner(struct PosixCIntBase *PosixCBase)
{
    return PosixCBase->fd_owner ? PosixCBase->fd_owner : PosixCBase;
}

#define FD_SLOTS(base) (__fd_owner(base)->fd_slots)
#define FD_ARRAY(base) (__fd_owner(base)->fd_array)
#define FD_POOL(base)  (__fd_owner(base)->internalpool)

/* Serialise access to the shared descriptor table against a concurrent
   grow (see __getfdslot). The lock lives in the owning base and is a
   SignalSemaphore, so nested locking within one Task (e.g. __open ->
   __getfdslot -> close -> __setfdesc) is safe. */
#define FD_LOCK(base)   ObtainSemaphore(&__fd_owner(base)->fd_sem)
#define FD_UNLOCK(base) ReleaseSemaphore(&__fd_owner(base)->fd_sem)

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

    *arrayptr = FD_ARRAY(PosixCBase);
    *slotsptr = FD_SLOTS(PosixCBase);
}

void __setfdarray(APTR array, int slots)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    FD_ARRAY(PosixCBase) = array;
    FD_SLOTS(PosixCBase) = slots;
}

void __setfdarraybase(struct PosixCIntBase *PosixCBase2)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    FD_ARRAY(PosixCBase) = FD_ARRAY(PosixCBase2);
    FD_SLOTS(PosixCBase) = FD_SLOTS(PosixCBase2);
}

int __getfdslots(void)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    return FD_SLOTS(PosixCBase);
}

fdesc *__getfdesc(register int fd)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    fdesc *local;

    /* Read the slot count and array entry atomically w.r.t. a grow. */
    FD_LOCK(PosixCBase);
    local = ((FD_SLOTS(PosixCBase)>fd) && (fd>=0)) ?
                FD_ARRAY(PosixCBase)[fd] : NULL;
    FD_UNLOCK(PosixCBase);

    /* Standard streams (0..2) are always process-local. */
    if (fd >= 0 && fd <= STDERR_FILENO)
        return local;

    /* For fd > 2 fd.library is the authority on descriptor ownership.  If it
       says this number is owned by another subsystem (e.g. a bsdsocket
       socket) or is free, any local fd_array entry is stale - the descriptor
       is not a posixc file.  Returning NULL lets the caller dispatch through
       the owner's hooks instead of acting on a wrong file. */
    if (fd > STDERR_FILENO && __fdlib_available(PosixCBase)) {
        struct Library *FDBase = PosixCBase->PosixCFDBase;
        if (FD_GetOwner(fd) != FD_OWNER_POSIXC)
            return NULL;
    }

    return local;
}

const struct fd_hooks *__getfdhooks(int fd, APTR *datap)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct Library *FDBase;
    fd_owner_t owner;
    const struct fd_hooks *hooks;

    if (fd < 0 || !__fdlib_available(PosixCBase))
        return NULL;

    FDBase = PosixCBase->PosixCFDBase;

    /* A plain posixc file descriptor is served from the local fd array; only
       descriptors owned by another subsystem (e.g. bsdsocket sockets) are
       dispatched through their registered hooks. */
    owner = FD_GetOwner(fd);
    hooks = (owner == FD_OWNER_NONE || owner == FD_OWNER_POSIXC)
              ? NULL : FD_GetOwnerHooks(owner);
    if (owner == FD_OWNER_NONE || owner == FD_OWNER_POSIXC)
        return NULL;

    if (hooks && datap)
        *datap = FD_GetData(fd);

    return hooks;
}

void __setfdesc(register int fd, fdesc *desc)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    /* FIXME: Check if fd is in valid range... */
    /* Publish the slot pointer under the table guard so a concurrent grow
       (which reallocates the array) cannot write into a stale array. */
    FD_LOCK(PosixCBase);
    FD_ARRAY(PosixCBase)[fd] = desc;
    FD_UNLOCK(PosixCBase);

    /* Standard streams (0..2) are inherited DOS handles kept process-local;
       they are never published in fd.library, whose descriptor numbers are
       system-wide.  Publishing them there would collide with the stdio of
       other processes (and with sockets allocated by bsdsocket.library). */
    if (fd > STDERR_FILENO && __fdlib_available(PosixCBase)) {
        struct Library *FDBase = PosixCBase->PosixCFDBase;
        if (desc)
            FD_SetData(fd, FD_OWNER_POSIXC, desc);
        else
            FD_Free(fd, FD_OWNER_POSIXC);
    }
}

int __getfirstfd(register int startfd)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    /* FIXME: Check if fd is in valid range... */
    if (!__fdlib_available(PosixCBase)) {
        for (
            ;
            startfd < FD_SLOTS(PosixCBase) && FD_ARRAY(PosixCBase)[startfd];
            startfd++
        );

        return startfd;
    }

    struct Library *FDBase = PosixCBase->PosixCFDBase;
    for (;;) {
        if (startfd < FD_SLOTS(PosixCBase) && FD_ARRAY(PosixCBase)[startfd]) {
            startfd++;
            continue;
        }
        if (FD_Check(startfd) == 0)
            return startfd;
        startfd++;
    }
}

int __getfdslot(int wanted_fd)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    LONG error;

    /* Hold the table guard across the grow/close below: the realloc+free
       must not run while another Task is indexing the array. close() and
       the FreePooled path re-enter under the same Task, which the
       recursive SignalSemaphore permits. */
    FD_LOCK(PosixCBase);

    if (wanted_fd>=FD_SLOTS(PosixCBase))
    {
        void *tmp;

        /* Note: internalpool is created with MEMF_CLEAR (see __posixc_init.c),
           so AllocPooled() returns zeroed memory - the slots above the copied
           range are already NULL. Do not call memset()/other stdc helpers
           here: __getfdslot() runs during posixc OpenLib (via __init_stdfiles)
           before stdc.library's relbase is available. */
        tmp = AllocPooled(FD_POOL(PosixCBase), (wanted_fd+1)*sizeof(fdesc *));

        if (!tmp) { FD_UNLOCK(PosixCBase); return -1; }

        if (FD_ARRAY(PosixCBase))
        {
            size_t size = FD_SLOTS(PosixCBase)*sizeof(fdesc *);
            CopyMem(FD_ARRAY(PosixCBase), tmp, size);
            FreePooled(FD_POOL(PosixCBase), FD_ARRAY(PosixCBase), size);
        }

        FD_ARRAY(PosixCBase) = tmp;
        FD_SLOTS(PosixCBase) = wanted_fd+1;
    }
    else if (wanted_fd < 0)
    {
        FD_UNLOCK(PosixCBase);
        __set_errno(PosixCBase, EINVAL);
        return -1;
    }
    else if (FD_ARRAY(PosixCBase)[wanted_fd])
    {
        /* The slot is grown above for an out-of-range wanted_fd (which is
           empty, so nothing is closed if that allocation fails); only an
           in-range, occupied slot is closed here.  The FD_Reserve() below then
           re-claims the very fd just freed, which does not realistically fail -
           so dup2()/open() do not lose wanted_fd on the common failure paths.
           A strictly-atomic guarantee would require splitting close() into
           "close file" + "release fd" to retain the reservation across the
           swap; see CRT_REVIEW.md (dup2 ENOMEM edge). */
        close(wanted_fd);
    }

    FD_UNLOCK(PosixCBase);

    /* Standard streams (0..2) stay process-local; do not reserve their
       numbers in the system-wide fd.library table (see __setfdesc()). */
    if (wanted_fd > STDERR_FILENO && __fdlib_available(PosixCBase)) {
        struct Library *FDBase = PosixCBase->PosixCFDBase;
        /* A vfork child inherits the parent's descriptor table (__copy_fdarray);
           those fd numbers are already reserved system-wide by the parent as
           FD_OWNER_POSIXC and are shared, not re-created.  Re-reserving them
           would fail and abort the child's posixc OpenLib (seen as a spurious
           ENOMEM from fork()).  Only reserve numbers not already owned by us. */
        if (FD_GetOwner(wanted_fd) != FD_OWNER_POSIXC) {
            error = FD_Reserve(wanted_fd, FD_OWNER_POSIXC, NULL);
            if (error) {
                __set_errno(PosixCBase, error);
                return -1;
            }
        }
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
    if (flags & O_RDONLY)
        openmode = MODE_OLDFILE;
    if (flags & O_WRONLY)
        openmode = MODE_OLDFILE;
    if (flags & O_RDWR)
        openmode = MODE_OLDFILE;
    if (flags & O_READ)
        openmode = MODE_OLDFILE;
    if (flags & O_WRITE)
        openmode = MODE_READWRITE;
    if (flags & O_CREAT)
        /* Handled later */;
    if (flags & O_APPEND)
        /* Handled later */;
    if (flags & O_TRUNC)
        /* Handled later */;
    if (flags & O_EXEC)
        /* Ignored */;
    if (flags & O_NONBLOCK)
        /* Ignored */;

    return openmode;
}

int __open(int wanted_fd, const char *pathname, int flags, int mode)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    BPTR fh = BNULL, lock = BNULL;
    fdesc *currdesc = NULL;
    int reserved_fd = 0;
    fcb *cblock = NULL;
    struct FileInfoBlock *fib = NULL;
    LONG  openmode = __oflags2amode(flags);

    if (PosixCBase->doupath && pathname[0] == '\0')
    {
        /* On *nix "" is not really a valid file name.  */
        __set_errno(PosixCBase, ENOENT);
        return -1;
    }

    pathname = __path_u2a(pathname);
    if (!pathname) return -1;

    D(bug("__open: entering, wanted fd = %d, path = %s, flags = %d, mode = %d\n", wanted_fd, pathname, flags, mode));

    if (openmode == -1)
    {
        __set_errno(PosixCBase, EINVAL);
        D(bug( "__open: bad mode, exiting with error EINVAL\n"));
        return -1;
    }

    cblock = AllocVec(sizeof(fcb), MEMF_ANY | MEMF_CLEAR);
    if (!cblock) { D(bug("__open: no memory [1]\n")); goto err; }
    InitSemaphore(&cblock->io_lock);
    currdesc = __alloc_fdesc();
    if (!currdesc) { D(bug("__open: no memory [2]\n")); goto err; }
    currdesc->fdflags = 0;
    currdesc->fcb = cblock;

    wanted_fd = __getfdslot(wanted_fd);
    if (wanted_fd == -1) { D(bug("__open: no free fd\n")); goto err; }
    reserved_fd = 1;

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
                __set_errno(PosixCBase, ENOTDIR);
                goto err;
            }

            if
            (
                (IoErr() != ERROR_OBJECT_NOT_FOUND) ||
                /* If the file doesn't exist and the flag O_CREAT is not set return an error */
                (IoErr() == ERROR_OBJECT_NOT_FOUND && !(flags & O_CREAT))
            )
            {
                __set_errno(PosixCBase, __stdc_ioerr2errno(IoErr()));
                goto err;
            }
        }
        else
        {
            /* If the file exists, but O_EXCL is set, then return an error */
            if (flags & O_EXCL)
            {
                __set_errno(PosixCBase, EEXIST);
                goto err;
            }

            fib = AllocDosObject(DOS_FIB, NULL);
            if (!fib)
            {
                __set_errno(PosixCBase, __stdc_ioerr2errno(IoErr()));
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
                    __set_errno(PosixCBase, EISDIR);
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
    else
    {
        /* Handle O_CREAT (creates the file only if it does not exist) */
        if (flags & O_CREAT)
        {
            BPTR tmp = Open((char *)pathname, MODE_NEWFILE);
            if (tmp != BNULL) Close(tmp);
            /* File is closed as O_CREAT does not define access
             * mode. This is handled by R/W modes.
             */
        }
    }

    if (!(fh = Open((char *)pathname, openmode)))
    {
        ULONG ioerr = IoErr();
        D(bug("__open: Open ioerr=%d\n", ioerr));
        __set_errno(PosixCBase, __stdc_ioerr2errno(ioerr));
        goto err;
    }

    /* Allow opening NIL: (/dev/null) regardless of additional flags/modes */
    if (!strcasecmp(pathname, "NIL:"))
        goto success;

    /* Handle O_TRUNC */
    if ((flags & O_TRUNC) && (flags & (O_RDWR | O_WRONLY)))
    {
        if(SetFileSize(fh, 0, OFFSET_BEGINNING) != 0)
        {
            ULONG ioerr = IoErr();
            /* Ignore error if ACTION_SET_FILE_SIZE is not implemented */
            if(ioerr != ERROR_NOT_IMPLEMENTED &&
               ioerr != ERROR_ACTION_NOT_KNOWN)
            {
                D(bug("__open: SetFileSize ioerr=%d\n", ioerr));
                __set_errno(PosixCBase, __stdc_ioerr2errno(ioerr));
                goto err;
            }
        }
    }

    /* Handle O_APPEND */
    if ((flags & O_APPEND) && (flags & (O_RDWR | O_WRONLY)))
    {
        if(Seek(fh, 0, OFFSET_END) != 0) {
            __set_errno(PosixCBase, __stdc_ioerr2errno(IoErr()));
            goto err;
        }
    }


success:
    currdesc->fcb->handle    = fh;
    currdesc->fcb->flags     = flags;
    currdesc->fcb->opencount = 1;

    /* Detect 64-bit filesystem support once, and cache it in the fcb */
    if (!(currdesc->fcb->privflags & _FCB_ISDIR))
        __dos64_probe(currdesc->fcb);

    __setfdesc(wanted_fd, currdesc);

    D(bug("__open: exiting fd=%d\n", wanted_fd));

    return wanted_fd;

err:
    if (fib) FreeDosObject(DOS_FIB, fib);
    FreeVec(cblock);
    if (currdesc) __free_fdesc(currdesc);
    if (fh && fh != lock) Close(fh);
    if (lock) UnLock(lock);
    if (reserved_fd)
        __setfdesc(wanted_fd, NULL);

    D(bug("__open: exiting with error %d\n", errno ));

    return -1;
}

fdesc *__alloc_fdesc(void)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    fdesc * desc;

    desc = AllocPooled(FD_POOL(PosixCBase), sizeof(fdesc));
    if (desc)
        desc->allocpool = FD_POOL(PosixCBase);

    D(bug("Allocated fdesc %x from %x pool\n", desc, FD_POOL(PosixCBase)));

    return desc;
}

void __free_fdesc(fdesc *desc)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    /* A descriptor table can cross posixc instances during vfork/launcher
       handoff; free each fdesc to the pool that actually allocated it. */
    APTR pool = desc->allocpool ? desc->allocpool : FD_POOL(PosixCBase);

    D(bug("Freeing fdesc %x from %x pool\n", desc, pool));
    FreePooled(pool, desc, sizeof(fdesc));
}

static void stderrlogic(struct Process *me, fcb *fcb)
{
    if ((fcb->handle != BNULL) && !(fcb->privflags & _FCB_DONTCLOSE_FH))
        Close(fcb->handle);
    if (me->pr_CES != BNULL)
    {
        fcb->handle = me->pr_CES;
        fcb->privflags |= _FCB_DONTCLOSE_FH;
    }
    else
    {
        fcb->handle = Open("NIL:", MODE_OLDFILE);
        fcb->privflags &= ~_FCB_DONTCLOSE_FH;
    }
    /* stderr is expected to be unbuffered for POSIX. */
    SetVBuf(fcb->handle, NULL, BUF_NONE, -1);
}

/* FIXME: perhaps this has to be handled in a different way...  */
int __init_stdfiles(struct PosixCIntBase *PosixCBase)
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

    me = (struct Process *)FindTask (NULL);

    infcb->handle = Input();
    InitSemaphore(&infcb->io_lock);
    InitSemaphore(&outfcb->io_lock);
    InitSemaphore(&errfcb->io_lock);

    infcb->flags  = O_RDONLY;
    infcb->opencount = 1;
    /* Remove (remaining) command line args on first read */
    infcb->privflags = _FCB_FLUSHONREAD;
    infcb->privflags |= _FCB_DONTCLOSE_FH;
    indesc->fcb = infcb;
    indesc->fdflags = 0;
    D(bug("[__init_stdfiles]Input(): %p, infcb->handle: %p\n",
          BADDR(Input()), BADDR(infcb->handle)
    ));

    outfcb->handle = Output();
    outfcb->flags = O_WRONLY | O_APPEND;
    outfcb->opencount = 1;
    outfcb->privflags |= _FCB_DONTCLOSE_FH;
    outdesc->fcb = outfcb;
    outdesc->fdflags = 0;
    D(bug("[__init_stdfiles]Output(): %p, outfcb->handle: %p\n",
          BADDR(Output()), BADDR(outfcb->handle)
    ));

    errfcb->privflags = 0;
    stderrlogic(me, errfcb);
    errfcb->flags = O_WRONLY | O_APPEND;
    errfcb->opencount = 1;
    errdesc->fcb = errfcb;
    errdesc->fdflags = 0;
    D(bug("[__init_stdfiles]me->pr_CES: %p, errfcb->handle: %p\n",
          BADDR(me->pr_CES), BADDR(errfcb->handle)
    ));

    /* Std streams may be redirected to files; detect 64-bit support */
    __dos64_probe(infcb);
    __dos64_probe(outfcb);
    __dos64_probe(errfcb);

    FD_ARRAY(PosixCBase)[STDIN_FILENO]  = indesc;
    FD_ARRAY(PosixCBase)[STDOUT_FILENO] = outdesc;
    FD_ARRAY(PosixCBase)[STDERR_FILENO] = errdesc;

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

            if((FD_ARRAY(PosixCBase)[i] = __alloc_fdesc()) == NULL)
                return 0;

            FD_ARRAY(PosixCBase)[i]->fdflags = __src_fd_array[i]->fdflags;
            FD_ARRAY(PosixCBase)[i]->fcb = __src_fd_array[i]->fcb;
            /* The source (parent) may still be using this fcb - bump its
               shared reference count atomically. */
            __fcb_lock(FD_ARRAY(PosixCBase)[i]->fcb);
            FD_ARRAY(PosixCBase)[i]->fcb->opencount++;
            __fcb_unlock(FD_ARRAY(PosixCBase)[i]->fcb);
        }
    }

    return 1;
}

int __init_fd(struct PosixCIntBase *PosixCBase)
{
    struct PosixCIntBase *pPosixCBase =
        (struct PosixCIntBase *)__GM_GetBaseParent(PosixCBase);
    struct Task *task = FindTask(NULL);

    D(bug("Found parent PosixCBase %p with flags 0x%x\n",
          pPosixCBase, pPosixCBase ? pPosixCBase->flags : 0
    ));

    /* Initialise this base's own table lock up front, before any descriptor
       call can reach FD_LOCK: the owner path's __init_stdfiles() below takes
       it via __getfdslot(), and a borrower's later fdopen() (from
       __init_stdio, a lower OpenLib priority that runs afterwards) takes it
       via __getfdesc().  A borrower normally locks the owner's lock through
       __fd_owner(), leaving its own unused - but every base keeping its own
       initialised means a lookup that resolves to self (a borrower whose
       owner is not yet resolved) never obtains an uninitialised semaphore.
       This was the "ObtainSemaphore called on a not initialized semaphore"
       seen for pthread workers under load. */
    InitSemaphore(&PosixCBase->fd_sem);

    if (pPosixCBase &&
        !(pPosixCBase->flags & (VFORK_PARENT | EXEC_PARENT)) &&
        (task->tc_Node.ln_Type == NT_TASK ||
         (task->tc_Node.ln_Type == NT_PROCESS &&
          ((struct Process *)task)->pr_CLI == BNULL)))
    {
        /*
         * pthread workers are implemented by CreateNewProcTags(NP_Entry) and
         * therefore appear as CLI-less Exec Processes (some alternate thread
         * implementations use plain Tasks).  They receive a distinct per-task
         * posixc.library base, but the POSIX file descriptors belong to the
         * creator Process.  Route descriptor lookups and allocation through
         * the creator's owning base while retaining this task's own errno,
         * stdio, signal mask and other per-base state.
         *
         * A CLI child (a vfork launcher runs with NP_Cli=TRUE) takes the
         * independent table path below.
         */
        PosixCBase->fd_owner = __fd_owner(pPosixCBase);
        return TRUE;
    }

    PosixCBase->fd_owner = PosixCBase;

    if (pPosixCBase && (pPosixCBase->flags & (VFORK_PARENT | EXEC_PARENT)))
    {
        /* VFORK_PARENT use case - child manipulates file descriptors prior to calling exec* */
        int res = __copy_fdarray(pPosixCBase->fd_array, pPosixCBase->fd_slots);

        if (pPosixCBase->flags & EXEC_PARENT)
            /* EXEC_PARENT called through RunCommand which injected parameters to Input() */
            FD_ARRAY(PosixCBase)[STDIN_FILENO]->fcb->privflags |= _FCB_FLUSHONREAD;

        return res;
    }
    else
        return __init_stdfiles(PosixCBase);
}

void __exit_fd(struct PosixCIntBase *PosixCBase)
{
    int i;

    /* A pthread worker shares its creator Process' descriptor table; the
       creator owns and closes it, so a worker must not tear it down. */
    if (__fd_owner(PosixCBase) != PosixCBase)
        return;

    i = FD_SLOTS(PosixCBase);
    while (i)
    {
        if (FD_ARRAY(PosixCBase)[--i])
            close(i);
    }
}

void __close_on_exec_fdescs(void)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int i;
    fdesc *fd;

    for (i = FD_SLOTS(PosixCBase) - 1; i >= 0; i--)
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

    fcb = FD_ARRAY(PosixCBase)[STDIN_FILENO]->fcb;
    fcb->handle = Input();

    fcb = FD_ARRAY(PosixCBase)[STDOUT_FILENO]->fcb;
    fcb->handle = Output();

    fcb = FD_ARRAY(PosixCBase)[STDERR_FILENO]->fcb;
    stderrlogic(me, fcb);
}

ADD2OPENLIB(__init_fd, 2);
ADD2CLOSELIB(__exit_fd, 2);
