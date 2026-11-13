/* Lib.c
 *
 * Basic Library Resource Handling
 *
 * This code is based on the example shared library,
 * found on the DICE 3.0 disks.
 *
 * This code doesn't use any form of startup code (the example does).
 * This is not something the novice C-programmer should attemt to do.
 * You need to know what you are doing. :)
 */

#include "async.h"
#include <exec/libraries.h>
#include <exec/resident.h>
#include "rev.h"

#include <clib/asyncio_protos.h>

_LIBCALL struct Library	*LibInit( _REG( d0 ) struct Library *, _REG( a0 ) APTR, _REG( a6 ) struct ExecBase * );
_CALL    struct Library	*LibOpen( _REG( a6 ) struct Library * );
_CALL    APTR		LibClose( _REG( a6 ) struct Library * );
_LIBCALL APTR		LibExpunge( _REG( a6 ) struct Library * );


/* These variables will be zero, since "The memory used for bss
 * blocks is zeroed by the loader when it is allocated" (quote
 * from the AmigaDOS manual).
 */
struct Library		*AsyncIOBase;	/* Our library structure */
struct ExecBase		*SysBase;
struct Library		*UtilityBase;
struct DosLibrary	*DOSBase;
APTR			SegList;


/* In case the user tries to run us, simply exit immediately.
 * This code is also used for the reserved library function,
 * that all libraries currently must have.
 *
 * Note that this function *must* be placed before any const
 * data, or else the "exit if run" effect is lost (you are
 * more likely to get a "crash if run" effect ;).
 */
LONG
LibReserved( VOID )
{
	return( 0 );
}


/* The functions the library should have. Unfortunately, we can't
 * use the more compact format, using 16-bit relative entries,
 * due to the compiler (bug or limitation ;).
 */

static const APTR FuncTable[] =
{
	LibOpen,	/* Standard library functions */
	LibClose,
	LibExpunge,
	LibReserved,

	OpenAsync,	/* Our functions start here */
	OpenAsyncFromFH,
	CloseAsync,
	SeekAsync,
	ReadAsync,
	WriteAsync,
	ReadCharAsync,
	WriteCharAsync,
	ReadLineAsync,
	WriteLineAsync,
	FGetsAsync,
	FGetsLenAsync,
	PeekAsync,
	( APTR ) -1	/* Terminate the table */
};


/* Table describing the library. We need this, since we use the
 * autoinit feature. This means we don't need to call MakeLibrary()
 * etc. in LibInit.
 */
static const ULONG InitTable[] =
{
	sizeof( struct Library ),	/* Size of our library base, excluding jump table. We have no extra data here */
	( ULONG ) FuncTable,		/* The functions we have */
	NULL,				/* InitStruct data. We init stuff ourselves instead */
	( ULONG ) LibInit		/* The library init function */
};


static const TEXT LibId[] = "asyncio.library "VERSION" (" DATE ")\r\n";
static const TEXT LibName[] = "asyncio.library";


/* And finaly the resident structure, used by InitResident(),
 * in order to initialize everything.
 */
static const struct Resident RomTag =
{
	RTC_MATCHWORD,		/* rt_MatchWord */
	&RomTag,		/* rt_MatchTag */
	LibExpunge,		/* rt_EndSkip */
	RTF_AUTOINIT,		/* rt_Flags */
	VERNUM,			/* rt_Version */
	NT_LIBRARY,		/* rt_Type */
	0,			/* rt_Pri */
	LibName,		/* rt_Name */
	LibId,			/* rt_IDString */
	InitTable		/* rt_Init */
};


/* This small function frees the library structure,
 * and any other allocated resources.
 */
static VOID
FreeLib( struct Library *lib )
{
	if( DOSBase )	/* In case we are loaded under kick 35 or earlier */
	{
		CloseLibrary( ( struct Library * ) DOSBase );
		CloseLibrary( UtilityBase );
	}

	FreeMem( ( UBYTE * ) lib - lib->lib_NegSize, lib->lib_NegSize + lib->lib_PosSize );
}


