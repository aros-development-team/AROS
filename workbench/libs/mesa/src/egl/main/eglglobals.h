#ifndef EGLGLOBALS_INCLUDED
#define EGLGLOBALS_INCLUDED


#include "egltypedefs.h"
#include "eglmutex.h"


/**
 * Global library data
 */
struct _egl_global
{
   _EGLMutex *Mutex;

   /* the list of all displays */
   _EGLDisplay *DisplayList;

   EGLint NumAtExitCalls;
   void (*AtExitCalls[10])(void);
};


extern struct _egl_global _eglGlobal;


extern void
_eglAddAtExitCall(void (*func)(void));


#endif /* EGLGLOBALS_INCLUDED */
