/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Camd initialization code.
    Lang: English
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/libraries.h>
#ifdef _AROS
#  include <aros/libcall.h>
#  include <aros/asmcall.h>
#endif

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include "camd_intern.h"
#include "libdefs.h"

/****************************************************************************************/

#undef SysBase
#undef UtilityBase
#undef DOSBase

struct DosLibrary *DOSBase;
struct ExecBase *SysBase;
struct UtilityBase *UtilityBase;

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (CB(lib)->sysbase)
#define LC_SEGLIST_FIELD(lib)   (CB(lib)->seglist)
#define LC_LIBBASESIZE		sizeof(struct CamdBase_intern)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)	(CB(lib)->library)

/* #define LC_NO_INITLIB    */
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
/* #define LC_NO_EXPUNGELIB */

#ifdef _AROS
#  include <libcore/libheader.c>

#  undef DEBUG
#  define DEBUG 1
#  include <aros/debug.h>

// #define SysBase			(LC_SYSBASE_FIELD(CamdBase))

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR CamdBase)
{

    SysBase = CB(CamdBase)->sysbase;

    D(bug("Inside Init func of camd.library\n"));

    if (!UtilityBase){
        (struct Library *)
		UtilityBase=OpenLibrary("utility.library", 37);
	CB(CamdBase)->utilitybase=UtilityBase;
    }
    if (!UtilityBase)
        return FALSE;

    if (!DOSBase){
        (struct Library *)
		DOSBase=OpenLibrary("dos.library", 37);
	CB(CamdBase)->dosbase=DOSBase;
    }
    if (!DOSBase)
        return FALSE;

    if(InitCamd((struct CamdBase *)LIBBASE)==FALSE){
      return FALSE;
    }

    return TRUE;
}



/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR CamdBase)
{
    D(bug("Inside Expunge func of camd.library\n"));

    if(UtilityBase==NULL){
	    CloseLibrary((struct Library *)UtilityBase);
	    UtilityBase = NULL;
    }

    if(DOSBase==NULL){
	    CloseLibrary((struct Library *)DOSBase);
	    DOSBase = NULL;
    }

    UninitCamd(LIBBASE);

}


/****************************************************************************************/



#else
#  include "CamdAmigaLibHeader.c"
#endif
