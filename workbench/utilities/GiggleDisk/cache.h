#ifndef CACHE_H
#define CACHE_H 1

/*************************************************************************/

#include <exec/types.h>

/*************************************************************************/

/*
** prototypes
*/

void Cache_Init( BPTR handle );
void Cache_Close( void );
void Cache_Store( void );
void Cache_PutChar( char chr );
void Cache_PutHex( UBYTE num );
void Cache_PutString( STRPTR str );
void Cache_Printf( STRPTR str, APTR args );

/*************************************************************************/

#endif /* CACHE_H */
