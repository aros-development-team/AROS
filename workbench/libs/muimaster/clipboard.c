/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/alib_protos.h>
#include <libraries/iffparse.h>
#include <datatypes/textclass.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/utility.h>

#include "mui.h"
#include "clipboard.h"
#include "support.h"

#include "muimaster_intern.h"

//#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

void clipboard_write_text(STRPTR text, LONG textlen)
{
    struct IFFHandle *iff;
    
    if((iff = AllocIFF()))
    {
	if((iff->iff_Stream = (IPTR)OpenClipboard(0)))
	{
	    InitIFFasClip(iff);
	    if(!OpenIFF(iff,IFFF_WRITE))
	    {
		if(!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN))
		{
		    if(!PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN))
		    {
		        WriteChunkBytes(iff, text, textlen);
			
			PopChunk(iff);
			
		    } /* if(!PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN)) */
		    PopChunk(iff);
		    
		} /* if(!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN)) */
		CloseIFF(iff);
		
	    } /* if(!OpenIFF(iff,IFFF_WRITE)) */
	    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
	    
	} /* if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit))) */
	FreeIFF(iff);
	
    } /* if((iff = AllocIFF()))) */
    
}

STRPTR clipboard_read_text(void)
{    
    struct IFFHandle    *iff;
    struct ContextNode  *cn;
    STRPTR  	    	 retval = 0;
    
    if((iff = AllocIFF()))
    {
	if((iff->iff_Stream = (IPTR)OpenClipboard(0)))
	{
	    InitIFFasClip(iff);

	    if(!OpenIFF(iff, IFFF_READ))
	    {
		if (!(StopChunk(iff, ID_FTXT, ID_CHRS)))
		{
		    if (!ParseIFF(iff, IFFPARSE_SCAN))
		    {
		        cn = CurrentChunk(iff);

               		if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS) && (cn->cn_Size > 0))
			{
			    if ((retval = mui_alloc(cn->cn_Size + 1)))
			    {
			        ReadChunkBytes(iff, retval, cn->cn_Size);
				
				retval[cn->cn_Size] = '\0';
			    }
			    
			} /* if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS) && (cn->cn_Size > 0)) */
			
		    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
		    
		} /* if (!(StopChunk(iff, ID_FTXT, ID_CHRS))) */
		
		CloseIFF(iff);
		
	    } /* if(!OpenIFF(iff, IFFF_READ)) */
	    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
	    
	} /* if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit))) */
	FreeIFF(iff);
	
    } /* if((iff = AllocIFF()))) */

    return retval;
}

void clipboard_free_text(STRPTR text)
{
    mui_free(text);
}


