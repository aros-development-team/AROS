/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <aros/debug.h>

#include <datatypes/pictureclass.h>

#include <proto/dos.h>
#include <proto/alib.h>

#include <zlib.h>

#include <aros/bigendianio.h>
#include <aros/asmcall.h>

#include "icon_intern.h"

/****************************************************************************************/

#define ID_ICON MAKE_ID('I','C','O','N')
#define ID_FACE MAKE_ID('F','A','C','E')
#define ID_IMAG MAKE_ID('I','M','A','G')
#define ID_png  MAKE_ID('p','n','g',' ')
#define ID_ARGB MAKE_ID('A','R','G','B')

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

static LONG MyDOSStreamHandler(struct Hook *hook, struct IFFHandle * iff, struct IFFStreamCmd * cmd)
{
    LONG error = 0;

    struct IconBase *IconBase = hook->h_Data;
    
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

static LONG MyMemStreamHandler(struct Hook *hook, struct IFFHandle * iff, struct IFFStreamCmd * cmd)
{
    LONG error = 0;
    APTR *buffp = (APTR *)iff->iff_Stream;
    APTR buff = *buffp;

    switch(cmd->sc_Command)
    {
    	case IFFCMD_INIT:
	case IFFCMD_CLEANUP:
	    error = 0;
	    break;
	    
	case IFFCMD_READ:
	    error = 0;
	    CopyMem(buff, cmd->sc_Buf, cmd->sc_NBytes);
	    (*buffp) += cmd->sc_NBytes;
	    break;
	    
	case IFFCMD_WRITE:
	    error = 0;
	    CopyMem(cmd->sc_Buf, buff, cmd->sc_NBytes);
	    (*buffp) += cmd->sc_NBytes;
	    break;
	    
	case IFFCMD_SEEK:
	    (*buffp) += cmd->sc_NBytes;
	    error = 0;
	    break;
    }
    
    return error;
}

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
	    if (*(src++) == transpcolor)
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
void *malloc(size_t size)
{
    return AllocVec(size, MEMF_PUBLIC);
}

void free(void *ptr)
{
    FreeVec(ptr);
}

STATIC BOOL ReadARGB35(struct DiskObject *icon, struct IFFHandle *iff, struct FileFaceChunk *fc,
    	    	    	LONG chunksize, UBYTE **argbptr,
			struct IconBase *IconBase)
{
    UBYTE *src, *mem, *argb = NULL;
    LONG imagew, imageh;
    ULONG zsize;
    uLongf size;
    int err;

    imagew = fc->Width + 1;
    imageh = fc->Height + 1;
    size = imagew * imageh * 4;
    
    D(bug("[%s] ARGB size is %dx%d, %d => %d bytes\n", __func__, imagew, imageh, chunksize, size));

    src = mem = AllocMemIcon(icon, chunksize, MEMF_PUBLIC, IconBase);
    if (!src) return FALSE;
    
    if (ReadChunkBytes(iff, src, chunksize) != chunksize)
    {
	goto fail;
    }
    
    argb = AllocMemIcon(icon, size, MEMF_PUBLIC, IconBase);
    if (!argb) goto fail;
 
    zsize = AROS_BE2WORD(*((UWORD *)(src + 6))) + 1;
    err = uncompress(argb, &size, src + 10, zsize);
    if (err != Z_OK) {
        D(bug("%s: Can't uncompress %d ARGB bytes: %s\n", __func__, zsize, zError(err)));
        goto fail;
    }

    *argbptr = argb;
    
    return TRUE;

fail:
    return FALSE;
}

/****************************************************************************************/

STATIC BOOL ReadImage35(struct DiskObject *icon, struct IFFHandle *iff, struct FileFaceChunk *fc,
    	    	    	struct FileImageChunk *ic, UBYTE **imgdataptr,
			struct ColorRegister **imgpalptr, struct IconBase *IconBase)
{
    UBYTE *src, *mem, *imgdata = NULL;
    struct ColorRegister *imgpal = NULL;
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

    D(bug("[%s] Image size is %dx%d, %d bytes\n", __func__, imagew, imageh, imgsize));
    D(bug("[%s] Palette size is %d colors, %d bytes\n", __func__, numcols, palsize));

    size = imgsize + palsize;
        
    src = mem = AllocMemIcon(icon, size, MEMF_PUBLIC, IconBase);
    if (!src) return FALSE;
    
    if (ReadChunkBytes(iff, src, size) != size)
    {
	return FALSE;
    }
    
    if (ic->Flags & IMAGE35F_HASPALETTE)
    {
        if (numcols < 1 || numcols > 256)
            return FALSE;
    
    	size = numcols * sizeof(struct ColorRegister);
    	imgpal = AllocMemIcon(icon, size, MEMF_PUBLIC, IconBase);
    	if (!imgpal) goto fail;
    }

    size = imagew * imageh;
    imgdata = AllocMemIcon(icon, size, MEMF_PUBLIC, IconBase);
    if (!imgdata) goto fail;
    
    if (ic->ImageFormat == 0)
    {
    	memcpy(imgdata, src, imagew * imageh);
    }
    else
    {
    	Decode35(src, imgdata, imgsize, ic->Depth, imagew * imageh);
    }
    src += imgsize;

    if (ic->Flags & IMAGE35F_HASPALETTE)
    {
    	struct ColorRegister *dst;
	
	dst = imgpal;

        D(bug("[%s] PaletteFormat %d, src %p\n", __func__, ic->PaletteFormat, src));
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
	    CONST_STRPTR err;
	    
	    err = Decode35(src, (UBYTE *)imgpal, palsize, 8, 3 * numcols);
	    if (err) {
	        D(bug("[%s] Palette decode error: %s\n", __func__, err));
	        imgpal = NULL;
            }

	    	    
	    if (imgpal && sizeof(struct ColorRegister) != 3)
	    {
	    	struct ColorRegister 	*dst;
	    	UWORD 	    	    	 i, r, g, b;

	    	D(bug("[%s] Realigning ColorRegister\n", __func__));
	
	    	src = (APTR)imgpal + 3 * numcols;
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

    *imgdataptr = imgdata;
    *imgpalptr  = imgpal;
    
    return TRUE;

fail:
    return FALSE;
}

/****************************************************************************************/

BOOL ReadIcon35(struct NativeIcon *icon, struct Hook *streamhook,
    	    	void *stream, struct IconBase *IconBase)
{
    static const LONG const stopchunks[] =
    {
    	ID_ICON, ID_FACE,
	ID_ICON, ID_IMAG,
	ID_ICON, ID_png,
	ID_ICON, ID_ARGB,
    };
    
    struct IFFHandle 	    *iff;
    struct Hook     	     iffhook;
    struct FileFaceChunk     fc;
    struct FileImageChunk    ic1, ic2;
    UBYTE                   *argb1data = NULL, *argb2data = NULL;
    UBYTE   	    	    *img1data = NULL, *img2data = NULL;
    struct ColorRegister    *img1pal = NULL, *img2pal = NULL;
    BOOL  have_face = FALSE, have_imag1 = FALSE, have_imag2 = FALSE;
    BOOL                     have_argb1 = FALSE, have_argb2 = FALSE;
    int have_png = 0;
    LONG here, extrasize;
    APTR data;
    
    D(bug("ReadIcon35, stream %p\n", stream));

    here = Seek((BPTR)stream, 0, OFFSET_CURRENT);
    D(bug("[%s] Extra data starts at %d\n", __func__, here));
    Seek((BPTR)stream, 0, OFFSET_END);
    extrasize = Seek((BPTR)stream, 0, OFFSET_CURRENT);
    D(bug("[%s] Extra data ends at %d\n", __func__, extrasize));
    extrasize -= here;
    Seek((BPTR)stream, here, OFFSET_BEGINNING);
    D(bug("[%s]   .. %d bytes\n", __func__, extrasize));

    /* No extra data? That's fine. */
    if (extrasize <= 0) {
        D(bug("[%s] No extra data\n", __func__));
        return TRUE;
    }

    if (IFFParseBase == NULL)
    	return FALSE;
    
    data = AllocMemIcon(&icon->ni_DiskObject, extrasize, MEMF_PUBLIC,
        IconBase);
    if (data == NULL)
        return FALSE;

    if (Read((BPTR)stream, data, extrasize) != extrasize) {
        D(bug("[%s] Can't read extra data\n", __func__));
        return FALSE;
    }

    icon->ni_Extra.Data = data;
    icon->ni_Extra.Size = extrasize;

    iffhook.h_Entry    = (HOOKFUNC)HookEntry;
    iffhook.h_SubEntry = (HOOKFUNC)MyMemStreamHandler;
    iffhook.h_Data     = (APTR)IconBase;
   
    if ((iff = AllocIFF()))
    {
    	D(bug("ReadIcon35. AllocIFF okay\n"));
 
    	iff->iff_Stream = (IPTR)&data;
	
	InitIFF(iff, IFFF_RSEEK, &iffhook);
	
	if (!OpenIFF(iff, IFFF_READ))
	{
    	    D(bug("ReadIcon35. OpenIFF okay\n"));
	    
	    if (!StopChunks(iff, stopchunks, 4))
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
			    	if (ReadImage35(&icon->ni_DiskObject, iff, &fc, &ic2, &img2data, &img2pal, IconBase))
				{
			    	    have_imag2 = TRUE;
				}
			    }
			    else
			    {
			    	if (ReadImage35(&icon->ni_DiskObject, iff, &fc, &ic1, &img1data, &img1pal, IconBase))
				{
			    	    have_imag1 = TRUE;
				}
			    }
			    
			}
                    } else if (cn->cn_ID == ID_ARGB) {
		    	D(bug("ReadIcon35. Found ARGB chunk\n"));
                        if (have_argb1)
                        {
                            if (ReadARGB35(&icon->ni_DiskObject, iff, &fc, cn->cn_Size, &argb2data, IconBase))
                            {
                                have_argb2 = TRUE;
                            }
                        }
                        else
                        {
                            if (ReadARGB35(&icon->ni_DiskObject, iff, &fc, cn->cn_Size, &argb1data, IconBase))
                            {
                                have_argb1 = TRUE;
                            }
			}
                    } else if (cn->cn_ID == ID_png) {
                        if (have_png < 2) {
                            LONG here = data - icon->ni_Extra.Data;
                            D(bug("[%s] Found PNG image %d\n", __func__, have_png));
                            icon->ni_Extra.PNG[have_png].Offset = here;
                            icon->ni_Extra.PNG[have_png].Size = cn->cn_Size;
                        } else {
                            D(bug("[%s] Ignoring PNG image %d\n", __func__, have_png));
                        }
                        have_png++;
		    } else {
		        D(bug("[%s] Unknown chunk ID 0x%08x\n", __func__, cn->cn_ID));
                    }
		    
		} /* while(!ParseIFF(iff, IFFPARSE_SCAN)) */
				
	    } /* if (!StopChunks(iff, stopchunks, 4)) */
	    
 	} /* if (!OpenIFF(iff, IFFF_READ)) */
	
	FreeIFF(iff);
	
    } /* if ((iff = AllocIFF())) */

    /* Only use the Width and Height fields of a FACE section
     * if it also contains image data.
     */
    if (have_face) {
        if (have_argb1 || have_argb2 || have_imag1 || have_imag2) {
            icon->ni_Face.Width  = fc.Width + 1;
            icon->ni_Face.Height = fc.Height + 1;
        }
        //icon->icon35.flags  = fc.Flags;
        icon->ni_Face.Aspect = fc.Aspect;
    }

    if (have_argb1)
    {
        icon->ni_Image[0].ARGB = (APTR)argb1data;
    }

    if (have_argb2)
    {
        icon->ni_Image[1].ARGB = (APTR)argb2data;
    }

    if (have_imag1)
    {
	icon->ni_Image[0].ImageData 	    = img1data;
	icon->ni_Image[0].Palette  	    = img1pal;
	icon->ni_Image[0].Pens  	    = ic1.NumColors + 1;
	icon->ni_Image[0].TransparentColor  = ic1.TransparentColor;

	if (icon->ni_Image[0].Pens < 1 || icon->ni_Image[0].Pens > 256)
	    return FALSE;
	
	if (have_imag2)
	{
            icon->ni_Image[1].ImageData         = img2data;
            icon->ni_Image[1].TransparentColor  = ic2.TransparentColor;
	    
	    if (img2pal)
	    {
                icon->ni_Image[1].Palette       = img2pal;
                icon->ni_Image[1].Pens          = ic2.NumColors + 1;
                if (icon->ni_Image[1].Pens < 1 || icon->ni_Image[0].Pens > 256)
                    return FALSE;
	
	    }
	    else
	    {
                icon->ni_Image[1].Palette       = img1pal;
                icon->ni_Image[1].Pens          = ic1.NumColors + 1;
	    }
	}
    }
    
    return TRUE;   
}

