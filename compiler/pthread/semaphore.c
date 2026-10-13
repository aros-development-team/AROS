/*
    Copyright (C) 2015 Szilard Biro
    Copyright (C) 2018 Harry Sintonen

    This software is provided 'as-is', without any express or implied
    warranty. In no event will the authors be held liable for any damages
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

#include <proto/exec.h>

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "semaphore.h"
#include "debug.h"

#if defined(__AMIGA__)
#include <exec/execbase.h>
#include <inline/alib.h>
#define NEWLIST(a) NewList(a)
#endif

#ifndef EOVERFLOW
#define EOVERFLOW EINVAL
#endif

static struct List semaphores;
static struct SignalSemaphore sema_sem;
static pthread_once_t once_control = PTHREAD_ONCE_INIT;

static void _Init_Semaphore(void)
{
    DB2(bug("%s()\n", __FUNCTION__));

    InitSemaphore(&sema_sem);
    NEWLIST(&semaphores);
}

/****************************************************************************
    NAME */
        sem_t *sem_open(

/*  SYNOPSIS */
        const char *name, int oflag, mode_t mode, unsigned int value)

/*  FUNCTION
        Look up or create a named semaphore in the pthread library's local
        registry.

    INPUTS
        name - Non-NULL NUL-terminated semaphore name.
        oflag - Open flags; O_CREAT requests creation, O_EXCL with O_CREAT
                rejects an existing name.
        mode - Creation permissions; accepted but not used.
        value - Initial count for a newly created semaphore; must not exceed
                SEM_VALUE_MAX.

    RESULT
        Pointer to the named sem_t on success; SEM_FAILED with errno=EINVAL,
        EEXIST, ENOENT, ENOMEM, or an error from sem_init() on failure.

    NOTES
        Initializes a process-local linked list once, searches by FindName(),
        and allocates a named object only when absent and O_CREAT is set.
        Existing objects retain their initial count. The returned address is
        borrowed from the registry, not a separately owned handle.

    BUGS
        No cross-process namespace, permissions enforcement, reference
        counting, or POSIX open-instance ownership is implemented. sem_close()
        is a no-op; sem_unlink() can immediately invalidate previously
        returned pointers.

    SEE ALSO
        sem_close()
        sem_unlink()
        sem_init()

*****************************************************************************/
{
    sem_t *sem;

    D(bug("%s(%s, %d, %u, %u)\n", __FUNCTION__, name, oflag, mode, value));

    pthread_once(&once_control, _Init_Semaphore);

    if (name == NULL)
    {
        errno = EINVAL;
        return SEM_FAILED;
    }

    ObtainSemaphore(&sema_sem);
    sem = (sem_t *)FindName(&semaphores, (STRPTR)name);
    if (sem != NULL)
    {
        if ((oflag & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
        {
            ReleaseSemaphore(&sema_sem);
            errno = EEXIST;
            return SEM_FAILED;
        }
    }
    else
    {
        if (!(oflag & O_CREAT))
        {
            ReleaseSemaphore(&sema_sem);
            errno = ENOENT;
            return SEM_FAILED;
        }

        sem = malloc(sizeof(sem_t) + strlen(name) + 1);
        if (sem == NULL)
        {
            ReleaseSemaphore(&sema_sem);
            errno = ENOMEM;
            return SEM_FAILED;
        }

        if (sem_init(sem, 0, value))
        {
            free(sem);
            ReleaseSemaphore(&sema_sem);
            return SEM_FAILED;
        }
        sem->node.ln_Name = (char *) (sem + 1);
        strcpy(sem->node.ln_Name, name);
        AddTail(&semaphores, (struct Node *)sem);
    }
    ReleaseSemaphore(&sema_sem);

    return sem;
}

/****************************************************************************
    NAME */
        int sem_close(

/*  SYNOPSIS */
        sem_t *sem)

/*  FUNCTION
        Return from a named-semaphore close request without changing registry
        state.

    INPUTS
        sem - Semaphore pointer; not inspected by the implementation.

    RESULT
        Always returns 0, including for NULL and invalid pointers.

    NOTES
        This is a no-op. It neither frees the object, releases a reference,
        nor unlinks its name.

    BUGS
        The POSIX reference-counting and close lifecycle are not implemented.
        A success result is not evidence that resources were released.

    SEE ALSO
        sem_open()
        sem_unlink()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, sem));

    return 0;
}

/****************************************************************************
    NAME */
        int sem_unlink(

/*  SYNOPSIS */
        const char *name)

/*  FUNCTION
        Remove a semaphore name and free its registry object immediately.

    INPUTS
        name - Non-NULL name of an object previously created through
               sem_open().

    RESULT
        0 if the object is destroyed and removed; -1 with errno=EINVAL for
        NULL, ENOENT for an unknown name, or an error propagated by
        sem_destroy().

    NOTES
        Looks up the object in the library-local list, calls sem_destroy(),
        removes its list node, and frees its allocation. All pointers returned
        by sem_open() for that name then become invalid.

    BUGS
        Unlike POSIX deferred unlink semantics, this implementation does not
        retain an object while it has open users. Synchronize all consumers
        before unlinking; concurrent semaphore operations can use freed
        memory.

    SEE ALSO
        sem_open()
        sem_close()
        sem_destroy()

*****************************************************************************/
{
    sem_t *sem;

    D(bug("%s(%s)\n", __FUNCTION__, name));

    pthread_once(&once_control, _Init_Semaphore);

    if (name == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    ObtainSemaphore(&sema_sem);
    sem = (sem_t *)FindName(&semaphores, (STRPTR)name);

    if (sem == NULL)
    {
        ReleaseSemaphore(&sema_sem);
        errno = ENOENT;
        return -1;
    }

    if (sem_destroy(sem) != 0)
    {
        ReleaseSemaphore(&sema_sem);
        return -1;
    }

    Remove((struct Node *)sem);
    free(sem);
    ReleaseSemaphore(&sema_sem);

    return 0;
}

/****************************************************************************
    NAME */
        int sem_init(

/*  SYNOPSIS */
        sem_t *sem,
        int pshared,
        unsigned int value)

/*  FUNCTION
        Initialize an unnamed counting semaphore in caller-provided storage.
        Set its available count to value and initialize the internal mutex and
        condition variable used to wait for a positive count.

    INPUTS
        sem - Non-NULL writable semaphore storage that is not already in use.
        pshared - Use 0 for threads in the same pthread runtime. The argument
            is ignored; nonzero does not enable process sharing.
        value - Initial count in 0..SEM_VALUE_MAX.

    RESULT
        0 on success. -1 with errno set to EINVAL if sem is NULL or value exceeds
        SEM_VALUE_MAX. Validation failure leaves the object unchanged.

    NOTES
        Include <semaphore.h>; link with pthread. No separate semaphore storage
        is allocated. Initialize before publishing to other tasks, retain the
        object while any operation can use it, and destroy it only after all
        users have finished. Reinitializing an active semaphore is invalid.
        The semaphore must not be copied while in use.

    BUGS
        Process-shared operation is not implemented or rejected. Successful
        initialization with nonzero pshared does not establish that capability.

    SEE ALSO
        sem_wait(),
        sem_post(),
        sem_destroy()

*****************************************************************************/
{
    D(bug("%s(%p, %d, %u)\n", __FUNCTION__, sem, pshared, value));

    if (sem == NULL || value > (unsigned int)SEM_VALUE_MAX)
    {
        errno = EINVAL;
        return -1;
    }

    sem->value = value;
    sem->waiters_count = 0;
    pthread_mutex_init(&sem->lock, NULL);
    pthread_cond_init(&sem->count_nonzero, NULL);

    return 0;
}

/****************************************************************************
    NAME */
        int sem_destroy(

/*  SYNOPSIS */
        sem_t *sem)

/*  FUNCTION
        Attempt to destroy an unused unnamed semaphore's internal mutex and
        condition variable, then clear its count and waiter count.

    INPUTS
        sem - Initialized semaphore with no active operations or waiters.
            Prevent all new operations before destroying or releasing storage.

    RESULT
        0 after the internal destruction calls. -1 with errno EINVAL if sem is
        NULL, or EBUSY if its internal mutex cannot be acquired immediately.

    NOTES
        Include <semaphore.h>; link with pthread. The ownership probe does not
        wait. The caller's storage is not freed. Reinitialize before reuse.
        Do not use this function to cancel or wake waiting tasks.

    BUGS
        Return values from pthread_mutex_destroy() and pthread_cond_destroy()
        are ignored. A sleeping waiter can leave the mutex available for the
        probe, so destruction can return 0 despite busy internal objects.
        Success does not establish that waiting tasks are gone. Quiescence
        must be established by the application before this call.

    SEE ALSO
        sem_init(),
        sem_wait(),
        sem_post()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, sem));

    if (sem == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (pthread_mutex_trylock(&sem->lock) != 0)
    {
        errno = EBUSY;
        return -1;
    }

    pthread_mutex_unlock(&sem->lock);
    pthread_mutex_destroy(&sem->lock);
    pthread_cond_destroy(&sem->count_nonzero);
    sem->value = sem->waiters_count = 0;

    return 0;
}

/****************************************************************************
    NAME */
        int sem_trywait(

/*  SYNOPSIS */
        sem_t *sem)

/*  FUNCTION
        Attempt to decrement a semaphore when a positive count is available.

    INPUTS
        sem - Initialized semaphore.

    RESULT
        0 after decrement; -1 with errno=EINVAL for NULL or errno=EAGAIN if
        the count is zero.

    NOTES
        Protects the count with the semaphore's embedded pthread mutex. A
        successful call consumes one permit without notifying other waiters.

    BUGS
        Although the count test is nonblocking, pthread_mutex_lock() can wait
        for the embedded mutex. The function is therefore not guaranteed to
        return immediately under contention.

    SEE ALSO
        sem_wait()
        sem_timedwait()
        sem_post()

*****************************************************************************/
{
    int result = 0;

    D(bug("%s(%p)\n", __FUNCTION__, sem));

    if (sem == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&sem->lock);

    if (sem->value > 0)
        sem->value--;
    else
        result = EAGAIN;

    pthread_mutex_unlock(&sem->lock);

    if (result != 0)
    {
        errno = result;
        return -1;
    }

    return 0;
}

/****************************************************************************
    NAME */
        int sem_timedwait(

/*  SYNOPSIS */
        sem_t *sem, const struct timespec *abstime)

/*  FUNCTION
        Wait for and consume one semaphore permit, subject to an absolute
        deadline.

    INPUTS
        sem - Initialized semaphore.
        abstime - Absolute deadline forwarded to pthread_cond_timedwait();
                  NULL requests an unlimited wait.

    RESULT
        0 after decrement; -1 on error, with errno=EINVAL for NULL or the
        status returned by the condition wait, including ETIMEDOUT.

    NOTES
        Acquires the embedded mutex, increments waiters_count, loops until a
        permit arrives or the condition wait fails, and then decrements the
        waiter count. The default condition clock is CLOCK_REALTIME.

    BUGS
        No upfront abstime validation is performed. The implementation enters
        the timed condition wait only while value is zero; cancellation during
        that wait needs particular care because waiter accounting and mutex
        cleanup are not protected by a cleanup handler.

    SEE ALSO
        sem_wait()
        sem_trywait()
        sem_post()

*****************************************************************************/
{
    int result = 0;

    D(bug("%s(%p, %p)\n", __FUNCTION__, sem, abstime));

    if (sem == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&sem->lock);

    sem->waiters_count++;

    while (sem->value == 0 && result == 0)
        result = pthread_cond_timedwait(&sem->count_nonzero, &sem->lock, abstime);

    sem->waiters_count--;

    if (result != 0)
    {
        pthread_mutex_unlock(&sem->lock);
        errno = result;
        return -1;
    }

    sem->value--;

    pthread_mutex_unlock(&sem->lock);

    return 0;
}

/****************************************************************************
    NAME */
        int sem_wait(

/*  SYNOPSIS */
        sem_t *sem)

/*  FUNCTION
        Wait until the semaphore count is positive, then decrement it by one.
        A positive count is consumed without a condition wait. The internal
        mutex can still delay acquisition even when a unit is available.

    INPUTS
        sem - Initialized semaphore. Keep its storage valid until every active
            operation and waiter has finished.

    RESULT
        0 after consuming one unit. -1 with errno EINVAL if sem is NULL.
        An internal condition-wait failure is returned as -1 with its error
        number in errno. Cancellation during a condition wait exits the worker
        instead of returning.

    NOTES
        Include <semaphore.h>; link with pthread. Call from the initial program
        task or a registered worker. There is no timeout. Posting is not tied to
        ownership; another task can supply the unit with sem_post(). CTRL_C without
        active cancellation does not produce EINTR: the count is checked again.
        Do not destroy the semaphore to terminate this wait.

    BUGS
        Cancellation inside the condition wait occurs with the internal mutex
        reacquired. No cleanup handler releases it or decrements waiters_count,
        so cancellation can leave the semaphore locked and unusable. Avoid
        cancellation of a worker blocked here; arrange explicit wakeup and
        orderly termination instead.

        A pending request is not checked when a unit is available immediately.
        Repeated posts can also coalesce on one condition waiter; see sem_post().

    SEE ALSO
        sem_post(),
        sem_init(),
        sem_destroy()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, sem));

    return sem_timedwait(sem, NULL);
}

/****************************************************************************
    NAME */
        int sem_post(

/*  SYNOPSIS */
        sem_t *sem)

/*  FUNCTION
        Increment the semaphore count and signal one condition waiter when the
        recorded waiter count is nonzero. No waiting task owns a particular
        posted unit until it successfully decrements the count.

    INPUTS
        sem - Initialized semaphore whose storage remains valid throughout
            this operation and all concurrent waits.

    RESULT
        0 after incrementing. -1 with errno EINVAL if sem is NULL, or EOVERFLOW
        if the count is already SEM_VALUE_MAX. No unit is added on overflow.
        Builds without EOVERFLOW define it as EINVAL in this implementation.

    NOTES
        Include <semaphore.h>; link with pthread. The call acquires the internal
        mutex and can block. It is not safe in interrupt context or an
        asynchronous signal handler. The semaphore is counting, not owned;
        a posting task need not have performed a corresponding wait.

    BUGS
        pthread_cond_signal() can repeatedly target the same listed waiter
        before it removes itself. Multiple posts can therefore leave other
        waiters asleep despite a positive count. No fairness or distinct-waiter
        wakeup guarantee is provided.

    SEE ALSO
        sem_wait(),
        sem_init(),
        sem_destroy()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, sem));

    if (sem == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&sem->lock);

    if (sem->value >= SEM_VALUE_MAX)
    {
        pthread_mutex_unlock(&sem->lock);
        errno = EOVERFLOW;
        return -1;
    }

    sem->value++;

    if (sem->waiters_count > 0)
        pthread_cond_signal(&sem->count_nonzero);

    pthread_mutex_unlock(&sem->lock);

    return 0;
}

/****************************************************************************
    NAME */
        int sem_getvalue(

/*  SYNOPSIS */
        sem_t *sem, int *sval)

/*  FUNCTION
        Read an instantaneous semaphore count or an approximation of blocked
        waiters.

    INPUTS
        sem - Initialized semaphore.
        sval - Non-NULL writable integer output.

    RESULT
        0 if an output value is written; -1 with errno=EINVAL if sem or sval
        is NULL.

    NOTES
        When the internal mutex is available, returns value unless a condition
        wait is recorded, in which case it returns -waiters_count. If the
        mutex is busy, returns -waiters_count without acquiring it.

    BUGS
        The busy-mutex path reads waiters_count without synchronization and
        can report zero or a negative count even when permits exist. This is
        not an atomic count/waiter snapshot.

    SEE ALSO
        sem_post()
        sem_trywait()
        sem_wait()

*****************************************************************************/
{
    D(bug("%s(%p)\n", __FUNCTION__, sem));

    if (sem == NULL || sval == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    // if one or more threads are waiting to lock the semaphore,
    // then return the negative of the waiters
    if (pthread_mutex_trylock(&sem->lock) == 0)
    {
        if (sem->lock.incond)
            *sval = -sem->waiters_count;
        else
            *sval = sem->value;
        pthread_mutex_unlock(&sem->lock);
    }
    else
    {
        // TODO: should I lock the mutex here?
        *sval = -sem->waiters_count;
    }

    return 0;
}
