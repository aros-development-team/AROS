/*
  Copyright (C) 2014 Szilard Biro
  Copyright (C) 2018 Harry Sintonen
  Copyright (C) 2019 Stefan "Bebbo" Franke - AmigaOS 3 port

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifdef __MORPHOS__
#include <sys/time.h>
#endif
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#ifdef __AROS__
#include <aros/symbolsets.h>
#define    TIMESPEC_TO_TIMEVAL(tv, ts) {    \
    (tv)->tv_sec = (ts)->tv_sec;        \
    (tv)->tv_usec = (ts)->tv_nsec / 1000; }
#elif !defined(__AMIGA__) || defined(__MORPHOS__)
#include <constructor.h>
#define StackSwapArgs PPCStackSwapArgs
#define NewStackSwap NewPPCStackSwap
#endif

#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "pthread.h"
#include "debug.h"

#if defined(__AMIGA__) && !defined(__MORPHOS__)
#include <exec/execbase.h>
#include <inline/alib.h>
#define NEWLIST(a) NewList(a)

#include <stabs.h>

#ifndef IPTR
#define IPTR ULONG
#endif

#   define ForeachNode(l,n) \
    for (n=(void *)(((struct List *)(l))->lh_Head); \
        ((struct Node *)(n))->ln_Succ; \
        n=(void *)(((struct Node *)(n))->ln_Succ))

#define GET_THIS_TASK SysBase->ThisTask;

#else

#define GET_THIS_TASK FindTask(NULL)

#endif


#define SIGB_PARENT SIGBREAKB_CTRL_F
#define SIGF_PARENT (1 << SIGB_PARENT)
#define SIGB_COND_FALLBACK SIGBREAKB_CTRL_E
#define SIGF_COND_FALLBACK (1 << SIGB_COND_FALLBACK)
#define SIGB_TIMER_FALLBACK SIGBREAKB_CTRL_D
#define SIGF_TIMER_FALLBACK (1 << SIGB_TIMER_FALLBACK)

#define NAMELEN 32
#define PTHREAD_FIRST_THREAD_ID (1)
#define PTHREAD_BARRIER_FLAG (1UL << 31)

typedef struct
{
    struct MinNode node;
    struct Task *task;
    UBYTE sigbit;
} CondWaiter;

typedef struct
{
    void (*destructor)(void *);
    BOOL used;
} TLSKey;

typedef struct
{
    struct MinNode node;
    void (*routine)(void *);
    void *arg;
} CleanupHandler;

typedef struct
{
    void *(*start)(void *);
    void *arg;
    struct Task *parent;
    struct Task *waiter;
    int finished;
    struct Task *task;
    void *ret;
    jmp_buf jmp;
    pthread_attr_t attr;
    void *tlsvalues[PTHREAD_KEYS_MAX];
    struct MinList cleanup;
    int cancelstate;
    int canceltype;
    int canceled;
    int detached;
} ThreadInfo;

extern ThreadInfo threads[PTHREAD_THREADS_MAX];
extern struct SignalSemaphore thread_sem;
extern TLSKey tlskeys[PTHREAD_KEYS_MAX];
extern struct SignalSemaphore tls_sem;

/* .c */
extern int SemaphoreIsInvalid(struct SignalSemaphore *sem);
extern int SemaphoreIsMine(struct SignalSemaphore *sem);
extern ThreadInfo *GetThreadInfo(pthread_t thread);
extern pthread_t GetThreadId(struct Task *task);

/* .c */
extern int _pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime, BOOL relative);

/* .c */
extern int _pthread_cond_broadcast(pthread_cond_t *cond, BOOL onlyfirst);

/* .c */
extern int _pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr, BOOL staticinit);

/* .c */
extern BOOL OpenTimerDevice(struct IORequest *io, struct MsgPort *mp, struct Task *task);
extern void CloseTimerDevice(struct IORequest *io);
