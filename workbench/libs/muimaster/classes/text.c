/* 
    Copyright  1999, David Le Corfec.
    Copyright  2002-2006, The AROS Development Team.
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
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "textengine.h"

extern struct Library *MUIMasterBase;

//#define MYDEBUG 1
#include "debug.h"


struct MUI_TextData
{
    ULONG mtd_Flags;
    STRPTR contents;
    CONST_STRPTR preparse;
    TEXT hichar;
    ZText *ztext;
    LONG xpixel;        /* needed for cursor up/down movements, can be -1 */
    struct MUI_EventHandlerNode ehn;

    LONG update;        /* type of update: 1 - everything,
                         * 2 - insert char, no scroll */
};

#define MTDF_SETMIN    (1<<0)
#define MTDF_SETMAX    (1<<1)
#define MTDF_SETVMAX   (1<<2)
#define MTDF_HICHAR    (1<<3)
#define MTDF_HICHARIDX (1<<4)


static const int __version = 1;
static const int __revision = 1;

static void setup_text(struct MUI_TextData *data, Object *obj);

/**************************************************************************
OM_NEW
**************************************************************************/
IPTR Text__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_TextData *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->mtd_Flags = MTDF_SETMIN | MTDF_SETVMAX;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Text_Contents:
            if (tag->ti_Data)
                data->contents = StrDup((STRPTR) tag->ti_Data);
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
            data->preparse = StrDup((STRPTR) tag->ti_Data);
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

    if (!data->preparse)
        data->preparse = StrDup("");
    if (!data->contents)
        data->contents = StrDup("");

    if (!data->contents || !data->preparse)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return (IPTR) NULL;
    }

/*      D(bug("Text_New(0x%lx)\n", obj)); */

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    data->xpixel = -1;

    return (IPTR) obj;
}

/**************************************************************************
OM_DISPOSE
**************************************************************************/
IPTR Text__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    FreeVec(data->contents);
    FreeVec((APTR) data->preparse);

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
OM_SET
**************************************************************************/
IPTR Text__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Text_Contents:
            {
                char *new_contents =
                    StrDup(((char *)tag->ti_Data) ? (char *)tag->
                    ti_Data : "");
                if (new_contents)
                {
                    if (data->ztext)
                    {
                        zune_text_destroy(data->ztext);
                        data->ztext = NULL;
                    }
                    FreeVec(data->contents);
                    data->contents = new_contents;
                    if (_flags(obj) & MADF_SETUP)
                        setup_text(data, obj);
                    MUI_Redraw(obj, MADF_DRAWOBJECT); /* should be optimized */
                }
            }
            break;

        case MUIA_Text_PreParse:
            {
                char *new_preparse =
                    StrDup(((char *)tag->ti_Data) ? (char *)tag->
                    ti_Data : "");
                if (new_preparse)
                {
                    if (data->ztext)
                    {
                        zune_text_destroy(data->ztext);
                        data->ztext = NULL;
                    }
                    FreeVec((APTR) data->preparse);
                    data->preparse = new_preparse;
                    if (_flags(obj) & MADF_SETUP)
                        setup_text(data, obj);
                    MUI_Redraw(obj, MADF_DRAWOBJECT); /* should be optimized */
                }
            }
            break;

        case MUIA_Selected:
            D(bug("Text_Set(%p) : MUIA_Selected val=%ld sss=%d\n", obj,
                    tag->ti_Data, ! !(_flags(obj) & MADF_SHOWSELSTATE)));
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/**************************************************************************
OM_GET
**************************************************************************/
IPTR Text__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

#define STORE *(msg->opg_Storage)
    switch (msg->opg_AttrID)
    {
    case MUIA_Text_Contents:
        STORE = (IPTR) data->contents;
        return TRUE;

    case MUIA_Text_PreParse:
        STORE = (IPTR) data->preparse;
        return TRUE;

    case MUIA_Version:
        STORE = __version;
        return TRUE;

    case MUIA_Revision:
        STORE = __revision;
        return TRUE;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
...
**************************************************************************/
static void setup_text(struct MUI_TextData *data, Object *obj)
{
    if (data->mtd_Flags & MTDF_HICHAR)
    {
        data->ztext = zune_text_new(data->preparse, data->contents,
            ZTEXT_ARG_HICHAR, data->hichar);
    }
    else if (data->mtd_Flags & MTDF_HICHARIDX)
    {
        data->ztext = zune_text_new(data->preparse, data->contents,
            ZTEXT_ARG_HICHARIDX, data->hichar);
    }
    else
    {
        data->ztext = zune_text_new(data->preparse, data->contents,
            ZTEXT_ARG_NONE, 0);
    }
    zune_text_get_bounds(data->ztext, obj);

/*      D(bug("muimaster.library/text.c: ZText of 0x%lx at 0x%lx\n", */
/*          obj, data->ztext)); */
}

/**************************************************************************
MUIM_Setup
**************************************************************************/
IPTR Text__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return FALSE;

    setup_text(data, obj);

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);
    return TRUE;
}

