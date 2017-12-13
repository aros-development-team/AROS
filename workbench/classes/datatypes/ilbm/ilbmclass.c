/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <graphics/modeid.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>

#ifdef __AROS__
#include <aros/symbolsets.h>
ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);
#else
#include "compilerspecific.h"
#endif

#include "debug.h"

#include "methods.h"


/**************************************************************************************************/

struct FileBitMapHeader
{
    UBYTE        bmh_Width[2];
    UBYTE        bmh_Height[2];
    UBYTE        bmh_Left[2];
    UBYTE        bmh_Top[2];
    UBYTE        bmh_Depth;
    UBYTE        bmh_Masking;
    UBYTE        bmh_Compression;
    UBYTE        bmh_Pad;
    UBYTE        bmh_Transparent[2];
    UBYTE        bmh_XAspect;
    UBYTE        bmh_YAspect;
    UBYTE        bmh_PageWidth[2];
    UBYTE        bmh_PageHeight[2];
};

/**************************************************************************************************/

static LONG propchunks[] =
{
    ID_ILBM, ID_BMHD,
    ID_ILBM, ID_CMAP,
    ID_ILBM, ID_CAMG
};

/**************************************************************************************************/

static UBYTE *UnpackByteRun1(UBYTE *source, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE c;

    for(;;)
    {
	c = (BYTE)(*source++);
	if (c >= 0)
	{
    	    while(c-- >= 0)
	    {
		*dest++ = *source++;
		if (--unpackedsize <= 0) return source;
	    }
	}
	else if (c != -128)
	{
    	    c = -c;
	    r = *source++;

	    while(c-- >= 0)
	    {
		*dest++ = r;
		if (--unpackedsize <= 0) return source;
	    }
	}
    }   
}

/**************************************************************************************************/

static BOOL ReadBitMapPic(Class *cl, Object *o, struct IFFHandle *handle, struct BitMapHeader *bmhd,
    	    	       struct FileBitMapHeader *file_bmhd, struct ContextNode *body_cn)
{
    struct BitMap *bm;
    UBYTE   	  *src, *body, *uncompress_buf;
    LONG    	   y, p, w16, bm_bpr, body_bpr, copy_bpr, totdepth;
    
    totdepth = bmhd->bmh_Depth;
    if (file_bmhd->bmh_Masking == mskHasMask) totdepth++;
    
    w16      = (bmhd->bmh_Width + 15) & ~15;
    body_bpr = (w16 >> 3);

    p = body_cn->cn_Size;
    if ((file_bmhd->bmh_Compression == cmpByteRun1))
    {
    	p += body_bpr * totdepth;
    }
    
    body = AllocVec(p, MEMF_ANY);
    if (!body)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return FALSE;
    }
    
    if (ReadChunkBytes(handle, body, body_cn->cn_Size) != body_cn->cn_Size)
    {
    	FreeVec(body);
	SetIoErr(ERROR_UNKNOWN);
	return FALSE;
    }
    
    bm = AllocBitMap(bmhd->bmh_Width, bmhd->bmh_Height, bmhd->bmh_Depth, BMF_CLEAR, NULL);
    if (!bm)
    {
    	FreeVec(body);
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    
    bm_bpr   = bm->BytesPerRow;
    copy_bpr = (body_bpr < bm_bpr) ? body_bpr : bm_bpr;
    
    switch(file_bmhd->bmh_Compression)
    {
    	case cmpNone:
	    src = body;
	    for(y = 0; y < bmhd->bmh_Height; y++)
	    {
	    	for(p = 0; p < bmhd->bmh_Depth; p++)
		{
		    UBYTE *dest;
		    
		    dest = bm->Planes[p] + y * bm_bpr;
		    
		    CopyMem(src, dest, copy_bpr);
		    src += body_bpr;
		}
		
		if (file_bmhd->bmh_Masking == mskHasMask) src += body_bpr;
	    }
	    break;
	    
	case cmpByteRun1:
	    uncompress_buf = body + body_cn->cn_Size;
	    src = body;
	    
	    for(y = 0; y < bmhd->bmh_Height; y++)
	    {
	    	UBYTE *copysrc = uncompress_buf;
		
	    	src = UnpackByteRun1(src, uncompress_buf, body_bpr * totdepth);
		
		for(p = 0; p < bmhd->bmh_Depth; p++)
		{
		    UBYTE *dest;

		    dest = bm->Planes[p] + y * bm_bpr;
		    
		    CopyMem(copysrc, dest, copy_bpr);
		    copysrc += body_bpr;
		}
		
	    }
	    break;
	    
    } /* switch(file_bmhd->bmh_Compression) */
    
    SetDTAttrs(o, NULL, NULL, PDTA_BitMap, (IPTR)bm, TAG_DONE);

    FreeVec(body);
    SetIoErr(0);
    
    return TRUE;
}

