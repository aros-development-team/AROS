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


struct MUI_StringData {
    ULONG  mtd_Flags;
    STRPTR contents;
    CONST_STRPTR accept; /* MUIA_String_Accept */
    ZText *ztext;
    LONG xpixel; /* needed for cursor up/down movements, can be -1 */
    LONG xpos;
    LONG ypos;
    struct MUI_EventHandlerNode ehn;

    LONG update; /* type of update 1 - everything, 2 - insert char, no scroll */
    LONG update_arg1;
    LONG update_arg2;
};

#define MTDF_MULTILINE (1<<6)
#define MTDF_ADVANCEONCR (1<<7)

static const int __version = 1;
static const int __revision = 1;

static void setup_text (struct MUI_StringData *data, Object *obj);

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG String_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_StringData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_String_Contents:
		    if (tag->ti_Data) data->contents = StrDup((STRPTR)tag->ti_Data);
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
	}
    }

    if (!data->contents) data->contents = StrDup("");

    if (!data->contents)
    {
	CoerceMethod(cl, obj, OM_DISPOSE);
	return NULL;
    }

    D(bug("muimaster.library/string.c: String Object created at 0x%lx\n",obj));

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
static ULONG String_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

    if (data->contents) FreeVec(data->contents);

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG String_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    struct TagItem      *tags = msg->ops_AttrList;
    struct TagItem      *tag;

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_String_Contents:
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
			set(obj, MUIA_String_Contents, buf);
		    }
		    break;

	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG String_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

#define STORE *(msg->opg_Storage)
    switch(msg->opg_AttrID)
    {
	case	MUIA_String_Contents:
		if (data->ztext)
		{
		    /* Convert the ztext to plain chars */
		    char *new_cont = zune_text_iso_string(data->ztext);
		    if (new_cont)
		    {
		    	if (data->contents) FreeVec(data->contents);
		    	data->contents = new_cont;
		    }
		}
		STORE = (ULONG)data->contents;
		return 1;

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
static void setup_text (struct MUI_StringData *data, Object *obj)
{
    data->ztext = zune_text_new(NULL, data->contents,
				ZTEXT_ARG_NONE, 0);

    zune_text_get_bounds(data->ztext, obj);

    D(bug("muimaster.library/string.c: ZString of 0x%lx at 0x%lx\n",obj,data->ztext));
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
static ULONG String_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
	return FALSE;

    setup_text(data, obj);

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);
    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
static ULONG String_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);

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
static ULONG String_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    int height;
    struct MUI_StringData *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    height = data->ztext->height;
    if (_font(obj)->tf_YSize > height) height = _font(obj)->tf_YSize;

    msg->MinMaxInfo->MinWidth += _font(obj)->tf_XSize*4;
    msg->MinMaxInfo->DefWidth += _font(obj)->tf_XSize*12;
    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;

    if (!(data->mtd_Flags & MTDF_MULTILINE))
    {
	msg->MinMaxInfo->MinHeight += height;
	msg->MinMaxInfo->DefHeight += height;
	msg->MinMaxInfo->MaxHeight += height;
    } else
    {
	msg->MinMaxInfo->MinHeight += _font(obj)->tf_YSize;
	msg->MinMaxInfo->DefHeight += _font(obj)->tf_YSize*10;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    }

    D(bug("muimaster.library/string.c: String_AskMinMax 0x%lx: Min=%ldx%ld Max=%ldx%ld Def=%ldx%ld\n", obj,
	msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight,
	msg->MinMaxInfo->MaxWidth, msg->MinMaxInfo->MaxHeight,
	msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->DefHeight));

    return TRUE;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG String_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
    Object *act;
    APTR clip;

    D(bug("muimaster.library/string.c: Draw String Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

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

        if (act == obj)
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
        {
            int y;
            if (data->mtd_Flags & MTDF_MULTILINE)
            {
		y = 0;
            } else
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
static ULONG String_Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
    struct MUI_StringData *data = INST_DATA(cl, obj);
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
static ULONG String_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    STRPTR id;
    STRPTR s;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
	if ((s = (STRPTR)DoMethod(msg->dataspace, MUIM_Dataspace_Find, id)))
	{
	    set(obj, MUIA_String_Contents, s);
	}
    }
    return 0;
}

/**************************************************************************
 MUIM_GoActive
**************************************************************************/
static ULONG String_GoActive(struct IClass * cl, Object * o, Msg msg)
{
  struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, o);

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
static ULONG String_GoInactive(struct IClass * cl, Object * o, Msg msg)
{
  struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, o);

  DoMethod(_win(o), MUIM_Window_RemEventHandler, (IPTR)&data->ehn);
  data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
  DoMethod(_win(o), MUIM_Window_AddEventHandler, (IPTR)&data->ehn);

  data->update = 1;
  MUI_Redraw(o, MADF_DRAWUPDATE);
  return 0;
}

/**************************************************************************
 Returns wheater object needs redrawing
**************************************************************************/
int String_HandleVanillakey(struct IClass *cl, Object * obj, unsigned char code)
{
    struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, obj);
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
	    set(obj,MUIA_String_Acknowledge,buf);
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
#ifdef __AROS__
	DeinitRastPort(&rp);
#endif
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
#ifdef __AROS__
	DeinitRastPort(&rp);
#endif
	return 1;
    }

    if (code == '\t')
    {
	if (!(data->mtd_Flags & MTDF_MULTILINE))
	{
	    UBYTE *buf = NULL;
	    get(obj,MUIA_String_Contents, &buf);
	    set(_win(obj),MUIA_Window_ActiveObject,MUIV_Window_ActiveObject_Next);
	    set(obj,MUIA_String_Acknowledge,buf);
#ifdef __AROS__
	    DeinitRastPort(&rp);
#endif
	    return 0;
	}
    }

    if (data->accept)
    {
    	/* Check if character is accepted */
	if (!strchr(data->accept,code))
	{
#ifdef __AROS__
	    DeinitRastPort(&rp);
#endif
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
#ifdef __AROS__
		    DeinitRastPort(&rp);
#endif
		    return 2;
		}
	    }
        }
    }

#ifdef __AROS__
    DeinitRastPort(&rp);
#endif

    return 1;
}


/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
static ULONG String_HandleEvent(struct IClass *cl, Object * obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_StringData *data = (struct MUI_StringData*) INST_DATA(cl, obj);
    ULONG retval = 0;
    int update = 0;

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
					    update = String_HandleVanillakey(cl,obj,code);
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


BOOPSI_DISPATCHER(IPTR, String_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return String_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return String_Dispose(cl, obj, msg);
	case OM_SET: return String_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return String_Get(cl, obj, (struct opGet *)msg);
	case MUIM_AskMinMax: return String_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw: return String_Draw(cl, obj, (APTR)msg);
	case MUIM_Setup: return String_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return String_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Export: return String_Export(cl, obj, (APTR)msg);
	case MUIM_Import: return String_Import(cl, obj, (APTR)msg);
	case MUIM_GoActive: return String_GoActive(cl, obj, (APTR)msg);
	case MUIM_GoInactive: return String_GoInactive(cl,obj,(APTR)msg);
	case MUIM_HandleEvent: return String_HandleEvent(cl,obj,(APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_String_desc = { 
    MUIC_String,
    MUIC_Area,
    sizeof(struct MUI_StringData), 
    (void*)String_Dispatcher
};
