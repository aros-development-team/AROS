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

#ifndef PTHREAD_H
#define PTHREAD_H

#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <exec/types.h>
#include <exec/semaphores.h>
#include <sched.h>

//
// Basic types
//

/*#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
struct timespec
{
    long tv_sec;
    long tv_nsec;
};
#endif*/

#ifndef _SCHED_PARAM_DEFINED
#define _SCHED_PARAM_DEFINED
struct sched_param
{
    int sched_priority;
};
#endif

#ifndef SCHED_RR
#define SCHED_RR 0
#endif

//
// POSIX options
//

#define _POSIX_THREADS
#define _POSIX_READER_WRITER_LOCKS
#define _POSIX_SPIN_LOCKS
#define _POSIX_BARRIERS
#define _POSIX_THREAD_SAFE_FUNCTIONS
#define _POSIX_THREAD_ATTR_STACKSIZE
#define _POSIX_THREAD_PRIORITY_SCHEDULING

//
// POSIX limits
//

#define PTHREAD_KEYS_MAX                      64
#define PTHREAD_STACK_MIN                     40960
#define PTHREAD_THREADS_MAX                   2019

//
// POSIX pthread types
//

typedef unsigned int pthread_t;
typedef unsigned int pthread_key_t;

//
// POSIX thread attribute values
//

#define PTHREAD_CREATE_JOINABLE       0
#define PTHREAD_CREATE_DETACHED       1

#define PTHREAD_INHERIT_SCHED         0
#define PTHREAD_EXPLICIT_SCHED        1

#define PTHREAD_SCOPE_PROCESS         0
#define PTHREAD_SCOPE_SYSTEM          1

#define PTHREAD_CANCEL_ENABLE         0
#define PTHREAD_CANCEL_DISABLE        1

#define PTHREAD_CANCEL_ASYNCHRONOUS   0
#define PTHREAD_CANCEL_DEFERRED       1

#define PTHREAD_PROCESS_PRIVATE       0
#define PTHREAD_PROCESS_SHARED        1

//
// Threads
//

struct pthread_attr
{
    void *stackaddr;
    size_t stacksize;
    int detachstate;
    struct sched_param param;
    int inheritsched;
    int contentionscope;
};

typedef struct pthread_attr pthread_attr_t;

//
// Once key
//

struct pthread_once
{
    volatile int done;        // Indicates if user function executed
    int started;            // First thread to increment this value 
                            // to zero executes the user function
};

typedef struct pthread_once pthread_once_t;

#define PTHREAD_ONCE_INIT       {0, -1}

//
// Mutex
//

#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_RECURSIVE  1
//#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_DEFAULT    PTHREAD_MUTEX_NORMAL

struct pthread_mutexattr
{
    int pshared;
    int kind;
};

typedef struct pthread_mutexattr pthread_mutexattr_t;

struct pthread_mutex
{
    int kind;
    struct SignalSemaphore semaphore;
};

typedef struct pthread_mutex pthread_mutex_t;

#define NULL_MINLIST {0, 0, {0}}
#define NULL_MINNODE {0, 0}
#define NULL_NODE {0, 0, 0, 0, 0}
#define NULL_SEMAPHOREREQUEST {NULL_MINNODE, 0}
#define NULL_SEMAPHORE {NULL_NODE, 0, NULL_MINLIST, NULL_SEMAPHOREREQUEST, 0, 0}

#define PTHREAD_MUTEX_INITIALIZER {PTHREAD_MUTEX_NORMAL, NULL_SEMAPHORE}
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER {PTHREAD_MUTEX_RECURSIVE, NULL_SEMAPHORE}

//
// Condition variables
//

struct pthread_condattr
{
    int pshared;
};

typedef struct pthread_condattr pthread_condattr_t;

struct pthread_cond
{
    int waiting;
    struct SignalSemaphore semaphore;
    struct MinList waiters;
};

typedef struct pthread_cond pthread_cond_t;

#define PTHREAD_COND_INITIALIZER {0, NULL_SEMAPHORE, NULL_MINLIST}

//
// POSIX thread routines
//

#ifdef  __cplusplus
extern "C" {
#endif

//
// Thread attribute functions
//

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getstack(const pthread_attr_t *attr, void **stackaddr, size_t *stacksize);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getschedparam(const pthread_attr_t *attr, struct sched_param *param);
int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param);
int pthread_attr_getschedpolicy(pthread_attr_t *attr, int *policy);
int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
int pthread_attr_getinheritsched(pthread_attr_t *attr, int *inheritsched);
int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched);
int pthread_attr_getscope(const pthread_attr_t *attr, int *contentionscope);
int pthread_attr_setscope(pthread_attr_t *attr, int contentionscope);

//
// Thread functions
//

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg);
int pthread_detach(pthread_t thread);
int pthread_equal(pthread_t t1, pthread_t t2);
void pthread_exit(void *value_ptr);
int pthread_join(pthread_t thread, void **value_ptr);
pthread_t pthread_self(void);
int pthread_cancel(pthread_t thread);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

//
// Scheduling functions
//

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param);
int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param);
int pthread_setconcurrency(int level);
int pthread_getconcurrency(void);

//
// Thread specific data functions
//

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);

//
// Mutex attribute functions
//

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int *pshared);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
int pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *kind);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind);

//
// Mutex functions
//

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

//
// Condition variable functions
//

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

//
// NP
//

int pthread_setname_np(pthread_t thread, const char *name);
int pthread_getname_np(pthread_t thread, char *name, size_t len);

//
// Cancellation cleanup
//

void pthread_cleanup_push(void (*routine)(void *), void *arg);
void pthread_cleanup_pop(int execute);

//
// Signalling
//

int pthread_kill(pthread_t thread, int sig);

#ifdef  __cplusplus
}
#endif

#endif
