#include "lib.h"
#include <aros/symbolsets.h>

struct Library * OpenURLBase;

struct RxsLib          *RexxSysBase = NULL;
struct SignalSemaphore lib_sem;
struct SignalSemaphore lib_prefsSem;
struct SignalSemaphore lib_memSem;

APTR                   lib_pool = NULL;
ULONG                  lib_use = 0;
ULONG                  lib_flags = 0;
struct URL_Prefs       *lib_prefs = NULL;

/****************************************************************************/

int LibInit(struct Library *base)
{
    BOOL success = FALSE;

    // Init global library base
    OpenURLBase = base;
    
    InitSemaphore(&lib_sem);
    InitSemaphore(&lib_prefsSem);
    InitSemaphore(&lib_memSem);
  
    // protect access to initBase()
    ObtainSemaphore(&lib_sem);

    success = initBase();

    // unprotect initBase()
    ReleaseSemaphore(&lib_sem);

    return success;
}

/****************************************************************************/

int LibExpunge(struct Library *base)
{
    // free all our private data and stuff.
    ObtainSemaphore(&lib_sem);

    freeBase();
  
    // unprotect
    ReleaseSemaphore(&lib_sem);

    OpenURLBase = NULL;

    return TRUE;
}

/****************************************************************************/

ADD2INITLIB(LibInit, 0);
ADD2EXPUNGELIB(LibExpunge, 0);

/***********************************************************************/
