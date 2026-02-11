#include <proto/exec.h>
#include <proto/asyncio.h>


void __regargs __autoopenfail( char * );


struct Library	*AsyncIOBase;
static void *libbase;

extern long __oslibversion;
extern long __asiolibversion;


LONG
_STI_110_OpenAsyncIO( VOID )
{
	AsyncIOBase = libbase = OpenLibrary( "asyncio.library", __asiolibversion );

	if( AsyncIOBase == NULL )
	{
		__oslibversion = __asiolibversion;
		__autoopenfail( "asyncio.library" );
		return( 1 );
	}

	return( 0 );
}


VOID
_STD_110_CloseAsyncIO( VOID )
{
	if( libbase )
	{
		CloseLibrary( libbase );
		libbase = AsyncIOBase = NULL;
	}
}
