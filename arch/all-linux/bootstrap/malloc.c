/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * AROS-thread-safe versions of libc memory allocation routines
 * This does not work with Android's bionic.
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
