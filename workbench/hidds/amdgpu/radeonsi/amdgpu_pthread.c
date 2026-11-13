/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: Stack size for the threads radeonsi creates. AROS threads inherit
          their creator's stack size, which for a GL client started from a
          CLI is far less than radeonsi's shader compiler threads need.
*/

#include <pthread.h>

#define AMDGPU_THREAD_STACK (8 * 1024 * 1024)

int __real_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start)(void *), void *arg);

int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start)(void *), void *arg)
{
    pthread_attr_t own;
    size_t size = 0;

    if (attr)
        own = *attr;
    else
        pthread_attr_init(&own);
    pthread_attr_getstacksize(&own, &size);
    if (size < AMDGPU_THREAD_STACK)
        pthread_attr_setstacksize(&own, AMDGPU_THREAD_STACK);
    return __real_pthread_create(thread, &own, start, arg);
}
