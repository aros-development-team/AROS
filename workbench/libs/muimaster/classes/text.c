/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/dos.h>
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

/*  #define MYDEBUG 1 */
#include "debug.h"


struct MUI_TextData {
    ULONG  mtd_Flags;
    STRPTR contents;
    CONST_STRPTR preparse;
    TEXT   hichar;
    ZText *ztext;
};

#define MTDF_SETMIN    (1<<0)
#define MTDF_SETMAX    (1<<1)
#define MTDF_SETVMAX   (1<<2)
#define MTDF_HICHAR    (1<<3)
#define MTDF_HICHARIDX (1<<4)

static const int __version = 1;
static const int __revision = 1;

static void setup_text (struct MUI_TextData *data, Object *obj);

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Text_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_TextData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);
    data->mtd_Flags = MTDF_SETMIN | MTDF_SETVMAX;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Text_Contents:
		    if (tag->ti_Data) data->contents = StrDup((STRPTR)tag->ti_Data);
		    break;

	    case    MUIA_Text_HiChar:
		    data->hichar = tag->ti_Data;
		    _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_HICHAR);
		    break;

	    case    MUIA_Text_HiCharIdx:
		    data->hichar = tag->ti_Data;
		    _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_HICHARIDX);
		    break;

	    case    MUIA_Text_PreParse:
		    data->preparse = StrDup((STRPTR)tag->ti_Data);
		    break;

	    case    MUIA_Text_SetMin:
		    _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETMIN);
		    break;

	    case    MUIA_Text_SetMax:
		    _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETMAX);
		    break;

	    case    MUIA_Text_SetVMax:
		    _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETVMAX);
		    break;
	}
    }

    if (!data->preparse) data->preparse = StrDup("");
    if (!data->contents) data->contents = StrDup("");

    if (!data->contents || !data->preparse)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    D(bug("muimaster.library/text.c: Text Object created at 0x%lx\n",obj));

    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Text_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (data->contents) FreeVec(data->contents);
    if (data->preparse) FreeVec((APTR)data->preparse);

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG Text_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    struct TagItem      *tags = msg->ops_AttrList;
    struct TagItem      *tag;

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Text_Contents:
		    {
		    	char *new_contents = StrDup(((char*)tag->ti_Data)?(char*)tag->ti_Data:"");
		    	if (new_contents)
		    	{
			    if (data->ztext)
			    {
				zune_text_destroy(data->ztext);
				data->ztext = NULL;
			    }
			    if (data->contents) FreeVec(data->contents);
			    data->contents = new_contents;
			    if (_flags(obj) & MADF_SETUP) setup_text(data, obj);
			    MUI_Redraw(obj,MADF_DRAWOBJECT);
			}
		    }
		    break;

	    case    MUIA_Text_PreParse:
		    {
		    	char *new_preparse = StrDup(((char*)tag->ti_Data)?(char*)tag->ti_Data:"");
		    	if (new_preparse)
		    	{
			    if (data->ztext)
			    {
				zune_text_destroy(data->ztext);
				data->ztext = NULL;
			    }
			    if (data->preparse) FreeVec((APTR)data->preparse);
			    data->preparse = new_preparse;
			    if (_flags(obj) & MADF_SETUP) setup_text(data, obj);
			    MUI_Redraw(obj,MADF_DRAWOBJECT);
			}
		    }
		    break;
	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG Text_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

