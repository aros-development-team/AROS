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

#define DEBUG 0
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
	    error = (FRead((BPTR)iff->iff_Stream, cmd->sc_Buf, 1, cmd->sc_NBytes)) != cmd->sc_NBytes;
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

/* Decode35() based on ModifyIcon by Dirk Stöcker */

/****************************************************************************************/

static char *Decode35(unsigned char *buffer, unsigned char *outbuffer, long numbytes, long bits, long entries)
{
    int cop = 0, loop = 0, numbits = 0, mask, curentry = 0;
    unsigned long bitbuf = 0;
    unsigned char byte;

    mask = (1<<bits)-1;

    while((numbytes || (cop && numbits >= bits)) && (curentry < entries))
    {
	if(!cop)
	{
	    if(numbits < 8)
	    {
        	bitbuf = (bitbuf<<8)|*(buffer++); --numbytes;
        	numbits += 8;
	    }
	    byte = (bitbuf>>(numbits-8))&0xFF;
	    numbits -= 8;
	    if(byte <= 127)     cop = byte+1;
	    else if(byte > 128) loop = 256-byte+1;
	    else                continue;
	}
	if(cop) ++loop;

	if(numbits < bits)
	{
	    bitbuf = (bitbuf<<8)|*(buffer++); --numbytes;
	    numbits += 8;
	}
	byte = (bitbuf>>(numbits-bits))&mask;
	while(loop && (curentry < entries))
	{
	    *outbuffer++ = byte;
	    ++curentry;
	    --loop;
	}
	if(cop) --cop;
	numbits -= bits;
    }
    return curentry != entries ? "error decoding new icon data" : 0;
}

/****************************************************************************************/