/**************************************************************************************************/

const UBYTE bitmask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0 };

static BOOL ReadRGBPic(Class *cl, Object *o, struct IFFHandle *handle, struct BitMapHeader *bmhd,
    	    	       struct FileBitMapHeader *file_bmhd, struct ContextNode *body_cn, UBYTE *coltab)
{
    UBYTE   	*src, *srcline, *srclinestart, *chunkystart, *chunky, *body, *compressed=0, *uncompressed=0, *maskptr;
    int		width, height, numplanes, mask, hamrot1, hamrot2;
    LONG    	x, y, p, w16, body_bpr, bodysize;
    ULONG	rgb;
    UBYTE	r, g, b, hmask, mmask;
    BOOL	compress;
    
    width  = bmhd->bmh_Width;
    height = bmhd->bmh_Height;
    numplanes = bmhd->bmh_Depth;
    w16      = (width + 15) & ~15;
    body_bpr = (w16 >> 3);
    bodysize = body_cn->cn_Size;

    p = bodysize + width * 3;
    if ((file_bmhd->bmh_Compression == cmpByteRun1))
    {
    	p += body_bpr * numplanes;
    }
    
    body = AllocVec(p, MEMF_ANY);
    if (!body)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return FALSE;
    }
    
    // D(bug("ilbm.datatype/ReadRGB: Width %d Height %d Depth %d body_bpr %ld bodysize %ld p %ld body %lx\n", width, height, numplanes, body_bpr, bodysize, p, body));

    if (ReadChunkBytes(handle, body, bodysize) != bodysize)
    {
    	FreeVec(body);
	SetIoErr(ERROR_UNKNOWN);
	return FALSE;
    }
    
    hamrot1 = 10 - numplanes;
    hamrot2 = numplanes - 2;
    mmask = 0xff << (numplanes - 6);
    hmask = ~mmask;
    compress = FALSE;
    switch( file_bmhd->bmh_Compression )
    {
	case cmpByteRun1:
	    compressed = body;
	    uncompressed = body + bodysize + width * 3;
	    compress = TRUE;
    	case cmpNone:
	    src = body;
	    chunkystart = body + bodysize;
	    for(y = 0; y < height; y++)
	    {
		chunky = chunkystart;
		srclinestart = src;
		if( compress )
		{
		    compressed = UnpackByteRun1(compressed, uncompressed, body_bpr * numplanes);
		    srclinestart = uncompressed;
		}
		r = g = b = 0;
		maskptr = (UBYTE *) bitmask;
	    	for(x = 0; x < width; x++)
		{
		    mask = *maskptr++;
		    if( !mask )
		    {
			maskptr = (UBYTE *) bitmask;
			mask = *maskptr++;
			srclinestart++;
		    }
		    srcline = srclinestart;
		    rgb = 0;
		    for(p = 0; p < numplanes; p++)
		    {
			rgb = (rgb >> 1) | (*srcline & mask ? 0x800000 : 0);
			srcline += body_bpr;
		    }
		    // D(bug("ilbm.datatype/ReadRGB: RGB %06lx mask %02x srcline %lx chunky %lx\n", rgb, mask, srcline, chunky));

		    /* Process HAM or RGB data (a color table implies HAM) */
		    if( coltab )
		    {
			rgb >>= 14;
			rgb |= (rgb & 0xff) >> hamrot2;
			switch( rgb & 0x300 )
			{
			    case 0x000:
				rgb >>= hamrot1;
				rgb *= 3;
				*chunky++ = r = coltab[rgb++];
				*chunky++ = g = coltab[rgb++];
				*chunky++ = b = coltab[rgb];
				break;
			    case 0x100:
				*chunky++ = r;
				*chunky++ = g;
				*chunky++ = b = (rgb & mmask) | (b & hmask);
				break;
			    case 0x200:
				*chunky++ = r = (rgb & mmask) | (r & hmask);
				*chunky++ = g;
				*chunky++ = b;
				break;
			    case 0x300:
				*chunky++ = r;
				*chunky++ = g = (rgb & mmask) | (g & hmask);
				*chunky++ = b;
				break;
			}
		    }
		    else
		    {
			*chunky++ = rgb & 0xff;
			*chunky++ = (rgb >> 8) & 0xff;
			*chunky++ = (rgb >> 16) & 0xff;
		    }
		}
		
		if( !DoSuperMethod(cl, o,
			    PDTM_WRITEPIXELARRAY,   /* Method_ID */
			    (IPTR) chunkystart,	    /* PixelData */
			    PBPAFMT_RGB,	    /* PixelFormat */
			    width*3,		    /* PixelArrayMod (number of bytes per row) */
			    0,			    /* Left edge */
			    y,			    /* Top edge */
			    width,		    /* Width */
			    1))			    /* Height (here: one line) */
		{
		    D(bug("ilbm.datatype/ReadRGB: WRITEPIXELARRAY failed\n"));
		    FreeVec(body);
		    SetIoErr(ERROR_UNKNOWN);
		    return FALSE;
		}

		src += body_bpr * numplanes;
	    }
	    break;
    } /* switch(file_bmhd->bmh_Compression) */
    
    FreeVec(body);
    SetIoErr(0);
    
    return TRUE;
}

