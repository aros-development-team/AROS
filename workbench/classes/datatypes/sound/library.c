
/*
**
**	sound.datatype v41
**	© 1998/99 by Stephan Rupprecht
**	All Rights reserved
**
*/

#include <exec/resident.h>
#include <exec/initializers.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <datatypes/soundclass.h>

#ifdef __MAXON__
#include <pragma/exec_lib.h>
#include <pragma/intuition_lib.h>
#include <pragma/dos_lib.h>
#include <pragma/utility_lib.h>
#include <pragma/datatypes_lib.h>
#else
#include <inline/exec.h>
#include <inline/intuition.h>
#include <inline/dos.h>
#include <inline/utility.h>
#include <inline/datatypes.h>
#endif

#include "classbase.h"

#ifndef __MAXON__
#include "CompilerSpecific.h"
#endif

/****************************************************************************/

#define BPTR		ULONG

#ifdef __MAXON__
#define REG(r, a)		register __## r a
#define __regargs
#endif

#define VERSION		41
#define REVISION	11
#define VERSIONSTR	"41.11"
#define DATE		"11.08.00"

/****************************************************************************/

STATIC struct Library *LibInit(REG(a0, ULONG Segment), REG(d0, struct ClassBase *ClassBase), REG(a6, struct Library *) );
STATIC struct Library *LibOpen( REG(a6, struct ClassBase *ClassBase) );
STATIC ULONG LibExpunge( REG(a6, struct ClassBase *ClassBase) );
STATIC ULONG LibClose( REG(a6, struct ClassBase *ClassBase) );
STATIC LONG LibExtFunc(void);

BOOL __regargs L_OpenLibs(struct ClassBase *);
void __regargs L_CloseLibs(struct ClassBase *);
Class * __regargs initClass (struct ClassBase *);
Class  * ObtainClass( REG(a6, struct ClassBase *) );

extern ULONG   Dispatch( REG(a0, Class *), REG(a2, Object *), REG(a1, Msg) );

/****************************************************************************/

UBYTE	LibName[] = "v41sound.datatype",
		LibID[] = "v41sound.datatype "VERSIONSTR" ("DATE") © 1998-2000 by Stephan Rupprecht";

LONG	LibVersion = VERSION,
		LibRevision = REVISION;

APTR LibVectors[] =
{
	(APTR) LibOpen,
	(APTR) LibClose,
	(APTR) LibExpunge,
	(APTR) LibExtFunc,
	(APTR) ObtainClass,
	(APTR) -1L
};

ULONG LibInitTab[] =
{
	(ULONG) sizeof(struct ClassBase),
	(ULONG) LibVectors,
	(ULONG) NULL,
	(ULONG) LibInit
};

struct Resident ROMTag =     /* do not change */
{
	RTC_MATCHWORD,
 	&ROMTag,
 	&ROMTag + 1L,
	RTF_AUTOINIT,
 	VERSION,
	NT_LIBRARY,
 	0,
	LibName,
	LibID,
	(APTR) LibInitTab
};

/****************************************************************************/

LONG dummy(void)
{
	return(-1L);
}

/****************************************************************************/

LONG LibExtFunc(void)
{
	return(0L);
}

/****************************************************************************/

#ifndef __MAXON__
#define IntuitionBase	cb->cb_IntuitionBase
#define GfxBase		cb->cb_GfxBase
#define DOSBase		cb->cb_DOSBase
#define SysBase		cb->cb_SysBase
#define DataTypesBase	cb->cb_DataTypesBase
#define UtilityBase		cb->cb_UtilityBase
#define IFFParseBase	cb->cb_IFFParseBase
#define TapeDeckBase	cb->cb_TapeDeckBase
#else
struct Library		*DOSBase, *SysBase, *DataTypesBase, *TapeDeckBase,
				*IntuitionBase, *GfxBase, *UtilityBase, *IFFParseBase;
#endif

/****************************************************************************/

struct Library *
  LibInit( REG(a0, ULONG Segment), REG(d0, struct ClassBase *cb), REG(a6, struct Library *ExecBase) )
{
	SysBase = ExecBase;

	InitSemaphore( &cb->cb_LibLock );
	ObtainSemaphore( &cb->cb_LibLock );
	
	cb->LibSegment = Segment;
	cb->LibNode.lib_Revision = REVISION;
	
	if( ((struct ExecBase *)ExecBase)->AttnFlags & AFF_68020 )
	{
		if(L_OpenLibs(cb))
		{	
			if(cb->cb_Class = initClass(cb))
			{	
				ReleaseSemaphore( &cb->cb_LibLock );
				return((struct Library *)cb);
			} 
		}
	}

	L_CloseLibs( cb );

	ReleaseSemaphore( &cb->cb_LibLock );

	FreeMem((BYTE *)cb-cb->LibNode.lib_NegSize, 
		cb->LibNode.lib_NegSize + cb->LibNode.lib_PosSize);
	
	return(NULL);
}

