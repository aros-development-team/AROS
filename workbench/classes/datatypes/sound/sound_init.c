/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Text.datatype initialization code.
    Lang: English.
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <clib/alib_protos.h>

#include LC_LIBDEFS_FILE

#include "classbase.h"

/***************************************************************************************************/

struct ExecBase      *sysbase, *SysBase;
struct DosLibrary    *DOSBase;
struct Library	     *DataTypesBase;
struct Library	     *TapeDeckBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase	     *GfxBase;
struct UtilityBase   *UtilityBase;
struct Library	     *IFFParseBase;

BPTR		SegList;

/***************************************************************************************************/

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   sysbase
#define LC_SEGLIST_FIELD(lib)   lib->LibSegment
#define LC_LIBBASESIZE          sizeof(struct ClassBase)
#define LC_LIBHEADERTYPEPTR     struct ClassBase *
#define LC_LIB_FIELD(lib)       lib->LibNode

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

#define AROS_LC_PRE_EXPUNGELIB(lib) CheckExpunge(lib)
static BOOL CheckExpunge(LC_LIBHEADERTYPEPTR lh);

#include <libcore/libheader.c>

#undef SysBase

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

/***************************************************************************************************/

BOOL L_OpenLibs( struct ClassBase *cb );
void L_CloseLibs( struct ClassBase *cb );
Class *initClass (struct ClassBase *cb );

extern IPTR Dispatcher(Class *, Object *, Msg );

/***************************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    ULONG result = FALSE;
    
    SysBase = sysbase;

    InitSemaphore(&lh->cb_LibLock);
    ObtainSemaphore(&lh->cb_LibLock);
    
    D(bug("Inside initfunc of sound.datatype\n"));

    if (L_OpenLibs(lh))
    {
    	if ((lh->cb_Class = initClass(lh)))
	{	
            result = TRUE;
    	}
    }

    ReleaseSemaphore(&lh->cb_LibLock);
        
    D(bug("Leaving initfunc of sound.datatype. result = %d\n", result));
    
    return result;
}

/***************************************************************************************************/

static BOOL CheckExpunge(LC_LIBHEADERTYPEPTR lh)
{
    BOOL retval = TRUE;
    
    /* stegerg: make this check, as I want to be sure the L_ExpungeLib
       gets called if I return TRUE here. See c_lib -> libcore -> libheader.c
       
       Because if I return TRUE here I will return with the semaphore locked!
    */
       
    ObtainSemaphore(&lh->cb_LibLock);
    if (!LC_LIB_FIELD(lh).lib_OpenCnt)
    {
    	if (lh->cb_Class)
	{
	    if (!FreeClass(lh->cb_Class))
	    {
	    	AddClass(lh->cb_Class);
		ReleaseSemaphore(&lh->cb_LibLock);
		retval = FALSE;
	    }
	}
    }
    
    /* On success leave with semaphore locked! */
    return retval;
    
}

/***************************************************************************************************/

void SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    /* LibLock already locked, see CheckExpunge() above */
    
    if (DataTypesBase->lib_Version >= 45L)
    {
    	FreeDTMethods(lh->cb_Methods);
    }
    else
    {
    	FreeVec(lh->cb_Methods);
    }
    
    L_CloseLibs(lh);
}

/***************************************************************************************************/

BOOL L_OpenLibs( struct ClassBase *cb )
{
    if( ( DOSBase = (struct DosLibrary *)OpenLibrary( "dos.library", 0L ) ) &&
        ( IntuitionBase = (struct IntuitionBase *)OpenLibrary( "intuition.library", 0L ) ) &&
        ( GfxBase = (struct GfxBase *)OpenLibrary( "graphics.library", 0L ) ) &&
        ( DataTypesBase = OpenLibrary( "datatypes.library", 0L ) ) &&
        ( UtilityBase = (struct UtilityBase *)OpenLibrary( "utility.library", 0L ) ) &&
        ( IFFParseBase = OpenLibrary( "iffparse.library", 0L ) )    )
    {    
        TapeDeckBase = OpenLibrary( "gadgets/tapedeck.gadget", 39L );
        return TRUE;
    }
    
    return FALSE;
}

/***************************************************************************************************/

void L_CloseLibs( struct ClassBase *cb )
{
    CloseLibrary((struct Library *)DOSBase );
    CloseLibrary((struct Library *)IntuitionBase );
    CloseLibrary((struct Library *)GfxBase );
    CloseLibrary(DataTypesBase );
    CloseLibrary((struct Library *)UtilityBase );
    CloseLibrary( IFFParseBase );
    CloseLibrary( TapeDeckBase );
}

/***************************************************************************************************/

Class *initClass ( struct ClassBase *cb )
{
    Class *cl;

    if((cl = MakeClass(SOUNDDTCLASS, DATATYPESCLASS, NULL, sizeof( struct InstanceData ), 0L)))
    {
   	cl->cl_Dispatcher.h_Entry = HookEntry;
   	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) Dispatcher;
   	cl->cl_UserData = (ULONG) cb;
   	AddClass(cl);
    }

    return (cl);
}

/***************************************************************************************************/