/**************************************************************************************************/

static void FixColRegs(ULONG numcolors, UBYTE *srcstart)
{
    WORD i;
    UBYTE n = 0, *src;

    /* Check if all color elements have an empty lower nibble */
    src = srcstart;
    for (i = 0; i < numcolors * 3; i++)
	n |= *src++;
    src = srcstart;

    /* If so, scale all color elements */
    if ((n & 0xf) == 0)
    {
	for (i = 0; i < numcolors * 3; i++)
	{
	    n = *src;
	    n |= n >> 4;
	    *src++ = n;
	}
    }
}

/**************************************************************************************************/

static void CopyColRegs(Object *o, ULONG numcolors, UBYTE *srcstart, BOOL ehb)
{
    struct ColorRegister    *colorregs;
    ULONG    	    	    *cregs;

    SetDTAttrs(o, NULL, NULL, PDTA_NumColors, numcolors, TAG_DONE);
    
    if (GetDTAttrs(o, PDTA_ColorRegisters   , (IPTR)&colorregs,
		      PDTA_CRegs    	    , (IPTR)&cregs	,
		      TAG_DONE	    	    	    	    	 ) == 2)
    {
	if (colorregs && cregs)
	{
	    LONG	i, j;
	    int		cnt = 0;
	    int		r, g, b;
	    UBYTE	*src;
	    
	    if( ehb )
	    {
		cnt = 1;
	    }
	    for(j = 0; j <= cnt; j++)
	    {
		src = srcstart;
		for(i = 0; i < numcolors; i++)
		{
		    r = *src++;
		    g = *src++;
		    b = *src++;

		    /* Halve the brightness on the second (EHB) round */
		    if( j )
		    {
			r >>= 1;
			g >>= 1;
			b >>= 1;
		    }

		    colorregs->red   = r;
		    colorregs->green = g;
		    colorregs->blue  = b;
		    colorregs++;
		    
		    *cregs++ = (ULONG)r * 0x01010101;
		    *cregs++ = (ULONG)g * 0x01010101;
		    *cregs++ = (ULONG)b * 0x01010101;
		}
	    }
	}
    }
}

/**************************************************************************************************/

