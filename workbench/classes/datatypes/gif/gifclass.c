/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Using GIF requires this statement in both technical and user documentation:
      "The Graphics Interchange Format(c) is the Copyright property of
      CompuServe Incorporated. GIF(sm) is a Service Mark property of
      CompuServe Incorporated."
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
#include <cybergraphx/cybergraphics.h>
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

#include <aros/symbolsets.h>

#include "debug.h"

#include "methods.h"
#include "gifclass.h"
#include "codec.h"

/* Open superclass */
ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**************************************************************************************************/

/* if 1: use legacy picture.datatype interface using Bitmaps (for debugging) */
#define LEGACY 0

#define FILEBUFSIZE 65536
#define MAXCOLORS   256
const char GIFheader[] = "GIF87a";

/**************************************************************************************************/

static void GIF_Exit(GifHandleType *gifhandle, LONG errorcode)
{
    D(if (errorcode) bug("gif.datatype/GIF_Exit() --- IoErr %ld\n", errorcode));
    if (gifhandle->filebuf)
    {
	FreeMem(gifhandle->filebuf, gifhandle->filebufsize);
    }
    if (gifhandle->linebuf)
    {
	FreeMem(gifhandle->linebuf, gifhandle->linebufsize);
    }
    if (gifhandle->codecvars)
    {
	FreeVec(gifhandle->codecvars);
    }
    FreeMem(gifhandle, sizeof(GifHandleType));
    SetIoErr(errorcode);
}

/**************************************************************************************************/

/* This buffered file access might look complicated, but it is very fast, because for byte accesses
** no subroutine calls are neccessary, except when the buffer is empty. */
BOOL SaveGIF_EmptyBuf(GifHandleType *gifhandle, long minbytes)
{
    long                bytes, bytestowrite;
    
    bytestowrite = gifhandle->filebufsize - (gifhandle->filebufbytes + minbytes);
    D(bug("gif.datatype/SaveGIF_EmptyBuf() --- minimum %ld bytes, %ld bytes to write\n", (long)minbytes, (long)bytestowrite));
    bytes = Write(gifhandle->filehandle, gifhandle->filebuf, bytestowrite);
    if ( bytes < bytestowrite )
    {
	D(bug("gif.datatype/SaveGIF_EmptyBuf() --- writing failed, wrote %ld bytes\n", (long)bytes));
	return FALSE;
    }
    gifhandle->filebufpos = gifhandle->filebuf;
    gifhandle->filebufbytes = gifhandle->filebufsize - minbytes;
    D(bug("gif.datatype/SaveGIF_EmptyBuf() --- wrote %ld bytes\n", (long)bytes));
    return TRUE;
}

/* This buffered file access might look complicated, but it is very fast, because for byte accesses
** no subroutine calls are neccessary, except when the buffer is empty. */
BOOL LoadGIF_FillBuf(GifHandleType *gifhandle, long minbytes)
{
    long                i, bytes;
    
//  D(bug("gif.datatype/LoadGIF_FillBuf() --- minimum %ld bytes of %ld bytes\n", (long)minbytes, (long)gifhandle->filebufbytes));
    if ( gifhandle->filebufbytes >= 0 )
	return TRUE;
    bytes = gifhandle->filebufbytes + minbytes;
//  D(bug("gif.datatype/LoadGIF_FillBuf() --- %ld bytes requested, %ld bytes left\n", (long)minbytes, (long)bytes));
    if (bytes > 0)
    {       /* practically, this copying isn't needed, because decompression does single byte accesses */
	D(bug("gif.datatype/LoadGIF_FillBuf() --- existing %ld old bytes\n", (long)bytes));
	for (i=0; i<bytes; i++)     /* copy existing bytes to start of buffer */
	    gifhandle->filebuf[i] = gifhandle->filebufpos[i];
    }
    gifhandle->filebufpos = gifhandle->filebuf;
    bytes = Read(gifhandle->filehandle, gifhandle->filebuf + bytes, gifhandle->filebufsize - bytes);
    if (bytes < 0 ) bytes = 0;
    gifhandle->filebufbytes += bytes;
//  D(bug("gif.datatype/LoadGIF_FillBuf() --- read %ld bytes, remaining new %ld bytes\n", (long)bytes, (long)gifhandle->filebufbytes));
    if (gifhandle->filebufbytes >= 0)
	return TRUE;
    return FALSE;
}

