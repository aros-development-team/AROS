
/*
** cache.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "cache.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include "cache.h"
#include "sprintf.h"

/*************************************************************************/

#define CACHE_SIZEOF 0x10000

ULONG cache_free;

char *cache_memory;
char *cache_pointer;

BPTR cache_handle;     /* handle of the file we're performing a write cache on */

/*************************************************************************/

/* /// Cache_Init()
**
*/

/*************************************************************************/

void Cache_Init( BPTR handle )
{

	if( ( cache_memory  = Memory_AllocVec( CACHE_SIZEOF ) ) ) {
        cache_pointer = cache_memory;
        cache_free    = CACHE_SIZEOF;
        cache_handle  = handle;
    } else {
        cache_handle  = 0;
    }
}
/* \\\ */
/* /// Cache_Close()
**
*/

/*************************************************************************/

void Cache_Close( void )
{
    Cache_Store();
	Memory_FreeVec( cache_memory );
    cache_memory = NULL;
}
/* \\\ */
/* /// Cache_Store()
**
*/

/*************************************************************************/

void Cache_Store( void )
{
    if( cache_handle ) {
		Write( cache_handle, cache_memory, CACHE_SIZEOF - cache_free );
        cache_pointer = cache_memory;
        cache_free    = CACHE_SIZEOF;
    }
}
/* \\\ */
/* /// Cache_PutChar()
**
*/

/*************************************************************************/

void Cache_PutChar( char chr )
{
    *cache_pointer++ = chr;
	if( !( --cache_free ) ) {
        Cache_Store();
    }
}
/* \\\ */
/* /// Cache_PutHex()
**
*/

/*************************************************************************/

void Cache_PutHex( UBYTE num )
{
char chr;

    chr = ((num>>4) & 0x0f) + '0';
    if( chr > '9' ) {
        chr += 'A'-'0'-10;
    }
    Cache_PutChar( chr);

    chr = (num & 0x0f) + '0';
    if( chr > '9' ) {
        chr += 'A'-'0'-10;
    }
    Cache_PutChar( chr);
}
/* \\\ */
/* /// Cache_PutString()
**
*/

/*************************************************************************/

void Cache_PutString( STRPTR str )
{
	while( (*cache_pointer++ = *str++ ) ) {
		if( !( --cache_free ) ) {
            debug("cache store\n");
            Cache_Store();
        }
    }
    cache_pointer--;
}
/* \\\ */
/* /// Cache_Printf()
*/

/*************************************************************************/

void Cache_Printf( STRPTR str, APTR args )
{
ULONG length;

    Cache_Store();
    length = SPrintfn( str, cache_pointer, cache_free, (ULONG *) args ) -1; /* subtract 0x00 */
    cache_pointer += length;
    cache_free -= length;
}
/* \\\ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       






