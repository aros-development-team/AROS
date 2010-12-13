#ifndef AHI_Drivers_Common_DriverBase_h
#define AHI_Drivers_Common_DriverBase_h

#include <dos/dos.h>
#include <exec/execbase.h>
#include <intuition/intuitionbase.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

struct DriverBase
{
    struct Library  library;
    UWORD           pad;
    BPTR            seglist;
#ifndef DRIVER_NEEDS_GLOBAL_EXECBASE
    struct ExecBase* execbase;
#endif
    struct IntuitionBase* intuitionbase;
    struct UtilityBase* utilitybase;

#ifdef __AMIGAOS4__
# ifndef DRIVER_NEEDS_GLOBAL_EXECBASE
    struct ExecIFace*      iexec;
# endif
    struct AHIsubIFace*    iahisub;
    struct IntuitionIFace* iintuition;
    struct UtilityIFace*   iutility;
#endif
};

#ifndef DRIVER_NEEDS_GLOBAL_EXECBASE
# define SysBase      (AHIsubBase->execbase)
#endif

#define IntuitionBase ((struct IntuitionBase*) AHIsubBase->intuitionbase)
#define UtilityBase   ((struct UtilityBase*)   AHIsubBase->utilitybase)

#ifdef __AMIGAOS4__
# ifndef DRIVER_NEEDS_GLOBAL_EXECBASE
#  define IExec       (AHIsubBase->iexec)
# endif
# define IAHIsub      (AHIsubBase->iahisub)
# define IIntuition   (AHIsubBase->iintuition)
# define IUtility     (AHIsubBase->iutility)
#endif


struct DriverData
{
};


BOOL
DriverInit( struct DriverBase* AHIsubBase );

VOID
DriverCleanup( struct DriverBase* AHIsubBase );

#endif /* AHI_Drivers_Common_DriverBase_h */
