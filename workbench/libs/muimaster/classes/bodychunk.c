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

struct MUI_BodychunkData
{
    struct BitMap   *bm;
    UBYTE   	    *body;
    LONG    	     width, height, depth;
    UBYTE   	     compression;
    UBYTE   	     masking;
};

static void planar2chunky(UBYTE *src, UBYTE *dest, WORD width, WORD depth, LONG bpr)
{
    UBYTE *s, *d, byte;
    UBYTE pmask, dmask = 1, notdmask = ~1;
    WORD  x, pl;
   
    for(pl = 0; pl < depth; pl++)
    {
    	pmask = 0x80;
	s = src;
	d = dest;
	
	byte = *s++;
	
	for(x = 0; x < width; x++)
	{
	    if (byte & pmask)
	    {
	    	*d++ |= dmask;
	    }
	    else
	    {
	    	*d++ &= notdmask;
	    }
	    
    	    if (pmask == 0x1)
	    {
	    	pmask = 0x80;
		byte = *s++;
	    }
	    else
	    {
	    	pmask >>= 1;
	    }
	}
	
	dmask <<= 1; notdmask = ~dmask;
	
	src += bpr;
    }
}

static UBYTE *unpack_byterun1(UBYTE *src, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE  c;
    
    for(;;)
    {
	c = (BYTE)(*src++);
	if (c >= 0)
	{
    	    while(c-- >= 0)
	    {
		*dest++ = *src++;
		if (--unpackedsize <= 0) return src;
	    }
	}
	else if (c != -128)
	{
    	    c = -c;
	    r = *src++;

	    while(c-- >= 0)
	    {
		*dest++ = r;
		if (--unpackedsize <= 0) return src;
	    }
	}
    }
        
}