static BOOL ReadILBM(Class *cl, Object *o)
{
    struct FileBitMapHeader *file_bmhd;
    struct BitMapHeader     *bmhd;
    struct IFFHandle	    *handle;
    struct StoredProperty   *bmhd_prop, *cmap_prop, *camg_prop;
    struct ContextNode	    *cn;
    ULONG   	    	    numcolors;
    IPTR    	    	    sourcetype;
    LONG    	    	    error;
 
    D(bug("ilbm.datatype/ReadILBM()\n"));
       
    if (GetDTAttrs(o, DTA_SourceType	, (IPTR)&sourcetype ,
    	    	      DTA_Handle    	, (IPTR)&handle     , 
		      PDTA_BitMapHeader , (IPTR)&bmhd	    ,
		      TAG_DONE	    	    	    	     ) != 3)
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if ((sourcetype != DTST_FILE) && (sourcetype != DTST_CLIPBOARD))
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if (!handle || !bmhd)
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }

    if (PropChunks(handle, propchunks, 3) != 0)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
	D(bug("ilbm.datatype error propchunks\n"));
	return FALSE;
    }
   
    if (StopChunk(handle, ID_ILBM, ID_BODY) != 0)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
	D(bug("ilbm.datatype error stopchunks\n"));
	return FALSE;
    }
    
    error = ParseIFF(handle, IFFPARSE_SCAN);
    if (error)
    {
    	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	D(bug("ilbm.datatype error parseiff\n"));
	return FALSE;
    }
    
    bmhd_prop = FindProp(handle, ID_ILBM, ID_BMHD);
    cmap_prop = FindProp(handle, ID_ILBM, ID_CMAP);
    camg_prop = FindProp(handle, ID_ILBM, ID_CAMG);

    cn = CurrentChunk(handle);
    if ((cn->cn_Type != ID_ILBM) ||
    	(cn->cn_ID != ID_BODY) ||
	(bmhd_prop == NULL))
    {
    	SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	D(bug("ilbm.datatype error currentchunk\n"));
	return FALSE;
    }

    file_bmhd = (struct FileBitMapHeader *)bmhd_prop->sp_Data;
    bmhd->bmh_Width  	  = bmhd->bmh_PageWidth  = (file_bmhd->bmh_Width [0] << 8) + file_bmhd->bmh_Width [1];
    bmhd->bmh_Height 	  = bmhd->bmh_PageHeight = (file_bmhd->bmh_Height[0] << 8) + file_bmhd->bmh_Height[1];
    bmhd->bmh_Depth  	  = file_bmhd->bmh_Depth;
    bmhd->bmh_Masking 	  = file_bmhd->bmh_Masking;
    bmhd->bmh_Transparent = (file_bmhd->bmh_Transparent[0] << 8) + file_bmhd->bmh_Transparent[1];
    
    {
    	IPTR name = (IPTR) NULL;
	
	GetDTAttrs(o, DTA_Name, (IPTR)&name, TAG_DONE);
	
    	SetDTAttrs(o, NULL, NULL, DTA_ObjName, name,
				  DTA_NominalHoriz, bmhd->bmh_Width ,
				  DTA_NominalVert , bmhd->bmh_Height,
				  TAG_DONE);
    }
    
    if ((file_bmhd->bmh_Depth == 24) && (file_bmhd->bmh_Compression <= 1))
    {
	D(bug("ilbm.datatype/ReadILBM: 24 bit\n"));
	if( !ReadRGBPic(cl, o, handle, bmhd, file_bmhd, cn, NULL) )
	{
	    D(bug("ilbm.datatype error readrgbpic\n"));
	    return FALSE;
	}
    }
    else if ( (file_bmhd->bmh_Depth <= 8) && (file_bmhd->bmh_Compression <= 1) && cmap_prop )
    {
	UBYTE *data;
	BOOL ham = FALSE;
	BOOL ehb = FALSE;

	if ( camg_prop && (camg_prop->sp_Size == 4) )
	{
	    ULONG mode;
	    
	    data = (UBYTE *)camg_prop->sp_Data;
	    mode = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
	    if (mode & HAM_KEY)
	    {
		ham = TRUE;
	    }
	    if (mode & EXTRAHALFBRITE_KEY)
	    {
		ehb = TRUE;
	    }
	    D(bug("ilbm.datatype/ReadILBM: modeid %08lx%s%s\n", mode, ham ? " HAM" : "", ehb ? " EHB" : ""));
	}

	numcolors = cmap_prop->sp_Size / 3;
	D(bug("ilbm.datatype/ReadILBM: %d bit %d colors\n", (int)file_bmhd->bmh_Depth, numcolors));
	data = (UBYTE *)cmap_prop->sp_Data;
	FixColRegs(numcolors, data);
	CopyColRegs(o, numcolors, data, ehb);

	if ( ham )
	{
	    /* picture.datatype can't cope with a transparent colour when we
             * convert a HAM image to an RGB image */
	    if (bmhd->bmh_Masking == mskHasTransparentColor)
		bmhd->bmh_Masking = mskNone;

	    if( !ReadRGBPic(cl, o, handle, bmhd, file_bmhd, cn, (UBYTE *)cmap_prop->sp_Data) )
	    {
		D(bug("ilbm.datatype error readrgbpic\n"));
		return FALSE;
	    }
	    bmhd->bmh_Depth = 24;
	}
	else
	{
	    if( !ReadBitMapPic(cl, o, handle, bmhd, file_bmhd, cn) )
	    {
		D(bug("ilbm.datatype error readbitmappic\n"));
		return FALSE;
	    }
	}
    }
    else
    {
	D(bug("ilbm.datatype unknown\n"));
    	SetIoErr(ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }

    return TRUE;
}

