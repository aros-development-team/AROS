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
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "textengine.h"

extern struct Library *MUIMasterBase;

//#define MYDEBUG 1
#include "debug.h"


struct MUI_TextData {
    ULONG  mtd_Flags;
    STRPTR contents;
    CONST_STRPTR preparse;
    CONST_STRPTR accept; /* MUIA_String_Accept */
    TEXT   hichar;
    ZText *ztext;
    LONG xpixel; /* needed for cursor up/down movements, can be -1 */
    LONG xpos;
    LONG ypos;
    struct MUI_EventHandlerNode ehn;

    LONG update; /* type of update 1 - everything, 2 - insert char, no scroll */
    LONG update_arg1;
    LONG update_arg2;
};

#define MTDF_SETMIN    (1<<0)
#define MTDF_SETMAX    (1<<1)
#define MTDF_SETVMAX   (1<<2)
#define MTDF_HICHAR    (1<<3)
#define MTDF_HICHARIDX (1<<4)
#if 0
#define MTDF_EDITABLE  (1<<5)
#define MTDF_MULTILINE (1<<6)
#endif
#define MTDF_ADVANCEONCR (1<<7)

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
	    case    MUIA_String_Contents:
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

#if 0
	    case    MUIA_Text_Editable:
		    _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_EDITABLE);
		    break;

	    case    MUIA_Text_Multiline:
		    _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_MULTILINE);
		    break;

            case    MUIA_String_AdvanceOnCR:
		    _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_ADVANCEONCR);
		    break;

	    case    MUIA_String_Accept:
		    data->accept = (CONST_STRPTR)tag->ti_Data;
		    break;

	    case    MUIA_String_Integer:
		    set(obj,MUIA_String_Integer,tag->ti_Data);
		    break;
#endif
	}
    }

    if (!data->preparse) data->preparse = StrDup("");
    if (!data->contents) data->contents = StrDup("");

    if (!data->contents || !data->preparse)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

/*      D(bug("Text_New(0x%lx)\n", obj)); */

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

    data->xpixel = -1;

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
	    case    MUIA_String_Contents:
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
			    MUI_Redraw(obj,MADF_DRAWOBJECT); /* should be opimized */
			}
		    }
		    break;

	    case    MUIA_String_Accept:
		    data->accept = (CONST_STRPTR)tag->ti_Data;
		    break;

	    case    MUIA_String_Integer:
		    {
			char buf[20];
			sprintf(buf,"%ld",tag->ti_Data);
			set(obj, MUIA_String_Contents, (IPTR) buf);
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
			    MUI_Redraw(obj,MADF_DRAWOBJECT); /* should be opimized */
			}
		    }
		    break;
	    case MUIA_Selected:
		D(bug("Text_Set(%p) : MUIA_Selected val=%ld sss=%d\n", obj, tag->ti_Data, !!(_flags(obj) & MADF_SHOWSELSTATE)));
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
#if 0
	case	MUIA_String_Contents:

		if (data->mtd_Flags & MTDF_EDITABLE && data->ztext)
		{
		    /* Convert the ztext to plain chars */
		    char *new_cont = zune_text_iso_string(data->ztext);
		    if (new_cont)
		    {
		    	if (data->contents) FreeVec(data->contents);
		    	data->contents = new_cont;
		    }
		}
#endif
		STORE = (ULONG)data->contents;
		return 1;

#if 0
	case    MUIA_String_Integer:
		{
		    /* This actually is slower then necessary, a integer gadget should contain no
                     * style infos and no newline, so zune_text_iso_string is not needed - but it's
                     * simpler so */
		    STRPTR buf = NULL;
		    get(obj,MUIA_String_Contents, &buf);
		    if (buf)
		    {
		    	LONG val;
		    	StrToLong(buf,&val);
		    	STORE = val;
		    	return 1;
		    }
		}
		return 0;

	case	MUIA_String_Accept:
		STORE = (ULONG)data->accept;
		return 1;
#endif

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