STATIC BOOL ReadImage35(struct IFFHandle *iff, struct FileFaceChunk *fc,
    	    	    	struct FileImageChunk *ic, UBYTE **imgdataptr,
			UBYTE **imgpalptr, UBYTE **imgmaskptr, struct IconBase *IconBase)
{
    UBYTE *src, *mem, *imgdata, *imgpal = NULL, *imgmask = NULL;
    LONG size, imgsize, palsize, imagew, imageh, numcols;

    imagew = fc->Width + 1;
    imageh = fc->Height + 1;
    numcols = ic->NumColors + 1;
    
    imgsize = ic->NumImageBytes[0] * 256 + ic->NumImageBytes[1] + 1;
    if (ic->Flags & IMAGE35F_HASPALETTE)
    {
    	palsize = ic->NumPaletteBytes[0] * 256 + ic->NumPaletteBytes[1] + 1;
    }
    else
    {
    	palsize = 0;
    }

    size = imgsize + palsize;
        
    src = mem = AllocVec(size, MEMF_ANY);
    if (!src) return FALSE;
    
    if (ReadChunkBytes(iff, src, size) != size)
    {
    	FreeVec(src);
	return FALSE;
    }
    
    size = imagew * imageh;
    
    if (ic->Flags & IMAGE35F_HASPALETTE)
    {
    	size += numcols * sizeof(ULONG);
    }

    if (ic->Flags & IMAGE35F_HASTRANSPARENTCOLOR)
    {
    	size += RASSIZE(imagew, imageh);
    }
    
    imgdata = AllocVec(size, MEMF_ANY);
    if (!imgdata)
    {
    	FreeVec(src);
    	return FALSE;
    }
    
    if (ic->ImageFormat == 0)
    {
    	memcpy(imgdata, src, imagew * imageh);
    }
    else
    {
    	Decode35(src, imgdata, imgsize, ic->Depth, imagew * imageh);
    }

    if (ic->Flags & IMAGE35F_HASPALETTE)
    {
    	src += imgsize;
	
	imgpal = imgdata + imagew * imageh;
	
	if (ic->PaletteFormat == 0)
	{
	    ULONG i, r, g, b;

	    for(i = 0; i < numcols; i++)
	    {
	    	r = *src++;
		g = *src++;
		b = *src++;
		
		*((ULONG *)src)++ = (r << 16) | (g << 8) | b;
	    }
	}
	else
	{
	    ULONG i, r, g, b;
	    UBYTE *dst;
	    
	    Decode35(src, imgpal, palsize, 8, 3 * numcols);
	    
	    src = imgpal + 3 * numcols;
	    dst = imgpal + sizeof(ULONG) * numcols;
	    
	    for (i = 0; i < numcols; i++)
	    {
	    	b = *--src;
		g = *--src;
		r = *--src;
		
		*--((ULONG *)dst) = (r << 16) | (g << 8) | b;
	    }
	}

    } /* if (ic->Flags & IMAGE35F_HASPALETTE) */

    if (ic->Flags & IMAGE35F_HASTRANSPARENTCOLOR)
    {
    	UBYTE *dst, transpcolor;
    	WORD x, y, bpr, mask;

    	imgmask = imgdata + imagew * imageh;
	if (ic->Flags & IMAGE35F_HASPALETTE)
	{
	    imgmask += numcols * sizeof(ULONG);
	}
	
	memset(imgmask, 0xFF, RASSIZE(imagew, imageh));
	
	bpr = ((imagew + 15) & ~15) / 8;
	src = imgdata;
	dst = imgmask;
	
	transpcolor = ic->TransparentColor;
	
	for(y = 0; y < imageh; y++)
	{
	    UBYTE *dstx = dst;
	    
	    mask = 0x80;
	    
	    for(x = 0; x < imagew; x++)
	    {
	    	if (*src++ == transpcolor)
		{
		    *dstx &= ~mask;
		}
		
	    	mask >>= 1;
		if (!mask)
		{
		    mask = 0x80;
		    dstx++;
		}
	    }
	    dst += bpr;
	}
    }
    
    *imgdataptr = imgdata;
    *imgpalptr  = imgpal;
    *imgmaskptr = imgmask;
    
    FreeVec(mem);
    
    return TRUE;
}

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
    UBYTE   	    	    *img1data = NULL, *img2data = NULL,
    	    	    	    *img1pal = NULL, *img2pal = NULL,
			    *img1mask = NULL, *img2mask = NULL;
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
		    else if (have_face && (cn->cn_ID == ID_IMAG) && (cn->cn_Size >= sizeof(struct FileImageChunk)))
		    {
		    	D(bug("ReadIcon35. Found IMAG chunk\n"));
		    	if (ReadChunkBytes(iff, (have_imag1 ? &ic2 : &ic1), sizeof(ic1)) == sizeof(ic1))
			{
			    if (have_imag1)
			    {
			    	if (ReadImage35(iff, &fc, &ic2, &img2data, &img2pal, &img2mask, IconBase))
				{
			    	    have_imag2 = TRUE;
				}
			    }
			    else
			    {
			    	if (ReadImage35(iff, &fc, &ic1, &img1data, &img1pal, &img1mask, IconBase))
				{
			    	    have_imag1 = TRUE;
				}
			    }
			    
			}
			
		    }
		    
		    
		} /* while(!ParseIFF(iff, IFFPARSE_SCAN)) */
				
	    } /* if (!StopChunks(iff, stopchunks, 2)) */
	    
 	} /* if (!OpenIFF(iff, IFFF_READ)) */
	
    } /* if ((iff = AllocIFF())) */
    
    if (have_imag1)
    {
    	icon->icon35.width  = fc.Width + 1;
	icon->icon35.height = fc.Height + 1;
	icon->icon35.flags  = fc.Flags;
	icon->icon35.aspect = fc.Aspect;
	
	icon->icon35.img1.imagedata 	    = img1data;
	icon->icon35.img1.palette  	    = img1pal;
	icon->icon35.img1.mask	    	    = img1mask;
	icon->icon35.img1.numcolors 	    = ic1.NumColors + 1;
	icon->icon35.img1.depth     	    = ic1.Depth;
	icon->icon35.img1.flags     	    = ic1.Flags;
	icon->icon35.img1.transparentcolor  = ic1.TransparentColor;
	
	if (have_imag2)
	{
	    icon->icon35.img2.imagedata     	= img2data;
	    icon->icon35.img2.mask  	    	= img2mask;
	    icon->icon35.img2.depth 	    	= ic2.Depth;
	    icon->icon35.img2.flags 	    	= ic2.Flags;
	    icon->icon35.img2.transparentcolor	= ic2.TransparentColor;
	    
	    if (img2pal)
	    {
	    	icon->icon35.img2.palette   = img2pal;
		icon->icon35.img2.numcolors = ic2.NumColors + 1;		
	    }
	    else
	    {
	    	icon->icon35.img2.palette   = icon->icon35.img1.palette;
		icon->icon35.img2.numcolors = icon->icon35.img1.numcolors;
	    }
	}
    }
    
    return TRUE;   
}

/****************************************************************************************/

VOID FreeIcon35(struct NativeIcon *icon, struct IconBase *IconBase)
{
    if (icon->icon35.img1.imagedata) FreeVec(icon->icon35.img1.imagedata);
    if (icon->icon35.img2.imagedata) FreeVec(icon->icon35.img2.imagedata);
    
    /* We don't free img.palette/img.mask because imagedata/palette/mask
       were allocated together in one AllocVec() call */
    
}
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/

