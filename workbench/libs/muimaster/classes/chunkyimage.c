/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

struct MUI_ChunkyImageData
{
    struct BitMap   *bm;
    UBYTE   	    *pixels;
    UBYTE   	    *palette;
    ULONG   	    *palette32;
    UWORD   	     width, height, numcolors, modulo;
};

static BOOL make_bitmap(struct IClass *cl, Object *obj)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);
    struct RastPort 	     temprp;
    WORD    	    	     depth;
    
    if (!data->pixels 	    	 ||
    	(data->width  	    < 1) ||
    	(data->height 	    < 1) ||
	(data->numcolors    < 1) ||
	(data->numcolors    > 256))
    {
    	return FALSE;
    }
    
    if (data->palette)
    {
    	data->palette32 = AllocVec(data->numcolors * 3 * sizeof(ULONG), MEMF_ANY);
    	if (!data->palette32) return FALSE;
	
    	if (data->palette32)
	{
	    UBYTE *src = data->palette;
	    ULONG *dest = data->palette32;
	    WORD i;
	    
	    for(i = 0; i < data->numcolors; i++)
	    {
	    	*dest++ = ((ULONG)(*src++)) * 0x01010101;
	    	*dest++ = ((ULONG)(*src++)) * 0x01010101;
	    	*dest++ = ((ULONG)(*src++)) * 0x01010101;
	    }
	}
    }
    
    for(depth = 1; (1L << depth) < data->numcolors; depth++)
    {
    }
    
    data->bm = AllocBitMap(data->width, data->height, depth, BMF_CLEAR, NULL);
    if (!data->bm)
    {
    	if (data->palette32)
	{
	    FreeVec(data->palette32);
	    data->palette32 = NULL;
	}
    	return FALSE;
    }
    
    InitRastPort(&temprp);
    temprp.BitMap = data->bm;
    
    WriteChunkyPixels(&temprp,
    	    	      0,
		      0,
		      data->width - 1,
		      data->height - 1,
		      data->pixels,
		      data->modulo);
    
    DeinitRastPort(&temprp);
    
    return TRUE;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR ChunkyImage_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ChunkyImageData    *data;
    struct TagItem  	    	*tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
	
    /* parse initial taglist */

    data->modulo = 0xFFFF;
    
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_ChunkyImage_Pixels:
	    	data->pixels = (UBYTE *)tag->ti_Data;
		break;
		
	    case MUIA_ChunkyImage_Palette:
	    	data->palette = (UBYTE *)tag->ti_Data;
		break;
		
	    case MUIA_ChunkyImage_NumColors:
	    	data->numcolors = (LONG)tag->ti_Data;
		break;

	    case MUIA_ChunkyImage_Modulo:
	    	data->modulo = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Width:
	    	data->width = (LONG)tag->ti_Data;
		break;

	    case MUIA_Bitmap_Height:
	    	data->height = (LONG)tag->ti_Data;
		break;
		
    	}
    }
    
    if (data->modulo == 0xFFFF) data->modulo = data->width;
    
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR ChunkyImage_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);

    if (data->bm)
    {
    	WaitBlit();
    	FreeBitMap(data->bm);
    }
    
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR ChunkyImage_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ChunkyImageData    *data  = INST_DATA(cl, obj);
    struct TagItem          	*tags  = msg->ops_AttrList;
    struct TagItem          	*tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_ChunkyImage_Pixels:
	    	data->pixels = (UBYTE *)tag->ti_Data;
		break;
		
	    case MUIA_ChunkyImage_Palette:
	    	data->palette = (UBYTE*)tag->ti_Data;
		break;
		
	    case MUIA_ChunkyImage_NumColors:
	    	data->numcolors = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_ChunkyImage_Modulo:
	    	data->modulo = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Width:
	    	data->width = (LONG)tag->ti_Data;
		break;

	    case MUIA_Bitmap_Height:
	    	data->height = (LONG)tag->ti_Data;
		break;

	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR ChunkyImage_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
	case MUIA_ChunkyImage_Pixels:
	    STORE = (IPTR)data->pixels;
	    return TRUE;

	case MUIA_ChunkyImage_Palette:
	    STORE = (IPTR)data->palette;
	    return TRUE;

	case MUIA_ChunkyImage_NumColors:
	    STORE = (IPTR)data->numcolors;
	    return TRUE;

	case MUIA_ChunkyImage_Modulo:
	    STORE = (IPTR)data->modulo;
	    return TRUE;

    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR ChunkyImage_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);

    if (!make_bitmap(cl, obj)) return FALSE;
    
    set(obj, MUIA_Bitmap_Bitmap, (IPTR)data->bm);
    
    if (data->palette32)
    {
    	set(obj, MUIA_Bitmap_SourceColors, (IPTR)data->palette32);
    }
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg))
    {
    	set(obj, MUIA_Bitmap_Bitmap, NULL);
	WaitBlit();
	FreeBitMap(data->bm);
	data->bm = NULL;
	if (data->palette32)
	{
	    set(obj, MUIA_Bitmap_SourceColors, NULL);
	    
	    FreeVec(data->palette32);
	    data->palette32 = NULL;
	}
	return FALSE;
    }
       
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR ChunkyImage_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);
    IPTR    	    	      retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    if (data->bm)
    {
    	set(obj, MUIA_Bitmap_Bitmap, NULL);
	
    	WaitBlit();
	
	FreeBitMap(data->bm);
	data->bm = NULL;
	
	if (data->palette32)
	{
	    set(obj, MUIA_Bitmap_SourceColors, NULL);
	    FreeVec(data->palette32);
	    data->palette32 = NULL;
	}
    }
    
    return retval;
}


BOOPSI_DISPATCHER(IPTR, ChunkyImage_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return ChunkyImage_New(cl, obj, (struct opSet *)msg);
	    
	case OM_DISPOSE:
	    return ChunkyImage_Dispose(cl, obj, msg);
	    
	case OM_SET:
	    return ChunkyImage_Set(cl, obj, (struct opSet *)msg);
	    
	case OM_GET:
	    return ChunkyImage_Get(cl, obj, (struct opGet *)msg);

	case MUIM_Setup:
	    return ChunkyImage_Setup(cl, obj, (struct MUIP_Setup *)msg);
	    
	case MUIM_Cleanup:
	    return ChunkyImage_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	    	       
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_ChunkyImage_desc = { 
    MUIC_ChunkyImage, 
    MUIC_Bitmap, 
    sizeof(struct MUI_ChunkyImageData), 
    (void*)ChunkyImage_Dispatcher 
};