/**************************************************************************
MUIM_Cleanup
**************************************************************************/
IPTR Text__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);

    if (data->ztext)
    {
        zune_text_destroy(data->ztext);
        data->ztext = NULL;
    }

    return (DoSuperMethodA(cl, obj, (Msg) msg));
}

/**************************************************************************
MUIM_Show
**************************************************************************/
IPTR Text__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    //struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return FALSE;

    return TRUE;
}

/**************************************************************************
MUIM_Hide
**************************************************************************/
IPTR Text__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    //struct MUI_TextData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
MUIM_AskMinMax
**************************************************************************/
IPTR Text__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    int height;
    struct MUI_TextData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    height = data->ztext->height;
    if (_font(obj)->tf_YSize > height)
        height = _font(obj)->tf_YSize;
/*      D(bug("YSize=%ld\n", _font(obj)->tf_YSize)); */


    msg->MinMaxInfo->MinWidth += data->ztext->width;
    msg->MinMaxInfo->DefWidth += data->ztext->width;
    msg->MinMaxInfo->MaxWidth += data->ztext->width;

    msg->MinMaxInfo->MinHeight += height;
    msg->MinMaxInfo->DefHeight += height;

    if (!(data->mtd_Flags & MTDF_SETVMAX))
        msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
    else
        msg->MinMaxInfo->MaxHeight += height;

    if (!(data->mtd_Flags & MTDF_SETMAX))
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

    if (!(data->mtd_Flags & MTDF_SETMIN))
        msg->MinMaxInfo->MinWidth = 0;

    D(bug
        ("Text_AskMinMax 0x%lx (%s): Min=%ldx%ld Max=%ldx%ld Def=%ldx%ld\n",
            obj, data->contents, msg->MinMaxInfo->MinWidth,
            msg->MinMaxInfo->MinHeight, msg->MinMaxInfo->MaxWidth,
            msg->MinMaxInfo->MaxHeight, msg->MinMaxInfo->DefWidth,
            msg->MinMaxInfo->DefHeight));

    return TRUE;
}

/**************************************************************************
MUIM_Draw
**************************************************************************/
IPTR Text__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);
    Object *act = NULL;
    APTR clip;

/*    D(bug("muimaster.library/text.c: Draw Text Object at " */
/*        "0x%lx %ldx%ldx%ldx%ld\n", obj, _left(obj), _top(obj), */
/*        _right(obj), _bottom(obj))); */

    DoSuperMethodA(cl, obj, (Msg) msg);

    if ((msg->flags & MADF_DRAWUPDATE) && !data->update)
        return 0;

    if (!(msg->flags & MADF_DRAWUPDATE))
        data->update = 0;

    if (msg->flags & MADF_DRAWUPDATE && data->update == 1)
    {
        DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj),
            _mwidth(obj), _mheight(obj), _mleft(obj), _mtop(obj), 0);
    }

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
        _mwidth(obj), _mheight(obj));

    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);

    {
        get(_win(obj), MUIA_Window_ActiveObject, &act);
        {
            int y = (_mheight(obj) - data->ztext->height) / 2;
            zune_text_draw(data->ztext, obj,
                _mleft(obj), _mright(obj), _mtop(obj) + y);
        }
    }

    MUI_RemoveClipping(muiRenderInfo(obj), clip);
    data->update = 0;
    return TRUE;
}

/**************************************************************************
MUIM_Export : to export an object's "contents" to a dataspace object.
**************************************************************************/
IPTR Text__MUIM_Export(struct IClass *cl, Object *obj,
    struct MUIP_Export *msg)
{
    //struct MUI_TextData *data = INST_DATA(cl, obj);
    //STRPTR id;

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
MUIM_Import : to import an object's "contents" from a dataspace object.
**************************************************************************/
IPTR Text__MUIM_Import(struct IClass *cl, Object *obj,
    struct MUIP_Import *msg)
{
    //STRPTR id;
    //STRPTR s;

#if 0
    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        if ((s = (STRPTR) DoMethod(msg->dataspace,
                    MUIM_Dataspace_FindString, _U(id), _U("contents"))))
        {
            set(obj, MUIA_Text_Contents, _U(s));
        }
    }
#endif
    return 0;
}


BOOPSI_DISPATCHER(IPTR, Text_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Text__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Text__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Text__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Text__OM_GET(cl, obj, (struct opGet *)msg);

    case MUIM_AskMinMax:
        return Text__MUIM_AskMinMax(cl, obj, (APTR) msg);
    case MUIM_Draw:
        return Text__MUIM_Draw(cl, obj, (APTR) msg);
    case MUIM_Setup:
        return Text__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Text__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_Show:
        return Text__MUIM_Show(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Text__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_Export:
        return Text__MUIM_Export(cl, obj, (APTR) msg);
    case MUIM_Import:
        return Text__MUIM_Import(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
* Class descriptor.
*/
const struct __MUIBuiltinClass _MUI_Text_desc =
{
    MUIC_Text,
    MUIC_Area,
    sizeof(struct MUI_TextData),
    (void *) Text_Dispatcher
};
