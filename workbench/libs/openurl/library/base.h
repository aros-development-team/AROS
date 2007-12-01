/*
**  openurl.library - universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
*/


#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#if defined(__amigaos4__) && !defined(EXEC_INTERFACES_H)
 #include <exec/interfaces.h>
#endif

/***************************************************************************/

extern UBYTE                  lib_name[];
extern UBYTE                  lib_ver[];
extern UBYTE                  lib_fullName[];
extern ULONG                  lib_version;
extern ULONG                  lib_revision;

extern struct ExecBase        *SysBase;
extern struct DosLibrary      *DOSBase;
extern struct Library         *UtilityBase;
extern struct Library         *IFFParseBase;
extern struct RxsLib          *RexxSysBase;

extern struct SignalSemaphore lib_sem;
extern struct SignalSemaphore lib_prefsSem;
extern struct SignalSemaphore lib_memSem;

extern APTR                   lib_pool;
extern struct URL_Prefs       *lib_prefs;

extern struct Library         *lib_base;
extern ULONG                  lib_segList;
extern struct SignalSemaphore lib_libSem;
extern ULONG                  lib_use;
extern ULONG                  lib_flags;

/***************************************************************************/

enum
{
    BASEFLG_Init  = 1<<0,
    BASEFLG_Trans = 1<<1,
};

/***************************************************************************/
