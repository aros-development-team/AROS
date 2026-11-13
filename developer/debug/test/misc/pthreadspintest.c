/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>
#include <pthread.h>
#include <stdio.h>

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static int once_calls;

static void init_once(void)
{
    once_calls++;
}

static int check(const char *operation, int actual, int expected)
{
    if (actual == expected)
        return 0;

    printf("FAIL: %s returned %d, expected %d\n", operation, actual,
        expected);
    return 1;
}

int main(void)
{
    pthread_spinlock_t lock;

    if (check("pthread_spin_init", pthread_spin_init(&lock,
            PTHREAD_PROCESS_PRIVATE), 0))
        return 1;

    /* On m68k this catches a test-and-set that returns the stored value. */
    if (check("pthread_spin_trylock (free)", pthread_spin_trylock(&lock), 0))
        return 1;
    if (check("pthread_spin_trylock (held)", pthread_spin_trylock(&lock),
            EBUSY))
        return 1;
    if (check("pthread_spin_unlock", pthread_spin_unlock(&lock), 0))
        return 1;

    if (check("pthread_spin_lock", pthread_spin_lock(&lock), 0))
        return 1;
    if (check("pthread_spin_unlock", pthread_spin_unlock(&lock), 0))
        return 1;
    if (check("pthread_spin_destroy", pthread_spin_destroy(&lock), 0))
        return 1;

    if (check("pthread_once (first)", pthread_once(&once_control, init_once),
            0))
        return 1;
    if (check("pthread_once (second)", pthread_once(&once_control, init_once),
            0))
        return 1;
    if (check("pthread_once calls", once_calls, 1))
        return 1;

    puts("pthread spinlock test passed");
    return 0;
}