#define STORE *(msg->opg_Storage)
    switch(msg->opg_AttrID)
    {
	case	MUIA_Text_Contents:
		STORE = (ULONG)data->contents;
		return 1;

	case	MUIA_Text_PreParse:
		STORE = (ULONG)data->preparse;
		return 1;

	case	MUIA_Version:
		STORE = __version;
		return 1;

	case	MUIA_Revision:
		STORE = __revision;
		return 1;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
 ...
**************************************************************************/
static void setup_text (struct MUI_TextData *data, Object *obj)
{
    if (data->mtd_Flags & MTDF_HICHAR)
	data->ztext = zune_text_new(data->preparse, data->contents,
				    ZTEXT_ARG_HICHAR, data->hichar);
    else if (data->mtd_Flags & MTDF_HICHARIDX)
    {
	data->ztext = zune_text_new(data->preparse, data->contents,
				    ZTEXT_ARG_HICHARIDX, data->hichar);
    }
    else
	data->ztext = zune_text_new(data->preparse, data->contents,
				    ZTEXT_ARG_NONE, 0);

    zune_text_get_bounds(data->ztext, obj);

    D(bug("muimaster.library/text.c: ZText of 0x%lx at 0x%lx\n",obj,data->ztext));
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG Text_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    setup_text(data, obj);
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Text_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (data->ztext)
    {
	zune_text_destroy(data->ztext);
	data->ztext = NULL;
    }

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Text_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    int height;
    struct MUI_TextData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    height = data->ztext->height;
    if (_font(obj)->tf_YSize > height) height = _font(obj)->tf_YSize;

    msg->MinMaxInfo->MinWidth += data->ztext->width;
    msg->MinMaxInfo->DefWidth += data->ztext->width;
    msg->MinMaxInfo->MaxWidth += data->ztext->width;

    msg->MinMaxInfo->MinHeight += height;
    msg->MinMaxInfo->DefHeight += height;
    msg->MinMaxInfo->MaxHeight += height;

    if (!(data->mtd_Flags & MTDF_SETVMAX))
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    if (!(data->mtd_Flags & MTDF_SETMAX))
	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

    if (!(data->mtd_Flags & MTDF_SETMIN))
	msg->MinMaxInfo->MinWidth = 0;

    D(bug("muimaster.library/text.c: Text_AskMinMax 0x%lx: Min=%ldx%ld Max=%ldx%ld Def=%ldx%ld\n", obj,
	msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight,
	msg->MinMaxInfo->MaxWidth, msg->MinMaxInfo->MaxHeight,
	msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->DefHeight));

    return TRUE;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Text_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    Object *act;
    APTR clip;
    int y;

    D(bug("muimaster.library/text.c: Draw Text Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
	return 0;

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
			   _mwidth(obj), _mheight(obj));

    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);

    y = (_mheight(obj) - data->ztext->height) / 2;

    zune_text_draw(data->ztext, obj,
		   _mleft(obj), _mright(obj),
		   _mtop(obj) + y);

    MUI_RemoveClipping(muiRenderInfo(obj), clip);
    return TRUE;
}

/**************************************************************************
 MUIM_Export : to export an objects "contents" to a dataspace object.
**************************************************************************/
static ULONG Text_Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    STRPTR id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	DoMethod(msg->dataspace, MUIM_Dataspace_Add,
		 data->contents, strlen(data->contents) + 1, id);
    }
    return 0;
}


/**************************************************************************
 MUIM_Import : to import an objects "contents" from a dataspace object.
**************************************************************************/
static ULONG Text_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    STRPTR id;
    STRPTR s;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	if ((s = (STRPTR)DoMethod(msg->dataspace, MUIM_Dataspace_Find, id)))
	{
	    set(obj, MUIA_Text_Contents, s);
	}
    }
    return 0;
}

BOOPSI_DISPATCHER(IPTR, Text_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Text_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Text_Dispose(cl, obj, msg);
	case OM_SET: return Text_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Text_Get(cl, obj, (struct opGet *)msg);
	case MUIM_AskMinMax: return Text_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw: return Text_Draw(cl, obj, (APTR)msg);
	case MUIM_Setup: return Text_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Text_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Export: return Text_Export(cl, obj, (APTR)msg);
	case MUIM_Import: return Text_Import(cl, obj, (APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Text_desc = { 
    MUIC_Text,
    MUIC_Area, 
    sizeof(struct MUI_TextData), 
    (void*)Text_Dispatcher
};
