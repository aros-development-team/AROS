/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

/****************************************************************************************/

struct colormap_data
{
    HIDDT_ColorLUT clut;
};

/****************************************************************************************/

#define csd ((struct class_static_data *)cl->UserData)

/****************************************************************************************/

static OOP_Object *colormap_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct colormap_data    *data;
    ULONG   	    	    numentries;
    struct TagItem  	    *tag, *tstate;
    BOOL    	    	    ok = FALSE;
    
    numentries = 256;
    
    for (tstate = msg->attrList; (tag = NextTagItem((const struct TagItem **)&tstate)); )
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
    
    return o;
}

/****************************************************************************************/

static VOID colormap_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct colormap_data *data;
    
    data = OOP_INST_DATA(cl, o);
   
    if (NULL != data->clut.colors)
    {
	FreeMem(data->clut.colors, data->clut.entries * sizeof (HIDDT_Color));

	/* To detect use of allready freed mem */
	data->clut.colors = 0xDEADBEEF;
    }
	     
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

/****************************************************************************************/

static VOID colormap_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct colormap_data *data;
    ULONG   	    	idx;
    
    data = OOP_INST_DATA(cl, o);
    
    if (IS_COLORMAP_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_ColorMap_NumEntries:
	    	*msg->storage = data->clut.entries;
		break;
	    
	    default:
	    	D(bug("!!! Unknow colormap attr in ColorMap::Get()\n"));
		OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
		break;
	}
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    
    return;
}

/****************************************************************************************/

static BOOL colormap_setcolors(OOP_Class *cl, OOP_Object *o,
    	    	    	       struct pHidd_ColorMap_SetColors *msg)
{
    struct colormap_data    *data;
    ULONG   	    	    numnew;
    ULONG   	    	    i, col_idx;
    HIDDT_Color     	    *col;
    HIDDT_PixelFormat 	    *pf;

    data = OOP_INST_DATA(cl, o);

    numnew = msg->firstColor + msg->numColors;
    
    /* See if there is enpugh space in the array  */
    
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

/****************************************************************************************/

inline HIDDT_Pixel int_map_truecolor(HIDDT_Color *color, HIDDT_PixelFormat *pf)
{
    HIDDT_Pixel red	= color->red;
    HIDDT_Pixel green	= color->green;
    HIDDT_Pixel blue	= color->blue;
    
    
    /* This code assumes that sizeof (HIDDT_Pixel is a multimple of sizeof(col->#?)
       which should be true for most (all ?) systems. (I have never heard
       of any system with for example 3 byte types.
    */

    if (HIDD_PF_SWAPPIXELBYTES(pf))
    {
	#warning "int_map_truecolor assuming that SwapPixelBytes flag only set for 2-byte/16-bit pixel formats"

    	HIDDT_Pixel pixel = MAP_RGB(red, green, blue, pf);
			
	color->pixval = SWAPBYTES_WORD(pixel);;
    }
    else
    {
    	color->pixval = MAP_RGB(red, green, blue, pf);
    }

    return color->pixval;
}

/****************************************************************************************/

static HIDDT_Pixel colormap_getpixel(OOP_Class *cl, OOP_Object *o,
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
	return (HIDDT_Pixel)-1L;
	
    }
    
    return data->clut.colors[msg->pixelNo].pixval;
}

/****************************************************************************************/

static BOOL colormap_getcolor(OOP_Class *cl, OOP_Object *o,
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

/****************************************************************************************/

#undef OOPBase
#undef SysBase

#undef csd

#define OOPBase (csd->oopbase)
#define SysBase (csd->sysbase)

#define NUM_ROOT_METHODS   3
#define NUM_COLORMAP_METHODS 3

/****************************************************************************************/

OOP_Class *init_colormapclass(struct class_static_data *csd)
{
    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] =
    {
        {(IPTR (*)())colormap_new    , moRoot_New    	},
        {(IPTR (*)())colormap_dispose, moRoot_Dispose	},
        {(IPTR (*)())colormap_get    , moRoot_Get    	},
        {NULL	    	    	     , 0UL  	    	}
    };

    struct OOP_MethodDescr colormap_descr[NUM_COLORMAP_METHODS + 1] =
    {
        {(IPTR (*)())colormap_setcolors , moHidd_ColorMap_SetColors	},
        {(IPTR (*)())colormap_getpixel	, moHidd_ColorMap_GetPixel	},
        {(IPTR (*)())colormap_getcolor	, moHidd_ColorMap_GetColor	},
        {NULL	    	    	    	, 0UL	    	    	    	}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr 	, IID_Root	    , NUM_ROOT_METHODS	    },
        {colormap_descr , IID_Hidd_ColorMap , NUM_COLORMAP_METHODS  },
        {NULL	    	, NULL	    	    , 0     	    	    }
    };

    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID	    	, (IPTR) CLID_Root  	    	    	},
        {aMeta_InterfaceDescr	, (IPTR) ifdescr    	    	    	},
        {aMeta_ID   	    	, (IPTR) CLID_Hidd_ColorMap 	    	},
        {aMeta_InstSize     	, (IPTR) sizeof(struct colormap_data)	},
        {TAG_DONE   	    	, 0UL	    	    	    	    	}
    };
    
    OOP_Class *cl = NULL;
    
    EnterFunc(bug("init_colormapclass()\n"));

    if(MetaAttrBase)
    {
    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	
    	if(NULL != cl)
	{
            csd->colormapclass = cl;
            cl->UserData     = (APTR) csd;
	    OOP_AddClass(cl);
        }

    } /* if(MetaAttrBase) */
    
    if (NULL == cl)
	free_colormapclass(csd);
	
    ReturnPtr("init_colormapclass", OOP_Class *, cl);
}

/****************************************************************************************/

void free_colormapclass(struct class_static_data *csd)
{
    EnterFunc(bug("free_colormapclass()\n"));

    if(NULL != csd)
    {
	if (NULL != csd->colormapclass)
	{
    	    OOP_RemoveClass(csd->colormapclass);
    	    OOP_DisposeObject((OOP_Object *) csd->colormapclass);
    	    csd->colormapclass = NULL;
	}
    }
    
    ReturnVoid("free_colormapclass");
}

/****************************************************************************************/
