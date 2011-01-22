#ifndef EGLMUTEX_INCLUDED
#define EGLMUTEX_INCLUDED

#include "eglcompiler.h"

#ifdef PTHREADS
#include <pthread.h>

typedef pthread_mutex_t _EGLMutex;

static INLINE void _eglInitMutex(_EGLMutex *m)
{
   pthread_mutex_init(m, NULL);
}

static INLINE void
_eglDestroyMutex(_EGLMutex *m)
{
   pthread_mutex_destroy(m);
}

static INLINE void
_eglLockMutex(_EGLMutex *m)
{
   pthread_mutex_lock(m);
}

static INLINE void
_eglUnlockMutex(_EGLMutex *m)
{
   pthread_mutex_unlock(m);
}

#define _EGL_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define _EGL_DECLARE_MUTEX(m) \
   _EGLMutex m = _EGL_MUTEX_INITIALIZER

#elif defined(_EGL_OS_AROS)

#include <proto/exec.h>

typedef struct SignalSemaphore _EGLMutex;

static INLINE void _eglInitMutex(_EGLMutex *m)
{
   InitSemaphore(m);
}

static INLINE void
_eglDestroyMutex(_EGLMutex *m)
{
   /* No Op */
}

static INLINE void
_eglLockMutex(_EGLMutex *m)
{
   ObtainSemaphore(m);
}

static INLINE void
_eglUnlockMutex(_EGLMutex *m)
{
   ReleaseSemaphore(m);
}

/* This is not supported. _eglInitMutex must alway be used */
/* #define _EGL_MUTEX_INITIALIZER */

#include <aros/symbolsets.h> /* For ADD2INIT */
/* Declare variable, declare init function, add to auto init. Ugly but works. */
#define _EGL_DECLARE_MUTEX(m)   \
   _EGLMutex m;                 \
static void init##m()           \
{                               \
    _eglInitMutex(&m);          \
}                               \
ADD2INIT(init##m, 5);

#else

typedef int _EGLMutex;
static INLINE void _eglInitMutex(_EGLMutex *m) { (void) m; }
static INLINE void _eglDestroyMutex(_EGLMutex *m) { (void) m; }
static INLINE void _eglLockMutex(_EGLMutex *m) { (void) m; }
static INLINE void _eglUnlockMutex(_EGLMutex *m) { (void) m; }

#define _EGL_MUTEX_INITIALIZER 0
#define _EGL_DECLARE_MUTEX(m) \
   _EGLMutex m = _EGL_MUTEX_INITIALIZER

#endif

#endif /* EGLMUTEX_INCLUDED */
