/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "compilerspecific.h"
#include "debug.h"

#include "gifclass.h"
#include "codec.h"

/**************************************************************************************************/

	
short DecompressInit(GifHandleType *gifhandle)
{
	return TRUE;
}

short DecompressLine(GifHandleType *gifhandle)
{
	UBYTE	b;
    while (1)
	{
			if ( !(gifhandle->filebufbytes--) && !LoadGIF_FillBuf(gifhandle, 1) )
			{
				D(bug("gif.datatype/DecompressLine() --- buffer underrun\n"));
				return -1;
			}
			b = *(gifhandle->filebufpos)++;
		if ( !(gifhandle->linebufbytes--) )
		{
			D(bug("gif.datatype/DecompressLine() --- line buffer full\n"));
			return -2;
		}
		*(gifhandle->linebufpos)++ = b;
	}
return TRUE;
}
