#ifndef ___FDESC_H
#define ___FDESC_H

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: file descriptors handling internals - header file
    Lang: english
*/

#include <dos/dos.h>
#include <exec/semaphores.h>

/* file control block - one per file handle */
typedef struct _fcb
{
    BPTR handle;// if privflags has flag _FCB_ISDIR: BCPL pointer to struct FileLock
                // otherwise: BCPL pointer to struct FileHandle
    int  flags;
    unsigned int opencount;
    unsigned int privflags;
    /* Serialises DOS operations on the shared handle. A single fcb (one DOS
       FileHandle) is shared between descriptors that are dup'd, inherited
       across vfork()/exec*(), or reached from pthread workers through the
       creator's descriptor table. DOS FileHandles carry buffer/position
       state and are not safe for concurrent use, so read/write serialise
       on this. Initialised at fcb creation. */
    struct SignalSemaphore io_lock;
} fcb;

#include <proto/exec.h>
/* Serialise a whole POSIX/stdio operation against concurrent use of the
   same shared DOS handle. Recursive (SignalSemaphore), so a syscall may
   hold it across a sequence that itself calls the __dos64_* primitives
   (which also lock). A NULL/absent handle needs no serialisation. */
static inline void __fcb_lock(fcb *f)   { if (f) ObtainSemaphore(&f->io_lock); }
static inline void __fcb_unlock(fcb *f) { if (f) ReleaseSemaphore(&f->io_lock); }

/* privflags */
#define _FCB_ISDIR        ((unsigned int)1<<0)
#define _FCB_DONTCLOSE_FH ((unsigned int)1<<1)
#define _FCB_FLUSHONREAD  ((unsigned int)1<<2)
#define _FCB_CONSOLERAW   ((unsigned int)1<<3)
#define _FCB_FH64         ((unsigned int)1<<14)
#define _FCB_FS64         ((unsigned int)1<<15)

#define FLUSHONREADCHECK                                                \
    if (__builtin_expect(fdesc->fcb->privflags & _FCB_FLUSHONREAD, 0))  \
    {                                                                   \
        fdesc->fcb->privflags &= ~_FCB_FLUSHONREAD;                     \
        __fcb_lock(fdesc->fcb);                                         \
        Flush(fdesc->fcb->handle);                                      \
        __fcb_unlock(fdesc->fcb);                                       \
    }

/* file descriptor structure - one per descriptor */
typedef struct _fdesc
{
    fcb  *fcb;
    int  fdflags;
    APTR allocpool;   /* pool this fdesc was allocated from; a descriptor
                         table can cross posixc.library instances during
                         vfork/launcher handoff, so free it to the allocator
                         it came from rather than the current base's pool. */
} fdesc;

struct PosixCIntBase;
int __getfdslots(void);
void __getfdarray(APTR *arrayptr, int *slotsptr);
void __setfdarray(APTR array, int slots);
void __setfdarraybase(struct PosixCIntBase *PosixCBase2);
fdesc *__getfdesc(register int fd);
void __setfdesc(register int fd, fdesc *fdesc);
int __getfdslot(int wanted_fd);
int __getfirstfd(register int startfd);
int __open(int wanted_fd, const char *pathname, int flags, int mode);
void __updatestdio(void);
LONG __oflags2amode(int flags);
fdesc *__alloc_fdesc(void);
void __free_fdesc(fdesc *fdesc);
void __close_on_exec_fdescs(void);

/* Resolve the operation hooks for a descriptor owned by another subsystem
   (e.g. a bsdsocket.library socket) through fd.library.  Returns the hooks
   and, via datap, the owner data to act on; returns NULL for a plain posixc
   file descriptor (handled locally) or when fd.library is unavailable. */
struct fd_hooks;
const struct fd_hooks *__getfdhooks(int fd, APTR *datap);

#endif /* ___FDESC_H */
