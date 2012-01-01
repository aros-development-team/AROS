/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics colormap class implementation.
    Lang: english
*/

/****************************************************************************************/

#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/arossupport.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include <graphics/text.h>

#include <hidd/graphics.h>

#include "graphics_intern.h"

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************************

    NAME
        aoHidd_ColorMap_NumEntries

    SYNOPSIS
        [I.G], ULONG

    LOCATION
        CLID_Hidd_ColorMap

    FUNCTION
        Number of colors in the colormap.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

OOP_Object *CM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct colormap_data    *data;
    ULONG   	    	    numentries;
    struct TagItem  	    *tag, *tstate;
    BOOL    	    	    ok = FALSE;
    
    EnterFunc(bug("ColorMap::New()\n"));
    numentries = 256;
    
    for (tstate = msg->attrList; (tag = NextTagItem(&tstate)); )
    {
    	ULONG idx;
	
    	if (IS_COLORMAP_ATTR(tag->ti_Tag, idx))
	{
	    switch (idx)
	    {
	    	case aoHidd_ColorMap_NumEntries:
		    numentries = tag->ti_Data;
		    if (numentries > 256 || numentries < 0)
		    {
		     	D(bug("!!! ILLEGAL value for NumEntries in ColorMap::New()\n"));
		    }
		    break;
		   
	    } /* switch */
	
	}
    }
    
    /* Create the object */
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (NULL == o)
    	return NULL;
	
    data = OOP_INST_DATA(cl, o);
    
    data->clut.entries = numentries;
    
    data->clut.colors = AllocMem(sizeof (HIDDT_Color) * data->clut.entries, MEMF_CLEAR);
    if (NULL != data->clut.colors)
    {
	ok = TRUE;
    }
    
    if (!ok)
    {
    	ULONG dispose_mid;
	
	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
	o = NULL;
    }
    
    ReturnPtr("ColorMap::New", OOP_Object *, o);
}

/****************************************************************************************/

VOID CM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct colormap_data *data;
    
    data = OOP_INST_DATA(cl, o);
   
    if (NULL != data->clut.colors)
    {
	FreeMem(data->clut.colors, data->clut.entries * sizeof (HIDDT_Color));

	/* To detect use of already freed mem */
	data->clut.colors = (void *)0xDEADBEEF;
    }
	     
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

/****************************************************************************************/

VOID CM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct colormap_data *data;
    ULONG   	    	idx;
    
    EnterFunc(bug("ColorMap::Get()\n"));
    data = OOP_INST_DATA(cl, o);
    
    if (IS_COLORMAP_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_ColorMap_NumEntries:
	    	*msg->storage = data->clut.entries;
		break;
	    
	    default:
	    	D(bug("!!! Unknown colormap attr in ColorMap::Get()\n"));
		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    
    ReturnVoid("ColorMap::Get");
}

/****************************************************************************************/

static inline HIDDT_Pixel int_map_truecolor(HIDDT_Color *color, HIDDT_PixelFormat *pf)
{
    HIDDT_Pixel red     = color->red;
    HIDDT_Pixel green   = color->green;
    HIDDT_Pixel blue    = color->blue;
    HIDDT_Pixel alpha   = color->alpha;

    /* This code assumes that sizeof(HIDDT_Pixel) is a multiple of sizeof(col->#?),
       which should be true for most (all?) systems. (I have never heard of any
       system with for example 3 byte types.)
    */

    if (HIDD_PF_SWAPPIXELBYTES(pf))
    {
        /* FIXME: int_map_truecolor assuming that SwapPixelBytes flag only set for 2-byte/16-bit pixel formats */

        HIDDT_Pixel pixel = MAP_RGBA(red, green, blue, alpha, pf);

        color->pixval = SWAPBYTES_WORD(pixel);
    }
    else
    {
        color->pixval = MAP_RGBA(red, green, blue, alpha, pf);
    }

    return color->pixval;
}