/* This function is called when the library is loaded, and the library base
 * have been allocated. We are in a forbid section here, so don't do anything
 * time-consuming, Wait() or similar.
 *
 * If all ok, return the library base. If anything went wrong, deallocate
 * the library structure, and return NULL.
 */
_LIBCALL struct Library *
LibInit( _REG( d0 ) struct Library *lib, _REG( a0 ) APTR seglist, _REG( a6 ) struct ExecBase *sysBase )
{
	SysBase = sysBase;

	/* Opening libraries in LibInit might not be a good idea under OS 1.x.
	 * Should be safe enough under 2.x and 3.x.
	 */
	if( ( DOSBase = ( struct DosLibrary * ) OpenLibrary( "dos.library", 37 ) ) &&
		( UtilityBase = OpenLibrary( "utility.library", 37 ) ) )
	{
		lib->lib_Node.ln_Type	= NT_LIBRARY;
		lib->lib_Node.ln_Pri	= 0;
		lib->lib_Node.ln_Name	= LibName;
		/* Request that checksum should be calculated */
		lib->lib_Flags		= LIBF_CHANGED | LIBF_SUMUSED;
		lib->lib_Version	= VERNUM;
		lib->lib_Revision	= REVNUM;
		lib->lib_IdString	= ( APTR ) LibId;
		SegList = seglist;
		AsyncIOBase = lib;
	}
	else
	{
		FreeLib( lib );
		lib = NULL;
	}

	return( lib );
}


/* Open is given the library pointer. Either return the library pointer or NULL.
 * Remove the delayed-expunge flag. Exec has Forbid() for us during the call.
 *
 * Since we don't refer to any smalldata here (directly or indirectly),
 * we can safely skip "_LIBCALL", to save a few bytes.
 */
_CALL struct Library *
LibOpen( _REG( a6 ) struct Library *lib )
{
	++lib->lib_OpenCnt;
	lib->lib_Flags &= ~LIBF_DELEXP;
	return( lib );
}


/* Close is given the library pointer. Be sure not to decrement the open
 * count if already zero. If the open count is or becomes zero AND there
 * is a LIBF_DELEXP, we expunge the library and return the seglist.
 * Otherwise we return NULL.
 *
 * Note that this routine never sets LIBF_DELEXP on its own.
 *
 * Exec has Forbid() for us during the call.
 *
 * Since we don't refer to any smalldata here (directly. Indirectly we refer via
 * LibExpunge, which is LibCall declared), we can safely skip "LibCall", to save
 * a few bytes.
 */
_CALL APTR
LibClose( _REG( a6 ) struct Library *lib )
{
	if( lib->lib_OpenCnt && --lib->lib_OpenCnt )
	{
		return( NULL );
	}

	if( lib->lib_Flags & LIBF_DELEXP )
	{
		return( LibExpunge( lib ) );
	}

	return( NULL );
}


/* We expunge the library and return the Seglist ONLY if the open count is zero.
 * If the open count is not zero we set the delayed-expunge flag and return NULL.
 *
 * Exec has Forbid() for us during the call. NOTE ALSO that Expunge might be
 * called from the memory allocator and thus we CANNOT DO A Wait() or otherwise
 * take a long time to complete (straight from RKM).
 *
 * RemLibrary() calls our expunge routine and would therefore freeze if we called
 * it ourselves. LibExpunge() must remove the library itself as shown below.
 */
_LIBCALL APTR
LibExpunge( _REG( a6 ) struct Library *lib )
{
	if( lib->lib_OpenCnt )
	{
		lib->lib_Flags |= LIBF_DELEXP;
		return( NULL );
	}

	Remove( &lib->lib_Node );
	FreeLib( lib );

	return( SegList );
}
