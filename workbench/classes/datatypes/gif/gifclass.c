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

#include "compilerspecific.h"
#include "debug.h"

#include "methods.h"
#include "gifclass.h"
#include "codec.h"

/**************************************************************************************************/

#define FILEBUFSIZE 4096
#define MAXCOLORS	256
const char GIFheader[] = "GIF87a";

/**************************************************************************************************/

static void LoadGIF_Exit(GifHandleType *gifhandle, LONG errorcode)
{
    D(bug("gif.datatype/LoadGIF_Exit()\n"));
	if (gifhandle->filebuf)
	{
		FreeMem(gifhandle->filebuf, gifhandle->filebufsize);
	}
	if (gifhandle->linebuf)
	{
		FreeMem(gifhandle->linebuf, gifhandle->linebufsize);
	}
    SetIoErr(errorcode);
}

BOOL LoadGIF_FillBuf(GifHandleType *gifhandle, long minbytes)
{
	long				i, bytes;
	
//	D(bug("gif.datatype/LoadGIF_FillBuf() --- minimum %ld bytes of %ld bytes\n", (long)minbytes, (long)gifhandle->filebufbytes));
	gifhandle->filebufbytes -= minbytes;
	if ( gifhandle->filebufbytes >= 0 )
		return TRUE;
	bytes = gifhandle->filebufbytes + minbytes;
	D(bug("gif.datatype/LoadGIF_FillBuf() --- %ld bytes requested, %ld bytes left\n", (long)minbytes, (long)bytes));
	if (bytes > 0)
	{
		D(bug("gif.datatype/LoadGIF_FillBuf() --- existing %ld old bytes\n", (long)bytes));
		for (i=0; i<bytes; i++)		/* copy existing bytes to start of buffer */
			gifhandle->filebuf[i] = gifhandle->filebufpos[i];
	}
	gifhandle->filebufpos = gifhandle->filebuf;
	bytes = Read(gifhandle->filehandle, gifhandle->filebufpos, gifhandle->filebufsize - bytes);
	if (bytes < 0 ) bytes = 0;
	gifhandle->filebufbytes += bytes;
	D(bug("gif.datatype/LoadGIF_FillBuf() --- read %ld bytes, remaining new %ld bytes\n", (long)bytes, (long)gifhandle->filebufbytes));
	if (gifhandle->filebufbytes >= 0)
		return TRUE;
	return FALSE;
}

static BOOL LoadGIF_Colormap(GifHandleType *gifhandle, int havecolmap, int *numcolors,
							struct ColorRegister *colormap, ULONG* colregs)
{
	unsigned int		i, j;

	if (havecolmap && *numcolors && *numcolors <= MAXCOLORS)
	{
		j = 0;
		for (i = 0; i < *numcolors; i++)
		{
			if ( !LoadGIF_FillBuf(gifhandle, 3) )
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
//			D(if (i<5) bug("gif r %02lx g %02lx b %02lx\n", colormap[i].red, colormap[i].green, colormap[i].blue));
		}
		D(bug("gif.datatype/LoadGIF_Colormap() --- colormap loaded\n"));
	}
	else
	{
		*numcolors = 0;
		D(bug("gif.datatype/LoadGIF_Colormap() --- no colormap load needed\n"));
	}
	return TRUE;
}

