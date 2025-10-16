/****************************************************************
*
* Name : SetPSM.c
*
* Description : Set the public screen flags SHANGHAI and POPPUBSCREEN
*
*****************************************************************/


/*****************************************************************************

    NAME

        SetPSM

    SYNOPSIS

        SHANGHAI/S,POP=POPPUBSCREEN/S

    LOCATION

        C:

    FUNCTION

        Sets the "Shanghai" and "PopPubScreen" flags in intition.library

    INPUTS

        SHANGHAI   --  Force workbench windows to open on the default public screen.
        POP=POPPUBSCREEN  --  Pop the publicscreen to the front when a window opens on it.

    RESULT
        Returns 5 if SHANGHAI was previously set, and 10 if POPPUBSCREEN was set.

    NOTES

    EXAMPLE

        SetPSM SHANGHAI

    BUGS

    SEE ALSO

        intuition.library/SetPubScreenModes()

    INTERNALS

******************************************************************************/

#define DOSLIB  "dos.library"
#define DOSVER  36L
#define INTUILIB  "intuition.library"
#define INTUIVER  36L

#define THISPROC   ((struct Process *)(FindTask(NULL)))
#define Result2(x) THISPROC->pr_Result2 = x

#define TEMPLATE  "SHANGHAI/S,POP=POPPUBSCREEN/S"
#define OPT_SHANG 0
#define OPT_POP   1
#define OPT_COUNT 2

/******** */

#define __USE_SYSBASE
#include <exec/types.h>
#include <exec/execbase.h>
#include <intuition/intuition.h>
#include <dos/rdargs.h>

#if !defined(__AROS__)
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/intuition_protos.h>

#include <pragmas/exec_pragmas.h>
#include <pragmas/dos_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#else
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#endif

#include <string.h>

/******** */

static char VersTag[]="\0$VER: SetPSM 1.0 (20.3.97) Tak Tang";

/******** */

int main(void) {
#if !defined(__AROS__)
    struct ExecBase *SysBase = (*((struct ExecBase **) 4));
#endif
    struct DosLibrary *DOSBase;
    struct Library *IntuitionBase;
    IPTR opts[OPT_COUNT];
    LONG rc;
    struct RDArgs *rdargs;
    UWORD oldmode;

    rc = RETURN_FAIL;
    if ((DOSBase = (struct DosLibrary *)OpenLibrary(DOSLIB, DOSVER))) {
        memset((char *)opts, 0, sizeof(opts));
        rdargs = ReadArgs(TEMPLATE, opts, NULL);
        if ( rdargs ) {
            if ((IntuitionBase = (struct Library *)OpenLibrary(INTUILIB, INTUIVER))) {
                oldmode=SetPubScreenModes( ((opts[OPT_SHANG]) ? SHANGHAI : 0) |
                                 ((opts[OPT_POP]) ? POPPUBSCREEN : 0) );
                rc = (oldmode && SHANGHAI ? 5 : 0) + (oldmode && POPPUBSCREEN ? 10 : 0);
            } else {
                PrintFault(IoErr(), NULL);
            }
            CloseLibrary(IntuitionBase);

            FreeArgs(rdargs);
        } else {
            PrintFault(IoErr(), NULL);
        }
        CloseLibrary((struct Library *)DOSBase);
    } else {
        Result2(ERROR_INVALID_RESIDENT_LIBRARY);
    }

    return(rc);
} // main

/******** */

