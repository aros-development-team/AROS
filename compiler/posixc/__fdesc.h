#ifndef ___FDESC_H
#define ___FDESC_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: file descriptors handling internals - header file
    Lang: english
*/

#include <dos/dos.h>

/* file control block - one per file handle */
typedef struct _fcb
{
    BPTR handle;// if privflags has flag _FCB_ISDIR: BCPL pointer to struct FileLock  
                // otherwise: BCPL pointer to struct FileHandle
    int  flags;
    unsigned int opencount;
    unsigned int privflags;
} fcb;

/* privflags */
#define _FCB_ISDIR        ((unsigned int)1<<0)
#define _FCB_DONTCLOSE_FH ((unsigned int)1<<1)
#define _FCB_FLUSHONREAD  ((unsigned int)1<<2)

#define FLUSHONREADCHECK                                                \
    if (__builtin_expect(fdesc->fcb->privflags & _FCB_FLUSHONREAD, 0))  \
    {                                                                   \
        fdesc->fcb->privflags &= ~_FCB_FLUSHONREAD;                     \
        Flush(fdesc->fcb->handle);                                      \
    }

/* file descriptor structure - one per descriptor */
typedef struct _fdesc
{
    fcb  *fcb;
    int  fdflags;
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

#endif /* ___FDESC_H */
