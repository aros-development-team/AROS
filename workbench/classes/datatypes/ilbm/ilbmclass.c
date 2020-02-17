/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id: ilbmclass.c 51525 2016-02-24 14:26:39Z jmcmullan $ Update 2020-02-16.
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
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

LONG WriteBytes(BPTR file, char *data, LONG offset, LONG length)
{
    //Write contents of data buffer to file
    LONG count = 0;
    Seek(file,offset,OFFSET_BEGINNING);    	
    count = Write(file, data, length);
    if (count != length)
    {
    	//("Write error!");
	return 0;
    }
    return count;
}

/**************************************************************************************************/

UBYTE *ScanLineFromBitplanes(Object *DTImage, struct BitMapHeader *bmhd, int numplanes, int bytesPerRow, int yoffset)
{
	int y;
	int bytesPerPixel;	
	UBYTE *pixelArray;
    UBYTE *scanLine;    
	int lineSize = (bytesPerRow * numplanes);

	pixelArray = (UBYTE *)AllocVec(lineSize, MEMF_ANY);
	                
	if (numplanes == 24)
		bytesPerPixel = 3;                            
	if (numplanes == 32)
		bytesPerPixel = 4;                           
    
        struct pdtBlitPixelArray pbpa;

        pbpa.MethodID = PDTM_READPIXELARRAY;
        pbpa.pbpa_PixelData = pixelArray;        
	pbpa.pbpa_PixelFormat = (numplanes == 24) ? PBPAFMT_RGB : PBPAFMT_RGBA;
        pbpa.pbpa_PixelArrayMod = bmhd->bmh_Width * bytesPerPixel;
        pbpa.pbpa_Left = 0;
        pbpa.pbpa_Top = yoffset; //Top = yoffset = 0,1,2;
        pbpa.pbpa_Width = bmhd->bmh_Width;
        pbpa.pbpa_Height = 1;

        if (DoMethodA(DTImage, (Msg)&pbpa) != 0)
        {
            scanLine = pixelArray;            
        }    

    if (pixelArray)
    {
         FreeVec(pixelArray);
    } 
    return scanLine;
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
	//FixColRegs(numcolors, data);
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

static BOOL SaveBitMapPic(Class *cl, Object *o, struct dtWrite *dtw )
{    
    BPTR		      filehandle;
    int transparent, pad; 
    unsigned int    width, height, numplanes, y, p;
    int   numcolors, alignwidth, bytesPerRow, i;
    struct BitMapHeader     *bmhd;
    struct BitMap           *bm;
    struct ColorRegister    *colormap;    
    ULONG                   *colorregs;
    ULONG 		    ulbuff;
    UBYTE                   byteBuffer[4];
    

    
	D(bug("ilbm.datatype/SaveBitMap()\n"));

	/* A NULL file handle is a NOP */
	if( !dtw->dtw_FileHandle )
	{
		D(bug("ilbm.datatype/SaveBitMap() --- empty Filehandle - just testing\n"));
		return TRUE;
	}
	filehandle = dtw->dtw_FileHandle;
    
    
    /* GET DATATYPE ATTRIBUTES FROM DTO */    

	/* Get BitMapHeader */
	if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR) &bmhd,                                
				TAG_DONE ) != 1UL ||
			!bmhd )
	{
		D(bug("ilbm.datatype/SaveBitMap() --- missing attributes\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

        /* Get BitMap */
	if( GetDTAttrs( o,  PDTA_BitMap,       (IPTR) &bm,                                
				TAG_DONE ) != 1UL ||
			!bmhd )
	{
		D(bug("ilbm.datatype/SaveBitMap() --- missing attributes\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}            
        
        /* Prepare Simple Values */
        int comp = 0;
        width = bmhd->bmh_Width;
        height = bmhd->bmh_Height;
        pad = bmhd->bmh_Pad;
        numplanes = bmhd->bmh_Depth;
        transparent = bmhd->bmh_Transparent;
    
        
        /* Used to get correct filesize */
	    alignwidth = (width + 15) & ~15;
        bytesPerRow = (alignwidth / 8);

    
        /* Set Number of Colors */
        if ( numplanes > 8 )
            numcolors = 0;
        else
            numcolors = 1<<( numplanes );
        
    
	if( numplanes > 8 )
	{
		D(bug("ilbm.datatype/SaveBitMap() --- color depth %d, can save only depths less than or eq to 8\n", numplanes));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	D(bug("ilbm.datatype/SaveBitMap() --- Picture size %d x %d (x %d bit)\n", width, height, numplanes));
    
    
        
    /* WRITE FILE HEADER INFORMATION TO FILE */
    
    /** Write file signature to file **/
    LONG offset = 0;    
    WriteBytes( filehandle, "FORM", offset, 4 );     
    
    //Calculate file size less (4+4=8, FORM+size).
    //Add padding byte at end of file if needed.    
    LONG bodySize = ((bytesPerRow * numplanes) * height);
    LONG fileSize = (bodySize + (numcolors * 3) + 48);
    
    
    /* WriteBytesB_Uint32 - Write ilbm fileSize */ 
    offset = 4;
    ulbuff = AROS_LONG2BE(fileSize);
    memcpy(byteBuffer, &ulbuff, 4);
    WriteBytes( filehandle, byteBuffer, offset, 4 );

    offset = 8;    
    WriteBytes( filehandle, "ILBM", offset, 4 );

    offset = 12;        
    WriteBytes( filehandle, "BMHD", offset, 4 );

    /* WriteBytesB_Uint32 - Write bmhd chunkSize */ 
    offset = 16;
    LONG chunkSize = 20;
    ulbuff = AROS_LONG2BE(chunkSize);
    memcpy(byteBuffer, &ulbuff, 4);    
    WriteBytes( filehandle, byteBuffer, offset, 4 );    
       
    
    /* WRITE ILBM BMHD TO FILE */
    
    /* Prepare BMHD information. */
    UBYTE BMHD[20];
    BMHD[0] =  (UBYTE)(width >> 8); //Width
    BMHD[1] =  (UBYTE)(width & 0xFF);
    BMHD[2] =  (UBYTE)(height >> 8); //Height
    BMHD[3] =  (UBYTE)(height & 0xFF);
    BMHD[4] =  BMHD[5] = 0; //Left Offset
    BMHD[6] =  BMHD[7] = 0; //Top Offset
    BMHD[8] =  numplanes;
    BMHD[9] =  0; //Masking
    BMHD[10] = 0; //Compression
    BMHD[11] = pad; //Padding
    BMHD[12] = (UBYTE)(transparent >> 8); 
    BMHD[13] = (UBYTE)(transparent & 0xFF);
    BMHD[14] = 10; //XAspect;
    BMHD[15] = 11; //YAspect;
    BMHD[16] = (UBYTE)(width >> 8); //PageWidth
    BMHD[17] = (UBYTE)(width & 0xFF);
    BMHD[18] = (UBYTE)(height >> 8); //PageHeight
    BMHD[19] = (UBYTE)(height & 0xFF);

    /* Write BMHD to File. */    
    offset += 4; //offset = 20;    
    WriteBytes( filehandle, BMHD, offset, 20 );    
    

    /* WRITE ILBM CMAP TO FILE */    

    /* Prepare ColorMap Information. */    
    if ( numplanes <= 8 )
    { 
        /* Get ColorMap */
	    if( GetDTAttrs( o,  PDTA_ColorRegisters, (IPTR)&colormap, 
                PDTA_CRegs, (IPTR)&colorregs,                               
		TAG_DONE ) != 2UL ||
		!colormap || !colorregs )
	    {
		    D(bug("ilbm.datatype/SaveBitMap() --- missing attributes\n"));
		    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		    return FALSE;
	    }

        offset = 40;        
	    WriteBytes( filehandle, "CMAP", offset, 4 );
        
        offset += 4;
        LONG cmapChunkSize = numcolors*3;
        ulbuff = AROS_LONG2BE(cmapChunkSize);
        memcpy(byteBuffer, &ulbuff, 4);
        WriteBytes( filehandle, byteBuffer, offset, 4 ); 

        //Convert CMAP from ColorRegister Colormap.
        UBYTE Cmap[numcolors * 3];
        for( i = 0; i < numcolors ; i++)
        {
	    Cmap[(i*3)] = colormap[i].red;
            Cmap[(i*3)+1] = colormap[i].green;
            Cmap[(i*3)+2] = colormap[i].blue;
        }        
        //Write CMAP to File.
        offset += 4;        
        WriteBytes( filehandle, Cmap, offset, (numcolors*3) );       
    }
    
    
    /* Prepare BODY Information. */
    offset = (numcolors * 3) + 48;    
    WriteBytes( filehandle, "BODY", offset, 4 );    
          
    if (comp == 0)
    {
        //bodySize = (bytesPerRow * numplanes) * height;        
        chunkSize = bodySize;
    }    

     /* Write bodySize to File. */ 
    offset += 4;    
    ulbuff = AROS_LONG2BE(bodySize);
    memcpy(byteBuffer, &ulbuff, 4);
    WriteBytes( filehandle, byteBuffer, offset, 4 );    
    
    
    /* WRITE ILBM BODY TO FILE */
    
	/* Now read the picture data line by line and write it to a chunky buffer */	
	D(bug("ilbm.datatype/SaveBitMap() --- copying picture using CopyFromBitplanes.\n"));
    
    offset += 4;
    UBYTE *src; 
    //src = (UBYTE *)AllocVec(scanLineSize, MEMF_ANY);
    bytesPerRow = bm->BytesPerRow;
    
    //Copy planar data from the bitplanes of the bitmap.
    for(y = 0; y < height; y++)
    {
        for(p = 0; p < numplanes; p++)
        {
            src = bm->Planes[p] + (y * bytesPerRow);            

            WriteBytes(filehandle, src, offset, bytesPerRow);
            offset += bytesPerRow;
        }
    } 
        
	D(bug("ilbm.datatype/SaveBitMap() --- Normal Exit\n"));
	//FreeVec(src);
	SetIoErr(0);
	return TRUE;	
}

/**************************************************************************************************/

static BOOL SaveRGBPic(Class *cl, Object *o, struct dtWrite *dtw )
{    
    BPTR		      filehandle;
    int transparent, pad; 
    unsigned int    width, height, numplanes, y, p;
    int   numcolors, alignwidth, bytesPerRow, i, j, k;
    struct BitMapHeader     *bmhd;
    struct BitMap           *bm;
    struct ColorRegister    *colormap;    
    ULONG                   *colorregs;
    ULONG 		            ulbuff;
    UBYTE                   byteBuffer[4];
    

    
	D(bug("ilbm.datatype/SaveBitMap()\n"));

	/* A NULL file handle is a NOP */
	if( !dtw->dtw_FileHandle )
	{
		D(bug("ilbm.datatype/SaveBitMap() --- empty Filehandle - just testing\n"));
		return TRUE;
	}
	filehandle = dtw->dtw_FileHandle;
    
    
    /* GET DATATYPE ATTRIBUTES FROM DTO */    

	/* Get BitMapHeader */
	if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR) &bmhd,                                
				TAG_DONE ) != 1UL ||
			!bmhd )
	{
		D(bug("ilbm.datatype/SaveBitMap() --- missing attributes\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

        /* Get BitMap */
	if( GetDTAttrs( o,  PDTA_BitMap,       (IPTR) &bm,                                
				TAG_DONE ) != 1UL ||
			!bmhd )
	{
		D(bug("ilbm.datatype/SaveBitMap() --- missing attributes\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}            
        
        /* Prepare Simple Values */
        int comp = 0;
        width = bmhd->bmh_Width;
        height = bmhd->bmh_Height;
        pad = bmhd->bmh_Pad;
        numplanes = bmhd->bmh_Depth;
        transparent = bmhd->bmh_Transparent;
        int bytesPerPixel;
        int lineOffset = 0;
        int imageWidth = bmhd->bmh_Width;
        int imageHeight = bmhd->bmh_Height;
    
        
        /* Used to get correct filesize */
	    alignwidth = (width + 15) & ~15;
        bytesPerRow = (alignwidth / 8);
        int scanLength = (alignwidth * 3);

    
        /* Set Number of Colors */
        if ( numplanes > 8 )
            numcolors = 0;
        else
            numcolors = 1<<( numplanes );
        
    
	if( numplanes < 24 )
	{
		D(bug("ilbm.datatype/SaveRGBPic() --- color depth %d, can save only depths of 24", numplanes));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	D(bug("ilbm.datatype/SaveRGBPic() --- Picture size %d x %d (x %d bit)\n", width, height, numplanes));
    
    
        
    /* WRITE FILE HEADER INFORMATION TO FILE */
    
    /** Write file signature to file **/
    LONG offset = 0;    
    WriteBytes( filehandle, "FORM", offset, 4 );     
    
    //Calculate file size less (4+4=8, FORM+size).
    //Add padding byte at end of file if needed.    
    LONG bodySize = ((bytesPerRow * numplanes) * height);
    LONG fileSize = (bodySize + (numcolors * 3) + 40);
    
    
    /* WriteBytesB_Uint32 - Write ilbm fileSize */ 
    offset = 4;
    ulbuff = AROS_LONG2BE(fileSize);
    memcpy(byteBuffer, &ulbuff, 4);
    WriteBytes( filehandle, byteBuffer, offset, 4 );

    offset = 8;    
    WriteBytes( filehandle, "ILBM", offset, 4 );

    offset = 12;        
    WriteBytes( filehandle, "BMHD", offset, 4 );

    /* WriteBytesB_Uint32 - Write bmhd chunkSize */ 
    offset = 16;
    LONG chunkSize = 20;
    ulbuff = AROS_LONG2BE(chunkSize);
    memcpy(byteBuffer, &ulbuff, 4);    
    WriteBytes( filehandle, byteBuffer, offset, 4 );    
       
    
    /* WRITE ILBM BMHD TO FILE */
    
    /* Prepare BMHD information. */
    UBYTE BMHD[20];
    BMHD[0] =  (UBYTE)(width >> 8); //Width
    BMHD[1] =  (UBYTE)(width & 0xFF);
    BMHD[2] =  (UBYTE)(height >> 8); //Height
    BMHD[3] =  (UBYTE)(height & 0xFF);
    BMHD[4] =  BMHD[5] = 0; //Left Offset
    BMHD[6] =  BMHD[7] = 0; //Top Offset
    BMHD[8] =  numplanes;
    BMHD[9] =  0; //Masking
    BMHD[10] = 0; //Compression
    BMHD[11] = pad; //Padding
    BMHD[12] = (UBYTE)(transparent >> 8); 
    BMHD[13] = (UBYTE)(transparent & 0xFF);
    BMHD[14] = 10; //XAspect;
    BMHD[15] = 11; //YAspect;
    BMHD[16] = (UBYTE)(width >> 8); //PageWidth
    BMHD[17] = (UBYTE)(width & 0xFF);
    BMHD[18] = (UBYTE)(height >> 8); //PageHeight
    BMHD[19] = (UBYTE)(height & 0xFF);

    /* Write BMHD to File. */    
    offset += 4; //offset = 20;    
    WriteBytes( filehandle, BMHD, offset, 20 );
    
    
    /* Prepare BODY Information. */
    offset = (numcolors * 3) + 48;    
    WriteBytes( filehandle, "BODY", offset, 4 );    
          
    if (comp == 0)
    {
        //bodySize = (bytesPerRow * numplanes) * height;        
        chunkSize = bodySize;
    }    

     /* Write bodySize to File. */ 
    offset += 4;    
    ulbuff = AROS_LONG2BE(bodySize);
    memcpy(byteBuffer, &ulbuff, 4);
    WriteBytes( filehandle, byteBuffer, offset, 4 );    
    
    
   /* WRITE ILBM BODY TO FILE */
    
    UBYTE  r,g,b,a;
    int pBufferOffset = 0;
    int pBufferLength = (bytesPerRow * 8);
    int rowSize = (bytesPerRow * 8);
    UBYTE colorBytes[8]; //Block of 8 colors
    UBYTE *Buffer;
    UBYTE *ScanLine; 
    UBYTE *planarScanLine;
    UBYTE bitBuffer[rowSize];
    UBYTE planarR[rowSize];
    UBYTE planarG[rowSize];
    UBYTE planarB[rowSize];
    UBYTE planarA[rowSize];
    UBYTE redBuffer[rowSize];
    UBYTE greenBuffer[rowSize];
    UBYTE blueBuffer[rowSize];
    UBYTE chunkyBuffer[bytesPerRow * numplanes];
    UBYTE planarBuffer[bytesPerRow * numplanes];
    int scanLineSize = (bytesPerRow * numplanes);	
	

	//////////////////////////////////////////////////////////////////////
    // Process to Extract RGB Planar ScanLines then write them to File. //
    //////////////////////////////////////////////////////////////////////
    
    
	/* Now read the picture data line by line and write it to a chunky buffer */
	if( !(ScanLine = AllocVec(scanLineSize, MEMF_ANY)) ) //RGB Or RGBA
	{
		SetIoErr(ERROR_NO_FREE_STORE);
		return FALSE;
	}
        D(bug("ilbm.datatype/SaveRGBPic() --- ScanLine AllocVec to Store Pixel Data\n"));
    
    

	/* BEGIN WRITING TO FILE */
    
	
	/* Begin Loop to Convert C2P & Write Planar Data to File */
	for (y=0; y<height; y++) //For - Loop ( y-offset )
	{
		D(bug("ilbm.datatype/SaveRGBPic() --- begin copying picture data with READPIXELARRAY\n"));
		
		/* Read Chunky RGB ScanLine then Convert to Planar RGB ScanLine. */
        ScanLine = ScanLineFromBitplanes( o, bmhd, numplanes, bytesPerRow, y);         

		//=====================================================================//
		
		D(bug("ilbm.datatype/SaveRGBPic() --- get r,g,b components into 3 sets of 8 bitplanes\n"));		
		
		/* 24bit is saved as r0..r7g0..g7b0..b7 */
		if (bmhd->bmh_Depth == 24)
		{
		    //for (j = 0; j < (scanWidth); j++)
		    pBufferOffset = 0;
		    for (j = 0; j < imageWidth; j++)
		    {
			r = ScanLine[pBufferOffset];
			g = ScanLine[pBufferOffset+1];
			b = ScanLine[pBufferOffset+2];

			planarR[pBufferOffset/3] = r;
			planarG[pBufferOffset/3] = g;
			planarB[pBufferOffset/3] = b;
			pBufferOffset += 3;
		    }
		}		
		/* 32bit is saved as r0..r7g0..g7b0..b7a0..a7 */
		if (bmhd->bmh_Depth == 32)
		{
		    pBufferOffset = 0;
		    for (j = 0; j < imageWidth; j++)
		    {
			a = ScanLine[pBufferOffset];
			r = ScanLine[pBufferOffset+1];
			g = ScanLine[pBufferOffset+2];
			b = ScanLine[pBufferOffset+3];

			planarA[pBufferOffset/4] = a;
			planarR[pBufferOffset/4] = r;
			planarG[pBufferOffset/4] = g;
			planarB[pBufferOffset/4] = b;
			pBufferOffset += 4;
		    }
		}
	
		//=====================================================================//
		
		D(bug("ilbm.datatype/SaveRGBPic() --- copy chunky scanline segments to chunky buffers\n"));		
		
		/* 24bit is r,g,b */
		if (bmhd->bmh_Depth == 24)
		{
		     // Note: pBufferLength is the length of a set of 8 bitplanes.
		     pBufferOffset = 0;
		     memcpy(chunkyBuffer, planarR, pBufferLength);
		     pBufferOffset += pBufferLength;
		     memcpy(chunkyBuffer + pBufferOffset, planarG, pBufferLength);
		     pBufferOffset += pBufferLength;
		     memcpy(chunkyBuffer + pBufferOffset, planarB, pBufferLength);            
		}			
		/* 32bit is r,g,b,a */
		if (bmhd->bmh_Depth == 32)
		{
		     // Note: pBufferLength is the length of a set of 8 bitplanes.
		     // Copy Alpha Pixel Data to End of Chunky Buffer for R,G,B,A.
		     pBufferOffset = 0;
		     memcpy(chunkyBuffer, planarR, pBufferLength);
		     pBufferOffset += pBufferLength;
		     memcpy(chunkyBuffer + pBufferOffset, planarG, pBufferLength);
		     pBufferOffset += pBufferLength;
		     memcpy(chunkyBuffer + pBufferOffset, planarB, pBufferLength);
		     pBufferOffset += pBufferLength;
		     memcpy(chunkyBuffer + pBufferOffset, planarA, pBufferLength);
		}

		//=====================================================================//
		
		D(bug("ilbm.datatype/SaveRGBPic() --- prepare chunky buffers & variables before conversion.\n"));
		
		/* Convert Chunky ScanLine to Planar ScanLine. */	    
		UBYTE *bytesBuffer[4];
		bytesBuffer[0] = planarR;
		bytesBuffer[1] = planarG;
		bytesBuffer[2] = planarB;
		bytesBuffer[3] = planarA;
		UBYTE bitBuffer[bytesPerRow * 8];
		    
		//Setup & Initialize Variables.
		int bit = 0;
		int hOffset = 0;
		int bitplaneIndex = 0;
		pBufferOffset = 0;
		    
		//Setup Variables for 'K Loop'.
		UBYTE *source;
		int x, y, p, bpr, bpl;		
		bpr = bytesPerRow;

		//=====================================================================//
		
		D(bug("ilbm.datatype/SaveRGBPic() --- convert one chunky scanline to planar data\n"));
		
		//** Conversion using 'K Loop' **//

		//Use bytesPerPixel (3,4) for 'K Loop'. //bpp.
		int bpp = (bmhd->bmh_Depth / 8); //bpp = 3, 4.
		for (k = 0; k < bpp; k++)
		{
		    source = bytesBuffer[k];
		    for(x = 0; x < imageWidth; x++)
			{
			    LONG mask   = 0x80 >> (x & 7);
			    LONG offset = x / 8;
			    UBYTE chunkypix = source[x];

			    for(p = 0; p < 8; p++)
			    {
				    if (chunkypix & (1 << p))
				    bitBuffer[p * bpr + offset] |= mask;
				    else
					bitBuffer[p * bpr + offset] &= ~mask;
			    }
			}
		    //Copy planar data to correct location in planar buffer.
		    memcpy(planarBuffer + pBufferOffset, bitBuffer, pBufferLength);
		    pBufferOffset +=  pBufferLength;    

		} /* End For - Loop ('k Loop') */

		//=====================================================================//
		
		D(bug("ilbm.datatype/SaveRGBPic() --- write planar scanline data to file\n"));

		/* Write Planar ScanLine to File. */		
		int pOffset = offset;        
		WriteBytes(filehandle, planarBuffer, pOffset, (bytesPerRow * numplanes));        
		offset += scanLength;   

	//=====================================================================//		
		
	} //End For - Loop ( y-offset )
	
	/* END WRITING TO FILE */	
	

	D(bug("ilbm.datatype/SaveRGBPic() --- Normal Exit\n"));
	FreeVec(ScanLine);
	SetIoErr(0);
	return TRUE;

	
} //End SaveRGBPic


/**************************************************************************************************/

/**************************************************************************************************/

static BOOL SaveILBM(Class *cl, Object *DTImage, struct dtWrite *dtw )
{	
    struct BitMapHeader     *bmhd;
    
    //Save ILBM Function Directs Traffic to Save_BitMapPic or Save_RGBPic by BitDepth.	
	if (DTImage)
	{
        /* Get BitMapHeader. */
        if( GetDTAttrs(DTImage,  PDTA_BitMapHeader, &bmhd,
			TAG_DONE ) != 1UL || !bmhd )
        {	        
            SetIoErr(ERROR_OBJECT_NOT_FOUND);
	        return FALSE;
        }

        //If bmhd->depth <= 8,24,32 then save ILBM with correct no. bitplanes
        //else Process fails. Incorrect number of bitplanes.
        if (bmhd->bmh_Depth <= 8) //Or less than 8. if (bmhd->bmh_Depth <= 8)
        { 
            SaveBitMapPic(cl, DTImage, dtw);
        }
        else if ((bmhd->bmh_Depth == 24) || (bmhd->bmh_Depth == 32))
        {            
            SaveRGBPic(cl, DTImage, dtw);            
        }
        else
        {
            SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	        D(bug("ilbm.datatype error parseiff\n"));
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

IPTR ILBM__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    D(bug("ILBM.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
    if( (dtw -> dtw_Mode) == DTWM_RAW)
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

