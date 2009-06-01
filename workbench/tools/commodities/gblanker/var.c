/*
 *	Copyright (c) 1994 Michael D. Bayne.
 *	All rights reserved.
 *
 *	Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <string.h>

#include "includes.h"
#include "protos/protos.h"

LONG GetVar37( STRPTR Name, STRPTR Buffer, LONG Size, ULONG Flags )
{
	LONG RetVal;

	if( ((struct Library *)SysBase)->lib_Version < 39 )
	{
		BYTE VarFileName[108];
		BPTR VarFile;

		strcpy( VarFileName, "ENVARC:" );
		AddPart( VarFileName, Name, 108 );

		if( VarFile = Open( VarFileName, MODE_OLDFILE ))
		{
			if( Flags & GVF_BINARY_VAR )
			{
				if( Flags & GVF_DONT_NULL_TERM )
					RetVal = Read( VarFile, Buffer, Size );
				else
				{
					LONG Bytes = Read( VarFile, Buffer, Size );

					Buffer[min( Bytes, Size-1 )] = '\0';
					RetVal = min( Bytes, Size-1 );
				}
			}
			else
			{
				FGets( VarFile, Buffer, Size );
				if( Buffer[strlen( Buffer )-1] == '\n' )
					Buffer[strlen( Buffer )-1] = '\0';
				RetVal = ( LONG )strlen( Buffer );
			}
			Close( VarFile );
		}
		else
			RetVal = -1;
	}
	else
		RetVal = GetVar( Name, Buffer, Size, Flags );

	return RetVal;
}

LONG SetVar37( STRPTR Name, STRPTR Buffer, LONG Size, ULONG Flags )
{
	if( !SetVar( Name, Buffer, Size, Flags ))
		return FALSE;
	
	if( ((struct Library *)SysBase)->lib_Version < 39 )
	{
		if( Flags & GVF_SAVE_VAR )
		{
			BYTE VarFileName[108];
			BPTR VarFile;

			strcpy( VarFileName, "ENVARC:" );
			AddPart( VarFileName, Name, 108 );

			if( VarFile = Open( VarFileName, MODE_NEWFILE ))
			{
				if(( Flags & GVF_DONT_NULL_TERM )||
				   ( Flags & GVF_BINARY_VAR ))
					Write( VarFile, Buffer, Size );
				else
				{
					Buffer[Size-1] = '\0';
					FPrintf( VarFile, "%s", Buffer );
				}
				Close( VarFile );
			}
			else
				return FALSE;
		}
	}

	return TRUE;
}
