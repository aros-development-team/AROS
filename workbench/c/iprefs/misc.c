/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <prefs/prefhdr.h>
#include <aros/macros.h>

#define DEBUG 0
#include <aros/debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CHECK_PRHD_VERSION 1
#define CHECK_PRHD_SIZE    1

/*********************************************************************************************/

struct FilePrefHeader
{
    UBYTE ph_Version;
    UBYTE ph_Type;
    UBYTE ph_Flags[4];
};

/*********************************************************************************************/

struct IFFHandle *CreateIFF(STRPTR filename, LONG *stopchunks, LONG numstopchunks)
{
    struct IFFHandle *iff;
    
    D(bug("CreateIFF: filename = \"%s\"\n", filename));
    
    if ((iff = AllocIFF()))
    {
    	D(bug("CreateIFF: AllocIFF okay.\n"));
    	if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE)))
	{
    	    D(bug("CreateIFF: Open() okay.\n"));
	    InitIFFasDOS(iff);
	    
	    if (OpenIFF(iff, IFFF_READ) == 0)
	    {
	    	BOOL ok = FALSE;

    	    	D(bug("CreateIFF: OpenIFF okay.\n"));
		
	    	if ((StopChunk(iff, ID_PREF, ID_PRHD) == 0) &&
		    (StopChunks(iff, stopchunks, numstopchunks) == 0))
		{
    	    	    D(bug("CreateIFF: StopChunk(PRHD) okay.\n"));
		    
		    if (ParseIFF(iff, IFFPARSE_SCAN) == 0)
		    {
		    	struct ContextNode *cn;

			cn = CurrentChunk(iff);
			
    	    	    	D(bug("CreateIFF: ParseIFF okay. Chunk Type = %c%c%c%c  ChunkID = %c%c%c%c\n",
			      cn->cn_Type >> 24,
			      cn->cn_Type >> 16,
			      cn->cn_Type >> 8,
			      cn->cn_Type,
			      cn->cn_ID >> 24,
			      cn->cn_ID >> 16,
			      cn->cn_ID >> 8,
			      cn->cn_ID));
			
			if ((cn->cn_ID == ID_PRHD)
    	    	    #if CHECK_PRHD_SIZE
			    && (cn->cn_Size == sizeof(struct FilePrefHeader))
    	    	    #endif
    	    	    	   )
			{
			    struct FilePrefHeader h;
			    
    	    	    	    D(bug("CreateIFF: PRHD chunk okay.\n"));

		    	    if (ReadChunkBytes(iff, &h, sizeof(h)) == sizeof(h))
			    {
    	    	    	    	D(bug("CreateIFF: Reading PRHD chunk okay.\n"));

    	    	    	    #if CHECK_PRHD_VERSION
			    	if (h.ph_Version == PHV_CURRENT)
				{
    	    	    	    	    D(bug("CreateIFF: PrefHeader version is correct.\n"));
				    ok = TRUE;
				}
    	    	    	    #else
    	    	    	    	ok = TRUE;
    	    	    	    #endif	
			    			
			    }
			    
			}
			
		    } /* if (ParseIFF(iff, IFFPARSE_SCAN) == 0) */
		    
		} /* if ((StopChunk(iff, ID_PREF, ID_PRHD) == 0) && (StopChunks(... */
		
		if (!ok)
		{
		    CloseIFF(iff);
	    	    Close((BPTR)iff->iff_Stream);
		    FreeIFF(iff);
		    iff = NULL;
		}
		
	    } /* if (OpenIFF(iff, IFFF_READ) == 0) */
	    else
	    {
	    	Close((BPTR)iff->iff_Stream);
		FreeIFF(iff);
		iff = NULL;
	    }
	    
	} /* if ((iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE))) */
	else
	{
	   FreeIFF(iff); 
	   iff = NULL;
	}
	
    } /* if ((iff = AllocIFF())) */
    
    return iff;
}


/*********************************************************************************************/

void KillIFF(struct IFFHandle *iff)
{
    if (iff)
    {
    	CloseIFF(iff);
	Close((BPTR)iff->iff_Stream);
	FreeIFF(iff);
    }
}

/*********************************************************************************************/

APTR LoadChunk(struct IFFHandle *iff, LONG size, ULONG memtype)
{
    APTR data;
    struct ContextNode *cn = CurrentChunk(iff);

    D(bug("LoadChunk: Chunk size is %d, requested %d\n", cn->cn_Size, size));
    if (cn->cn_Size >= size) {
	data = AllocVec(cn->cn_Size, memtype);
	if (data) {
	    D(bug("[LoadChunk] Allocated buffer\n"));
	    if (ReadChunkBytes(iff, data, cn->cn_Size) == cn->cn_Size) {
   	        D(bug("LoadChunk: Reading chunk successful.\n"));

		return data;
	    }
	    FreeVec(data);
	}
    }
    return NULL;
}