static BOOL LoadGIF_Colormap(GifHandleType *gifhandle, int havecolmap, int *numcolors,
			    struct ColorRegister *colormap, ULONG *colregs)
{
    unsigned int        i, j;

    if (havecolmap && *numcolors && *numcolors <= MAXCOLORS)
    {
	j = 0;
	for (i = 0; i < *numcolors; i++)
	{
	    if ( (gifhandle->filebufbytes -= 3) < 0 && !LoadGIF_FillBuf(gifhandle, 3) )
	    {
		D(bug("gif.datatype/LoadGIF_Colormap() --- colormap loading failed\n"));
		return FALSE;
	    }
	    colormap[i].red = *(gifhandle->filebufpos)++;
	    colormap[i].green = *(gifhandle->filebufpos)++;
	    colormap[i].blue = *(gifhandle->filebufpos)++;
	    colregs[j++] = ((ULONG)colormap[i].red)<<24;
	    colregs[j++] = ((ULONG)colormap[i].green)<<24;
	    colregs[j++] = ((ULONG)colormap[i].blue)<<24;
//          D(if (i<5) bug("gif r %02lx g %02lx b %02lx\n", colormap[i].red, colormap[i].green, colormap[i].blue));
	}
	D(bug("gif.datatype/LoadGIF_Colormap() --- %d colors loaded\n", numcolors));
    }
    else
    {
	*numcolors = 0;
	D(bug("gif.datatype/LoadGIF_Colormap() --- no colormap load needed\n"));
    }
    return TRUE;
}