/****************************************************************************/

struct Library *LibOpen( REG(a6, struct ClassBase *cb) )
{	
	ObtainSemaphore( &cb->cb_LibLock );
	cb->LibNode.lib_Flags &= ~LIBF_DELEXP;
	cb->LibNode.lib_OpenCnt++;
	ReleaseSemaphore( &cb->cb_LibLock );
	return((struct Library *)cb);
}

/****************************************************************************/

ULONG  LibExpunge( REG(a6, struct ClassBase *cb) )
{
	ULONG	retval = 0L;
	
	ObtainSemaphore( &cb->cb_LibLock );
	
 	if(!cb->LibNode.lib_OpenCnt) 
 	{
		if(cb->cb_Class) 
		{
			if( ! FreeClass(cb->cb_Class) )
			{
				AddClass( cb->cb_Class );
				ReleaseSemaphore( &cb->cb_LibLock );
				return( NULL );
			}
		}
#ifdef __GNUC__
		if( DataTypesBase->lib_Version >= 45L )
		{
			FreeDTMethods( cb->cb_Methods );
		}
#else
		FreeVec( cb->cb_Methods );
#endif
		
		L_CloseLibs(cb);
		
 		Remove((struct Node *)cb);

  		FreeMem((BYTE *)cb-cb->LibNode.lib_NegSize,
	          cb->LibNode.lib_NegSize + cb->LibNode.lib_PosSize);

		retval = cb->LibSegment;
 	} 
 	else 
	{
		cb->LibNode.lib_Flags |= LIBF_DELEXP;
	}

	ReleaseSemaphore( &cb->cb_LibLock );

 	return( retval );
}

/****************************************************************************/

ULONG  LibClose( REG(a6, struct ClassBase *cb) )
{	
	ULONG	retval = 0L;
	
	ObtainSemaphore( &cb->cb_LibLock );
	
 	if( ! cb->LibNode.lib_OpenCnt || ! --cb->LibNode.lib_OpenCnt )
	{
		if( cb->LibNode.lib_Flags & LIBF_DELEXP )
		{
			retval = LibExpunge( cb );
		}
	}

	ReleaseSemaphore( &cb->cb_LibLock );
	
	return( retval );
}

/****************************************************************************/

BOOL __regargs L_OpenLibs( struct ClassBase *cb )
{
	if(	( DOSBase = OpenLibrary( "dos.library", 0L ) ) &&
		( IntuitionBase = OpenLibrary( "intuition.library", 0L ) ) &&
		( GfxBase = OpenLibrary( "graphics.library", 0L ) ) &&
		( DataTypesBase = OpenLibrary( "datatypes.library", 0L ) ) &&
		( UtilityBase = OpenLibrary( "utility.library", 0L ) ) &&
		( IFFParseBase = OpenLibrary( "iffparse.library", 0L ) )	)
	{	
		TapeDeckBase = OpenLibrary( "gadgets/tapedeck.gadget", 39L );
		return TRUE;
	}
	
	return FALSE;
}

/****************************************************************************/

Class * __regargs initClass ( struct ClassBase *cb )
{
 	register Class *cl;

 	if(cl = MakeClass(LibName, DATATYPESCLASS, NULL, sizeof( struct InstanceData ), 0L))
  	{
   		cl->cl_Dispatcher.h_Entry = (HOOKFUNC) Dispatch;
   		cl->cl_UserData = (ULONG) cb;
   		AddClass(cl);
  	}

 	return (cl);
}

/****************************************************************************/

void __regargs L_CloseLibs( struct ClassBase *cb )
{
	CloseLibrary( DOSBase );
	CloseLibrary( IntuitionBase );
	CloseLibrary( GfxBase );
	CloseLibrary( DataTypesBase );
	CloseLibrary( UtilityBase );
	CloseLibrary( IFFParseBase );
	CloseLibrary( TapeDeckBase );
}

/****************************************************************************/

Class *ObtainClass( REG(a6, struct ClassBase *cb) )
{
	Class	*cl;
	
	ObtainSemaphoreShared( &cb->cb_LibLock );
	cl = cb->cb_Class;
	ReleaseSemaphore( &cb->cb_LibLock );
	
	return cl;
}

/****************************************************************************/