/*****************************************************************************************

    NAME
        moHidd_ColorMap_SetColors

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_ColorMap_SetColors *msg);

        BOOL HIDD_CM_SetColors(OOP_Object *obj, HIDDT_Color *colors, ULONG firstColor,
                               ULONG numColors, OOP_Object *pixFmt);

    LOCATION
        CLID_Hidd_ColorMap

    FUNCTION

    INPUTS
        obj        -
        colors     -
        firstColor -
        numColors  -
        pixFmt     -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

BOOL CM__Hidd_ColorMap__SetColors(OOP_Class *cl, OOP_Object *o,
				  struct pHidd_ColorMap_SetColors *msg)
{
    struct colormap_data    *data;
    ULONG   	    	    numnew;
    ULONG   	    	    i, col_idx;
    HIDDT_Color     	    *col;
    HIDDT_PixelFormat 	    *pf;

    data = OOP_INST_DATA(cl, o);

    numnew = msg->firstColor + msg->numColors;
    
    /* See if there is enough space in the array  */
    
    if (numnew > data->clut.entries)
    {
     	/* Reallocate and copy */
	HIDDT_Color *newmap;
	
	newmap = AllocMem(sizeof (*newmap) * numnew, MEMF_ANY);
	if (NULL == newmap)
	    return FALSE;
	    
	memcpy(newmap, data->clut.colors, sizeof (*newmap) * data->clut.entries);
	
	FreeMem(data->clut.colors, sizeof (*newmap) * data->clut.entries);
	
	data->clut.colors  = newmap;
	data->clut.entries = numnew;
    }
     
    /* Insert the new colors */
    col_idx = msg->firstColor;
    col = &data->clut.colors[msg->firstColor];
    pf = (HIDDT_PixelFormat *)msg->pixFmt;
    
    for (i = 0; i < msg->numColors; i ++)
    {    
    	/* Set the color */
	*col = msg->colors[i];
	
	/* Set the pixval using the supplied pixel format */
	if (IS_TRUECOLOR(pf))
	{
	    /* Map the color to a HIDDT_Pixel */
	    msg->colors[i].pixval = col->pixval = int_map_truecolor(col, pf);
	
	}
	else
	{
	    msg->colors[i].pixval = col->pixval = (HIDDT_Pixel)col_idx;
	}
	
/*	bug("ColMap::SetColors: col %d (%x %x %x %x) mapped to %x\n"
		, col_idx
		, col->red, col->green, col->blue, col->alpha
		, msg->colors[i].pixval);
	
*/

	col ++;
	col_idx ++;
    }
    
    return TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_ColorMap_GetPixel

    SYNOPSIS
        HIDDT_Pixel OOP_DoMethod(OOP_Object *obj, struct pHidd_ColorMap_GetPixel *msg);

        HIDDT_Pixel HIDD_CM_GetPixel(OOP_Object *obj, ULONG pixelNo);

    LOCATION
        CLID_Hidd_ColorMap

    FUNCTION

    INPUTS
        obj     -
        pixelNo -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

HIDDT_Pixel CM__Hidd_ColorMap__GetPixel(OOP_Class *cl, OOP_Object *o,
					struct pHidd_ColorMap_GetPixel *msg)
{
    struct colormap_data *data;
     
    data = OOP_INST_DATA(cl, o);
     
    if (msg->pixelNo < 0 || msg->pixelNo >= data->clut.entries)
    {
	D(bug("!!! Unvalid msg->pixelNo (%d) in ColorMap::GetPixel(). clutentries = %d\n",
		msg->pixelNo,
		data->clut.entries));
	
    	// *((ULONG *)0) = 0;
	return (HIDDT_Pixel)-1;
	
    }
    
    return data->clut.colors[msg->pixelNo].pixval;
}

/*****************************************************************************************

    NAME
        moHidd_ColorMap_GetColor

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *o, struct pHidd_ColorMap_GetColor *msg);

        BOOL HIDD_CM_GetColor(OOP_Object *obj, ULONG colorNo, HIDDT_Color *colorReturn);

    LOCATION
        CLID_Hidd_ColorMap

    FUNCTION

    INPUTS
        obj         -
        colorNo     -
        colorReturn -

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

BOOL CM__Hidd_ColorMap__GetColor(OOP_Class *cl, OOP_Object *o,
				 struct pHidd_ColorMap_GetColor *msg)
{
    struct colormap_data *data;
    
    data = OOP_INST_DATA(cl, o);
    
    if (msg->colorNo < 0 || msg->colorNo >= data->clut.entries)
    {
	D(bug("!!! Unvalid msg->colorNo (%d) in ColorMap::GetPixel(). clutentries = %d\n",
		msg->colorNo,
		data->clut.entries));
		
	return FALSE;
    }
    
    *msg->colorReturn = data->clut.colors[msg->colorNo];
    
    return TRUE;
}
