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
#include "muimaster_intern.h"
#include "support.h"

extern struct Library *MUIMasterBase;

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

static char *g_strdup(char *x)
{
    char *dup;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}

#define g_free(x) FreeVec(x);
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

    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
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
	    case MUIA_Text_Editable:
	        _handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_EDITABLE);
	        break;
            case MUIA_String_AdvanceOnCR:
            	_handle_bool_tag(data->mtd_Flags, tag->ti_Data, MTDF_ADVANCEONCR);
            	break;
	}
    }

    D(bug("muimaster.library/text.c: Text Object created at 0x%lx\n",obj));

    data->ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags    = 0;
    data->ehn.ehn_Object   = obj;
    data->ehn.ehn_Class    = cl;

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

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Text_Contents:
		if (data->ztext)
		{
		    zune_text_destroy(data->ztext);
		    data->ztext = NULL;
		}
		if (data->contents)
		    g_free(data->contents);
		data->contents = g_strdup((STRPTR)tag->ti_Data);
		if (_flags(obj) & MADF_SETUP)
		    setup_text(data, obj);
		MUI_Redraw(obj,MADF_DRAWOBJECT); /* should be opimized */
		break;
	    case MUIA_Text_PreParse:
		if (data->ztext)
		    zune_text_destroy(data->ztext);
		if (data->preparse)
		    g_free(data->preparse);
		data->preparse = g_strdup((STRPTR)tag->ti_Data);
		if (_flags(obj) & MADF_SETUP)
		    setup_text(data, obj);
		MUI_Redraw(obj,MADF_DRAWOBJECT); /* should be opimized */
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

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehn);
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG Text_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_TextData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);

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
    Object *act;
    APTR clip;

    D(bug("muimaster.library/text.c: Draw Text Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

    DoSuperMethodA(cl,obj,(Msg)msg);

    if ((msg->flags & MADF_DRAWUPDATE) && !data->update)
	return 0;

    if (!(msg->flags & MADF_DRAWUPDATE)) data->update = 0;

    if (msg->flags & MADF_DRAWUPDATE && data->update == 1)
    {
	DoMethod(obj,MUIM_DrawBackground, _mleft(obj),_mtop(obj),_mwidth(obj),_mheight(obj));
    }

    clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
			   _mwidth(obj), _mheight(obj));

    SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);

    if (data->update == 2)
    {
    	/* Note, scrolling won't work, if there would be a background, different then a plain pen */
        ScrollRaster(_rp(obj), -data->update_arg2, 0, data->update_arg1 + _mleft(obj), _mtop(obj), _mright(obj),_mtop(obj) + _font(obj)->tf_YSize);

	zune_text_draw_single(data->ztext, obj,
		   _mleft(obj), _mright(obj),
		   _mtop(obj) + (_mheight(obj) - data->ztext->height) / 2,
		   data->xpos - 1,data->ypos, FALSE);
    } else
    {
        get(_win(obj),MUIA_Window_ActiveObject,&act);

        if (act == obj && (data->mtd_Flags & MTDF_EDITABLE))
        {
	    zune_text_draw_cursor(data->ztext, obj,
		   _mleft(obj), _mright(obj),
		   _mtop(obj) + (_mheight(obj) - data->ztext->height) / 2,
		   data->xpos,data->ypos);
        } else
        {
	    zune_text_draw(data->ztext, obj,
		   _mleft(obj), _mright(obj),
		   _mtop(obj) + (_mheight(obj) - data->ztext->height) / 2);
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

/**************************************************************************
 MUIM_GoActive
**************************************************************************/
static ULONG Text_GoActive(struct IClass * cl, Object * o, Msg msg)
{
  struct MUI_TextData *data = (struct MUI_TextData*) INST_DATA(cl, o);
  if (!(data->mtd_Flags & MTDF_EDITABLE)) return DoSuperMethodA(cl,o,msg);

  DoMethod(_win(o), MUIM_Window_RemEventHandler, &data->ehn);
  data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
  DoMethod(_win(o), MUIM_Window_AddEventHandler, &data->ehn);

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

  DoMethod(_win(o), MUIM_Window_RemEventHandler, &data->ehn);
  data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
  DoMethod(_win(o), MUIM_Window_AddEventHandler, &data->ehn);

  data->update = 1;
  MUI_Redraw(o, MADF_DRAWUPDATE);
  return 0;
}

/**************************************************************************
 Returns wheater object needs redrawing
**************************************************************************/
int Text_HandleVanillakey(struct IClass *cl, Object * obj, unsigned char code)
{
    struct MUI_TextData *data = (struct MUI_TextData*) INST_DATA(cl, obj);
    struct ZTextLine *line;
    struct ZTextChunk *chunk;
    int offx,len;

    if (!code) return 0;

    if (code == '\r')
    {
	if (!(data->mtd_Flags & MTDF_MULTILINE))
	{
	    set(obj,MUIA_String_Acknowledge,TRUE);
	    if (data->mtd_Flags & MTDF_ADVANCEONCR) set(_win(obj),MUIA_Window_ActiveObject,MUIV_Window_ActiveObject_Next);
	    else set(_win(obj),MUIA_Window_ActiveObject,MUIV_Window_ActiveObject_None);
	    return 0;
	}
	return 1;
    }

    if (code == '\b')
    {
    	if (data->xpos && zune_text_get_char_pos(data->ztext, obj, data->xpos, data->ypos, &line, &chunk, &offx, &len))
    	{
	    if (len)
	    {
		strcpy(&chunk->str[len-1],&chunk->str[len]);
	    	data->xpos--;
	    } else
	    {
	    	/* delete char of previous node ... */
	    }
	    zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
    	}
	return 1;
    }

    if (code == 127) /* del */
    {
    	if (zune_text_get_char_pos(data->ztext, obj, data->xpos, data->ypos, &line, &chunk, &offx, &len))
    	{
	    if (chunk->str && chunk->str[len])
	    {
		strcpy(&chunk->str[len],&chunk->str[len+1]);
	    }
    	}
	return 1;
    }

    if (code == '\t')
    {
	if (!(data->mtd_Flags & MTDF_MULTILINE))
	{
	    set(obj,MUIA_String_Acknowledge,TRUE);
	    set(_win(obj),MUIA_Window_ActiveObject,MUIV_Window_ActiveObject_Next);
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
	    	data->update_arg1 = offx;
	    	data->update_arg2 = char_width;
		return 2;
	    }
        }
    }
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
	UWORD qual = msg->imsg->Qualifier;
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
			    if (act != obj) set(_win(obj), MUIA_Window_ActiveObject, obj);
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
				    if (data->xpos)
				    {
					data->xpos--;
					update = 1;
					zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
				    }
				    retval = MUI_EventHandlerRC_Eat;
				    break;

		    	    case    CURSORRIGHT:
				    if (data->xpos < zune_text_get_line_len(data->ztext,obj,data->ypos))
				    {
					data->xpos++;
					update = 1;
					zune_make_cursor_visible(data->ztext, obj, data->xpos, data->ypos, _mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
				    }
				    retval = MUI_EventHandlerRC_Eat;
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

#if 0
    case IDCMP_VANILLAKEY:
      {
	switch (code) {
	case 8:		/* backspace */
	  if (data->cursor_pos) {
	    LONG shift;
	    LONG i;

	    if ((qual & IEQUALIFIER_LSHIFT) || (qual & IEQUALIFIER_RSHIFT)) {
	      shift = data->cursor_pos;
	    } else
	      shift = 1;

	    i = data->cursor_pos = data->cursor_pos - shift;
	    while (data->edit_buffer[i + shift]) {
	      data->edit_buffer[i] = data->edit_buffer[i + shift];
	      i++;
	    }
	    data->edit_buffer[i] = 0;
	    redraw = 1;
	  }
	  break;

	case 127:		/* del */
	  {
	    if ((qual & IEQUALIFIER_LSHIFT) || (qual & IEQUALIFIER_RSHIFT)) {
	      data->edit_buffer[data->cursor_pos] = 0;
	    } else {
	      LONG i = data->cursor_pos;
	      while (data->edit_buffer[i]) {
		data->edit_buffer[i] = data->edit_buffer[i + 1];
		i++;
	      }
	    }
	    redraw = 1;
	  }
	  break;

	case 24:		/* CTRL X */
	  data->edit_buffer[0] = 0;
	  data->cursor_pos = 0;
	  redraw = 1;
	  break;

	case 13:		/* return */
	  set(_win(o), MUIA_Window_ActiveObject, NULL);
	  set(o, MUIA_TransparentString_Acknowledge, TRUE);
	  break;

	default:
	  if (!IsCntrl(data->locale, code)) {
	    LONG buf_len = strlen(data->edit_buffer);
	    LONG i;
	    if (buf_len < BUF_SIZE) {
	      for (i = buf_len; i >= data->cursor_pos; i--)
		data->edit_buffer[i + 1] = data->edit_buffer[i];

	      data->edit_buffer[data->cursor_pos++] = code;
	      data->edit_buffer[buf_len + 1] = 0;
	      redraw = 1;
	    }
	  }
	}
      }
      break;
    }
  }
  switch (msg->muikey) {
  case MUIKEY_LEFT:
    if (data->cursor_pos) {
      data->cursor_pos--;
      redraw = TRUE;
    }
    break;

  case MUIKEY_RIGHT:
    if (data->cursor_pos < strlen(data->edit_buffer)) {
      data->cursor_pos++;
      redraw = TRUE;
    }
    break;

  case MUIKEY_LINESTART:
    data->cursor_pos = 0;
    redraw = TRUE;
    break;

  case MUIKEY_LINEEND:
    data->cursor_pos = strlen(data->edit_buffer);
    redraw = TRUE;
    break;
  }
#endif


    if (update)
    {
    	data->update = update;
    	MUI_Redraw(obj, MADF_DRAWUPDATE);
    }
    return retval;
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
	case MUIM_GoActive: return Text_GoActive(cl, obj, (APTR)msg);
	case MUIM_GoInactive: return Text_GoInactive(cl,obj,(APTR)msg);
	case MUIM_HandleEvent: return Text_HandleEvent(cl,obj,(APTR)msg);
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