/**************************************************************************************************/
static BOOL LoadGIF(Object *o)
{
	GifHandleType			*gifhandle;
	UBYTE					*filebuf;
    IPTR					sourcetype;
	int						done, ret;
    int						i;
	long					j;
	unsigned int			width, height, leftedge, topedge;
	unsigned int			numplanes, numcolors, scrnumcolors, fillcolor;
	int						havecolmap, interlaced;
    struct BitMapHeader		*bmhd;
    struct BitMap			*bm;
	struct ColorRegister	*colormap;
	long					*colorregs;
	struct RastPort			rp;

    D(bug("gif.datatype/LoadGIF()\n"));
       
	if( !(gifhandle = AllocMem(sizeof(GifHandleType), MEMF_ANY | MEMF_CLEAR)) )
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return FALSE;
    }
	
    if( !(GetDTAttrs(o,	DTA_SourceType	, (IPTR)&sourcetype ,
						DTA_Handle    	, (IPTR)&(gifhandle->filehandle),
						PDTA_BitMapHeader , (IPTR)&(bmhd),
						TAG_DONE) == 3) ||
		(sourcetype != DTST_FILE) || !gifhandle->filehandle || !bmhd )
    {
    	LoadGIF_Exit(gifhandle, ERROR_OBJECT_NOT_FOUND);
		return FALSE;
    }
    
	/* initialize buffered file reads */
    D(bug("gif.datatype/LoadGIF() --- creating filebuf\n"));
	gifhandle->filebufbytes = 0;
	gifhandle->filebufsize = FILEBUFSIZE;
	if( !(gifhandle->filebuf = gifhandle->filebufpos = AllocMem(gifhandle->filebufsize, MEMF_ANY)) )
    {
    	LoadGIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
    	return FALSE;
    }

	/* load GIF header from file, make sure, there are at least 13 bytes in buffer */
	if( !LoadGIF_FillBuf(gifhandle, 13) )
	{
		D(bug("gif.datatype/LoadGIF() --- filling buffer with header failed\n"));
		LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
    	return FALSE;
    }
	
	/* check for GIF87a or GIF89a header */
	filebuf = gifhandle->filebufpos;	/* this makes things easier */
	gifhandle->filebufpos += 13;
	for (i = 0; i < 6; i++)
	if( (filebuf[i] != GIFheader[i]) && (i != 5) )
	{
		D(bug("gif.datatype/LoadGIF() --- type mismatch\n"));
		LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}

	/* get screen descriptor attributes and set BitMapHeader*/
    width = filebuf[6] | filebuf[7] << 8;
    height = filebuf[8] | filebuf[9] << 8;
    numplanes = (filebuf[10] & 0x0F) + 1;
    havecolmap = (filebuf[10] & 0x80) != 0;
    numcolors = 1 << numplanes;
    fillcolor = filebuf[11];		/* filebuf[12] is reserved */

	bmhd->bmh_Width  = bmhd->bmh_PageWidth  = width;
    bmhd->bmh_Height = bmhd->bmh_PageHeight = height;
    bmhd->bmh_Depth  = numplanes;

	/* get BitMap to draw into */
    D(bug("gif.datatype/LoadGIF() --- allocating bitmap\n"));
	if( !(bm = AllocBitMap(width, height, numplanes, BMF_CLEAR, NULL)) )
    {
		D(bug("gif.datatype/LoadGIF() --- error allocating bitmap\n"));
		LoadGIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
    	return FALSE;
    }
	InitRastPort(&rp);
	rp.BitMap=bm;

	/* get empty colormap to fill in colormap to use*/
	if( !(GetDTAttrs(o,	PDTA_ColorRegisters, &colormap,
						PDTA_CRegs, &colorregs,
						TAG_DONE ) == 2) ||
		!(colormap && colorregs) )
	{
		LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
 	D(bug("gif.datatype/LoadGIF() --- got colormap\n"));

	/* if we have a colormap for the screen, use it */
	if( !LoadGIF_Colormap(gifhandle, havecolmap, &numcolors, colormap, colorregs) )
	{
		LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}
	scrnumcolors = numcolors;

    D(bug( "gif scrwidth		%ld\n", (long)width));
    D(bug( "gif scrheight		%ld\n", (long)height));
    D(bug( "gif scrnumplanes	%ld\n", (long)numplanes));
    D(bug( "gif scrhavecolmap	%ld\n", (long)havecolmap));
    D(bug( "gif scrnumcolors	%ld\n", (long)numcolors));
    D(bug( "gif scrfillcolor	%ld\n", (long)fillcolor));

    /* Now display one or more GIF objects */
	done = FALSE;
    while(!done)
	{
		if ( !LoadGIF_FillBuf(gifhandle, 1) )
		{
			D(bug("gif.datatype/LoadGIF() --- early end 1\n"));
			LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
			return FALSE;
		}
#if 0
		D(bug("gif remaining bytes %02ld\n", (long)gifhandle->filebufbytes);
		for (j=-8; j<12; j+=4)
		{
			bug(" pos %02ld: %02lx %02lx %02lx %02lx\n", (long)j, (long)gifhandle->filebufpos[j],
				(long)gifhandle->filebufpos[j+1], (long)gifhandle->filebufpos[j+2], (long)gifhandle->filebufpos[j+3]);
		} )
#endif
		switch ( *(gifhandle->filebufpos)++ )
		{
		case ';':
			/* End of the GIF dataset */
			done = TRUE;
			break;

	    case ',':
			/* Start of an image object. Read the image description. */
			if ( !LoadGIF_FillBuf(gifhandle, 9) )
			{
				D(bug("gif.datatype/LoadGIF() --- early end 2\n"));
				LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
				return FALSE;
			}
			filebuf = gifhandle->filebufpos;	/* this makes things easier */
			gifhandle->filebufpos += 9;
			leftedge = filebuf[0] | filebuf[1] << 8;
			topedge = filebuf[2] | filebuf[3] << 8;
			width = filebuf[4] | filebuf[5] << 8;
			height = filebuf[6] | filebuf[7] << 8;
			numplanes = (filebuf[8] & 0x0F) + 1;
			interlaced = ((filebuf[8] & 0x70) >> 4) + 1;
			havecolmap = (filebuf[8] & 0x80) != 0;
			numcolors = 1 << numplanes;

			/* Get image colormap, if present. This overrides screen colormap. */
			if( !LoadGIF_Colormap(gifhandle, havecolmap, &numcolors, colormap, colorregs) ||
				(!numcolors && !scrnumcolors) )
			{
				D(bug("gif.datatype/LoadGIF() --- problem with image colors or no colors at all\n"));
				LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
				return FALSE;
			}

			D(bug( "gif winleftedge		%ld\n", (long)leftedge));
			D(bug( "gif wintopedge		%ld\n", (long)topedge));
			D(bug( "gif winwidth		%ld\n", (long)width));
			D(bug( "gif winheight		%ld\n", (long)height));
			D(bug( "gif winnumplanes	%ld\n", (long)numplanes));
			D(bug( "gif wininterlaced	%ld\n", (long)interlaced));
			D(bug( "gif winhavecolmap	%ld\n", (long)havecolmap));
			D(bug( "gif winnumcolors	%ld\n", (long)numcolors));

			/* decode */
		    done = TRUE;
			break;

	    case '!':
			/* Start of an extended object, but we cannot handle it */
			D(bug("gif.datatype/LoadGIF() --- extended object\n"));
			LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
			return FALSE;

	    default:
			D(bug("gif.datatype/LoadGIF() --- early end 3\n"));
			LoadGIF_Exit(gifhandle, ERROR_OBJECT_WRONG_TYPE);
			return FALSE;
	    }
	}

/**********************/

    D(bug("gif.datatype/LoadGIF() --- creating linebuf\n"));
	gifhandle->linebufbytes = 0;
	gifhandle->linebufsize = width*height;
	if (! (gifhandle->linebuf = gifhandle->linebufpos = AllocMem(gifhandle->linebufsize, MEMF_ANY)) )
    {
    	LoadGIF_Exit(gifhandle, ERROR_NO_FREE_STORE);
    	return FALSE;
    }
    
  for(j=0; j<width*height; j++)
  {
   (gifhandle->linebuf)[j] = j & 0xff;
  }

	D(bug("gif.datatype/LoadGIF() --- decode init\n"));
	ret = DecompressInit(gifhandle);
	D(bug("gif.datatype/LoadGIF() --- decode init ret %d\n", ret));
	D(bug("gif.datatype/LoadGIF() --- decode lines\n"));
	ret = DecompressLine(gifhandle);
	D(bug("gif.datatype/LoadGIF() --- decode lines ret %d\n", ret));
	D(bug("gif remaining bytes %02ld, next byte %02lx\n", (long)gifhandle->filebufbytes, (long)*(gifhandle->filebufpos)));

//	for(j=0; j<height; j++)
//	{
		WriteChunkyPixels(&rp, 0, 0, width-1, height-1, gifhandle->linebuf, width);
//	}

/**********************/
	
    SetDTAttrs(o, NULL, NULL, PDTA_NumColors, scrnumcolors,
							  TAG_DONE);
   	SetDTAttrs(o, NULL, NULL, DTA_NominalHoriz, bmhd->bmh_Width,
    	    	    	      DTA_NominalVert , bmhd->bmh_Height,
			      			  PDTA_BitMap     , (IPTR)bm,
			      			  TAG_DONE);

   	LoadGIF_Exit(gifhandle, 0);
    return TRUE;
}

