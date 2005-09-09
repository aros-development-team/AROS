/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <datatypes/pictureclass.h>

#include <proto/dos.h>
#include <proto/alib.h>

#include <aros/bigendianio.h>
#include <aros/asmcall.h>

#include "icon_intern.h"

#   define DEBUG 0
#   include <aros/debug.h>

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

static char *Decode35(UBYTE *buffer, UBYTE *outbuffer, LONG numbytes, LONG bits, LONG entries)
{
    int cop = 0, loop = 0, numbits = 0, mask, curentry = 0;
    ULONG bitbuf = 0;
    UBYTE byte;

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

VOID MakeMask35(UBYTE *imgdata, UBYTE *imgmask, UBYTE transpcolor, LONG imagew, LONG imageh)
{
    UBYTE *src, *dst;
    WORD x, y, bpr, mask;

    memset(imgmask, 0xFF, RASSIZE(imagew, imageh));

    bpr = ((imagew + 15) & ~15) / 8;
    src = imgdata;
    dst = imgmask;

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
    	size += numcols * sizeof(struct ColorRegister);
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
    	struct ColorRegister *dst;
	
    	src += imgsize;
	
	imgpal = imgdata + imagew * imageh;
	
	dst = (struct ColorRegister *)imgpal;
	
	if (ic->PaletteFormat == 0)
	{
	    ULONG i, r, g, b;

	    for(i = 0; i < numcols; i++)
	    {
	    	r = *src++;
		g = *src++;
		b = *src++;
		
		dst->red   = r;
		dst->green = g;
		dst->blue  = b;
		dst++;
	    }
	}
	else
	{
	    Decode35(src, imgpal, palsize, 8, 3 * numcols);
	    	    
	    if (sizeof(struct ColorRegister) != 3)
	    {
	    	struct ColorRegister 	*dst;
	    	UWORD 	    	    	 i, r, g, b;
	
	    	src = imgpal + 3 * numcols;
	    	dst = (struct ColorRegister *)(imgpal + sizeof(struct ColorRegister) * numcols);
	    
		for (i = 0; i < numcols; i++)
		{
	    	    b = *--src;
		    g = *--src;
		    r = *--src;

		    dst--;

		    dst->red = r;
		    dst->green = g;
		    dst->blue = b;
		}
	    }
	}

    } /* if (ic->Flags & IMAGE35F_HASPALETTE) */

    if (ic->Flags & IMAGE35F_HASTRANSPARENTCOLOR)
    {
    	imgmask = imgdata + imagew * imageh;
	if (ic->Flags & IMAGE35F_HASPALETTE)
	{
	    imgmask += numcols * sizeof(struct ColorRegister);
	}
	
	MakeMask35(imgdata, imgmask,ic->TransparentColor, imagew, imageh);
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
	
	FreeIFF(iff);
	
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

/* Encode35() based on ModifyIcon source by Dirk Stöcker */

/****************************************************************************************/

static UBYTE *Encode35(ULONG depth, UBYTE *dtype, LONG *dsize, ULONG size, UBYTE *src)
{
    int   i, j, k;
    ULONG bitbuf, numbits;
    UBYTE *buf;
    LONG  ressize, numcopy, numequal;

    buf = AllocVec(size * 2, MEMF_ANY);
    if (!buf) return NULL;

    numcopy = 0;
    numequal = 1;
    bitbuf = 0;
    numbits = 0;
    ressize = 0;
    k = 0; /* the really output pointer */

    for(i = 1; numequal || numcopy;)
    {
	if(i < size && numequal && (src[i-1] == src[i]))
	{
	    ++numequal; ++i;
	}
	else if(i < size && numequal*depth <= 16)
	{
	    numcopy += numequal; numequal = 1; ++i;
	}
	else
	{
	    /* care for end case, where it maybe better to join the two */
	    if(i == size && numcopy + numequal <= 128 && (numequal-1)*depth <= 8)
	    {
        	numcopy += numequal; numequal = 0;
	    }

	    if(numcopy)
	    {
        	if((j = numcopy) > 128) j = 128;

        	bitbuf = (bitbuf<<8) | (j-1);
        	numcopy -= j;
	    }
	    else
	    {
        	if((j = numequal) > 128) j = 128;

        	bitbuf = (bitbuf<<8) | (256-(j-1));
        	numequal -= j;
        	k += j-1;
        	j = 1;
	    }

	    buf[ressize++] = (bitbuf >> numbits);

	    while(j--)
	    {
        	numbits += depth;
        	bitbuf = (bitbuf<<depth) | src[k++];

        	if(numbits >= 8)
        	{
        	    numbits -= 8;
        	    buf[ressize++] = (bitbuf >> numbits);
        	}
	    }

	    if(i < size && !numcopy && !numequal)
	    {
        	numequal = 1; ++i;
	    }
	}
    }

    if(numbits)
        buf[ressize++] = bitbuf << (8-numbits);

    if(ressize > size) /* no RLE */
    {
        ressize = size;
        *dtype = 0;
        for(i = 0; i < size; ++i)
	    buf[i]= src[i];
    }
    else
        *dtype = 1;

    *dsize = ressize;

    return buf;
}

/****************************************************************************************/

static BOOL WriteImage35(struct IFFHandle *iff, struct Icon35 *icon35,
    	    	    	 struct Image35 *img, struct IconBase *IconBase)
{
    struct FileImageChunk   ic;
    UBYTE   	    	    *imagedata, *pal = NULL;
    LONG    	    	    imagesize, palsize = 0;
    UBYTE   	    	    imagepacked, palpacked = 0;
    UBYTE   	    	    imageflags;
    BOOL    	    	    ok = FALSE;
    
    imageflags = img->flags;
    
    if (img->palette && (img->flags & IMAGE35F_HASPALETTE))
    {
    	pal = Encode35(8, &palpacked, &palsize, img->numcolors * 3, img->palette);
	if (!pal) return FALSE;
    }
    else
    {
    	imageflags &= ~IMAGE35F_HASPALETTE;
    }
    
    imagedata = Encode35(img->depth, &imagepacked, &imagesize,
    	    	    	 icon35->width * icon35->height, img->imagedata);
		    
    if (!imagedata)
    {
    	if (pal) FreeVec(pal);
	return FALSE;
    }
    
    ic.TransparentColor     = img->transparentcolor;
    ic.NumColors    	    = img->numcolors - 1;
    ic.Flags	    	    = imageflags;
    ic.ImageFormat  	    = imagepacked;
    ic.PaletteFormat	    = palpacked;
    ic.Depth	    	    = img->depth;
    ic.NumImageBytes[0]	    = (imagesize - 1) / 256;
    ic.NumImageBytes[1]     = (imagesize - 1) & 255;
    ic.NumPaletteBytes[0]   = (palsize - 1) / 256;
    ic.NumPaletteBytes[1]   = (palsize - 1) & 255;
    
    if (!PushChunk(iff, ID_ICON, ID_IMAG, IFFSIZE_UNKNOWN))
    {
    	ok = TRUE;

    	if (WriteChunkBytes(iff, &ic, sizeof(ic)) != sizeof(ic))
	{
	    ok = FALSE;
	}

	if (ok)
	{
	    if (WriteChunkBytes(iff, imagedata, imagesize) != imagesize)
	    {
	    	ok = FALSE;
	    }
	}

    	if (ok && pal)
	{
	    if (WriteChunkBytes(iff, pal, palsize) != palsize)
	    {
	    	ok = FALSE;
	    }
	}
	
	
	PopChunk(iff);
	
    } /* if (!PushChunk(iff, ID_ICON, ID_IMAG, IFFSIZE_UNKNOWN)) */
    
    FreeVec(imagedata);
    if (pal) FreeVec(pal);
    
    return ok;
}

/****************************************************************************************/

BOOL WriteIcon35(struct NativeIcon *icon, struct Hook *streamhook,
    	    	 void *stream, struct IconBase *IconBase)
{
    struct IFFHandle 	    *iff;
    struct Hook     	     iffhook;
    struct FileFaceChunk     fc;
    BOOL    	    	     ok = FALSE;
    
    D(bug("WriteIcon35\n"));
    
    if (!GetNativeIcon(&icon->dobj, IconBase))
    {
    	return TRUE;
    }
    
    iffhook.h_Entry    = (HOOKFUNC)HookEntry;
    iffhook.h_SubEntry = (HOOKFUNC)MyDOSStreamHandler;
    
    if ((iff = AllocIFF()))
    {
    	D(bug("WriteIcon35. AllocIFF okay\n"));
 
    	iff->iff_Stream = (IPTR)stream;
	
	InitIFF(iff, IFFF_RSEEK, &iffhook);
	
	if (!OpenIFF(iff, IFFF_WRITE))
	{
    	    D(bug("WriteIcon35. OpenIFF okay\n"));
	    
	    if (!PushChunk(iff, ID_ICON, ID_FORM, IFFSIZE_UNKNOWN))
	    {
   	    	D(bug("WriteIcon35. PushChunk(ID_ICON, ID_FORM) okay\n"));
		
		if (!PushChunk(iff, ID_ICON, ID_FACE, sizeof(struct FileFaceChunk)))
		{
		    WORD cmapentries;
   	    	
		    D(bug("WriteIcon35. PushChunk(ID_ICON, ID_FACE) okay\n"));
		    
		    fc.Width  = icon->icon35.width - 1;
		    fc.Height = icon->icon35.height - 1;
		    fc.Flags  = icon->icon35.flags;
		    fc.Aspect = icon->icon35.aspect;
		    
		    cmapentries = icon->icon35.img1.numcolors;
		    
		    if (icon->icon35.img2.imagedata &&
		        icon->icon35.img2.palette &&
			(icon->icon35.img2.flags & IMAGE35F_HASPALETTE))
		    {
		    	if (icon->icon35.img2.numcolors > cmapentries)
			{
			    cmapentries = icon->icon35.img2.numcolors;
			}
		    }
		    
		    cmapentries = cmapentries * 3 - 1;
		    
		    fc.MaxPaletteBytes[0] = cmapentries / 256;
		    fc.MaxPaletteBytes[1] = cmapentries & 255;
		    
		    if (WriteChunkBytes(iff, &fc, sizeof(fc)) == sizeof(fc))
		    {
		    	D(bug("WriteIcon35. WriteChunkBytes of FACE chunk ok.\n"));
			
			PopChunk(iff);
			
			if (WriteImage35(iff, &icon->icon35, &icon->icon35.img1, IconBase))
			{
		    	    D(bug("WriteIcon35. WriteImage35() of 1st image ok.\n"));
			    
			    if (icon->icon35.img2.imagedata)
			    {
			    	if (WriteImage35(iff, &icon->icon35, &icon->icon35.img2, IconBase))
				{
		    	    	    D(bug("WriteIcon35. WriteImage35() of 2nd image ok.\n"));
				    
				    ok = TRUE;
				}
				
			    } /* if (icon->icon35.img2.imagedata) */
			    else
			    {
			    	ok = TRUE;
			    }
			    
			} /* if (WriteImage35(iff, &icon->icon35, &icon->icon35.img1, IconBase)) */
			
		    } /* if (WriteChunkBytes(iff, &fc, sizeof(fc)) == sizeof(fc)) */
		    else
		    {
		    	PopChunk(iff);
		    }
		       
		} /* if (!PushChunk(iff, ID_ICON, ID_FACE, sizeof(struct FileFaceChunk))) */
		
	    	PopChunk(iff);
		
	    } /* if (!PushChunk(iff, ID_ICON, ID_FORM, IFFSIZE_UNKNOWN)) */
	    
 	} /* if (!OpenIFF(iff, IFFF_READ)) */
	
    } /* if ((iff = AllocIFF())) */
        
    return ok;   
}

/****************************************************************************************/

VOID FreeIcon35(struct NativeIcon *icon, struct IconBase *IconBase)
{
    if (icon->icon35.img1.imagedata) FreeVec(icon->icon35.img1.imagedata);
    if (icon->icon35.img2.imagedata) FreeVec(icon->icon35.img2.imagedata);
    
    icon->icon35.img1.imagedata = NULL;
    icon->icon35.img2.imagedata = NULL;
    
    /* We don't free img.palette/img.mask because imagedata/palette/mask
       were allocated together in one AllocVec() call */
    
}

/****************************************************************************************/

