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
#include <proto/datatypes.h>

#include "compilerspecific.h"
#include "debug.h"

#include "methods.h"

/**************************************************************************************************/

typedef struct
{
	WORD		bfType;				// ASCII "BM"
	ULONG		bfSize;				// Size in bytes of the file
	WORD		bfReserved1;		// Zero
	WORD		bfReserved2;		// Zero
	ULONG		bfOffBits;			// Byte offset in files where image begins
} FileBitMapHeader __attribute__((packed));

typedef struct
{
	ULONG		biSize;				// Size of this header, 40 bytes
	LONG		biWidth;			// Image width in pixels
	LONG		biHeight;			// Image height in pixels
	WORD		biPlanes;			// Number of image planes, must be 1
	WORD		biBitCount;			// Bits per pixel, 1, 4, 8, 24, or 32
	ULONG		biCompression;		// Compression type, below
	ULONG		biSizeImage;		// Size in bytes of compressed image, or zero
	LONG		biXPelsPerMeter;	// Horizontal resolution, in pixels/meter
	LONG		biYPelsPerMeter;	// Vertical resolution, in pixels/meter
	ULONG		biClrUsed;			// Number of colors used, below
	ULONG		biClrImportant;		// Number of "important" colors
} BitmapInfoHeader __attribute__((packed));

/* "BM" backwards, due to LE byte order */
#define BITMAP_ID "MB"

/**************************************************************************************************/

static BOOL ReadBMP(Object *o)
{
    FileBitMapHeader 		*file_bmhd;
    BitmapInfoHeader		*file_bihd;
    struct BitMapHeader     *bmhd;
    struct FileHandle	    *handle;
    ULONG   	    	    numcolors;
    IPTR    	    	    sourcetype;
    LONG    	    	    error;
    struct BitMap			*bm;

    D(bug("bmp.datatype/ReadBMP()\n"));
       
    if (GetDTAttrs(o, DTA_SourceType	, (IPTR)&sourcetype ,
    	    	      DTA_Handle    	, (IPTR)&handle     , 
		      	/*	  PDTA_BitMapHeader , (IPTR)&bmhd	    ,*/
		      		  TAG_DONE	    	    	    	     ) != 2)
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
    
    D(bug("bmp.datatype/ReadBMP() --- reading headers\n"));
    
    /* Allocate memory for header structs */
    if (! (file_bmhd = AllocMem(sizeof(FileBitMapHeader), MEMF_ANY | MEMF_CLEAR)))
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return FALSE;
    }
    if (! (file_bihd = AllocMem(sizeof(BitmapInfoHeader), MEMF_ANY | MEMF_CLEAR)))
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return FALSE;
    }

	/* Read FileBitMapHeader struct entry by entry */
	FRead(handle, &file_bmhd->bfType,			2, 1);
	FRead(handle, &file_bmhd->bfSize,			4, 1);
	FRead(handle, &file_bmhd->bfReserved1,		2, 1);
	FRead(handle, &file_bmhd->bfReserved2,		2, 1);
	FRead(handle, &file_bmhd->bfOffBits,		4, 1);
	
	/* Read BitmapInfoHeader struct entry by entry */
	FRead(handle, &file_bihd->biSize,			4, 1);
	FRead(handle, &file_bihd->biWidth,			4, 1);
	FRead(handle, &file_bihd->biHeight,			4, 1);
	FRead(handle, &file_bihd->biPlanes,			2, 1);
	FRead(handle, &file_bihd->biBitCount,		2, 1);
	FRead(handle, &file_bihd->biCompression,	4, 1);
	FRead(handle, &file_bihd->biSizeImage,		4, 1);
	FRead(handle, &file_bihd->biXPelsPerMeter,	4, 1);
	FRead(handle, &file_bihd->biYPelsPerMeter,	4, 1);
	FRead(handle, &file_bihd->biClrUsed,		4, 1);
	FRead(handle, &file_bihd->biClrImportant,	4, 1);