/*      D(bug("muimaster.library/text.c: ZText of 0x%lx at 0x%lx\n",obj,data->ztext)); */
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

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Text_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

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
static ULONG Text_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    //struct MUI_TextData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    return TRUE;
}

/**************************************************************************
 MUIM_Hide
**************************************************************************/
static ULONG Text_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    //struct MUI_TextData *data = INST_DATA(cl, obj);
    return DoSuperMethodA(cl, obj, (Msg) msg);
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
/*      D(bug("YSize=%ld\n", _font(obj)->tf_YSize)); */

#if 0
    if (!(data->mtd_Flags & MTDF_EDITABLE))
#endif
    { 
	msg->MinMaxInfo->MinWidth += data->ztext->width;
	msg->MinMaxInfo->DefWidth += data->ztext->width;
	msg->MinMaxInfo->MaxWidth += data->ztext->width;
    }
#if 0
    else
    {
	msg->MinMaxInfo->MinWidth += _font(obj)->tf_XSize*4;
	msg->MinMaxInfo->DefWidth += _font(obj)->tf_XSize*12;
	msg->MinMaxInfo->MaxWidth += MUI_MAXMAX;
    }
#endif

#if 0
    if (!(data->mtd_Flags & MTDF_MULTILINE))
#endif
    {
	msg->MinMaxInfo->MinHeight += height;
	msg->MinMaxInfo->DefHeight += height;
	if (!(data->mtd_Flags & MTDF_SETVMAX))
	    msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
	else
	    msg->MinMaxInfo->MaxHeight += height;
    }
#if 0
    else
    {
	msg->MinMaxInfo->MinHeight += _font(obj)->tf_YSize;
	msg->MinMaxInfo->DefHeight += _font(obj)->tf_YSize*10;
	msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
    }
#endif

    if (!(data->mtd_Flags & MTDF_SETMAX))
	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

    if (!(data->mtd_Flags & MTDF_SETMIN))
	msg->MinMaxInfo->MinWidth = 0;

    D(bug("Text_AskMinMax 0x%lx (%s): Min=%ldx%ld Max=%ldx%ld Def=%ldx%ld\n", obj, data->contents,
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

 /*     D(bug("muimaster.library/text.c: Draw Text Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj))); */

    DoSuperMethodA(cl,obj,(Msg)msg);

    if ((msg->flags & MADF_DRAWUPDATE) && !data->update)
	return 0;

    if (!(msg->flags & MADF_DRAWUPDATE)) data->update = 0;

    if (msg->flags & MADF_DRAWUPDATE && data->update == 1)
    {
	DoMethod(obj,MUIM_DrawBackground, _mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj), _mleft(obj), _mtop(obj), 0);
    }

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
			   _mwidth(obj), _mheight(obj));

    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);

#if 0
    if (data->update == 2)
    {
    	/* Note, scrolling won't work, if there would be a background, different then a plain pen */
        ScrollRaster(_rp(obj), -data->update_arg2, 0, data->update_arg1 + _mleft(obj), _mtop(obj), _mright(obj),_mtop(obj) + _font(obj)->tf_YSize);

	zune_text_draw_single(data->ztext, obj,
		   _mleft(obj), _mright(obj),
		   _mtop(obj) + (_mheight(obj) - data->ztext->height) / 2,
		   data->xpos - 1,data->ypos, FALSE);
    } else
#endif
    {
        get(_win(obj),MUIA_Window_ActiveObject,&act);

#if 0
        if (act == obj && (data->mtd_Flags & MTDF_EDITABLE))
        {
            int y;
            if (data->mtd_Flags & MTDF_MULTILINE)
            {
		y = 0;
            } else
            {
		y = (_mheight(obj) - data->ztext->height) / 2;
            }

	    zune_text_draw_cursor(data->ztext, obj,
		   _mleft(obj), _mright(obj),
		   _mtop(obj) + y,
		   data->xpos,data->ypos);
        } else
#endif
        {
            int y;
#if 0
            if (data->mtd_Flags & MTDF_MULTILINE)
            {
		y = 0;
            } else
#endif
            {
		y = (_mheight(obj) - data->ztext->height) / 2;
            }

	    zune_text_draw(data->ztext, obj,
		   _mleft(obj), _mright(obj),
		   _mtop(obj) + y);
	}
    }

    MUI_RemoveClipping(muiRenderInfo(obj), clip);
    data->update = 0;
    return TRUE;
}

