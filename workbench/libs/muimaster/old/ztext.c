/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef __AROS__
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#endif

#include <zunepriv.h>
#include <builtin.h>
#include <string.h>
#include <Text.h>
#include <textdata.h>
#include <Notify.h>
#include <Area.h>
#include <pen.h>
#include <renderinfo.h>
#include <Dataspace.h>

static const int __version = 1;
static const int __revision = 1;

static void setup_text (struct MUI_TextData *data, Object *obj);

static ULONG
mNew(struct IClass *cl, Object *obj, struct opSet *msg)
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

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
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

    return (ULONG)obj;
}


static ULONG
mDispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    g_free(data->contents);
    g_free(data->preparse);

    return DoSuperMethodA(cl, obj, msg);
}


static ULONG
mSet(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    struct TagItem      *tags = msg->ops_AttrList;
    struct TagItem      *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
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


static ULONG
mGet(struct IClass *cl, Object *obj, struct opGet *msg)
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

static void
setup_text (struct MUI_TextData *data, Object *obj)
{
    if (data->mtd_Flags & MTDF_HICHAR)
	data->ztext = zune_text_new(data->preparse, data->contents,
				    ZTEXT_ARG_HICHAR, data->hichar);
    else if (data->mtd_Flags & MTDF_HICHARIDX)
    {
	STRPTR s;
	data->ztext = zune_text_new(data->preparse, data->contents,
				    ZTEXT_ARG_HICHARIDX, data->hichar);
	s = strchr(data->preparse, data->hichar);
	if (s == NULL)
	    s = strchr(data->contents, data->hichar);
	if (s && s[1])
	{
	    set(obj, MUIA_ControlChar, s[1]);
	}
    }
    else
	data->ztext = zune_text_new(data->preparse, data->contents,
				    ZTEXT_ARG_NONE, 0);
}

static ULONG
mSetup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    setup_text(data, obj);
    return TRUE;
}


static ULONG
mCleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (data->ztext)
	zune_text_destroy(data->ztext);

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}


static ULONG
mShow(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    return TRUE;
}


static ULONG
mHide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}


static ULONG
mAskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
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

g_print("Text_AskMinMax %p: Min=%dx%d Max=%dx%d Def=%dx%d\n", obj,
	msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight,
	msg->MinMaxInfo->MaxWidth, msg->MinMaxInfo->MaxHeight,
	msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->DefHeight);

    return TRUE;
}


static ULONG 
mDraw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    APTR clip;

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

/*
 * MUIM_Export : to export an objects "contents" to a dataspace object.
 */
static ULONG
mExport(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    STRPTR id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	DoMethod(msg->dataspace, MUIM_Dataspace_AddString,
		 _U(id), _U("contents"), _U(data->contents));
    }
    return 0;
}


/*
 * MUIM_Import : to import an objects "contents" from a dataspace object.
 */
static ULONG
mImport(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    STRPTR id;
    STRPTR s;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	if ((s = (STRPTR)DoMethod(msg->dataspace, MUIM_Dataspace_FindString,
				  _U(id), _U("contents"))))
	{
	    set(obj, MUIA_Text_Contents, _U(s));
	}
    }
    return 0;
}


AROS_UFH3S(IPTR, Text_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    switch (msg->MethodID)
    {
	case OM_NEW:
	    return(mNew(cl, obj, (struct opSet *) msg));
	case OM_DISPOSE:
	    return(mDispose(cl, obj, msg));
	case OM_SET:
	    return(mSet(cl, obj, (struct opSet *)msg));
	case OM_GET:
	    return(mGet(cl, obj, (struct opGet *)msg));
	case MUIM_AskMinMax :
	    return(mAskMinMax(cl, obj, (APTR)msg));
	case MUIM_Draw :
	    return(mDraw(cl, obj, (APTR)msg));
	case MUIM_Setup :
	    return(mSetup(cl, obj, (APTR)msg));
	case MUIM_Cleanup :
	    return(mCleanup(cl, obj, (APTR)msg));
	case MUIM_Show :
	    return(mShow(cl, obj, (APTR)msg));
	case MUIM_Hide :
	    return(mHide(cl, obj, (APTR)msg));
	case MUIM_Export :
	    return(mExport(cl, obj, (APTR)msg));
	case MUIM_Import :
	    return(mImport(cl, obj, (APTR)msg));	
    }

    return(DoSuperMethodA(cl, obj, msg));
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