D(bug("bmp.datatype/ReadBMP() --- done reading headers...\n"));
/*
D(bug("bmp.datatype/ReadBMP() --- before reading bmhd\n")); 
	FRead(handle, file_bmhd, sizeof(FileBitMapHeader), 1);
D(bug("bmp.datatype/ReadBMP() --- after reading bmhd\n"));
*/
/*    if (file_bmhd->bfType != BITMAP_ID)
    {
    	D(bug("bmp.datatype/ReadBMP() --- error reading bmhd\n"));
    	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
    	return FALSE;
    }
*/
/*
D(bug("bmp.datatype/ReadBMP() --- before reading bihd\n"));
   	FRead(handle, file_bihd, sizeof(BitmapInfoHeader), 1);
D(bug("bmp.datatype/ReadBMP() --- after reading bihd\n"));
*/
    if (file_bihd == NULL)
    {
    	D(bug("bmp.datatype/ReadBMP() --- error reading bihd\n"));
    	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
    	return NULL;
    }
    
    D(bug("bfType:           %x\n", file_bmhd->bfType));
    D(bug("bfSize:           %d\n", file_bmhd->bfSize));
    D(bug("bfReserved1:      %d\n", file_bmhd->bfReserved1));
    D(bug("bfReserved2:      %d\n", file_bmhd->bfReserved2));
    D(bug("bfOffBits:        %d\n", file_bmhd->bfOffBits));
    D(bug("biSize:           %d\n", file_bihd->biSize));
    D(bug("biWidth:          %d\n", file_bihd->biWidth));
    D(bug("biHeight:         %d\n", file_bihd->biHeight));
    D(bug("biPlanes:         %d\n", file_bihd->biPlanes));
    D(bug("biBitCount:       %d\n", file_bihd->biBitCount));
    D(bug("biCompression:    %d\n", file_bihd->biCompression));
    D(bug("biSizeImage:      %d\n", file_bihd->biSizeImage));
    D(bug("biXPelsPerMeter:  %d\n", file_bihd->biXPelsPerMeter));
    D(bug("biYPelsPerMeter:  %d\n", file_bihd->biYPelsPerMeter));
    D(bug("biClrUsed:        %d\n", file_bihd->biClrUsed));
    D(bug("biClrImportant:   %d\n", file_bihd->biClrImportant));
    
    D(bug("bmp.datatype/ReadBMP() --- before seeking to bitmap: %d\n", file_bmhd->bfOffBits));
	Seek(handle, file_bmhd->bfOffBits - 40, OFFSET_CURRENT);
	D(bug("bmp.datatype/ReadBMP() --- after seeking to bitmap\n"));
    
    D(bug("bmp.datatype/ReadBMP() --- before allocating bitmap\n"));
	bm = AllocBitMap(file_bihd->biWidth, file_bihd->biHeight, file_bihd->biBitCount, BMF_CLEAR | BMF_SPECIALFMT | (PIXFMT_BGRA32 << 24), NULL);    
	D(bug("bmp.datatype/ReadBMP() --- after allocating bitmap\n"));
    if (!bm)
    {
	    D(bug("bmp.datatype/ReadBMP() --- error allocating bitmap\n"));
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return FALSE;
    }
    
    D(bug("bmp.datatype/ReadBMP() --- before reading bitmap\n"));
    if (FRead(handle, bm, file_bihd->biSizeImage, 1) == 0)
    {
	    D(bug("bmp.datatype/ReadBMP() --- error reading bitmap\n"));	
    	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
    	return FALSE;
    }
    D(bug("bmp.datatype/ReadBMP() --- after reading bitmap\n"));
    
    numcolors = 2^file_bihd->biBitCount;
    
    bmhd->bmh_Width  = bmhd->bmh_PageWidth  = file_bihd->biWidth;
    bmhd->bmh_Height = bmhd->bmh_PageHeight = file_bihd->biHeight;
    bmhd->bmh_Depth  = file_bihd->biBitCount;

    SetDTAttrs(o, NULL, NULL, PDTA_NumColors, numcolors, TAG_DONE);
    
//   	IPTR name = NULL;
	
//	GetDTAttrs(o, DTA_Name, (IPTR)&name, TAG_DONE);
	
//   	SetDTAttrs(o, NULL, NULL, DTA_ObjName, name, TAG_DONE);
    
	SetDTAttrs(o, NULL, NULL, DTA_NominalHoriz, bmhd->bmh_Width ,
    	    	    	      DTA_NominalVert , bmhd->bmh_Height,
			      			  PDTA_BitMap     , (IPTR)bm      ,
			      			  TAG_DONE);

	/* Free mem */

    SetIoErr(0);
    
    return TRUE;
}

/**************************************************************************************************/

static IPTR BMP_New(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
    	if (!ReadBMP((Object *)retval))
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

    D(bug("bmp.datatype/DT_Dispatcher: Entering\n"));

    switch(msg->MethodID)
    {
	case OM_NEW:
	    D(bug("bmp.datatype/DT_Dispatcher: Method OM_NEW\n"));
	    retval = BMP_New(cl, o, (struct opSet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch(msg->MethodID) */

    D(bug("bmp.datatype/DT_Dispatcher: Leaving\n"));

    return retval;
    
#ifdef _AROS
    AROS_USERFUNC_EXIT
#endif
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *bmpbase)
{
    struct IClass *cl;
    
    cl = MakeClass("bmp.datatype", "picture.datatype", 0, 0, 0);

    D(bug("bmp.datatype/DT_MakeClass: DT_Dispatcher 0x%lx\n", (unsigned long) DT_Dispatcher));

    if (cl)
    {
#ifdef _AROS
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR)bmpbase; /* Required by datatypes (see disposedtobject) */
    }

    return cl;
}

/**************************************************************************************************/