/**************************************************************************
 MUIM_Export : to export an objects "contents" to a dataspace object.
**************************************************************************/
static ULONG Text_Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
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
 MUIM_Import : to import an objects "contents" from a dataspace object.
**************************************************************************/
static ULONG Text_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    //STRPTR id;
    //STRPTR s;

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

#if 0
/**************************************************************************
 MUIM_GoActive
**************************************************************************/
static ULONG Text_GoActive(struct IClass * cl, Object * o, Msg msg)
{
  struct MUI_TextData *data = (struct MUI_TextData*) INST_DATA(cl, o);
  if (!(data->mtd_Flags & MTDF_EDITABLE)) return DoSuperMethodA(cl,o,msg);

  D(bug("Text_GoActive(%p)\n", o));

  DoMethod(_win(o), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
  data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
  DoMethod(_win(o), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

  data->update = 1;
  MUI_Redraw(o, MADF_DRAWUPDATE);
  return 0;
}

/**************************************************************************
 MUIM_GoInactive
**************************************************************************/
static ULONG Text_GoInactive(struct IClass * cl, Object * o, Msg msg)
{
  struct MUI_TextData *data = (struct MUI_TextData*) INST_DATA(cl, o);
  if (!(data->mtd_Flags & MTDF_EDITABLE)) return DoSuperMethodA(cl,o,msg);

  D(bug("Text_GoInactive(%p)\n", o));

  DoMethod(_win(o), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
  data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
  DoMethod(_win(o), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

  data->update = 1;
  MUI_Redraw(o, MADF_DRAWUPDATE);
  return 0;
}
#endif

#if 0
/**************************************************************************
 Returns wheater object needs redrawing
**************************************************************************/
int Text_HandleVanillakey(struct IClass *cl, Object * obj, unsigned char code)
{
    struct MUI_TextData *data = (struct MUI_TextData*) INST_DATA(cl, obj);
    struct ZTextLine *line;
    struct ZTextChunk *chunk;
    int offx,len;
    struct RastPort rp;

    if (!code) return 0;

    data->xpixel = -1;

    if (code == '\r')
    {
	if (!(data->mtd_Flags & MTDF_MULTILINE))
	{
	    UBYTE *buf = NULL;
	    get(obj,MUIA_String_Contents, &buf);
	    if (data->mtd_Flags & MTDF_ADVANCEONCR) set(_win(obj),MUIA_Window_ActiveObject,MUIV_Window_ActiveObject_Next);
	    else set(_win(obj),MUIA_Window_ActiveObject,MUIV_Window_ActiveObject_None);
	    set(obj,MUIA_String_Acknowledge, (IPTR) buf);
	    return 0;
	} else
	{
	    ZText *new_text = zune_text_new(NULL, "\n", ZTEXT_ARG_NONE, 0);
	    if (new_text)
	    {
		zune_text_merge(data->ztext, obj, data->xpos, data->ypos, new_text);
	    }

	    data->xpos = 0;
	    data->ypos++;
	    zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
	}
	return 1;
    }

    InitRastPort(&rp);
    SetFont(&rp,_font(obj));

    if (code == '\b')
    {
    	if (data->xpos && zune_text_get_char_pos(data->ztext, obj, data->xpos, data->ypos, &line, &chunk, &offx, &len))
    	{
	    if (len)
	    {
	    	chunk->cwidth -= TextLength(&rp,&chunk->str[len-1],1);
		line->lwidth -= TextLength(&rp,&chunk->str[len-1],1);
		strcpy(&chunk->str[len-1],&chunk->str[len]);
	    	data->xpos--;
	    } else
	    {
	    	/* delete char of previous node ... */
	    }
	    zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
    	}
	
        DeinitRastPort(&rp);

	return 1;
    }

    if (code == 127) /* del */
    {
    	if (zune_text_get_char_pos(data->ztext, obj, data->xpos, data->ypos, &line, &chunk, &offx, &len))
    	{
	    if (chunk->str && chunk->str[len])
	    {
	    	chunk->cwidth -= TextLength(&rp,&chunk->str[len],1);
		line->lwidth -= TextLength(&rp,&chunk->str[len],1);
		strcpy(&chunk->str[len],&chunk->str[len+1]);
	    }
    	}
	
        DeinitRastPort(&rp);
	
        return 1;
    }

    if (code == '\t')
    {
	if (!(data->mtd_Flags & MTDF_MULTILINE))
	{
	    UBYTE *buf = NULL;
	    get(obj,MUIA_String_Contents, &buf);
	    set(_win(obj),MUIA_Window_ActiveObject,MUIV_Window_ActiveObject_Next);
	    set(obj,MUIA_String_Acknowledge,(IPTR)buf);
	    
            DeinitRastPort(&rp);
	    
            return 0;
	}
    }

    if (code == '\033')
    {
	Object *act;
	get(_win(obj),MUIA_Window_ActiveObject,&act);

	if (act == obj)
	    set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
	    
            DeinitRastPort(&rp);
	    
            return 0;
    }

    if (data->accept)
    {
    	/* Check if character is accepted */
	if (!strchr(data->accept,code))
	{
	    DeinitRastPort(&rp);
	    return 0;
	}
    }

    if (zune_text_get_char_pos(data->ztext, obj, data->xpos, data->ypos, &line, &chunk, &offx, &len))
    {
    	if (!chunk)
    	{
	    if ((chunk = mui_alloc_struct(struct ZTextChunk)))
	    {
		chunk->dripen = TEXTPEN;
		AddTail((struct List*)&line->chunklist,(struct Node*)&chunk->node);
	    }
    	}

    	if (chunk)
    	{
    	    int char_width;
	    if (!chunk->str)
	    {
		if ((chunk->str = (char*)mui_alloc(2)))
		{
		    chunk->str[0] = code;
		    chunk->str[1] = 0;
		}
	    }   else
	    {
	        char *newstr = (char*)mui_alloc(strlen(chunk->str)+2);
	        if (newstr)
	        {
		    strncpy(newstr,chunk->str,len);
		    newstr[len] = code;
		    strcpy(&newstr[len+1],&chunk->str[len]);
		    mui_free(chunk->str);
		    chunk->str = newstr;
	        }
	    }
	    data->xpos++;
	    char_width = TextLength(_rp(obj),&code,1);
	    chunk->cwidth += char_width;
	    line->lwidth += char_width;

	    if (!zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj)))
	    {
		if (!(data->mtd_Flags & MTDF_MULTILINE))
		{
		    data->update_arg1 = offx;
		    data->update_arg2 = char_width;
		    
                    DeinitRastPort(&rp);
		    
                    return 2;
		}
	    }
        }
    }

    DeinitRastPort(&rp);

    return 1;
}


/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG Text_HandleEvent(struct IClass *cl, Object * obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_TextData *data = (struct MUI_TextData*) INST_DATA(cl, obj);
    ULONG retval = 0;
    int update = 0;

    if (!(data->mtd_Flags & MTDF_EDITABLE)) return 0;

    if (msg->imsg)
    {
	UWORD code = msg->imsg->Code;
	//UWORD qual = msg->imsg->Qualifier;
	WORD x = msg->imsg->MouseX;
	WORD y = msg->imsg->MouseY;

	switch (msg->imsg->Class)
	{
	    case    IDCMP_MOUSEBUTTONS: /* set cursor and activate it */
		    if (code == SELECTDOWN)
		    {
			Object *act;
			get(_win(obj),MUIA_Window_ActiveObject,&act);

			if (_isinobject(x, y))
			{
			    if (act != obj) set(_win(obj), MUIA_Window_ActiveObject, (IPTR) obj);
			    retval = MUI_EventHandlerRC_Eat;
			}   else
			{
			    if (act == obj)
				set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
			}
		    }
		    break;

	    case    IDCMP_RAWKEY:
		    {
		    	switch (msg->imsg->Code)
		    	{
		    	    case    CURSORLEFT:
				    if (data->xpos > zune_text_get_line_len(data->ztext,obj,data->ypos)) data->xpos = zune_text_get_line_len(data->ztext,obj,data->ypos);
				    if (data->xpos)
				    {
					data->xpos--;
					update = 1;
					zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
				    }
				    data->xpixel = -1;
				    retval = MUI_EventHandlerRC_Eat;
				    break;

		    	    case    CURSORRIGHT:
				    if (data->xpos > zune_text_get_line_len(data->ztext,obj,data->ypos)) data->xpos = zune_text_get_line_len(data->ztext,obj,data->ypos);
				    if (data->xpos < zune_text_get_line_len(data->ztext,obj,data->ypos))
				    {
					data->xpos++;
					update = 1;
					zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
				    }
				    data->xpixel = -1;
				    retval = MUI_EventHandlerRC_Eat;
				    break;

			    case    CURSORUP:
				    if (data->mtd_Flags & MTDF_MULTILINE)
				    {
				    	if (data->xpixel == -1)
				    	{
					    struct ZTextLine *line;
					    int offx,len;
					    zune_text_get_char_pos(data->ztext, obj, data->xpos, data->ypos, &line, NULL, &offx, &len);
					    data->xpixel = offx;
					}

					if (data->ypos)
					{
					    data->ypos--;
					    data->xpos = zune_get_xpos_of_line(data->ztext, obj, data->ypos, data->xpixel);
					}

					zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
					update = 1;
					retval = MUI_EventHandlerRC_Eat;
				    };
				    break;

			    case    CURSORDOWN:
				    if (data->mtd_Flags & MTDF_MULTILINE)
				    {
				    	if (data->xpixel == -1)
				    	{
					    struct ZTextLine *line;
					    int offx,len;
					    zune_text_get_char_pos(data->ztext, obj, data->xpos, data->ypos, &line, NULL, &offx, &len);
					    data->xpixel = offx;
					}

				    	if (data->ypos + 1 < zune_text_get_lines(data->ztext))
				    	{
					    data->ypos++;
					    data->xpos = zune_get_xpos_of_line(data->ztext, obj, data->ypos, data->xpixel);
					}
					zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
					update = 1;
					retval = MUI_EventHandlerRC_Eat;
				    }
				    break;

			    default:
				    {
					unsigned char code = ConvertKey(msg->imsg);
					if (code)
					{
					    update = Text_HandleVanillakey(cl,obj,code);
				            retval = MUI_EventHandlerRC_Eat;
				        }
				    }
				    break;
		    	}
		    }
	    	    break;
	}
    }

    if (update)
    {
    	data->update = update;
    	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    return retval;
}
#endif

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
	case MUIM_Show: return Text_Show(cl, obj, (APTR)msg);
	case MUIM_Hide: return Text_Hide(cl, obj, (APTR)msg);
	case MUIM_Export: return Text_Export(cl, obj, (APTR)msg);
	case MUIM_Import: return Text_Import(cl, obj, (APTR)msg);
#if 0
	case MUIM_GoActive: return Text_GoActive(cl, obj, (APTR)msg);
	case MUIM_GoInactive: return Text_GoInactive(cl,obj,(APTR)msg);
	case MUIM_HandleEvent: return Text_HandleEvent(cl,obj,(APTR)msg);
#endif
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END



/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Text_desc = { 
    MUIC_Text,
    MUIC_Area, 
    sizeof(struct MUI_TextData), 
    (void*)Text_Dispatcher
};