static BOOL make_bitmap(struct IClass *cl, Object *obj)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);
    struct RastPort 	     bmrp, temprp;
    UBYTE   	    	     *chunkyline;
    UBYTE   	    	     *bodyptr, *unpackptr;
    LONG    	    	     w16, bpr, bpl;
    WORD    	    	     x, y, totdepth;
    
    if (!data->body 	    	||
    	(data->width  < 1)  	||
    	(data->height < 1)  	||
	(data->depth  < 1)  	||
	(data->depth  > 8)  	||
	(data->compression > 1) )
    {
    	return FALSE;
    }
    
    chunkyline = AllocVec(2 * (data->width + 16), MEMF_PUBLIC | MEMF_CLEAR);
    if (!chunkyline) return FALSE;
    
    unpackptr = chunkyline + data->width + 16;
    
    data->bm = AllocBitMap(data->width, data->height, data->depth, BMF_CLEAR, NULL);
    if (!data->bm)
    {
    	FreeVec(chunkyline);
    	return FALSE;
    }
    
    InitRastPort(&temprp);
    temprp.BitMap = AllocBitMap(data->width, 1, 1, 0, NULL);
    
    InitRastPort(&bmrp);
    bmrp.BitMap = data->bm;
    
    bodyptr = data->body;
    w16 = (data->width + 15) & ~15;
    bpr = w16 / 8;
    
    totdepth = data->depth + ((data->masking == 1) ? 1 : 0);
    bpl = bpr * totdepth;
    
    for(y = 0; y < data->height; y++)
    {
    	if (data->compression)
	{
	    bodyptr = unpack_byterun1(bodyptr, unpackptr, bpl);
	    planar2chunky(unpackptr, chunkyline, data->width, data->depth, bpr);
	}
	else
	{
	    planar2chunky(bodyptr, chunkyline, data->width, data->depth, bpr);
	    bodyptr += bpl;
	}
	
	if (temprp.BitMap)
	{
	    WritePixelLine8(&bmrp, 0, y, data->width, chunkyline, &temprp);
	}
	else
	{
	    for(x = 0; x < data->width; x++)
	    {
	    	SetAPen(&bmrp, chunkyline[x]);
		WritePixel(&bmrp, x, y);
	    }
	}
    }
    
    DeinitRastPort(&temprp);
    DeinitRastPort(&bmrp);
    
    if (temprp.BitMap)
    {
    	WaitBlit();
	FreeBitMap(temprp.BitMap);
    }
    
    FreeVec(chunkyline);
    
    return TRUE;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Bodychunk_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BodychunkData    *data;
    struct TagItem  	    	*tag, *tags;
    
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;
    
    data = INST_DATA(cl, obj);
	
    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Bodychunk_Body:
	    	data->body = (UBYTE *)tag->ti_Data;
		break;
		
	    case MUIA_Bodychunk_Compression:
	    	data->compression = (UBYTE)tag->ti_Data;
		break;
		
	    case MUIA_Bodychunk_Depth:
	    	data->depth = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bodychunk_Masking:
	    	data->masking = (UBYTE)tag->ti_Data;
		break;
		
	    case MUIA_Bitmap_Width:
	    	data->width = (LONG)tag->ti_Data;
		break;

	    case MUIA_Bitmap_Height:
	    	data->height = (LONG)tag->ti_Data;
		break;
		
    	}
    }
    
    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Bodychunk_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);

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
static IPTR Bodychunk_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BodychunkData    *data  = INST_DATA(cl, obj);
    struct TagItem          	*tags  = msg->ops_AttrList;
    struct TagItem          	*tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Bodychunk_Body:
	    	data->body = (UBYTE *)tag->ti_Data;
		break;
		
	    case MUIA_Bodychunk_Compression:
	    	data->compression = (UBYTE)tag->ti_Data;
		break;
		
	    case MUIA_Bodychunk_Depth:
	    	data->depth = (LONG)tag->ti_Data;
		break;
		
	    case MUIA_Bodychunk_Masking:
	    	data->masking = (UBYTE)tag->ti_Data;
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
static IPTR Bodychunk_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_BodychunkData *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
	case MUIA_Bodychunk_Body:
	    STORE = (IPTR)data->body;
	    return TRUE;

	case MUIA_Bodychunk_Compression:
	    STORE = (IPTR)data->compression;
	    return TRUE;

	case MUIA_Bodychunk_Depth:
	    STORE = (IPTR)data->depth;
	    return TRUE;

	case MUIA_Bodychunk_Masking:
	    STORE = (IPTR)data->masking;
	    return TRUE;

    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static IPTR Bodychunk_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);

    if (!make_bitmap(cl, obj)) return FALSE;
    
    set(obj, MUIA_Bitmap_Bitmap, (IPTR)data->bm);
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg))
    {
    	set(obj, MUIA_Bitmap_Bitmap, NULL);
	WaitBlit();
	FreeBitMap(data->bm);
	data->bm = NULL;
	
	return FALSE;
    }
       
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static IPTR Bodychunk_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);
    IPTR    	    	      retval;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    if (data->bm)
    {
    	set(obj, MUIA_Bitmap_Bitmap, NULL);
	
    	WaitBlit();
	
	FreeBitMap(data->bm);
	data->bm = NULL;	
    }
    
    return retval;
}


BOOPSI_DISPATCHER(IPTR, Bodychunk_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return Bodychunk_New(cl, obj, (struct opSet *)msg);
	    
	case OM_DISPOSE:
	    return Bodychunk_Dispose(cl, obj, msg);
	    
	case OM_SET:
	    return Bodychunk_Set(cl, obj, (struct opSet *)msg);
	    
	case OM_GET:
	    return Bodychunk_Get(cl, obj, (struct opGet *)msg);

	case MUIM_Setup:
	    return Bodychunk_Setup(cl, obj, (struct MUIP_Setup *)msg);
	    
	case MUIM_Cleanup:
	    return Bodychunk_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
	    	       
    }
    
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Bodychunk_desc = { 
    MUIC_Bodychunk, 
    MUIC_Bitmap, 
    sizeof(struct MUI_BodychunkData), 
    (void*)Bodychunk_Dispatcher 
};