/**************************************************************************************************/
static BOOL LoadGIF(struct IClass *cl, Object *o)
{
    GifHandleType           *gifhandle;
    UBYTE                   *filebuf;
    IPTR                    sourcetype;
    int                     done, ret;
    int                     i, line, inc;
    long                    j;
    unsigned int            scrwidth, scrheight, width, height, widthxheight, leftedge, topedge;
    unsigned int            scrnumplanes, scrnumcolors, numplanes, numcolors, fillcolor, colrez, colsorted;
    int                     havecolmap, interlaced;
    struct BitMapHeader     *bmhd;
    struct ColorRegister    *colormap;
    ULONG                   *colorregs;
    STRPTR                  name;
#if LEGACY
    struct BitMap           *bm;
    struct RastPort         rp;
#endif

    D(bug("gif.datatype/LoadGIF()\n"));

    if( !(gifhandle = AllocMem(sizeof(GifHandleType), MEMF_ANY)) )
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    gifhandle->filebuf = NULL;
    gifhandle->linebuf = NULL;
    gifhandle->codecvars = NULL;
    
    if( GetDTAttrs(o,   DTA_SourceType    , (IPTR)&sourcetype ,
			DTA_Handle        , (IPTR)&(gifhandle->filehandle),
			PDTA_BitMapHeader , (IPTR)&bmhd,
			TAG_DONE) != 3 )
    {
	GIF_Exit(gifhandle, ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if ( sourcetype == DTST_RAM && gifhandle->filehandle == NULL && bmhd )
    {
	D(bug("gif.datatype/LoadGIF() --- Creating an empty object\n"));
	GIF_Exit(gifhandle, 0);
	return TRUE;
    }
    if ( sourcetype != DTST_FILE || !gifhandle->filehandle || !bmhd )
    {
	D(bug("gif.datatype/LoadGIF() --- unsupported mode\n"));
	GIF_Exit(gifhandle, ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }
    
    /* initialize buffered file reads */
    gifhandle->filebufbytes = 0;
    gifhandle->filebufsize = FILEBUFSIZE;
    if( !(gifhandle->filebuf = gifhandle->filebufpos = AllocMem(gifhandle->filebufsize, MEMF_ANY)) )
    {
	GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }

    /* load GIF header from file, make sure, there are at least 13 bytes in buffer */
    if ( (gifhandle->filebufbytes -= 13) < 0 && !LoadGIF_FillBuf(gifhandle, 13) )
    {
	D(bug("gif.datatype/LoadGIF() --- filling buffer with header failed\n"));
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    
    /* check for GIF87a or GIF89a header */
    filebuf = gifhandle->filebufpos;    /* this makes things easier */
    gifhandle->filebufpos += 13;
    for (i = 0; i < 6; i++)
    if( (filebuf[i] != GIFheader[i] && i != 4) ||
	(i == 4 && filebuf[i] != '7' && filebuf[i] != '9') )
    {
	D(bug("gif.datatype/LoadGIF() --- type mismatch\n"));
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }

    /* get screen descriptor attributes and set BitMapHeader */
    scrwidth = filebuf[6] | filebuf[7] << 8;
    scrheight = filebuf[8] | filebuf[9] << 8;
    scrnumplanes = (filebuf[10] & 0x07) + 1;
    colsorted = (filebuf[10] & 0x08) != 0;
    colrez = ((filebuf[10] & 0x70) >> 4) + 1;
    havecolmap = (filebuf[10] & 0x80) != 0;
    numcolors = 1 << scrnumplanes;
    fillcolor = filebuf[11];        /* filebuf[12] is pixel aspect ratio */
    D(bug("gif.datatype/LoadGIF() --- GIF-Screen %d x %d x %d, bgcol %d, rez %d sort %d\n",
	  scrwidth, scrheight,  havecolmap ? scrnumplanes : 0, fillcolor, colrez, colsorted));

    bmhd->bmh_Width  = bmhd->bmh_PageWidth  = scrwidth;
    bmhd->bmh_Height = bmhd->bmh_PageHeight = scrheight;
    bmhd->bmh_Depth  = scrnumplanes;

    /* get empty colormap to fill in colormap to use*/
    if( !(GetDTAttrs(o, PDTA_ColorRegisters, (IPTR) &colormap,
			PDTA_CRegs, (IPTR) &colorregs,
			TAG_DONE ) == 2) ||
	!(colormap && colorregs) )
    {
	D(bug("gif.datatype/LoadGIF() --- got no colormap\n"));
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }

    /* if we have a colormap for the screen, use it */
    if( !LoadGIF_Colormap(gifhandle, havecolmap, &numcolors, colormap, colorregs) )
    {
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    scrnumcolors = numcolors;

    /* Now search for the first GIF picture object in file */
    done = FALSE;
    while(!done)
    {
	if ( !(gifhandle->filebufbytes--) && !LoadGIF_FillBuf(gifhandle, 1) )
	{
	    D(bug("gif.datatype/LoadGIF() --- early end 1\n"));
	    GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}
#if 0
	D(bug("gif remaining bytes %ld = %ld, read %ld\n", (long)gifhandle->filebufbytes, (long)(gifhandle->filebufsize - (gifhandle->filebufpos - gifhandle->filebuf)), (long)(gifhandle->filebufpos - gifhandle->filebuf));
	for (j=-8; j<12; j+=4)
	{
	    bug(" pos %02ld: %02lx %02lx %02lx %02lx\n", (long)j, (long)gifhandle->filebufpos[j],
		(long)gifhandle->filebufpos[j+1], (long)gifhandle->filebufpos[j+2], (long)gifhandle->filebufpos[j+3]);
	}
	)
#endif
	switch ( *(gifhandle->filebufpos)++ )
	{
	case ';':
	    /* End of the GIF dataset */
	    D(bug("gif.datatype/LoadGIF() --- early end 2\n"));
	    GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;

	case ',':
	    /* Start of an image object. Jump out of the loop. */
	    done = TRUE;
	    break;

	case '!':
	    /* Start of an extended object, just skip it */
	    if ( (gifhandle->filebufbytes -= 2) < 0 && !LoadGIF_FillBuf(gifhandle, 2) )
	    {
		GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	    }
	    D(bug("gif.datatype/LoadGIF() --- extended object type %02x len %d\n", (int)gifhandle->filebufpos[0], (int)gifhandle->filebufpos[1]));
	    gifhandle->filebufpos++;
	    while ( (i = *(gifhandle->filebufpos)++) )
	    {
		if ( (gifhandle->filebufbytes -= i+1) < 0 && !LoadGIF_FillBuf(gifhandle, i+1) )
		{
		    GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
		    return FALSE;
		}
		gifhandle->filebufpos += i;
	    }
	    break;

	default:
	    D(bug("gif.datatype/LoadGIF() --- unknown marker\n"));
	    GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}
    }

    /* Start of an image object. Read the image description. */
    if ( (gifhandle->filebufbytes -= 9) < 0 && !LoadGIF_FillBuf(gifhandle, 9) )
    {
	D(bug("gif.datatype/LoadGIF() --- early end 3\n"));
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    filebuf = gifhandle->filebufpos;    /* this makes things easier */
    gifhandle->filebufpos += 9;
    leftedge = filebuf[0] | filebuf[1] << 8;
    topedge = filebuf[2] | filebuf[3] << 8;
    width = filebuf[4] | filebuf[5] << 8;
    height = filebuf[6] | filebuf[7] << 8;
    numplanes = (filebuf[8] & 0x07) + 1;
    colsorted = (filebuf[8] & 0x20) != 0;
    interlaced = (filebuf[8] & 0x40) != 0;
    havecolmap = (filebuf[8] & 0x80) != 0;
    numcolors = 1 << numplanes;
    if( (leftedge + width) > scrwidth || (topedge + height) > scrheight )
    {
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    D(bug("gif.datatype/LoadGIF() --- GIF-Win: %d x %d x %d, edge: %d, %d, %s, sort %d\n",
	  width, height, havecolmap ? numplanes : 0, leftedge, topedge, interlaced ? "interlaced" : "non-interlaced", colsorted));

    /* Get image colormap, if present. This overrides screen colormap. */
    if( !LoadGIF_Colormap(gifhandle, havecolmap, &numcolors, colormap, colorregs) ||
	(!numcolors && !scrnumcolors) )
    {
	D(bug("gif.datatype/LoadGIF() --- problem with image colors or no colors at all\n"));
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }

    if (!numcolors)
    {
	numplanes = scrnumplanes;
	numcolors = scrnumcolors;
    }
    D(bug("gif.datatype/LoadGIF() --- numcolors %d\n", numcolors));

    /* Pass attributes to picture.datatype */
    GetDTAttrs( o, DTA_Name, (IPTR) &name, TAG_DONE );
    SetDTAttrs(o, NULL, NULL, PDTA_NumColors, numcolors,
			      DTA_NominalHoriz, scrwidth,
			      DTA_NominalVert , scrheight,
			      DTA_ObjName     , (IPTR) name,
			      TAG_DONE);

#if LEGACY
    /* Get BitMap to draw into and pass it to picture.datatype (legacy interface) */
    D(bug("gif.datatype/LoadGIF() --- allocating bitmap\n"));
    if( !(bm = AllocBitMap(scrwidth, scrheight, numplanes, BMF_CLEAR, NULL)) )
    {
	D(bug("gif.datatype/LoadGIF() --- error allocating bitmap\n"));
	GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    InitRastPort(&rp);
    rp.BitMap=bm;
    SetDTAttrs(o, NULL, NULL, PDTA_BitMap, (IPTR)bm, TAG_DONE);
#endif /* LEGACY */

    /* Now decode the picture data into a chunky buffer */
    /* For now, we use a full picture pixel buffer, not a single line, because GIF decoding is easier this way */
    widthxheight = width*height;
    gifhandle->linebufsize = gifhandle->linebufbytes = widthxheight;
    if (! (gifhandle->linebuf = gifhandle->linebufpos = AllocMem(gifhandle->linebufsize, MEMF_ANY)) )
    {
	GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
#if 0   
    for(j=0; j<widthxheight; j++)
    {
	(gifhandle->linebuf)[j] = j & 0xff; /* pre-fill picture with some crap */
    }
#endif
    ret = DecodeInit(gifhandle);
    D(bug("gif.datatype/LoadGIF() --- DecodeInit() returned %d\n", ret));
    ret = DecodeLines(gifhandle);
    D(bug("gif.datatype/LoadGIF() --- DecodeLines() returned %d\n", ret));
    ret = DecodeEnd(gifhandle);
    D(bug("gif.datatype/LoadGIF() --- DecodeEnd() returned %d\n", ret));
    if ( gifhandle->filebufbytes == 1 && gifhandle->filebufpos[0] == ';' )
	D(bug("gif.datatype/LoadGIF() --- reached normal end of file\n"));
    else
    {
	D(bug("gif.datatype/LoadGIF() --- not at end of file, next char 0x%02lx, %ld bytes remaining\n", (long)(gifhandle->filebufpos[0]), (long)(gifhandle->filebufbytes)));
#if 0
	D(bug("gif remaining bytes %ld = %ld, read %ld\n", (long)gifhandle->filebufbytes, (long)(gifhandle->filebufsize - (gifhandle->filebufpos - gifhandle->filebuf)), (long)(gifhandle->filebufpos - gifhandle->filebuf));
	for (j=-8; j<12; j+=4)
	{
	    D(bug(" pos %02ld: %02lx %02lx %02lx %02lx\n", (long)j, (long)gifhandle->filebufpos[j],
		(long)gifhandle->filebufpos[j+1], (long)gifhandle->filebufpos[j+2], (long)gifhandle->filebufpos[j+3]));
	}
	)
#endif
    }
    
    /* Copy the chunky buffer to the bitmap */
    if (!interlaced)
    {
#if LEGACY
	WriteChunkyPixels(&rp, leftedge, topedge, leftedge+width-1, topedge+height-1, gifhandle->linebuf, width);
#else
	if(!DoSuperMethod(cl, o,
			PDTM_WRITEPIXELARRAY,		/* Method_ID */
			(IPTR)gifhandle->linebuf,	/* PixelData */
			PBPAFMT_LUT8,			/* PixelFormat */
			width,				/* PixelArrayMod (number of bytes per row) */
			leftedge,			/* Left edge */
			topedge,			/* Top edge */
			width,				/* Width */
			height))			/* Height */
	{
	    D(bug("gif.datatype/LoadGIF() --- WRITEPIXELARRAY failed !\n"));
	    GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}
#endif /* LEGACY */
    }
    else
    {
	D(bug("gif.datatype/LoadGIF() --- de-interlacing\n"));
	line = 0;
	inc = 8;
	j = 0;
	for (i=0; i<widthxheight; i+=width)
	{
#if LEGACY
	    WriteChunkyPixels(&rp, leftedge, topedge+line, leftedge+width-1, topedge+line, gifhandle->linebuf+i, width);
#else
	    if(!DoSuperMethod(cl, o,
			    PDTM_WRITEPIXELARRAY,	/* Method_ID */
			    (IPTR)gifhandle->linebuf+i,	/* PixelData */
			    PBPAFMT_LUT8,		/* PixelFormat */
			    width,			/* PixelArrayMod (number of bytes per row) */
			    leftedge,			/* Left edge */
			    topedge+line,		/* Top edge */
			    width,			/* Width */
			    1))				/* Height (here: one line) */
	    {
		D(bug("gif.datatype/LoadGIF() --- WRITEPIXELARRAY failed !\n"));
		GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	    }
#endif /* LEGACY */
	    line += inc;
	    if (line >= height)
	    {
		j++;
		if (j > 3) break;
		switch (j)
		{
		    case 1: inc = 8; line = 4; break;
		    case 2: inc = 4; line = 2; break;
		    case 3: inc = 2; line = 1; break;
		}
	    }
	    
	}
    }

    D(bug("gif.datatype/LoadGIF() --- Normal Exit\n"));
    GIF_Exit(gifhandle, 0);
    return TRUE;
}

/**************************************************************************************************/

static BOOL SaveGIF(struct IClass *cl, Object *o, struct dtWrite *dtw )
{
    GifHandleType           *gifhandle;
    UBYTE                   *filebuf;
    unsigned int            width, height, widthxheight, numplanes, numcolors;
    struct BitMapHeader     *bmhd;
    long                    *colorregs;
    int                     i, ret;
#if LEGACY
    struct BitMap           *bm;
    struct RastPort         rp;
#endif

    D(bug("gif.datatype/SaveGIF()\n"));

    if( !(gifhandle = AllocMem(sizeof(GifHandleType), MEMF_ANY)) )
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    gifhandle->filebuf = NULL;
    gifhandle->linebuf = NULL;
    gifhandle->codecvars = NULL;

    /* A NULL file handle is a NOP */
    if( !dtw->dtw_FileHandle )
    {
	D(bug("gif.datatype/SaveGIF() --- empty Filehandle - just testing\n"));
	GIF_Exit(gifhandle, 0);
	return TRUE;
    }
    gifhandle->filehandle = dtw->dtw_FileHandle;

#if LEGACY
    /* Get BitMap, BitMapHeader and color palette */
    if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR) &bmhd,
			PDTA_BitMap,       (IPTR) &bm,
			PDTA_CRegs,        (IPTR) &colorregs,
			PDTA_NumColors,    (IPTR) &numcolors,
			TAG_DONE ) != 4UL ||
	!bmhd || !bm || !colorregs || !numcolors)
    {
	D(bug("gif.datatype/SaveGIF() --- missing attributes\n"));
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
#else
    /* Get BitMapHeader and color palette */
    if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR) &bmhd,
			PDTA_CRegs,        (IPTR) &colorregs,
			PDTA_NumColors,    (IPTR) &numcolors,
			TAG_DONE ) != 3UL ||
	!bmhd || !colorregs || !numcolors)
    {
	D(bug("gif.datatype/SaveGIF() --- missing attributes\n"));
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
#endif /* LEGACY */

    /* initialize buffered file writes */
    gifhandle->filebufsize = FILEBUFSIZE;
    gifhandle->filebufbytes = gifhandle->filebufsize;
    if( !(gifhandle->filebuf = gifhandle->filebufpos = AllocMem(gifhandle->filebufsize, MEMF_ANY)) )
    {
	GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }

    /* write GIF 87a header to file, make sure, there are at least 13 bytes in buffer */
    if ( (gifhandle->filebufbytes -= 13) < 0 && !SaveGIF_EmptyBuf(gifhandle, 13) )
    {
	D(bug("gif.datatype/SaveGIF() --- filling buffer with header failed\n"));
	GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    filebuf = gifhandle->filebufpos;    /* this makes things easier */
    gifhandle->filebufpos += 13;
    for (i = 0; i < 6; i++)
	filebuf[i] = GIFheader[i];

    /* set screen descriptor attributes (from BitMapHeader) */
    width = bmhd->bmh_Width;
    height = bmhd->bmh_Height;
    numplanes = bmhd->bmh_Depth - 1;
    numcolors = 1 << (numplanes + 1);
    D(bug("gif.datatype/SaveGIF() --- GIF-Image %d x %d x %d, cols %d\n", width, height, numplanes+1, numcolors));
    filebuf[6] = width & 0xff;
    filebuf[7] = width >> 8;
    filebuf[8] = height & 0xff;
    filebuf[9] = height >> 8;
    filebuf[10] = 0x80 | ((numplanes & 0x07) << 4) | (numplanes & 0x07) ; /* set numplanes, havecolmap=1 */
    filebuf[11] = 0;    /* this is fillcolor */
    filebuf[12] = 0;    /* this is pixel aspect ratio, 0 means unused */

    /* write screen colormap, we don't use an image colormap */
    for (i = 0; i < numcolors*3; i += 3)
    {
	if ( (gifhandle->filebufbytes -= 3) < 0 && !SaveGIF_EmptyBuf(gifhandle, 3) )
	{
	    GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	    return FALSE;
	}
	*(gifhandle->filebufpos)++ = colorregs[i] >> 24;
	*(gifhandle->filebufpos)++ = colorregs[i+1] >> 24;
	*(gifhandle->filebufpos)++ = colorregs[i+2] >> 24;
    }

    /* write image header, image has same size as screen */
    if ( (gifhandle->filebufbytes -= 10) < 0 && !SaveGIF_EmptyBuf(gifhandle, 10) )
    {
	GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    filebuf = gifhandle->filebufpos;    /* this makes things easier */
    gifhandle->filebufpos += 10;
    filebuf[0] = ',';       /* header ID */
    filebuf[1] = filebuf[2] = 0;    /* no left edge */
    filebuf[3] = filebuf[4] = 0;    /* no top edge */
    filebuf[5] = width & 0xff;
    filebuf[6] = width >> 8;
    filebuf[7] = height & 0xff;
    filebuf[8] = height >> 8;
    filebuf[9] = numplanes & 0x07; /* set numplanes, havecolmap=0, interlaced=0 */

    /* Now read the picture data and write it to a chunky buffer */
    /* For now, we use a full picture pixel buffer, not a single line */
    widthxheight = width*height;
    gifhandle->linebufsize = gifhandle->linebufbytes = widthxheight;
    if (! (gifhandle->linebuf = gifhandle->linebufpos = AllocMem(gifhandle->linebufsize, MEMF_ANY)) )
    {
	GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
#if LEGACY
    InitRastPort(&rp);
    rp.BitMap=bm;
    for (j=0; j<height; j++)
    {
	for (i=0; i<width; i++)
	{
	    ret = (UBYTE)ReadPixel(&rp, i, j);  /* very slow, to be changed */
	    *(gifhandle->linebufpos)++ = ret;
	}
    }
    gifhandle->linebufpos = gifhandle->linebuf;
#else
    if(!DoSuperMethod(cl, o,
		    PDTM_READPIXELARRAY,	/* Method_ID */
		    (IPTR)gifhandle->linebuf,	/* PixelData */
		    PBPAFMT_LUT8,		/* PixelFormat */
		    width,			/* PixelArrayMod (number of bytes per row) */
		    0,				/* Left edge */
		    0,				/* Top edge */
		    width,			/* Width */
		    height))			/* Height */
    {
	D(bug("gif.datatype/SaveGIF() --- READPIXELARRAY failed !\n"));
	GIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
#endif /* LEGACY */

    /* write the chunky buffer to file, after encoding */
    ret = EncodeInit(gifhandle, numplanes+1);
    D(bug("gif.datatype/SaveGIF() --- EncodeInit() returned %d\n", ret));
    ret = EncodeLines(gifhandle);
    D(bug("gif.datatype/SaveGIF() --- EncodeLines() returned %d\n", ret));
    ret = EncodeEnd(gifhandle);
    D(bug("gif.datatype/SaveGIF() --- EncodeEnd() returned %d\n", ret));
    
    /* write end-of-GIF marker */
    if ( !gifhandle->filebufbytes-- && !SaveGIF_EmptyBuf(gifhandle, 1) )
    {
	GIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    *(gifhandle->filebufpos)++ = ';';

    /* flush write buffer to file and exit */
    SaveGIF_EmptyBuf(gifhandle, 0);
    D(bug("gif.datatype/SaveGIF() --- Normal Exit\n"));
    GIF_Exit(gifhandle, 0);
    return TRUE;
}



/**************************************************************************************************/

IPTR GIF__OM_NEW(Class *cl, Object *o, Msg msg)
{
    Object *newobj;
    
    D(bug("gif.datatype: Method OM_NEW\n"));
    newobj = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (newobj)
    {
	if (!LoadGIF(cl, newobj))
	{
	    CoerceMethod(cl, newobj, OM_DISPOSE);
	    newobj = NULL;
	}
    }

    return (IPTR)newobj;
}
    
/**************************************************************************************************/

IPTR GIF__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    D(bug("gif.datatype: Method DTM_WRITE\n"));
    if( (dtw -> dtw_Mode) == DTWM_RAW )
    {
	/* Local data format requested */
	return SaveGIF(cl, o, dtw );
    }
    else
    {
	/* Pass msg to superclass (which writes an IFF ILBM picture)... */
	return DoSuperMethodA( cl, o, (Msg)dtw );
    }
}

/**************************************************************************************************/
