/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

/****************************************************************************************/

#include <proto/dos.h>
#include <proto/alib.h>

#include <aros/bigendianio.h>
#include <aros/asmcall.h>

#include "icon_intern.h"

#define DEBUG 1
#include <aros/debug.h>

/****************************************************************************************/

#define ID_ICON MAKE_ID('I','C','O','N')
#define ID_FACE MAKE_ID('F','A','C','E')
#define ID_IMAG MAKE_ID('I','M','A','G')

/****************************************************************************************/

struct FileFaceChunk
{
    UBYTE Width;
    UBYTE Height;
    UBYTE Flags;
    UBYTE Aspect;
    UBYTE MaxPaletteBytes[2];
};

struct FileImageChunk
{
    UBYTE TransparentColor;
    UBYTE NumColors;
    UBYTE Flags;
    UBYTE ImageFormat;
    UBYTE PaletteFormat;
    UBYTE Depth;  
    UBYTE NumImageBytes[2];
    UBYTE NumPaletteBytes[2];
};

/****************************************************************************************/

#if 1
LONG MyDOSStreamHandler(struct Hook *hook, struct IFFHandle * iff, struct IFFStreamCmd * cmd)
{
    LONG error = 0;
    
    switch(cmd->sc_Command)
    {
    	case IFFCMD_INIT:
	case IFFCMD_CLEANUP:
	    error = 0;
	    break;
	    
	case IFFCMD_READ:
k	    error = (FRead((BPTR)iff->iff_Stream, cmd->sc_Buf, 1, cmd->sc_NBytes)) != cmd->sc_NBytes;
	    break;
	    
	case IFFCMD_WRITE:
	    error = (FWrite((BPTR)iff->iff_Stream, cmd->sc_Buf, 1, cmd->sc_NBytes)) != cmd->sc_NBytes;
	    break;
	    
	case IFFCMD_SEEK:
	    Flush((BPTR)iff->iff_Stream);
	    error = (Seek((BPTR)iff->iff_Stream, cmd->sc_NBytes, OFFSET_CURRENT)) == -1;
	    break;
    }
    
    return error;
}
#endif

/****************************************************************************************/

BOOL ReadIcon35(struct NativeIcon *icon, struct Hook *streamhook,
    	    	void *stream, struct IconBase *IconBase)
{
    static LONG stopchunks[] =
    {
    	ID_ICON, ID_FACE,
	ID_ICON, ID_IMAG
    };
    
    struct IFFHandle 	    *iff;
    struct Hook     	     iffhook;
    struct FileFaceChunk     fc;
    struct FileImageChunk    ic1, ic2;
    BOOL  have_face = FALSE, have_imag1 = FALSE, have_imag2 = FALSE;
    
    D(bug("ReadIcon35\n"));
    
    iffhook.h_Entry    = (HOOKFUNC)HookEntry;
    iffhook.h_SubEntry = (HOOKFUNC)MyDOSStreamHandler;
    
    if ((iff = AllocIFF()))
    {
    	D(bug("ReadIcon35. AllocIFF okay\n"));
 
    	iff->iff_Stream = (IPTR)stream;
	
	InitIFF(iff, IFFF_RSEEK, &iffhook);
	
	if (!OpenIFF(iff, IFFF_READ))
	{
    	    D(bug("ReadIcon35. OpenIFF okay\n"));
	    
	    if (!StopChunks(iff, stopchunks, 2))
	    {
	    	LONG error;
		
	    	D(bug("ReadIcon35. StopChunks okay\n"));
		
		while(!(error = ParseIFF(iff, IFFPARSE_SCAN)))
		{
		    struct ContextNode *cn;
		    
		    cn = CurrentChunk(iff);

    	    	    D(bug("inside ParseIFF loop\n"));		    
		    if ((cn->cn_ID == ID_FACE) && (cn->cn_Size >= sizeof(struct FileFaceChunk)))
		    {
		    	D(bug("ReadIcon35. Found FACE chunk\n"));
		    	if (ReadChunkBytes(iff, &fc, sizeof(fc)) == sizeof(fc))
			{
			    have_face = TRUE;
			}			
		    }
		    else if ((cn->cn_ID == ID_IMAG) && (cn->cn_Size >= sizeof(struct FileImageChunk)))
		    {
		    	D(bug("ReadIcon35. Found IMAG chunk\n"));
		    	if (ReadChunkBytes(iff, (have_imag1 ? &ic2 : &ic1), sizeof(ic1)) == sizeof(ic1))
			{
			    if (have_imag1) have_imag2 = TRUE; else have_imag1 = TRUE;
			}
			
		    }
		    
		    
		} /* while(!ParseIFF(iff, IFFPARSE_SCAN)) */
		
		kprintf("parseiff error %d\n", error);
		
	    } /* if (!StopChunks(iff, stopchunks, 2)) */
	    
 	} /* if (!OpenIFF(iff, IFFF_READ)) */
	
    } /* if ((iff = AllocIFF())) */
    
    return TRUE;   
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/

