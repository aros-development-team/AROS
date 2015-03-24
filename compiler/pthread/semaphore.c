/*
    Copyright (C) 2015 Szilard Biro

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

#include <stdlib.h>
#include <fcntl.h>

#include "semaphore.h"
#include "debug.h"

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

sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value)
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
        
        sem = malloc(sizeof(sem_t));
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
        // TODO: this string should be duplicated
        sem->node.ln_Name = (char *)name;
        AddTail(&semaphores, (struct Node *)sem);
    }
    ReleaseSemaphore(&sema_sem);

    return sem;
}

int sem_close(sem_t *sem)
{
    D(bug("%s(%p)\n", __FUNCTION__, sem));

    return 0;
}

int sem_unlink(const char *name)
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

int sem_init(sem_t *sem, int pshared, unsigned int value)
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

int sem_destroy(sem_t *sem)
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

int sem_trywait(sem_t *sem)
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

int sem_timedwait(sem_t *sem, const struct timespec *abstime)
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

int sem_wait(sem_t *sem)
{
    D(bug("%s(%p)\n", __FUNCTION__, sem));

    return sem_timedwait(sem, NULL);
}

int sem_post(sem_t *sem)
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

int sem_getvalue(sem_t *sem, int *sval)
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
