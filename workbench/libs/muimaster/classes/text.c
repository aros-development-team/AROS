/* Zune -- a free Magic User Interface implementation
 * Copyright (C) 1999 David Le Corfec
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"

#define MYDEBUG 1
#include "debug.h"

#ifdef _AROS
#define g_strdup(x) 	    	    	    	    \
    ({	    	    	    	    	    	    \
    	UBYTE *dup; 	    	    	    	    \
	    	    	    	    	    	    \
	dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC); \
	if (dup) CopyMem((x), dup, strlen(x) + 1);  \
	dup; 	    	    	    	    	    \
    })	
#define g_free FreeVec
#else
#define g_strdup(x) strdup(x)
#define g_free(x) free(x);
#endif

extern struct Library *MUIMasterBase;

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

    data->contents = g_strdup("");
    data->preparse = g_strdup("");

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Text_Contents:
		if (tag->ti_Data)
		{
		    g_free(data->contents);
		    data->contents = g_strdup((STRPTR)tag->ti_Data);
		}
		break;
	    case MUIA_Text_HiChar:
		data->hichar = tag->ti_Data;
		_handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_HICHAR);
		break;
	    case MUIA_Text_HiCharIdx:
		data->hichar = tag->ti_Data;
		_handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_HICHARIDX);
		break;
	    case MUIA_Text_PreParse:
		if (tag->ti_Data)
		{
		    g_free(data->preparse);
		    data->preparse = g_strdup((STRPTR)tag->ti_Data);
		}
		break;
	    case MUIA_Text_SetMin:
		_handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETMIN);
		break;
	    case MUIA_Text_SetMax:
		_handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETMAX);
		break;
	    case MUIA_Text_SetVMax:
		_handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_SETVMAX);
		break;
	}
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

    g_free(data->contents);
    g_free(data->preparse);

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

    while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Text_Contents:
		if (data->ztext)
		    zune_text_destroy(data->ztext);
		if (data->contents)
		    g_free(data->contents);
		data->contents = g_strdup((STRPTR)tag->ti_Data);
		if (_flags(obj) & MADF_SETUP)
		    setup_text(data, obj);
		break;
	    case MUIA_Text_PreParse:
		if (data->ztext)
		    zune_text_destroy(data->ztext);
		if (data->preparse)
		    g_free(data->preparse);
		data->preparse = g_strdup((STRPTR)tag->ti_Data);
		if (_flags(obj) & MADF_SETUP)
		    setup_text(data, obj);
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
	case MUIA_Text_Contents:
	    STORE = (ULONG)data->contents;
	    return(TRUE);
	case MUIA_Text_PreParse:
	    STORE = (ULONG)data->preparse;
	    return(TRUE);

	case MUIA_Version:
	    STORE = __version;
	    return(TRUE);
	case MUIA_Revision:
	    STORE = __revision;
	    return(TRUE);
    }
    return(DoSuperMethodA(cl, obj, (Msg) msg));
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
//	STRPTR s;
	data->ztext = zune_text_new(data->preparse, data->contents,
				    ZTEXT_ARG_HICHARIDX, data->hichar);
//	s = strchr(data->preparse, data->hichar);
//	if (s == NULL)
//	    s = strchr(data->contents, data->hichar);
//	if (s && s[1])
//	{
//	    set(obj, MUIA_ControlChar, s[1]);
//	}
    }
    else
	data->ztext = zune_text_new(data->preparse, data->contents,
				    ZTEXT_ARG_NONE, 0);

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
	zune_text_destroy(data->ztext);

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}

/**************************************************************************
 MUIM_Show
**************************************************************************/
static ULONG Text_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    return TRUE;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static ULONG Text_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Text_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

/*      g_print("%d %d\n", _font(obj)->ascent, _font(obj)->descent); */

    zune_text_get_bounds(data->ztext, obj);

    msg->MinMaxInfo->MinWidth += data->ztext->width;
    msg->MinMaxInfo->DefWidth += data->ztext->width;
    msg->MinMaxInfo->MaxWidth += data->ztext->width;

    msg->MinMaxInfo->MinHeight += data->ztext->height;
    msg->MinMaxInfo->DefHeight += data->ztext->height;
    msg->MinMaxInfo->MaxHeight += data->ztext->height;

    if (!(data->mtd_Flags & MTDF_SETVMAX))
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    if (!(data->mtd_Flags & MTDF_SETMAX))
	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

    if (!(data->mtd_Flags & MTDF_SETMIN))
	msg->MinMaxInfo->MinWidth = 0;

    msg->MinMaxInfo->DefWidth = 0;

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
    APTR clip;

    D(bug("muimaster.library/text.c: Draw Text Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

    DoSuperMethodA(cl,obj,(Msg)msg);
    if (!(msg->flags & MADF_DRAWOBJECT))
	return(0);

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
			   _mwidth(obj), _mheight(obj));
    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);

    zune_text_draw(data->ztext, obj,
		   _mleft(obj), _mright(obj),
		   _mtop(obj) + (_mheight(obj) - data->ztext->height) / 2);

/*      zune_text_draw(data->ztext, obj, */
/*  		   _mleft(obj), */
/*  		   _mtop(obj) + (_mheight(obj) - data->ztext->height) / 2); */

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

#if 0
    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	DoMethod(msg->dataspace, MUIM_Dataspace_AddString,
		 _U(id), _U("contents"), _U(data->contents));
    }
#endif
    return 0;
}


/**************************************************************************
 MUIM_Import : to import an objects "contents" from a dataspace object.
**************************************************************************/
static ULONG Text_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    STRPTR id;
    STRPTR s;

#if 0
    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	if ((s = (STRPTR)DoMethod(msg->dataspace, MUIM_Dataspace_FindString,
				  _U(id), _U("contents"))))
	{
	    set(obj, MUIA_Text_Contents, _U(s));
	}
    }
#endif
    return 0;
}


#ifndef _AROS
__asm IPTR Text_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Text_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
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
	case MUIM_Show: return Text_Show(cl, obj, (APTR)msg);
	case MUIM_Hide: return Text_Hide(cl, obj, (APTR)msg);
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
    Text_Dispatcher
};