/**************************************************************************************************/

static IPTR GIF_New(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
    	if (!LoadGIF((Object *)retval))
	{
	    CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
	    retval = 0;
	}
    }
    
    return retval;
}

/**************************************************************************************************/

#ifdef _AROS
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(Msg, msg, A1))
#else
ASM IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 Msg msg)
#endif
{
#ifdef _AROS
    AROS_USERFUNC_INIT
#endif

    IPTR retval;

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

    D(bug("gif.datatype/DT_Dispatcher: Entering\n"));

    switch(msg->MethodID)
    {
	case OM_NEW:
	    D(bug("gif.datatype/DT_Dispatcher: Method OM_NEW\n"));
	    retval = GIF_New(cl, o, (struct opSet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch(msg->MethodID) */

    D(bug("gif.datatype/DT_Dispatcher: Leaving\n"));

    return retval;
    
#ifdef _AROS
    AROS_USERFUNC_EXIT
#endif
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *gifbase)
{
    struct IClass *cl;
    
    cl = MakeClass("gif.datatype", "picture.datatype", 0, 0, 0);

    D(bug("gif.datatype/DT_MakeClass: DT_Dispatcher 0x%lx\n", (unsigned long) DT_Dispatcher));

    if (cl)
    {
#ifdef _AROS
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR)gifbase; /* Required by datatypes (see disposedtobject) */
    }

    return cl;
}

/**************************************************************************************************/