/**************************************************************************************************/

unsigned char *WriteBytesBigEndian_UintBE32(LONG val, int offset)
{
    unsigned char *buffer = NULL;
    buffer[offset] = (UBYTE)((val >> 24) & 0xFF);
    buffer[offset + 1] = (UBYTE)((val >> 16) & 0xFF);
    buffer[offset + 2] = (UBYTE)((val >> 8) & 0xFF);
    buffer[offset + 3] = (UBYTE)(val & 0xFF);
    return buffer;
}

/**************************************************************************************************/

unsigned char *WriteBytesBigEndian_UintBE16(WORD val, int offset)
{
    unsigned char *buffer = NULL;
    buffer[offset] = (UBYTE)(val >> 8);
    buffer[offset + 1] = (UBYTE)(val & 0xFF);
    return buffer;
}

/**************************************************************************************************/

LONG WriteBytes(BPTR file, char *data, LONG offset, LONG length)
{    
    LONG count = 0;
    Seek(file,offset,OFFSET_BEGINNING);
    
	//Write contents of buffer to file.
    count = Write(file, data, length);
    if (count != length)
    {
    	//Write error!
        return 0;
    }

    return count;
}

/**************************************************************************************************/

static BOOL Save_BitMapPic(struct IClass *cl, Object *o, struct dtWrite *dtw )
{    
    //UBYTE                   *filebuf;    
    //int                      width, height, numcolors;
    BPTR                     fileHandle;    
    int                      i, numcolors;
    struct BitMapHeader     *bmhd;
    struct BitMap           *bm;
    //BitMapImage             *BMI = NULL;
    //struct RastPort         rp;
    struct ColorRegister    *colormap;
    LONG                     *colorregs;    
    //int                     i, j, ret; 

    D(bug("iff.datatype/Save_BitMapPic()\n"));
    
    /* No filehandle is not an error, just NOP */
    if (!dtw->dtw_FileHandle)
    {
	    D(bug("iff.datatype/Save_BitMapPic(): empty filehandle\n"));
	    //ILBM_Exit(fileHandle, 0);
        return TRUE;
    }
    fileHandle = dtw->dtw_FileHandle;

    /* Get BitMap and color palette */
    if( GetDTAttrs( o,  PDTA_BitMapHeader, &bmhd,
			PDTA_BitMap,       &bm,
			PDTA_CRegs,        (IPTR)&colorregs,
			PDTA_NumColors,    &numcolors,
			PDTA_ColorRegisters,     (IPTR)&colormap,
			TAG_DONE ) != 5UL ||
	!bmhd || !bm || !colorregs || !numcolors || !colormap)
    {
	    D(bug("bmp.datatype/Save_BitMapPic() --- missing attributes\n"));	
	    return FALSE;
    }

    /* Write fileID 'FORM' and typeID 'ILBM' and chunkID 'BMHD'. */

    //Write IFF File Signature 'FORM'.
    LONG offset = 0;
    LONG length = 4;
    unsigned char *fileID = "FORM";
    WriteBytes(fileHandle, fileID, offset, length);

    //Calculate file size less (4+4, FORM+size)
    LONG fileSize = (LONG)(((bm->BytesPerRow * bmhd->bmh_Depth) * bmhd->bmh_Height) + (numcolors * 3) + 48);
    //Add padding byte if needed.
    
    offset += 4;
    length = 4;    
    unsigned char *FileSize;
    FileSize = WriteBytesBigEndian_UintBE32(fileSize, 0);
    WriteBytes(fileHandle, FileSize, offset, length);

    //Write File Format Type 'ILBM'.    
    offset += 4;
    length = 4;
    unsigned char *typeID = "ILBM";
    WriteBytes(fileHandle, typeID, offset, length);

    //Write ChunkID 'BMHD'.
    
    offset += 4;
    length = 4;    
    WriteBytes(fileHandle, "BMHD", offset, length);    

    //Write BMHD ChunkSize.    
    offset += 4;
    length = 4;
    LONG chunkSize = 20;
    unsigned char *ChunkSize;
    ChunkSize = WriteBytesBigEndian_UintBE32(chunkSize, 0);
    WriteBytes(fileHandle, ChunkSize, offset, length);

    /* Prepare BMHD information. */
    //Set_BMHD.    
    offset += 4;
    length = 20;
    int count = 0;
    unsigned char BMHD[20];
    unsigned char *buffer;    
    buffer = WriteBytesBigEndian_UintBE16(bmhd->bmh_Width, 0);
    memcpy(BMHD + count, buffer, 2);
    buffer = WriteBytesBigEndian_UintBE16(bmhd->bmh_Height, 0);
    memcpy(BMHD + count+2, buffer, 2);
    buffer = WriteBytesBigEndian_UintBE16(bmhd->bmh_Left, 0);
    memcpy(BMHD + count+4, buffer, 2);
    buffer = WriteBytesBigEndian_UintBE16(bmhd->bmh_Top, 0);
    memcpy(BMHD + count+6, buffer, 2);
    BMHD[8] = bmhd->bmh_Depth;
    BMHD[9] = 0; //bmhd->bmh_Masking;
    BMHD[10] = 0; //bmhd->bmh_Compression;
    BMHD[11] = bmhd->bmh_Pad;
    buffer = WriteBytesBigEndian_UintBE16(bmhd->bmh_Transparent, 0);
    memcpy(BMHD + count+12, buffer, 2);
    BMHD[14] = 10; //bmhd->bmh_XAspect;
    BMHD[15] = 11; //bmhd->bmh_YAspect;
    buffer = WriteBytesBigEndian_UintBE16(bmhd->bmh_PageWidth, 0);
    memcpy(BMHD + count+16, buffer, 2);
    buffer = WriteBytesBigEndian_UintBE16(bmhd->bmh_PageHeight, 0);
    memcpy(BMHD + count+18, buffer, 2);
    //BMHD[21] = ('\0');

    /* Write BMHD to File. */
    WriteBytes(fileHandle, BMHD, offset, length);

    if (bmhd->bmh_Depth <= 8)
    {
        /* Prepare ColorMap information. */
        //Set_CMAP.
        offset = 40;
        length = 4;
        WriteBytes(fileHandle, "CMAP", offset, length);

        //Write chunkSize to File.        
        offset += 4;
        chunkSize = numcolors*3;
        ChunkSize = WriteBytesBigEndian_UintBE32(chunkSize, 0);
        WriteBytes(fileHandle, ChunkSize, offset, length);

        //Convert CMAP from Colormap.
        UBYTE Cmap[numcolors * 3];
        for( i = 0; i < numcolors ; i++)
        {            
	        Cmap[(i*3)] = colormap[i].red;
            Cmap[(i*3)+1] = colormap[i].green;
            Cmap[(i*3)+2] = colormap[i].blue;
        }

        /* Write ColorMap and CAMG information. */               
        offset += 4;
        length = numcolors*3;        
        count = WriteBytes(fileHandle, Cmap, offset, numcolors*3);        
    }

    //=====================================================================//
    
    //** Write CAMG for EHB and HAM only. Standard ILBM doesn't need CAMG.**//

    //=====================================================================//

    /* Prepare BODY information. */
    offset = (numcolors*3) + 48;
    length = 4;
    WriteBytes(fileHandle, "BODY", offset, length);

    //Set_BODY.
    //UBYTE *BODY;    
    //UBYTE *chunkyBuffer;    
    //UBYTE *planarBuffer;    
    //UBYTE *scanLine;    
    int comp = 0;
    UBYTE    *src;
    LONG     y, p;
    //int lineOffset = 0;    
    int numplanes = bmhd->bmh_Depth;
    //int imageWidth = bmhd->bmh_Width;
    int imageHeight = bmhd->bmh_Height;
    //UBYTE compress = bmhd->bmh_Compression ? cmpByteRun1 : cmpNone;    
    int bytesPerRow = bm->BytesPerRow;
    //int scanLength = (bytesPerRow * numplanes); 

    if (comp == 0)
    {
        chunkSize = (bytesPerRow * numplanes) * imageHeight;
    }

    /* Write chunkSize to File. */
    //Does NOT include Padding Byte!
    //Calculate BODY size without Padding Byte if Odd. 
    offset += 4;
    ChunkSize = WriteBytesBigEndian_UintBE32(chunkSize, 0);
    WriteBytes(fileHandle, ChunkSize, offset, length);

    /* For images with <= 8 bitplanes. */
    //Copy planar data from the bitplanes of the bitmap to the BODY buffer 
    //one scanline at a time. Compress as needed. 
   
    //Set the offset to write the planar data to file.    
    offset += 4;    
    
    //Copy planar data from the bitplanes of the bitmap.
    for(y = 0; y < imageHeight; y++)
    {
        for(p = 0; p < numplanes; p++)
        {
            src = bm->Planes[p] + y * bytesPerRow;
            //src = bm->Planes[p];

            WriteBytes(fileHandle, src, offset, bytesPerRow);
            offset += bytesPerRow;
        }
    }            
    //Add padding byte if needed.

    if (fileHandle)
        Close(fileHandle); 
    
    return TRUE;
}

