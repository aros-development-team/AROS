/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

#include <string.h>

extern struct Library *MUIMasterBase;

struct MUI_BitmapData
{
    struct BitMap *bm, *remapped_bm;
    ULONG   	  *sourcecolors;
    LONG    	   width, height, precision, transparent;
    UBYTE   	  *mappingtable;
    PLANEPTR  	   mask;
    WORD    	  *remaptable;
    BYTE    	   usefriend;
};

static void remap_bitmap(struct IClass *cl, Object *obj)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);
    struct BitMap   	  *friendbm = NULL;
    struct RastPort 	  temprp, bmrp, *scrrp;
    UBYTE   	    	  *linebuffer;
    ULONG   	    	  *cgfxcoltab = NULL;
    LONG    	    	  bmflags = 0;
    WORD    	    	  bmdepth, bmwidth, bmheight, bmcols, x, y;

    if (!data->mappingtable && !data->sourcecolors) return;
    if (!data->bm || (data->width < 1) || (data->height < 1)) return;
    
    /* Don't remap if bitmap is hicolor/truecolor */
    if (GetBitMapAttr(data->bm, BMA_DEPTH) > 8) return;
    
    if (!data->mappingtable && !data->remaptable)
    {
    	data->remaptable = AllocVec(256 * sizeof(WORD), MEMF_PUBLIC | MEMF_CLEAR);
	if (!data->remaptable) return;
    }
   
    scrrp = &_screen(obj)->RastPort;
    
    if (data->usefriend)
    {
    	friendbm = scrrp->BitMap;
	bmflags |= BMF_MINPLANES;
    }
    
    linebuffer = AllocVec(data->width + 16, MEMF_PUBLIC);
    if (!linebuffer) return;

    bmdepth = GetBitMapAttr(scrrp->BitMap, BMA_DEPTH);
    if (bmdepth > 8) bmdepth = 8;
    
    bmcols = 1L << bmdepth;
    
    data->remapped_bm = AllocBitMap(data->width, data->height, bmdepth, bmflags, friendbm);

    if (!data->remapped_bm)
    {
    	FreeVec(linebuffer);
    	return;
    }

    bmwidth = GetBitMapAttr(data->remapped_bm, BMA_WIDTH);
    bmheight = GetBitMapAttr(data->remapped_bm, BMA_HEIGHT);

    if (data->transparent != -1)
    {
	data->mask = AllocRaster(bmwidth,bmheight);
	memset(data->mask, 0xff, RASSIZE(bmwidth, bmheight));
    }

    if (CyberGfxBase &&
    	!data->mappingtable &&
	(GetBitMapAttr(data->remapped_bm, BMA_DEPTH) >= 15))
    {
    	cgfxcoltab = AllocVec(bmcols * sizeof(ULONG), MEMF_ANY);
	
	if (cgfxcoltab)
	{
	    for(y = 0; y < bmcols; y++)
	    {
	    	ULONG red = data->sourcecolors[y * 3] & 0xFF000000;
		ULONG green = data->sourcecolors[y * 3 + 1] & 0xFF000000;
		ULONG blue = data->sourcecolors[y * 3 + 2] & 0xFF000000;
		
	    	cgfxcoltab[y] = (red >> 8) | (green >> 16) | (blue >> 24);
	    }
	}
	
    }
    
    InitRastPort(&temprp);
    temprp.BitMap = AllocBitMap(data->width, 1, 1, 0, NULL);
    
    InitRastPort(&bmrp);

    for(y = 0; y < data->height; y++)
    {
    	/* Read a line from source bitmap */
	
        bmrp.BitMap = data->bm;
    	if (temprp.BitMap)
	{
	    ReadPixelLine8(&bmrp, 0, y, data->width, linebuffer, &temprp);
	}
	else
	{
	    for(x = 0; x < data->width; x++)
	    {
	    	linebuffer[x] = ReadPixel(&bmrp, x, y);
	    }
	}

	/* Build the mask, totaly slow but works */
	if (data->mask)
	{
	    UBYTE *mask = data->mask + y * bmwidth / 8;
	    UBYTE xmask = 0x80;
	    
	    for(x = 0; x < data->width; x++)
	    {
	    	if (linebuffer[x] == data->transparent)
	    	{
		    *mask &= ~xmask;
	    	}
		
		xmask >>= 1;
		if (!xmask)
		{
		    xmask = 0x80;
		    mask++;
		}
	    }
	}
	
	/* Remap the line */
	if (data->mappingtable)
	{
	    for(x = 0; x < data->width; x++)
	    {
	    	linebuffer[x] = data->mappingtable[linebuffer[x]];
	    }
	}
	else if (!cgfxcoltab)
	{
	    for(x = 0; x < data->width; x++)
	    {
	    	UBYTE pixel = linebuffer[x];
		UBYTE remappixel = data->remaptable[pixel];
		
	    	if (!remappixel)
		{
		    struct TagItem tags[3];
		    tags[0].ti_Tag = OBP_Precision;
		    tags[0].ti_Data = data->precision;
		    tags[1].ti_Tag = OBP_FailIfBad;
		    tags[1].ti_Data = FALSE;
		    tags[2].ti_Tag = 0;
		    
		    data->remaptable[pixel] = remappixel = ObtainBestPenA(_screen(obj)->ViewPort.ColorMap,
		    	    	    	    	      			  data->sourcecolors[pixel * 3],
									  data->sourcecolors[pixel * 3 + 1],
									  data->sourcecolors[pixel * 3 + 2],
									  tags) | 0x100;
		}
		
		linebuffer[x] = (remappixel & 0xFF);
	    }

	} /* else if (!cgfxcoltab) */
	
	/* Write line into destination bitmap */
	
        bmrp.BitMap = data->remapped_bm;

	if (cgfxcoltab)
	{
	    WriteLUTPixelArray(linebuffer,
	    	    	       0,
			       0,
			       data->width,
			       &bmrp,
			       cgfxcoltab,
			       0,
			       y,
			       data->width,
			       1,
			       CTABFMT_XRGB8);
	}
	else
	{
            bmrp.BitMap = data->remapped_bm;
    	    if (temprp.BitMap)
	    {
		WritePixelLine8(&bmrp, 0, y, data->width, linebuffer, &temprp);
	    }
	    else
	    {
		for(x = 0; x < data->width; x++)
		{
	    	    SetAPen(&bmrp, linebuffer[x]);
	    	    WritePixel(&bmrp, x, y);
		}
	    }
	    
	}
	
    } /* for(y = 0; y < data->height; y++) */
    
    DeinitRastPort(&temprp);
    DeinitRastPort(&bmrp);

    if (temprp.BitMap)
    {
    	WaitBlit();
	FreeBitMap(temprp.BitMap);
    }
    
    FreeVec(cgfxcoltab);
    FreeVec(linebuffer);
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Bitmap_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BitmapData   *data;
    struct TagItem  	    *tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);

    data->precision = PRECISION_GUI;
    data->transparent = -1;
	
    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Bitmap_Bitmap:
	    	data->bm = (struct BitMap *)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Height:
	    	data->height = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_MappingTable:
	    	data->mappingtable = (UBYTE *)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Precision:
	    	data->precision = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_SourceColors:
	    	data->sourcecolors = (ULONG *)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Transparent:
	    	data->transparent = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_UseFriend:
	    	data->usefriend = (tag->ti_Data != 0);
		break;
		
	    case MUIA_Bitmap_Width:
	    	data->width = (LONG)tag->ti_Data;
		break;
		
    	}
    }
    
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Bitmap_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);

    if (data->remapped_bm)
    {
    	WaitBlit();
    	FreeBitMap(data->remapped_bm);
    }
    if (data->remaptable) FreeVec(data->remaptable);
    
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Bitmap_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BitmapData   *data  = INST_DATA(cl, obj);
    struct TagItem          *tags  = msg->ops_AttrList;
    struct TagItem          *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Bitmap_Bitmap:
	    	if (!data->remapped_bm)
		{
		    data->bm = (struct BitMap *)tag->ti_Data;
		}
		break;
		
	    case MUIA_Bitmap_Height:
	    	data->height = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_MappingTable:
	    	data->mappingtable = (UBYTE *)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Precision:
	    	data->precision = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_SourceColors:
	    	data->sourcecolors = (ULONG *)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Transparent:
	    	data->transparent = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Width:
	    	data->width = (LONG)tag->ti_Data;
		break;

	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Bitmap_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_BitmapData *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
	case MUIA_Bitmap_Bitmap:
	    STORE = (IPTR)data->bm;
	    return TRUE;

	case MUIA_Bitmap_Height:
	    STORE = (IPTR)data->height;
	    return TRUE;

	case MUIA_Bitmap_MappingTable:
	    STORE = (IPTR)data->mappingtable;
	    return TRUE;

	case MUIA_Bitmap_Precision:
	    STORE = (IPTR)data->precision;
	    return TRUE;

	case MUIA_Bitmap_RemappedBitmap:
	    STORE = (IPTR)data->remapped_bm;
	    return TRUE;

	case MUIA_Bitmap_SourceColors:
	    STORE = (IPTR)data->sourcecolors;
	    return TRUE;

	case MUIA_Bitmap_Transparent:
	    STORE = (IPTR)data->transparent;
	    return TRUE;

	case MUIA_Bitmap_Width:
	    STORE = (IPTR)data->width;
	    return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Bitmap_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    //struct MUI_BitmapData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
	return FALSE;

    remap_bitmap(cl, obj);
    
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR Bitmap_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);

    if (data->mask)
    {
	LONG bmwidth = GetBitMapAttr(data->remapped_bm, BMA_WIDTH);
	LONG bmheight = GetBitMapAttr(data->remapped_bm, BMA_HEIGHT);
	FreeRaster(data->mask,bmwidth,bmheight);

	data->mask = NULL;
    }

    if (data->remapped_bm)
    {
    	WaitBlit();
	FreeBitMap(data->remapped_bm);
	data->remapped_bm = NULL;
	
	if (data->remaptable)
	{
	    WORD i;
	    
	    for(i = 0; i < 256; i++)
	    {
	    	if (data->remaptable[i])
		{
		    ReleasePen(_screen(obj)->ViewPort.ColorMap, data->remaptable[i] & 0xFF);
		    data->remaptable[i] = 0;
		}
	    }
	}
    }
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Bitmap_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg)msg);
    
    msg->MinMaxInfo->MinWidth  += 1;
    msg->MinMaxInfo->MinHeight += 1;
    
    msg->MinMaxInfo->DefWidth  += 1;
    msg->MinMaxInfo->DefHeight += 1;
    
    msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    
    return TRUE;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Bitmap_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);
    struct BitMap   	  *bm;
    
    DoSuperMethodA(cl,obj,(Msg)msg);
    
    bm = data->remapped_bm ? data->remapped_bm : data->bm;
    if (bm)
    {
    	LONG width, height;
	
	width = data->width;
	height = data->height;
	
	if (width  > _mwidth(obj)) width = _mwidth(obj);
	if (height > _mheight(obj)) height = _mheight(obj);
	
	if ((width > 0) && (height > 0))
	{
	    if (data->mask)
	    {
	    	BltMaskBitMapRastPort(bm, 0, 0, _rp(obj), _mleft(obj), _mtop(obj), width, height, 0xE0, data->mask);
	    }
	    else
	    {
	    	BltBitMapRastPort(bm, 0, 0, _rp(obj), _mleft(obj), _mtop(obj), width, height, 0xC0);
	    }
	}
    }
    
    return TRUE;
}


BOOPSI_DISPATCHER(IPTR, Bitmap_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return Bitmap_New(cl, obj, (struct opSet *)msg);
	    
	case OM_DISPOSE:
	    return Bitmap_Dispose(cl, obj, msg);
	    
	case OM_SET:
	    return Bitmap_Set(cl, obj, (struct opSet *)msg);
	    
	case OM_GET:
	    return Bitmap_Get(cl, obj, (struct opGet *)msg);
	    
	case MUIM_Setup:
	    return Bitmap_Setup(cl, obj, msg);
	    
	case MUIM_Cleanup:
	    return Bitmap_Cleanup(cl, obj, msg);
	    
	case MUIM_AskMinMax:
	    return Bitmap_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
	    
	case MUIM_Draw:
	    return Bitmap_Draw(cl, obj, (struct MUIP_Draw *)msg);
	    
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Bitmap_desc = { 
    MUIC_Bitmap, 
    MUIC_Area, 
    sizeof(struct MUI_BitmapData), 
    (void*)Bitmap_Dispatcher 
};