/****************************************************************************************/

/* Encode35() based on ModifyIcon source by Dirk Stöcker */

/****************************************************************************************/

static UBYTE *Encode35(struct DiskObject *icon, ULONG depth, UBYTE *dtype, LONG *dsize, ULONG size, CONST UBYTE *src, APTR IconBase)
{
    int   i, j, k;
    ULONG bitbuf, numbits;
    UBYTE *buf;
    LONG  ressize, numcopy, numequal;

    buf = AllocMemIcon(icon, size * 2, MEMF_PUBLIC, IconBase);
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

static BOOL WriteARGB35(struct IFFHandle *iff, struct NativeIcon *icon,
    	    	    	APTR ARGB, struct IconBase *IconBase)
{
    struct ARGB35_Header {
        ULONG ztype;    /* Always 1 */
        ULONG zsize;    /* Compressed size, -1 */
        UWORD resv;     /* Always 0 */
    } ahdr;
    Bytef *zdest;
    uLongf zsize, size;
    int err;
    BOOL ok = FALSE;

    /* Assume uncompressible.. */
    zsize = size = icon->ni_Face.Width * icon->ni_Face.Height * 4;

    zdest = AllocVec(zsize, MEMF_ANY);
    if (!zdest)
        return FALSE;

    err = compress(zdest, &zsize, ARGB, size);
    if (err != Z_OK) {
        D(bug("%s: Can't compress %d ARGB bytes: %s\n", __func__, size, zError(err)));
        goto exit;
    }

    ahdr.ztype = AROS_LONG2BE(1);
    ahdr.zsize = AROS_LONG2BE(zsize - 1);
    ahdr.resv = AROS_WORD2BE(0);

    if (!PushChunk(iff, ID_ICON, ID_ARGB, IFFSIZE_UNKNOWN))
    {
    	ok = TRUE;

    	if ((WriteChunkBytes(iff, &ahdr, sizeof(ahdr)) != sizeof(ahdr)) ||
            (WriteChunkBytes(iff, zdest, zsize) != zsize))
	{
	    ok = FALSE;
	}

	PopChunk(iff);
    }

exit:
    FreeVec(zdest);
    return ok;
}

/****************************************************************************************/

static BOOL WriteImage35(struct IFFHandle *iff, struct NativeIcon *icon,
    	    	    	 struct NativeIconImage *img, struct IconBase *IconBase)
{
    struct FileImageChunk   ic;
    UBYTE   	    	    *imagedata, *pal = NULL;
    LONG    	    	    imagesize, palsize = 0;
    UBYTE   	    	    imagepacked, palpacked = 0;
    UBYTE   	    	    imageflags = 0;
    BOOL    	    	    ok = FALSE;

    if (img->Palette)
    {
    	pal = Encode35(&icon->ni_DiskObject, 8, &palpacked, &palsize, img->Pens * 3, (APTR)img->Palette, IconBase);
	if (!pal) return FALSE;
    	imageflags |= IMAGE35F_HASPALETTE;
    }
    
    if (img->TransparentColor >= 0)
        imageflags |= IMAGE35F_HASTRANSPARENTCOLOR;
    
    imagedata = Encode35(&icon->ni_DiskObject, 8, &imagepacked, &imagesize,
    	    	    	 icon->ni_Face.Width * icon->ni_Face.Height, img->ImageData, IconBase);
		    
    if (!imagedata)
    {
	return FALSE;
    }
    
    ic.TransparentColor     = (imageflags & IMAGE35F_HASTRANSPARENTCOLOR) ? 255 : img->TransparentColor;
    ic.NumColors    	    = img->Pens - 1;
    ic.Flags	    	    = imageflags;
    ic.ImageFormat  	    = imagepacked;
    ic.PaletteFormat	    = palpacked;
    ic.Depth	    	    = 8;
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
    
    if (IFFParseBase == NULL)
    {
        D(bug("%s: IFFParseBase is NULL\n", __func__));
    	return FALSE;
    }
    
    if (!GetNativeIcon(&icon->ni_DiskObject, IconBase))
    {
        D(bug("%s: No native icon data\n", __func__));
    	return TRUE;
    }
    
    /* If the extra imagery wasn't modified, just write it out. */
    if (icon->ni_Extra.Size != 0) {
        LONG len;
        
        len = FWrite((BPTR)stream, icon->ni_Extra.Data, 1, icon->ni_Extra.Size);
        if (len < 0 || (ULONG)len != icon->ni_Extra.Size)
            return FALSE;

        return TRUE;
    }

    D(bug("%s: Write Icon %p\n", __func__, icon));
    D(bug("%s: n_Image[0].ImageData = %p\n", __func__, icon->ni_Image[0].ImageData));
    D(bug("%s: n_Image[1].ImageData = %p\n", __func__, icon->ni_Image[1].ImageData));
    D(bug("%s: n_Image[0].ARGB = %p\n", __func__, icon->ni_Image[0].ARGB));
    D(bug("%s: n_Image[1].ARGB = %p\n", __func__, icon->ni_Image[1].ARGB));

    if (icon->ni_Image[0].ImageData == NULL && icon->ni_Image[0].ARGB == NULL)
    {
        D(bug("%s: No image data to write\n", __func__));
    	return TRUE;
    }
    
    iffhook.h_Entry    = (HOOKFUNC)HookEntry;
    iffhook.h_SubEntry = (HOOKFUNC)MyDOSStreamHandler;
    iffhook.h_Data     = (APTR)IconBase;
    
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
		    
		    fc.Width  = icon->ni_Face.Width - 1;
		    fc.Height = icon->ni_Face.Height - 1;
		    fc.Flags  = (icon->ni_Frameless) ? ICON35F_FRAMELESS : 0;
		    fc.Aspect = icon->ni_Face.Aspect;
		    
		    cmapentries = icon->ni_Image[0].Pens;
		    
		    if (icon->ni_Image[1].ImageData &&
		        icon->ni_Image[1].Palette)
		    {
		    	if (icon->ni_Image[1].Pens > cmapentries)
			{
			    cmapentries = icon->ni_Image[1].Pens;
			}
		    }
		    
		    cmapentries = cmapentries * 3 - 1;
		    
		    fc.MaxPaletteBytes[0] = cmapentries / 256;
		    fc.MaxPaletteBytes[1] = cmapentries & 255;
		    
		    if (WriteChunkBytes(iff, &fc, sizeof(fc)) == sizeof(fc))
		    {
		    	D(bug("WriteIcon35. WriteChunkBytes of FACE chunk ok.\n"));
			
			PopChunk(iff);
			
			if (icon->ni_Image[0].ImageData)
                        {
                            if (WriteImage35(iff, icon, &icon->ni_Image[0], IconBase))
                            {
                                D(bug("WriteIcon35. WriteImage35() of 1st image ok.\n"));
                                
                                if (icon->ni_Image[1].ImageData)
                                {
                                    if (WriteImage35(iff, icon, &icon->ni_Image[1], IconBase))
                                    {
                                        D(bug("WriteIcon35. WriteImage35() of 2nd image ok.\n"));
                                        
                                        ok = TRUE;
                                    }
                                    
                                } /* if (icon->ni_Image[1].ImageData) */
                                else
                                {
                                    ok = TRUE;
                                }
                                
                            } /* if (WriteImage35(iff, &icon, &icon->ni_Image[0], IconBase)) */
                        } /* if (icon->ni_Image[0].ImageData) */

                        if (icon->ni_Image[0].ARGB)
                        {
                            if (WriteARGB35(iff, icon, &icon->ni_Image[0].ARGB, IconBase))
                            {
                                D(bug("WriteIcon35. WriteImage35() of 1st image ok.\n"));
                                if (icon->ni_Image[1].ARGB)
                                {
                                        if (WriteARGB35(iff, icon, &icon->ni_Image[1].ARGB, IconBase))
                                        {
                                            D(bug("WriteIcon35. WriteImage35() of 2nd image ok.\n"));
                                            ok = TRUE;
                                        }
                                } else {
                                    ok = TRUE;
                                }
                            } /* if (WriteARGB35(iff, icon, &icon->ni_Image[0].ARGB, IconBase) */
                        } /* if (icon->ni_Image[0].ARGB) */

		    } /* if (WriteChunkBytes(iff, &fc, sizeof(fc)) == sizeof(fc)) */
		    else
		    {
		    	PopChunk(iff);
		    }
		       
		} /* if (!PushChunk(iff, ID_ICON, ID_FACE, sizeof(struct FileFaceChunk))) */

		if (icon->ni_Extra.PNG[0].Size &&
		    !PushChunk(iff, ID_ICON, ID_png, IFFSIZE_UNKNOWN)) {
		    WriteChunkBytes(iff, icon->ni_Extra.Data + icon->ni_Extra.PNG[0].Offset, icon->ni_Extra.PNG[0].Size);

		    PopChunk(iff);
                }

		if (icon->ni_Extra.PNG[1].Size &&
		    !PushChunk(iff, ID_ICON, ID_png, IFFSIZE_UNKNOWN)) {
		    WriteChunkBytes(iff, icon->ni_Extra.Data + icon->ni_Extra.PNG[1].Offset, icon->ni_Extra.PNG[1].Size);

		    PopChunk(iff);
                }
		
	    	PopChunk(iff);
		
	    } /* if (!PushChunk(iff, ID_ICON, ID_FORM, IFFSIZE_UNKNOWN)) */
	    
 	} /* if (!OpenIFF(iff, IFFF_READ)) */
	
    } /* if ((iff = AllocIFF())) */
        
    return ok;   
}

/****************************************************************************************/

VOID FreeIcon35(struct NativeIcon *icon, struct IconBase *IconBase)
{
}

/****************************************************************************************/

