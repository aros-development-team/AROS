/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <string.h>

#include "includes.h"
#include "libraries.h"

#define NUM_LIBS 7

extern struct Library *SysBase;
struct Library *Libraries[NUM_LIBS];

STRPTR LibNames[] = {
	"dos.library", "intuition.library", "graphics.library", "icon.library",
	"commodities.library", "Garshnelib.library", "utility.library"
	};
LONG LibVersions[] = { 37, 37, 37, 37, 37, 37, 37 };

LONG OpenLibraries( VOID )
{
	LONG i, RetVal = 0L;

	for( i = 0; i < NUM_LIBS; i++ )
	{
		Libraries[i] = OpenLibrary( LibNames[i], LibVersions[i] );
		if( !Libraries[i] )
		{
			if( IntuitionBase )
			{
				struct EasyStruct ErrorReq = {
					sizeof( struct EasyStruct ), 0, "Information", 0L, "Ok" };
				BYTE ErrorStr[64];

				strcpy( ErrorStr, LibNames[i] );
				strcat( ErrorStr, " failed to open." );
				ErrorReq.es_TextFormat = ErrorStr;
				EasyRequestArgs( 0L, &ErrorReq, 0L, 0L );
			}
			RetVal = 1L;
		}
	}

	return RetVal;
}

VOID CloseLibraries( VOID )
{
	LONG i;

	for( i = 0; i < NUM_LIBS; i++ )
		if( Libraries[i] )
			CloseLibrary( Libraries[i] );
}
