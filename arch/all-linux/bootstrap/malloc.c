/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

/*
 * AROS-thread-safe versions of libc memory allocation routines
 * This does not work with Android's bionic.
 */

/*
    Explanation:

    Forbid-locking is  needed because AROS must protect calls to Linux memory
    allocation functions against reentrancy. Linux malloc's pthread mutex
    protection (or whatever it uses) will not work, because that one protects
    Linux thread #1 against other Linux threads. However AROS tasks are all
    inside the same Linux thread.

    So if AROS task #1 is in the middle of Linux malloc, AROS task #2 may end
    up doing Linux malloc call as well and when it hits Linux malloc's
    pthread mutex (or whatever protection), that one will see that lock is being
    held, but by same thread. If the used Linux locking allows nesting it causes
    havoc (2 malloc's running at the same time). If it doesn't allow nesting,
    it will hang.

    The locking is only valid inside the AROS Linux thread. Other threads within
    AROS process do not need it, because it will be handled by Linux malloc's
    pthread mutex. Moreover, calling Forbid() from multiple Linux threads will
    cause the TDNestCnt to become damaged.

    TDNestCnt is "per task" and while other Linux thread calls Forbid() the AROS
    Linux thread may be running AROS task #1. Then when that other Linux thread
    does matching Permit(), AROS Linux thread may have already switched to
    another AROS task. So the effect is like AROS task #1 calling Forbid() and
    AROS task #2 calling Permit().
*/

/* TODO:
   This code is compiled with the kernel compiler but calls code
   from exec.library. This can only work if function calling
   convention for kernel and target compiler are the same.
   Up to now this seems to be the case for all linux hosted ports
   as UNIX calling is often taken as reference for the AROS
   implementation.
*/

#ifndef __ANDROID__

#include <proto/exec.h>
#include <sys/types.h>

#include <unistd.h>
#include <syscall.h>

static int memnest;
extern pid_t arostid;

#define THREADID    pid_t thistid = syscall(SYS_gettid);
#define MEMLOCK     if (SysBase != NULL && thistid == arostid) Forbid();
#define MEMUNLOCK   if (SysBase != NULL && thistid == arostid) Permit();

extern struct ExecBase *SysBase;
extern void * __libc_malloc(size_t);
extern void __libc_free(void *);
extern void * __libc_calloc(size_t, size_t);
extern void * __libc_realloc(void * mem, size_t newsize);

void * malloc(size_t size)
{
    void *res;
    THREADID

    MEMLOCK
    memnest++;
    res = __libc_malloc(size);
    memnest--;
    MEMUNLOCK

    return res;
}

void free(void * addr)
{
    THREADID

    MEMLOCK
    memnest++;
    __libc_free(addr);
    memnest--;
    MEMUNLOCK
}

void * calloc(size_t n, size_t size)
{
    void *res;
    THREADID

    MEMLOCK
    memnest++;
    res = __libc_calloc(n, size);
    memnest--;
    MEMUNLOCK
    
    return res;
}

void *realloc(void *ptr, size_t size)
{
    void *res;
    THREADID

    MEMLOCK
    memnest++;
    res = __libc_realloc(ptr, size);
    memnest--;
    MEMUNLOCK;
    
    return res;
}

#endif
