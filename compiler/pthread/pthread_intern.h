/*
  Copyright (C) 2014 Szilard Biro

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

#include <setjmp.h>

#ifdef __AROS__
#include <aros/symbolsets.h>
#define    TIMESPEC_TO_TIMEVAL(tv, ts) {    \
    (tv)->tv_sec = (ts)->tv_sec;        \
    (tv)->tv_usec = (ts)->tv_nsec / 1000; }
#else
#include <constructor.h>
#define StackSwapArgs PPCStackSwapArgs
#define NewStackSwap NewPPCStackSwap
#endif

#include "pthread.h"

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
    ULONG sigmask;
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
} ThreadInfo;

extern ThreadInfo threads[PTHREAD_THREADS_MAX];
extern struct SignalSemaphore thread_sem;
extern TLSKey tlskeys[PTHREAD_KEYS_MAX];
extern struct SignalSemaphore tls_sem;

/* .c */
extern pthread_t GetThreadId(struct Task *task);
extern ThreadInfo *GetThreadInfo(pthread_t thread);
extern int SemaphoreIsInvalid(struct SignalSemaphore *sem);
extern int SemaphoreIsMine(struct SignalSemaphore *sem);

/* .c */
extern int _pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime, BOOL relative);

/* .c */
extern int _pthread_cond_broadcast(pthread_cond_t *cond, BOOL onlyfirst);

/* .c */
extern int _pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr, BOOL staticinit);