/**************************************************************************************************/

static BOOL SaveILBM(struct IClass *cl, Object *DTImage, struct dtWrite *dtw )
{		
    struct BitMapHeader     *bmhd;	

	if (DTImage)
	{
        /* Get BitMapHeader. */
        if( GetDTAttrs(DTImage,  PDTA_BitMapHeader, &bmhd,
			TAG_DONE ) != 1UL || !bmhd )
        {
	        D(bug("ilbm.datatype/SaveILBM --- missing attributes\n"))
	        return FALSE;
        }

        //If bmhd->depth <= 8,24,32 then save ILBM with correct no. bitplanes
        //else Process fails. Incorrect number of bitplanes.        

        if (bmhd->bmh_Depth <= 8) 
        {            
            Save_BitMapPic(cl, DTImage, dtw);
        }
        else if ((bmhd->bmh_Depth == 24) || (bmhd->bmh_Depth == 32))
        {            
            //Save_RGBPic(cl, DTImage, dtw);            
        }
        else
        {
            //Process fails. Incorrect number of bitplanes.            
            D(bug("ilbm.datatype/SaveILBM --- incorrect bitplanes\n"))
	        return FALSE;
        }
    }
    return TRUE;
}

/**************************************************************************************************/

IPTR ILBM__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
    	if (!ReadILBM(cl, (Object *)retval))
	{
	    CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
	    retval = 0;
	}
    }
    
    return retval;
}

/**************************************************************************************************/

IPTR IFF__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    D(bug("iff.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
    if( (dtw -> dtw_Mode) == DTWM_RAW )
    {
	    /* Local data format requested */
	    return SaveILBM(cl, o, dtw );
    }
    else
    {
	    /* Pass msg to superclass (which writes an IFF ILBM picture)... */
	    return DoSuperMethodA( cl, o, (Msg)dtw );
    }
}

/**************************************************************************************************/
