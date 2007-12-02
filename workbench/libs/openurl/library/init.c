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


#include "lib.h"
#include <dos/dostags.h>

/***********************************************************************/

void
freeBase(void)
{
    if (lib_prefs)
    {
        URL_FreePrefsA(lib_prefs,NULL);
        lib_prefs = NULL;
    }

#if defined(__amigaos4__)
    if( IRexxSys )
    {
        DropInterface( (struct Interface*)IRexxSys );
        IRexxSys = NULL;
    }

    if (IIFFParse)
    {
        DropInterface( (struct Interface*)IIFFParse );
        IIFFParse = NULL;
    }

    if( IUtility )
    {
        DropInterface( (struct Interface*)IUtility );
        IUtility = NULL;
    }
#endif

    if (RexxSysBase)
    {
        CloseLibrary((struct Library *)RexxSysBase);
        RexxSysBase = NULL;
    }

    if (IFFParseBase)
    {
        CloseLibrary(IFFParseBase);
        IFFParseBase = NULL;
    }

    if (UtilityBase)
    {
        CloseLibrary(UtilityBase);
        UtilityBase = NULL;
    }

#if defined(__amigaos4__)
    if( IDOS )
    {
        DropInterface( (struct Interface*)IDOS );
        IDOS = NULL;
    }
#endif

    if (DOSBase)
    {
        CloseLibrary((struct Library *)DOSBase);
        DOSBase = NULL;
    }

    if (lib_pool)
    {
        DeletePool(lib_pool);
        lib_pool = NULL;
    }

    lib_flags &= ~BASEFLG_Init;
}

/***********************************************************************/

ULONG
initBase(void)
{
    if ((lib_pool = CreatePool(MEMF_PUBLIC|MEMF_CLEAR,16384,8192)) &&
        (DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",36)) &&
        (UtilityBase = OpenLibrary("utility.library",36)) &&
        (IFFParseBase = OpenLibrary("iffparse.library",36)) &&
        (RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library",33)) &&
#if defined(__amigaos4__)
		  (IDOS = (struct DOSIFace *) GetInterface( (struct Library*)DOSBase, "main", 1L, NULL )) &&
        (IUtility = (struct UtilityIFace *) GetInterface( UtilityBase, "main", 1L, NULL)) &&
        (IIFFParse = (struct IFFParseIFace *) GetInterface( IFFParseBase, "main", 1L, NULL)) &&
        (IRexxSys = (struct RexxSysIFace *) GetInterface( (struct Library*)RexxSysBase, "main", 1L, NULL)) &&
#endif
        (lib_prefs = loadPrefsNotFail()))
    {
        lib_flags |= BASEFLG_Init;

        return TRUE;
    }

    freeBase();

    return FALSE;
}

/***********************************************************************/
