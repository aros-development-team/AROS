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

#include <stdlib.h>
#include <string.h>
#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

extern struct Library *MUIMasterBase;

#include "support.h"
#include "mui.h"

#define MYDEBUG 1
#include "debug.h"

#define g_strdup(x) strdup(x)
#define g_free(x) free(x)


struct TextFont *zune_font_get (LONG preset)
{
    if ((preset <= MUIV_Font_Inherit) && (preset >= MUIV_Font_NegCount))
    {
	if (preset > 0) return NULL;
	return __zprefs.fonts[-preset];
    }
    return (struct TextFont *)preset;
}

/*
Area.mui/MUIA_Background            done
Area.mui/MUIA_BottomEdge            done
Area.mui/MUIA_ContextMenu
Area.mui/MUIA_ContextMenuTrigger
Area.mui/MUIA_ControlChar
Area.mui/MUIA_CycleChain
Area.mui/MUIA_Disabled
Area.mui/MUIA_Draggable
Area.mui/MUIA_Dropable
Area.mui/MUIA_ExportID
Area.mui/MUIA_FillArea              done
Area.mui/MUIA_FixHeight             done
Area.mui/MUIA_FixHeightTxt          need text/font
Area.mui/MUIA_FixWidth              done
Area.mui/MUIA_FixWidthTxt           need text/font
Area.mui/MUIA_Font
Area.mui/MUIA_Frame                 done
Area.mui/MUIA_FramePhantomHoriz     done
Area.mui/MUIA_FrameTitle            need text/font
Area.mui/MUIA_Height                done
Area.mui/MUIA_HorizDisappear
Area.mui/MUIA_HorizWeight           done
Area.mui/MUIA_InnerBottom           done
Area.mui/MUIA_InnerLeft             done
Area.mui/MUIA_InnerRight            done
Area.mui/MUIA_InnerTop              done
Area.mui/MUIA_InputMode             done
Area.mui/MUIA_LeftEdge              done
Area.mui/MUIA_MaxHeight             done
Area.mui/MUIA_MaxWidth              done
Area.mui/MUIA_Pressed               done
Area.mui/MUIA_RightEdge             done
Area.mui/MUIA_Selected              done
Area.mui/MUIA_ShortHelp             done
Area.mui/MUIA_ShowMe                done
Area.mui/MUIA_ShowSelState
Area.mui/MUIA_Timer                 done
Area.mui/MUIA_TopEdge               done
Area.mui/MUIA_VertDisappear
Area.mui/MUIA_VertWeight            done
Area.mui/MUIA_Weight                done
Area.mui/MUIA_Width                 done
Area.mui/MUIA_Window
Area.mui/MUIA_WindowObject          done

Area.mui/MUIM_AskMinMax             done
Area.mui/MUIM_Cleanup               done
Area.mui/MUIM_ContextMenuBuild
Area.mui/MUIM_ContextMenuChoice
Area.mui/MUIM_CreateBubble
Area.mui/MUIM_CreateShortHelp
Area.mui/MUIM_DeleteBubble
Area.mui/MUIM_DeleteShortHelp
Area.mui/MUIM_DragBegin
Area.mui/MUIM_DragDrop
Area.mui/MUIM_DragFinish
Area.mui/MUIM_DragQuery
Area.mui/MUIM_DragReport
Area.mui/MUIM_Draw                  miss frame title
Area.mui/MUIM_DrawBackground        done
Area.mui/MUIM_HandleEvent           done
Area.mui/MUIM_HandleInput
Area.mui/MUIM_Hide                  done
Area.mui/MUIM_Setup                 done
Area.mui/MUIM_Show                  done
*/

static const int __version = 1;
static const int __revision = 1;

//#ifdef DEBUG
//static STRPTR zune_area_to_string (Object *area);
//#endif
static void area_update_data(struct MUI_AreaData *data);
static void setup_control_char (struct MUI_AreaData *data, Object *obj,
				struct IClass *cl);
static void cleanup_control_char (struct MUI_AreaData *data, Object *obj);
static void setup_cycle_chain (struct MUI_AreaData *data, Object *obj);
static void cleanup_cycle_chain (struct MUI_AreaData *data, Object *obj);



/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Area_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_AreaData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    data->mad_Flags = MADF_FILLAREA | MADF_SHOWME | MADF_SHOWSELSTATE;
    data->mad_HorizWeight = data->mad_VertWeight = 100;
    data->mad_InputMode = MUIV_InputMode_None;
#warning FIXME: mad_Background
#if 0
    data->mad_Background = (struct MUI_ImageSpec *)-1;
#endif

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((const struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
#warning FIXME: mad_Background
#if 0
	    case MUIA_Background:
		data->mad_Background = (struct MUI_ImageSpec *)tag->ti_Data;
		break;
#endif
	    case MUIA_ControlChar:
		data->mad_ControlChar = tag->ti_Data;
		break;
	    case MUIA_CycleChain:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_CYCLECHAIN);
		break;
	    case MUIA_FillArea:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_FILLAREA);
		break;
	    case MUIA_FixHeight:
		data->mad_Flags |= MADF_FIXHEIGHT;
		data->mad_HardHeight = tag->ti_Data;
		break;
	    case MUIA_FixWidth:
		data->mad_Flags |= MADF_FIXWIDTH;
		data->mad_HardWidth = tag->ti_Data;
		break;
	    case MUIA_Font:
		data->mad_FontPreset = tag->ti_Data;
		break;
	    case MUIA_Frame:
		data->mad_Frame = tag->ti_Data;
		break;
	    case MUIA_FramePhantomHoriz:
		data->mad_Flags |= MADF_FRAMEPHANTOM;
		break;
	    case MUIA_FrameTitle:
		/* strdup after tags parsing */
		data->mad_FrameTitle = (STRPTR)tag->ti_Data;
		break;
	    case MUIA_HorizWeight:
		data->mad_HorizWeight = tag->ti_Data;
		break;
	    case MUIA_InnerBottom:
		data->mad_Flags |= MADF_INNERBOTTOM;
		data->mad_HardIBottom = CLAMP((ULONG)tag->ti_Data, 0, 32);
		break;
	    case MUIA_InnerLeft:
		data->mad_Flags |= MADF_INNERLEFT;
		data->mad_HardILeft = CLAMP((ULONG)tag->ti_Data, 0, 32);
		break;
	    case MUIA_InnerRight:
		data->mad_Flags |= MADF_INNERRIGHT;
		data->mad_HardIRight = CLAMP((ULONG)tag->ti_Data, 0, 32);
		break;
	    case MUIA_InnerTop:
		data->mad_Flags |= MADF_INNERTOP;
		data->mad_HardITop =  CLAMP((ULONG)tag->ti_Data, 0, 32);
		break;
	    case MUIA_InputMode:
		data->mad_InputMode = tag->ti_Data;
		break;
	    case MUIA_MaxHeight:
		data->mad_Flags |= MADF_MAXHEIGHT;
		data->mad_HardHeight = tag->ti_Data;
		break;
	    case MUIA_MaxWidth:
		data->mad_Flags |= MADF_MAXWIDTH;
		data->mad_HardWidth = tag->ti_Data;
		break;
	    case MUIA_Selected:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_SELECTED);
		break;
	    case MUIA_ShortHelp:
		data->mad_ShortHelp = (STRPTR)tag->ti_Data;
		break;
	    case MUIA_ShowMe:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_SHOWME);
		break;
	    case MUIA_ShowSelState:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_SHOWSELSTATE);
		break;
	    case MUIA_VertWeight:
		data->mad_VertWeight = tag->ti_Data;
		break;
	    case MUIA_Weight:
		data->mad_HorizWeight = data->mad_VertWeight = tag->ti_Data;
		break;
	}
    }

#warning FIXME: mad_Background
#if 0
    if (data->mad_Background != (struct MUI_ImageSpec *)-1) /* own bg ? */
    {
	data->mad_Background =
	    zune_image_spec_to_structure((ULONG)data->mad_Background);
/*    		g_print("created img %p (%s) for area %p\n", data->mad_Background, */
/*  			zune_imspec_to_string(data->mad_Background), obj); */
    }
    else
    {
	data->mad_Background = NULL; /* will be filled by parent */
    }

    if ((data->mad_Flags & MADF_SHOWSELSTATE)
	&& (data->mad_InputMode != MUIV_InputMode_None))
    {
/*  	g_print("mad_SelBack for obj %p\n", obj); */
	data->mad_SelBack = zune_imspec_copy(__zprefs.images[MUII_SelectedBack]);
    }
#endif

    if ((data->mad_Frame != 0) && (data->mad_FrameTitle))
    {
    	char *frame_title = mui_alloc(strlen(data->mad_FrameTitle)+1);
    	if (frame_title) strcpy(frame_title,data->mad_FrameTitle);
    	data->mad_FrameTitle = NULL;
    }

    if (data->mad_InputMode != MUIV_InputMode_None)
    {
	data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
	data->mad_ehn.ehn_Priority = 0;
	data->mad_ehn.ehn_Flags    = 0;
	data->mad_ehn.ehn_Object   = obj;
	data->mad_ehn.ehn_Class    = cl;
    }

    D(bug("muimaster.library/area.c: Area Object created at 0x%lx\n",obj));

    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Area_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if ((data->mad_Frame > 0) && (data->mad_FrameTitle))
    {
	mui_free(data->mad_FrameTitle);
    }

#warning FIXME: mad_Background
#if 0
    if (data->mad_Background)
	zune_imspec_free(data->mad_Background);

    if (data->mad_SelBack)
	zune_imspec_free(data->mad_SelBack);
#endif

    return DoSuperMethodA(cl, obj, msg);
}


/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG Area_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_AreaData *data  = INST_DATA(cl, obj);
    struct TagItem             *tags  = msg->ops_AttrList;
    struct TagItem             *tag;

    while ((tag = NextTagItem((const struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
#warning FIXME: mad_Background
#if 0
	    case MUIA_Background:
		if (data->mad_Background)
		{
		    if (_flags(obj) & MADF_CANDRAW)
			zune_imspec_hide(data->mad_Background);
		    if (_flags(obj) & MADF_SETUP)
			zune_imspec_cleanup(&data->mad_Background, muiRenderInfo(obj));
		    zune_imspec_free(data->mad_Background);
		}
		data->mad_Background =
		    zune_image_spec_to_structure((ULONG)tag->ti_Data);
		if (_flags(obj) & MADF_SETUP)
		    zune_imspec_setup(&data->mad_Background, muiRenderInfo(obj));
		if (_flags(obj) & MADF_CANDRAW)
		{
		    zune_imspec_set_scaled_size(data->mad_Background,
						_width(obj), _height(obj));
		    zune_imspec_show(data->mad_Background, obj);
		}
/*    		g_print("set img %p for area %p\n", data->mad_Background, obj); */
		MUI_Redraw(obj, MADF_DRAWOBJECT);
		break;
#endif

	    case MUIA_ControlChar:
		if (_flags(obj) & MADF_SETUP)
		    cleanup_control_char(data, obj);
		data->mad_ControlChar = tag->ti_Data;
		if (_flags(obj) & MADF_SETUP)
		    setup_control_char(data, obj, cl);
		break;

	    case MUIA_CycleChain:
		if (data->mad_InputMode == MUIV_InputMode_None)
		    break;

		if ((!(_flags(obj) & MADF_CYCLECHAIN) && tag->ti_Data)
		    || ((_flags(obj) & MADF_CYCLECHAIN) && !tag->ti_Data))
		{
		    if (_flags(obj) & MADF_SETUP)
		    {
			if (_flags(obj) & MADF_CYCLECHAIN)
			{
			    _handle_bool_tag(data->mad_Flags,
					     tag->ti_Data, MADF_CYCLECHAIN);
			    cleanup_cycle_chain(data, obj);
			}
			else
			{
			    _handle_bool_tag(data->mad_Flags,
					     tag->ti_Data, MADF_CYCLECHAIN);
			    setup_cycle_chain(data, obj);
			}
		    }
		    else
		    {
			_handle_bool_tag(data->mad_Flags,
					 tag->ti_Data, MADF_CYCLECHAIN);
		    }
		}
		break;

	    case MUIA_HorizWeight:
		data->mad_HorizWeight = tag->ti_Data;
		break;

	    case MUIA_Pressed:
		if (tag->ti_Data)
		    data->mad_Flags |= MADF_PRESSED;
		else
		    data->mad_Flags &= ~MADF_PRESSED;
		break;

	    case MUIA_ShortHelp:
		data->mad_ShortHelp = (STRPTR)tag->ti_Data;
		break;

	    case MUIA_ShowMe:
		if (tag->ti_Data)
		    data->mad_Flags |= MADF_SHOWME;
		else
		    data->mad_Flags &= ~MADF_SHOWME;
		DoMethod(_win(obj), MUIM_Window_RecalcDisplay);
		break;

	    case MUIA_Selected:
		if (tag->ti_Data)
		    data->mad_Flags |= MADF_SELECTED;
		else
		    data->mad_Flags &= ~MADF_SELECTED;
		if (data->mad_InputMode != MUIV_InputMode_None)
		    MUI_Redraw(obj, MADF_DRAWOBJECT);
		break;

	    case MUIA_Timer:
		data->mad_Timeval = tag->ti_Data;
/*    		g_print("tick %d\n", data->mad_Timeval); */
		break;

	    case MUIA_VertWeight:
		data->mad_VertWeight = tag->ti_Data;
		break;
	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG Area_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_AreaData *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
	case MUIA_BottomEdge:
	    STORE = (ULONG)_bottom(obj);
	    return(TRUE);
	case MUIA_ControlChar:
	    STORE = data->mad_ControlChar;
	    return(TRUE);
	case MUIA_CycleChain:
	    STORE = ((data->mad_Flags & MADF_CYCLECHAIN) != 0);
	    return(TRUE);
	case MUIA_Font:
	    STORE = (ULONG)data->mad_FontPreset;
	    return(TRUE);
	case MUIA_Height:
	    STORE = (ULONG)_height(obj);
	    return(TRUE);
	case MUIA_HorizWeight:
	    STORE = (ULONG)data->mad_HorizWeight;
	    return(TRUE);
	case MUIA_InnerBottom:
	    if (data->mad_Flags & MADF_INNERBOTTOM)
		STORE = (ULONG)data->mad_HardIBottom;
	    else
		STORE = (ULONG)__zprefs.frames[data->mad_Frame].innerBottom;
	    break;
	case MUIA_InnerLeft:
	    if (data->mad_Flags & MADF_INNERLEFT)
		STORE = (ULONG)data->mad_HardILeft;
	    else if (data->mad_Flags & MADF_FRAMEPHANTOM)
		STORE = 0;
	    else
		STORE = (ULONG)__zprefs.frames[data->mad_Frame].innerLeft;
	    break;
	case MUIA_InnerRight:
	    if (data->mad_Flags & MADF_INNERRIGHT)
		STORE = (ULONG)data->mad_HardIRight;
	    else if (data->mad_Flags & MADF_FRAMEPHANTOM)
		STORE = 0;
	    else
		STORE = (ULONG)__zprefs.frames[data->mad_Frame].innerRight;
	    break;
	case MUIA_InnerTop:
	    if (data->mad_Flags & MADF_INNERTOP)
		STORE = (ULONG)data->mad_HardITop;
	    else
		STORE = (ULONG)__zprefs.frames[data->mad_Frame].innerTop;
	    break;
	case MUIA_LeftEdge:
	    STORE = (ULONG)_left(obj);
	    return(TRUE);
	case MUIA_Pressed:
	    STORE = (data->mad_Flags & MADF_PRESSED);
	    return(TRUE);
	case MUIA_RightEdge:
	    STORE = (ULONG)_right(obj);
	    return(TRUE);
	case MUIA_Selected:
	    STORE = (data->mad_Flags & MADF_SELECTED);
	    return(TRUE);
	case MUIA_ShortHelp:
	    STORE = (ULONG)data->mad_ShortHelp;
	    return(TRUE);
	case MUIA_ShowMe:
	    STORE = (data->mad_Flags & MADF_SHOWME);
	    return(TRUE);
	case MUIA_Timer:
	    return(TRUE);
	case MUIA_TopEdge:
	    STORE = (ULONG)_top(obj);
	    return(TRUE);
	case MUIA_VertWeight:
	    STORE = (ULONG)data->mad_VertWeight;
	    return(TRUE);
	case MUIA_Width:
	    STORE = (ULONG)_width(obj);
	    return(TRUE);
	case MUIA_Window:
	    if (muiAreaData(obj)->mad_RenderInfo)
		STORE = (ULONG)_window(obj);
	    else
		STORE = 0L;
	    return(TRUE);
	case MUIA_WindowObject:
	    if (muiAreaData(obj)->mad_RenderInfo)
		STORE = (ULONG)_win(obj);
	    else
		STORE = 0L;
	    return(TRUE);
    }

    return(DoSuperMethodA(cl, obj, (Msg) msg));
#undef STORE
}


/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static ULONG Area_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    msg->MinMaxInfo->MinWidth = _subwidth(obj);
    if (data->mad_TitleText)
    {
#warning FIXME: mAskMinMax
#if 0
	GdkFont *obj_font = _font(obj);

	_font(obj) = zune_font_get(MUIV_Font_Title);
kprintf("*** Area->AskMinMax calling zune_text_get_bounds()\n");
	zune_text_get_bounds(data->mad_TitleText, obj);
	_font(obj) = obj_font;
/*  	g_print("title %dx%d\n", data->mad_TitleText->width, data->mad_TitleText->height); */

	_subheight(obj) = _subheight(obj) - _addtop(obj)
	    + data->mad_TitleText->height;
	_addtop(obj) = data->mad_TitleText->height + 1;
	if (__zprefs.group_title_color == GROUP_TITLE_COLOR_3D)
	{
	    _subheight(obj) += 1;
	    _addtop(obj) += 1;
	}
#endif
    }

    msg->MinMaxInfo->MinHeight = _subheight(obj);

    msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;
    msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;

    return TRUE;
}

/*
 * Called after MUIM_AskMinMax, to handle fixed and max sizes.
 */
void __area_finish_minmax(Object *obj, struct MUI_MinMax *MinMaxInfo)
{
    if (_flags(obj) & MADF_FIXHEIGHT)
    {
	int h;

	if (muiAreaData(obj)->mad_HardHeight != 0)
	    h = CLAMP(muiAreaData(obj)->mad_HardHeight,
		  MinMaxInfo->MinHeight, MinMaxInfo->MaxHeight);
	else
	    h = MUI_MAXMAX;

	MinMaxInfo->MinHeight = MinMaxInfo->DefHeight =
	    CLAMP(muiAreaData(obj)->mad_HardHeight,
		  MinMaxInfo->MinHeight, MinMaxInfo->MaxHeight);

	MinMaxInfo->MaxHeight = h;
    }
    else if (_flags(obj) & MADF_MAXHEIGHT)
    {	
	MinMaxInfo->MaxHeight =
	    CLAMP(muiAreaData(obj)->mad_HardHeight,
		  MinMaxInfo->MinHeight,
		  MinMaxInfo->MaxHeight);
    }

    if (_flags(obj) & MADF_FIXWIDTH)
    {
	int w;

	if (muiAreaData(obj)->mad_HardWidth != 0)
	    w = CLAMP(muiAreaData(obj)->mad_HardWidth,
		  MinMaxInfo->MinWidth, MinMaxInfo->MaxWidth);
	else
	    w = MUI_MAXMAX;

	MinMaxInfo->MinWidth = MinMaxInfo->DefWidth =
	    CLAMP(muiAreaData(obj)->mad_HardWidth,
		  MinMaxInfo->MinWidth, MinMaxInfo->MaxWidth);
	MinMaxInfo->MaxWidth = w;
    }
    else if (_flags(obj) & MADF_MAXWIDTH)
    {
	MinMaxInfo->MaxWidth =
	    CLAMP(muiAreaData(obj)->mad_HardWidth,
		  MinMaxInfo->MinWidth,
		  MinMaxInfo->MaxWidth);
    }

    /* Set minmax */
    _minwidth(obj) = MinMaxInfo->MinWidth;
    _minheight(obj) = MinMaxInfo->MinHeight;
    _maxwidth(obj) = MinMaxInfo->MaxWidth;
    _maxheight(obj) = MinMaxInfo->MaxHeight;
    _defwidth(obj) = MinMaxInfo->DefWidth;
    _defheight(obj) = MinMaxInfo->DefHeight;
}


/**************************************************************************
 MUIM_Draw
**************************************************************************/
static ULONG Area_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    struct ZuneFrameGfx *zframe;
    struct Rectangle mrect, current;
    APTR areaclip;
    int xtext = _mleft(obj);

    D(bug("muimaster.library/area.c: Draw Area Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

    if (!(data->mad_Flags & MADF_CANDRAW)) /* not between show/hide */
    {
	msg->flags &= ~MADF_DRAWOBJECT;
	return 0;
    }

    if (msg->flags & MADF_DRAWALL)
	msg->flags |= MADF_DRAWOBJECT;

    if (!(msg->flags & MADF_DRAWOBJECT))
    {
	/* dont draw bg/frame, let subclass redraw content only
	 */
	if (msg->flags & MADF_DRAWUPDATE)
	    msg->flags |= MADF_DRAWOBJECT;
	return 0;
    }

    if ((data->mad_Flags & MADF_SELECTED) &&
	!(data->mad_Flags & MADF_SHOWSELSTATE))
    {
	if (!(msg->flags & MADF_DRAWALL))
	    msg->flags &= ~MADF_DRAWOBJECT;
	return 0;
    }

    zframe = zune_zframe_get (&__zprefs.frames[data->mad_Frame]);
/*
    areaclip = MUI_AddClipping(muiRenderInfo(obj),
			       muiRenderInfo(obj)->mri_ClipRect.x,
			       muiRenderInfo(obj)->mri_ClipRect.y,
			       muiRenderInfo(obj)->mri_ClipRect.width,
			       muiRenderInfo(obj)->mri_ClipRect.height);
*/

/*
 * Background drawing
 */

    if (data->mad_Flags & MADF_FILLAREA)
    {
#warning FIXME: mad_Background
#if 0
	struct MUI_ImageSpec *background;

	if (!(data->mad_Flags & MADF_SELECTED))
	    background = data->mad_Background;
	else
	    background = data->mad_SelBack;
/*  g_print("AREADRAW: area=(%s) bg=%p\n", zune_area_to_string(obj), */
/*  	background); */
	if (data->mad_TitleText)
	{
	    int y = MAX(muiRenderInfo(obj)->mri_ClipRect.y,
			_top(obj) + zframe->ythickness
			+ data->mad_TitleText->height / 2);
	    zune_draw_image(data->mad_RenderInfo, background,
			    muiRenderInfo(obj)->mri_ClipRect.x, y,
			    muiRenderInfo(obj)->mri_ClipRect.width,
			    MIN(muiRenderInfo(obj)->mri_ClipRect.height,
				_bottom(obj) - y),
			    _left(obj), _top(obj) + data->mad_TitleText->height / 2, 0);
	}
	else
	{
	    zune_draw_image(data->mad_RenderInfo, background,
			    muiRenderInfo(obj)->mri_ClipRect.x,
			    muiRenderInfo(obj)->mri_ClipRect.y,
			    muiRenderInfo(obj)->mri_ClipRect.width,
			    muiRenderInfo(obj)->mri_ClipRect.height,
			    _left(obj), _top(obj), 0);
	}
#else
	struct Rectangle *r = &data->mad_RenderInfo->mri_ClipRect;

	SetAPen(data->mad_RenderInfo->mri_RastPort,
	        data->mad_RenderInfo->mri_Pens[MPEN_BACKGROUND]);

/*	if (data->mad_TitleText)
	{
	    int y = MAX(r->y,
			_top(obj) + zframe->ythickness
			+ data->mad_TitleText->height / 2);
	    RectFill(data->mad_RenderInfo->mri_RastPort,
			    r->x, y,
			    r->width,
			    MIN(r->height, _bottom(obj) - y));
	}
	else*/
	{
	    /* clipping should be calculated */
	    RectFill(_rp(obj),_mleft(obj),_mtop(obj),_mright(obj),_mbottom(obj));
	}
#endif
    }

/*
 * Frame drawing
 */

    if (!(data->mad_Flags & MADF_FRAMEPHANTOM))
    {
	int state = __zprefs.frames[data->mad_Frame].state;
	if (data->mad_Flags & MADF_SELECTED)
	    state ^= 1;

	/* set clipping so that frame is not drawn behind title */

	if (data->mad_TitleText)
	{
#if 0
//	    GdkRegion *empty;
//	    GdkRegion *clip2;
//	    GdkRegion *full2;
//	    GdkRegion *clipreg;
//	    GdkBitmap *oldclip = NULL;
//	    GdkRectangle rect;
//	    int clipx, clipy;
	    APTR textdrawclip;
	    GdkFont *obj_font = _font(obj);
//	    GdkGCValues values;
	    int width;

	    width = data->mad_TitleText->width;
	    if (__zprefs.group_title_color == GROUP_TITLE_COLOR_3D)
		width += 1;

            switch (__zprefs.group_title_position)
            {
            case GROUP_TITLE_POSITION_RIGHT:
                xtext = _mright(obj) - width - 3;
                break;
            case GROUP_TITLE_POSITION_CENTERED:
                xtext = _mleft(obj) +
                    (_mwidth(obj) - width) / 2;
                break;
            default:
            }

            if (xtext < _mleft(obj))
                xtext = _mleft(obj);

#if 0
	    /* text rectangle, with spacing (2 pixels left & right) */
	    rect.x = xtext;
	    rect.y = _top(obj);
	    rect.width = MIN(_mwidth(obj), width + 4);
	    rect.height = data->mad_TitleText->height;
	    empty = gdk_region_new();
	    clip2 = gdk_region_union_with_rect(empty, &rect);
	    /* maximum area rectangle */
	    rect.x = _left(obj);
	    rect.y = _top(obj);
	    rect.width = _width(obj);
	    rect.height = _height(obj);
	    full2 = gdk_region_union_with_rect(empty, &rect);
	    clipreg = gdk_regions_subtract(full2, clip2);
	    gdk_region_destroy(full2);
	    gdk_region_destroy(clip2);
	    gdk_region_destroy(empty);

	    gdk_gc_get_values(_rp(obj), &values);

	    if (values.clip_mask)
		oldclip = gdk_bitmap_ref(values.clip_mask);
	    clipx = values.clip_x_origin;
	    clipy = values.clip_y_origin;
	    gdk_gc_set_clip_region(_rp(obj), clipreg);
	    gdk_region_destroy(clipreg);

	    zframe->draw[state](muiRenderInfo(obj), _left(obj),
				_top(obj) + data->mad_TitleText->height / 2,
				_width(obj),
				_height(obj) - data->mad_TitleText->height / 2);
	    gdk_gc_set_clip_mask(_rp(obj), oldclip);
	    gdk_gc_set_clip_origin(_rp(obj), clipx, clipy);
	    if (oldclip)
		gdk_bitmap_unref(oldclip);
#endif

	    /*
	     * Title text drawing
	     */
	    _font(obj) = zune_font_get(MUIV_Font_Title);

	    xtext += 1;
/*	    textdrawclip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj) + 2, _top(obj),
					   _mwidth(obj) - 4, data->mad_TitleText->height);
*/
	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	    if (__zprefs.group_title_color == GROUP_TITLE_COLOR_3D)
	    {
		zune_text_draw(data->mad_TitleText, obj,
			       xtext + 1, xtext + width,
			       _top(obj) + 1);
		SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
		zune_text_draw(data->mad_TitleText, obj,
			       xtext, xtext + width - 1, _top(obj));
	    }
	    else if (__zprefs.group_title_color == GROUP_TITLE_COLOR_WHITE)
		SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);

	    if (__zprefs.group_title_color != GROUP_TITLE_COLOR_3D)
	    {
		xtext += 1;
		zune_text_draw(data->mad_TitleText, obj,
			       xtext, xtext + width - 1, _top(obj));
	    }
/*	    MUI_RemoveClipping(muiRenderInfo(obj), textdrawclip);*/

	    _font(obj) = obj_font;
#endif
	}
	else
	{
	    zframe->draw[state](muiRenderInfo(obj), _left(obj), _top(obj),
				_width(obj), _height(obj));
	}
    }

    mrect.MinX = _mleft(obj);
    mrect.MinY = _mtop(obj);
    mrect.MaxX = _mright(obj);
    mrect.MaxY = _mbottom(obj);

    current = muiRenderInfo(obj)->mri_ClipRect;
/*  	    g_print("intersect area: mrect=(%d, %d, %d, %d) current=(%d, %d, %d, %d)\n", */
/*  		    mrect.x, mrect.y, */
/*  		    mrect.width, mrect.height, */
/*  		    current.x, current.y, */
/*  		    current.width, current.height); */

#if 0
    if (!gdk_rectangle_intersect(&mrect, &current,
				 &muiRenderInfo(obj)->mri_ClipRect))
    {
/*  	g_print("failed\n"); */
	msg->flags &= ~MADF_DRAWOBJECT;
    }
#endif

/*    MUI_RemoveClipping(muiRenderInfo(obj), areaclip);*/

    return TRUE;
}

/**************************************************************************
 MUIM_DrawBackgroup
**************************************************************************/
static ULONG Area_DrawBackground(struct IClass *cl, Object *obj, struct MUIP_DrawBackground *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    if (!(data->mad_Flags & MADF_CANDRAW)) /* not between show/hide */
	return FALSE;

#warning FIXME: mad_Background
#if 0
    zune_draw_image(data->mad_RenderInfo, data->mad_Background,
		    msg->left, msg->top, msg->width, msg->height,
		    msg->xoffset, msg->yoffset, 0);
#else
    RectFill(data->mad_RenderInfo->mri_RastPort,
		    msg->left,
		    msg->top,
		    msg->left + msg->width  - 1,
		    msg->top  + msg->height - 1);
#endif

    return TRUE;
}

/* Perverting the EventHandlerNode structure to specify a shortcut.
 */
static void setup_control_char (struct MUI_AreaData *data, Object *obj, struct IClass *cl)
{
    if (data->mad_InputMode != MUIV_InputMode_None)
    {
	data->mad_ccn.ehn_Events = data->mad_ControlChar;
	switch (data->mad_InputMode)
	{
	    case MUIV_InputMode_RelVerify:
		data->mad_ccn.ehn_Flags = MUIKEY_PRESS;
		break;
	    case MUIV_InputMode_Toggle:
		data->mad_ccn.ehn_Flags = MUIKEY_TOGGLE;
		break;
	    case MUIV_InputMode_Immediate:
		data->mad_ccn.ehn_Flags = MUIKEY_PRESS;
		break;
	}
	data->mad_ccn.ehn_Priority = 0;
	data->mad_ccn.ehn_Object = obj;
	data->mad_ccn.ehn_Class = cl;
	DoMethod(_win(obj), MUIM_Window_AddControlCharHandler, (IPTR)&data->mad_ccn);
    }    
}


static void
cleanup_control_char (struct MUI_AreaData *data, Object *obj)
{
    if (data->mad_InputMode != MUIV_InputMode_None)
    {
	DoMethod(_win(obj),
		 MUIM_Window_RemControlCharHandler, (IPTR)&data->mad_ccn);
    }
}

static void
setup_cycle_chain (struct MUI_AreaData *data, Object *obj)
{
    if (_flags(obj) & MADF_CYCLECHAIN
	&& (data->mad_InputMode != MUIV_InputMode_None))
    {
/*  g_print("append %p to cc\n", obj); */
#if 0
	muiWindowData(_win(obj))->wd_CycleChain =
	    g_list_append(muiWindowData(_win(obj))->wd_CycleChain,
			  obj);
#endif
    }
}

static void
cleanup_cycle_chain (struct MUI_AreaData *data, Object *obj)
{
    if (_flags(obj) & MADF_CYCLECHAIN
	&& (data->mad_InputMode != MUIV_InputMode_None))
    {
#if 0
	muiWindowData(_win(obj))->wd_CycleChain =
	    g_list_remove(muiWindowData(_win(obj))->wd_CycleChain,
			  obj);
#endif
    }
}


/**************************************************************************
 First method to be called after an OM_NEW, it is the place
 for all initializations depending on the environment, but not
 on the gadget size/position. Matched by MUIM_Cleanup.
**************************************************************************/
static ULONG Area_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    area_update_data(data);
    muiRenderInfo(obj) = msg->RenderInfo;

#warning FIXME: mad_Background
#if 0
/*      g_print("mSetup Area %p, background %p\n", obj, data->mad_Background); */
    zune_imspec_setup(&data->mad_Background, muiRenderInfo(obj));
/*      g_print("mSetup Area %p, background now %p\n", obj, data->mad_Background); */

    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
/*  	g_print("mSetup Area %p, selback %p\n", obj, data->mad_SelBack); */
	zune_imspec_setup(&data->mad_SelBack, muiRenderInfo(obj));
/*  	g_print("mSetup Area %p, selback now %p\n", obj, data->mad_SelBack); */
    }
#endif

    if (data->mad_InputMode != MUIV_InputMode_None)
    {
	DoMethod(_win(obj),
		 MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
    }
    setup_control_char (data, obj, cl);
    setup_cycle_chain (data, obj);

/*  g_print("font before = %p\n", data->mad_Font); */

    if (data->mad_FontPreset == MUIV_Font_Inherit)
    {
	if (_parent(obj) != NULL) data->mad_Font = _font(_parent(obj));
	else data->mad_Font = zune_font_get(MUIV_Font_Normal);
    }
    else
	data->mad_Font = zune_font_get(data->mad_FontPreset);

/*  g_print("font after = %p\n", data->mad_Font); */

#if 0
    if (data->mad_FrameTitle)
    {
	data->mad_TitleText = zune_text_new(NULL, data->mad_FrameTitle,
				    ZTEXT_ARG_NONE, 0);
    }
#endif

    if (data->mad_Flags & MADF_ACTIVE)
    {
	set(_win(obj), MUIA_Window_ActiveObject, obj);
    }
    _flags(obj) |= MADF_SETUP;
    return TRUE;
}


/**************************************************************************
 Called to match a MUIM_Setup, when environment is no more available.
**************************************************************************/
static ULONG Area_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    _flags(obj) &= ~MADF_SETUP;

    /* don't let a pending pointer on us, but be ready to reacquire
     * focus on next setup, if any. This will call GoInactive, that's why
     * we must set the active flag again.
     */
    if (data->mad_Flags & MADF_ACTIVE)
    {
	set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
	data->mad_Flags |= MADF_ACTIVE;
    }

#if 0
    if (data->mad_TitleText)
    {
	zune_text_destroy(data->mad_TitleText);
	data->mad_TitleText = NULL;
    }
#endif

    cleanup_cycle_chain (data, obj);
    cleanup_control_char (data, obj);

    if (data->mad_InputMode != MUIV_InputMode_None)
    {
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
    }

#warning FIXME: mad_Background
#if 0
    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
/*  	g_print("mCleanup Area %p, selback %p\n", obj, data->mad_SelBack); */
	zune_imspec_cleanup(&data->mad_SelBack, muiRenderInfo(obj));
/*  	g_print("mCleanup Area %p, selback now %p\n", obj, data->mad_SelBack); */
    }

/*      g_print("mCleanup Area %p, background %p\n", obj, data->mad_Background); */
    zune_imspec_cleanup(&data->mad_Background, muiRenderInfo(obj));
/*      g_print("mCleanup Area %p, background now %p\n", obj, data->mad_Background); */
#endif

    return TRUE;
}


/**************************************************************************
 Called after the window is open and the area layouted, but before
 any drawing. Matched by one MUIM_Hide.
 Good place to init things depending on gadget size/position.
**************************************************************************/
static ULONG Area_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    Object *activeobj;

    _flags(obj) |= MADF_CANDRAW;
/*  g_print("show %p, bg=%p (%s)\n", obj, data->mad_Background, */
/*  	zune_imspec_to_string(data->mad_Background)); */
/*  g_print("dims=%dx%d\n", _width(obj), _height(obj)); */

/*      g_print("showing %s\n", zune_area_to_string(obj)); */

#warning FIXME: mad_Background
#if 0
    zune_imspec_set_scaled_size(data->mad_Background, _width(obj), _height(obj));
    zune_imspec_show(data->mad_Background, obj);

    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
	zune_imspec_set_scaled_size(data->mad_SelBack, _width(obj), _height(obj));
	zune_imspec_show(data->mad_SelBack, obj);
    }
#endif

    get(_win(obj), MUIA_Window_ActiveObject, &activeobj);

    if (obj == activeobj)
	_zune_focus_new(obj);

    return TRUE;
}

/**************************************************************************
 Called when the window is about to be closed, to match MUIM_Show.
**************************************************************************/
static ULONG Area_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    Object *activeobj;

    get(_win(obj), MUIA_Window_ActiveObject, &activeobj);
    if (obj == activeobj)
	_zune_focus_destroy(obj);

#warning FIXME: mad_Background
#if 0
    zune_imspec_hide(data->mad_Background);
    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
	zune_imspec_hide(data->mad_SelBack);
    }
#endif

    _flags(obj) &= ~MADF_CANDRAW;
    return TRUE;
}


/**************************************************************************
 called by parent between OM_NEW and MUIM_Setup,
 init RenderInfo and GlobalInfo
**************************************************************************/
static ULONG  Area_ConnectParent(struct IClass *cl, Object *obj,
		   struct MUIP_ConnectParent *msg)
{
/*      struct MUI_AreaData *data = INST_DATA(cl, obj); */
    Object *parent = msg->parent;

//    ASSERT(parent != NULL);
//    ASSERT(muiAreaData(parent)->mad_RenderInfo != NULL);
//    ASSERT(muiNotifyData(parent)->mnd_GlobalInfo != NULL);

    _parent(obj) = parent;
    muiAreaData(obj)->mad_RenderInfo = muiAreaData(parent)->mad_RenderInfo;
    muiNotifyData(obj)->mnd_GlobalInfo =
	muiNotifyData(parent)->mnd_GlobalInfo;
    return TRUE;
}



/**************************************************************************
 called by window parent between OM_NEW and MUIM_Setup,
 init RenderInfo and GlobalInfo.
**************************************************************************/
static ULONG Area_ConnectParentWindow(struct IClass *cl, Object *obj,
			 struct MUIP_ConnectParentWindow *msg)
{
/*      struct MUI_AreaData *data = INST_DATA(cl, obj); */
    Object *parent = msg->win;

//  ASSERT(parent != NULL);
//  ASSERT(muiNotifyData(parent)->mnd_GlobalInfo != NULL);

    muiAreaData(obj)->mad_RenderInfo = msg->mri;
    muiNotifyData(obj)->mnd_GlobalInfo =
	muiNotifyData(parent)->mnd_GlobalInfo;
    return TRUE;
}


/**************************************************************************
 called by parent object.
**************************************************************************/
static ULONG Area_DisconnectParent(struct IClass *cl, Object *obj,
		      struct MUIP_DisconnectParent *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    /* don't be active if added elsewhere */
    data->mad_Flags &= ~MADF_ACTIVE;
    data->mad_RenderInfo = NULL;
    muiNotifyData(obj)->mnd_GlobalInfo = NULL;
    _parent(obj) = NULL;
    return TRUE;
}

/**************************************************************************
 Called when gadget activated
**************************************************************************/
static ULONG Area_GoActive(struct IClass *cl, Object *obj, Msg msg)
{
/*      g_print("mGoActive %p\n", obj); */
    if (!(_flags(obj) & MADF_ACTIVE))
    {
	if (_flags(obj) & MADF_CANDRAW)
	    _zune_focus_new(obj);
	_flags(obj) |= MADF_ACTIVE;
    }
    return TRUE;
}

/**************************************************************************
 Called when gadget deactivated
**************************************************************************/
static ULONG Area_GoInactive(struct IClass *cl, Object *obj, Msg msg)
{
/*      g_print("mGoInactive %p\n", obj); */
    if (_flags(obj) & MADF_ACTIVE)
    {
	if (_flags(obj) & MADF_CANDRAW)
	    _zune_focus_destroy(obj);
	_flags(obj) &= ~MADF_ACTIVE;
    }
    return TRUE;
}

/**************************************************************************
 This one or derived methods wont be called if short help is
 not set in area instdata. So set this to a dummy val if overriding
**************************************************************************/
static ULONG Area_CreateShortHelp(struct IClass *cl, Object *obj, struct MUIP_CreateShortHelp *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    return (ULONG)data->mad_ShortHelp;
}

/**************************************************************************
 ...
**************************************************************************/
static ULONG Area_DeleteShortHelp(struct IClass *cl, Object *obj, struct MUIP_DeleteShortHelp *msg)
{
    return TRUE;
}

static void
handle_popupmenu(struct IClass *cl, Object *obj)
{
}

/* either lmb or press key */
static void handle_press(struct IClass *cl, Object *obj)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

/*  g_print("handle press\n"); */
    switch (data->mad_InputMode)
    {
	case MUIV_InputMode_RelVerify:
	    set(obj, MUIA_Timer, ++muiAreaData(obj)->mad_Timeval);
#if defined(_AROS) || defined(_AMIGA)
#warning FIXME: input timeout
#else
	    data->mad_PreTimeout_id =
		g_timeout_add(500, add_intuiticks_client, obj);
/*  	    g_print("pretimer %d installed\n", muiAreaData(obj)->mad_PreTimeout_id); */
#endif
	    set(obj, MUIA_Selected, TRUE);
	    set(obj, MUIA_Pressed, TRUE);
	    break;
	case MUIV_InputMode_Immediate:
	    nnset(obj, MUIA_Selected, FALSE);
	    set(obj, MUIA_Selected, TRUE);
	    break;
	case MUIV_InputMode_Toggle:
	    set(obj, MUIA_Selected, !(data->mad_Flags & MADF_SELECTED));
	    break;
    }

}

/* either lmb or release key */
static void
handle_release(struct IClass *cl, Object *obj)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
/*  g_print("handle release\n"); */

    if (data->mad_InputMode == MUIV_InputMode_RelVerify)
    {
	if (data->mad_Flags & MADF_SELECTED)
	{
	    set(obj, MUIA_Pressed, FALSE);
	    set(obj, MUIA_Selected, FALSE);
	}
#if defined(_AROS) || defined(_AMIGA)
#warning FIXME: input timeout
#else
	if (data->mad_Timeout_id)
	{
	    g_source_remove(data->mad_Timeout_id);
/*  	    g_print("intuiticks %d removed\n", data->mad_Timeout_id); */
	    data->mad_Timeout_id = 0;
	}
	else if (data->mad_PreTimeout_id)
	{
	    g_source_remove(data->mad_PreTimeout_id);
/*  	    g_print("preTimer %d removed\n", data->mad_PreTimeout_id); */
	    data->mad_PreTimeout_id = 0;
	}
#endif
    }
}

#if defined(_AROS) || defined(_AMIGA)

static ULONG
event_button(Class *cl, Object *obj, struct IntuiMessage *imsg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    BOOL in = _between(_left(obj), imsg->MouseX, _right(obj))
           && _between(_top(obj),  imsg->MouseY, _bottom(obj));

    switch (imsg->Code)
    {
    case SELECTDOWN:
        if (in)
        {
            set(_win(obj), MUIA_Window_ActiveObject, obj);
            if ((data->mad_InputMode != MUIV_InputMode_Toggle)
             && (data->mad_Flags & MADF_SELECTED))
                break;
            handle_press(cl, obj);
            if (data->mad_InputMode == MUIV_InputMode_RelVerify)
            {
                DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
                data->mad_ehn.ehn_Events |= IDCMP_MOUSEMOVE;
                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
            }
            return MUI_EventHandlerRC_Eat;
        }
        break;

    case SELECTUP:
        if (data->mad_ehn.ehn_Events & IDCMP_MOUSEMOVE)
        {
            DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
            data->mad_ehn.ehn_Events &= ~IDCMP_MOUSEMOVE;
            DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
            if (!in)
                nnset(obj, MUIA_Pressed, FALSE);
            handle_release(cl, obj);
        }
        break;

    case MENUDOWN:
        if (in)
        {
            set(_win(obj), MUIA_Window_ActiveObject, obj);
            handle_popupmenu(cl, obj);
            return MUI_EventHandlerRC_Eat;
        }
        break;
    }

    return 0;
}

static ULONG
event_motion(Class *cl, Object *obj, struct IntuiMessage *imsg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if (imsg->Qualifier & IEQUALIFIER_LEFTBUTTON)
    {
	if (data->mad_InputMode == MUIV_InputMode_RelVerify)
	{
	    BOOL in = _between(_left(obj), imsg->MouseX, _right(obj))
	           && _between(_top(obj),  imsg->MouseY, _bottom(obj));
	    if (!in && (data->mad_Flags & MADF_SELECTED)) /* going out */
	    {
		set(obj, MUIA_Selected, FALSE);
	    }
	    else if (in && !(data->mad_Flags & MADF_SELECTED)) /* going in */
	    {
		set(obj, MUIA_Selected, TRUE);
#if 0
		/*
		 * going in when no timer has been set yet
		 */
		if (!data->mad_Timeout_id && !data->mad_PreTimeout_id)
		{
		    nnset(obj, MUIA_Timer, 0);
		    set(obj, MUIA_Timer, 1);
		    data->mad_PreTimeout_id =
			g_timeout_add(500, add_intuiticks_client, obj);
		}
#endif
	    }
	}
    }
    else /* move while key pressed -> go to normal */
    {

/* commented because there is no key release event on most hardware,
 * so release has to be done immediately after press.
 * Reuse this if you hack something based on timers, polling key state :)
 * This is a difference with the miga version because Ami has key release
 */
	/* normal draw
	 */
/*  	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->mad_ehn); */
/*  	data->mad_ehn.ehn_Events &= ~GDK_POINTER_MOTION_MASK; */
/*  	DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->mad_ehn); */
/*  	handle_release(cl, obj); */
    }
    return MUI_EventHandlerRC_Eat;
}

#else

static ULONG
event_button_press(struct IClass *cl, Object *obj, GdkEventButton *evb)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

/*  g_print("event_button_press obj %p\n", obj); */
    if (_between(_left(obj), evb->x, _right(obj))
	&& _between(_top(obj), evb->y, _bottom(obj)))
    {
	set(_win(obj), MUIA_Window_ActiveObject, obj);

	switch (evb->button)
	{
	    case 1:
		if (data->mad_InputMode != MUIV_InputMode_Toggle
		    && data->mad_Flags & MADF_SELECTED)
		    break;

		handle_press(cl, obj);
		if (data->mad_InputMode == MUIV_InputMode_RelVerify)
		{
/*  		g_print("catching motions\n"); */
		    DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->mad_ehn);
		    data->mad_ehn.ehn_Events |= GDK_BUTTON1_MOTION_MASK;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->mad_ehn);
		}
		break;

	    case 3:
		handle_popupmenu(cl, obj);
		break;
	}
	return MUI_EventHandlerRC_Eat;
    }
    else
    {
	return 0;
    }
}

static ULONG
event_button_release(struct IClass *cl, Object *obj, GdkEventButton *evb)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    /* this object was grabbing events */
    if (data->mad_ehn.ehn_Events & GDK_BUTTON1_MOTION_MASK)
    {
	if (evb->button == 1)
	{
	    DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->mad_ehn);
	    data->mad_ehn.ehn_Events &= ~GDK_BUTTON1_MOTION_MASK;
	    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->mad_ehn);
	    if (!(_between(_left(obj), evb->x, _right(obj))
		  && _between(_top(obj), evb->y, _bottom(obj))))
		nnset(obj, MUIA_Pressed, FALSE);
	    handle_release(cl, obj);
	}
    }
    return MUI_EventHandlerRC_Eat;
}

static ULONG
event_motion(struct IClass *cl, Object *obj, GdkEventMotion *evm)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if (evm->state & GDK_BUTTON1_MASK)
    {
	if (data->mad_InputMode == MUIV_InputMode_RelVerify)
	{
	    BOOL in = _between(_left(obj), evm->x, _right(obj))
		&& _between(_top(obj), evm->y, _bottom(obj));
	    if (!in && (data->mad_Flags & MADF_SELECTED)) /* going out */
	    {
		set(obj, MUIA_Selected, FALSE);
	    }
	    else if (in && !(data->mad_Flags & MADF_SELECTED)) /* going in */
	    {
		set(obj, MUIA_Selected, TRUE);
		/*
		 * going in when no timer has been set yet
		 */
		if (!data->mad_Timeout_id && !data->mad_PreTimeout_id)
		{
		    nnset(obj, MUIA_Timer, 0);
		    set(obj, MUIA_Timer, 1);
		    data->mad_PreTimeout_id =
			g_timeout_add(500, add_intuiticks_client, obj);
		}
	    }
	}
    }
    else /* move while key pressed -> go to normal */
    {

/* commented because there is no key release event on most hardware,
 * so release has to be done immediately after press.
 * Reuse this if you hack something based on timers, polling key state :)
 * This is a difference with the miga version because Ami has key release
 */
	/* normal draw
	 */
/*  	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->mad_ehn); */
/*  	data->mad_ehn.ehn_Events &= ~GDK_POINTER_MOTION_MASK; */
/*  	DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->mad_ehn); */
/*  	handle_release(cl, obj); */
    }
    return MUI_EventHandlerRC_Eat;
}

#endif

/**************************************************************************
 ...
**************************************************************************/
static ULONG Area_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if (msg->muikey != MUIKEY_NONE)
    {
	switch(msg->muikey)
	{
	    case MUIKEY_NONE:
		break;
	    case MUIKEY_PRESS:
		if (data->mad_Flags & MADF_SELECTED)
		    break;
		handle_press(cl, obj);

/*  	        DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->mad_ehn); */
/*  		data->mad_ehn.ehn_Events |= GDK_POINTER_MOTION_MASK; */
/*  		DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->mad_ehn); */
		return MUI_EventHandlerRC_Eat;
	    case MUIKEY_TOGGLE:
		if (data->mad_InputMode == MUIV_InputMode_Toggle)
		    set(obj, MUIA_Selected, !(data->mad_Flags & MADF_SELECTED));
		return MUI_EventHandlerRC_Eat;
	    case MUIKEY_RELEASE: /* fake, after a MUIKEY_PRESS */
		handle_release(cl, obj);
		return MUI_EventHandlerRC_Eat;
	    default :
		return 0;
	}
    }

    if (msg->imsg)
    {
#if defined(_AROS) || defined (_AMIGA)
	switch (msg->imsg->Class)
	{
	case IDCMP_MOUSEBUTTONS:
	  return event_button(cl, obj, msg->imsg);
	case IDCMP_MOUSEMOVE:
	  return event_motion(cl, obj, msg->imsg);
	}
#else
	switch (msg->imsg->type)
	{
	case GDK_BUTTON_PRESS:
	  return event_button_press(cl, obj, (GdkEventButton *)msg->imsg);
	case GDK_BUTTON_RELEASE:
	  return event_button_release(cl, obj, (GdkEventButton *)msg->imsg);
	case GDK_MOTION_NOTIFY:
	  return event_motion(cl, obj, (GdkEventMotion *)msg->imsg);
	default:
	  return 0;
       }
#endif
    }
    return 0;
}

/**************************************************************************
 Trivial; custom classes may override this to get dynamic menus.
**************************************************************************/
static ULONG Area_ContextMenuBuild(struct IClass *cl, Object *obj, struct MUIP_ContextMenuBuild *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    return (ULONG)data->mad_ContextMenu; /* a MenuStrip object */
}


/**************************************************************************
 MUIM_Export : to export an objects "contents" to a dataspace object.
**************************************************************************/
static ULONG Area_Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    STRPTR id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
#warning FIXME: Export
#if 0
	DoMethod(msg->dataspace, MUIM_Dataspace_AddInt,
		 id, "selected",
		 data->mad_Flags & MADF_SELECTED);
#endif
    }
    return 0;
}


/**************************************************************************
 MUIM_Import : to import an objects "contents" from a dataspace object.
**************************************************************************/
static ULONG Area_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    STRPTR id;
    BOOL val = FALSE;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
#if 0
	DoMethod(msg->dataspace, MUIM_Dataspace_FindInt,
		 id, "selected", &val);
#endif

	if (val)
	    data->mad_Flags |= MADF_SELECTED;
	else
	    data->mad_Flags &= ~MADF_SELECTED;
    }
    return 0;
}


/*
 * Calculates addleft, addtop, subwidth, subheight from current settings.
 * If frame phantom, ignore horizontal frame components.
 *
 * Because of BYTE storage, all values are clamped to 0..127
 * Inner dimensions being clamped to 0..32, it shouldnt cause too much harm
 */
static void area_update_data(struct MUI_AreaData *data)
{
    struct ZuneFrameGfx *zframe;

    zframe = zune_zframe_get (&__zprefs.frames[data->mad_Frame]);

    if (data->mad_Flags & MADF_FRAMEPHANTOM)
    {
	data->mad_addleft = ((data->mad_Flags & MADF_INNERLEFT) ?
			     data->mad_HardILeft : 0);

	data->mad_subwidth = data->mad_addleft +
	    ((data->mad_Flags & MADF_INNERRIGHT) ? data->mad_HardIRight : 0);
    }
    else
    {
	if (data->mad_Flags & MADF_INNERLEFT)
	    data->mad_addleft =
		CLAMP(data->mad_HardILeft + zframe->xthickness, 0, 127);
	else
	    data->mad_addleft =
		CLAMP(__zprefs.frames[data->mad_Frame].innerLeft
		      + zframe->xthickness, 0, 127);

	if (data->mad_Flags & MADF_INNERRIGHT)
	    data->mad_subwidth =
		CLAMP(data->mad_addleft + data->mad_HardIRight
		      + zframe->xthickness, 0, 127);
	else
	    data->mad_subwidth =
		CLAMP(data->mad_addleft
		      + __zprefs.frames[data->mad_Frame].innerRight
		      + zframe->xthickness, 0, 127);
    }

    if (data->mad_Flags & MADF_INNERTOP)
	data->mad_addtop = CLAMP(data->mad_HardITop + zframe->ythickness,
				 0, 127);
    else
	data->mad_addtop =
	    CLAMP(__zprefs.frames[data->mad_Frame].innerTop
		  + zframe->ythickness, 0, 127);

    if (data->mad_Flags & MADF_INNERBOTTOM)
	data->mad_subheight =
	    CLAMP(data->mad_addtop + data->mad_HardIBottom
		  + zframe->ythickness, 0, 127);
    else
	data->mad_subheight =
	    CLAMP(data->mad_addtop
		  + __zprefs.frames[data->mad_Frame].innerBottom
		  + zframe->ythickness, 0, 127);
}

#ifndef _AROS
__asm IPTR Area_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Area_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW: return Area_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Area_Dispose(cl, obj, msg);
	case OM_SET: return Area_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Area_Get(cl, obj, (struct opGet *)msg);
	case MUIM_AskMinMax: return Area_AskMinMax(cl, obj, (APTR)msg);
	case MUIM_Draw: return Area_Draw(cl, obj, (APTR)msg);
	case MUIM_DrawBackground: return Area_DrawBackground(cl, obj, (APTR)msg);
	case MUIM_Setup: return Area_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Area_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Show: return Area_Show(cl, obj, (APTR)msg);
	case MUIM_Hide: return Area_Hide(cl, obj, (APTR)msg);
	case MUIM_ConnectParent: return Area_ConnectParent(cl, obj, (APTR)msg);
	case MUIM_ConnectParentWindow: return Area_ConnectParentWindow(cl, obj, (APTR)msg);
	case MUIM_DisconnectParent: return Area_DisconnectParent(cl, obj, (APTR)msg);
	case MUIM_GoActive: return Area_GoActive(cl, obj, (APTR)msg);
	case MUIM_GoInactive: return Area_GoInactive(cl, obj, (APTR)msg);
	case MUIM_Layout: return 1;
	case MUIM_CreateShortHelp: return Area_CreateShortHelp(cl, obj, (APTR)msg);
	case MUIM_DeleteShortHelp: return Area_DeleteShortHelp(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Area_HandleEvent(cl, obj, (APTR)msg);
	case MUIM_ContextMenuBuild: return Area_ContextMenuBuild(cl, obj, (APTR)msg);

	case MUIM_Export: return Area_Export(cl, obj, (APTR)msg);
	case MUIM_Import: return Area_Import(cl, obj, (APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Area_desc = { 
    MUIC_Area, 
    MUIC_Notify, 
    sizeof(struct MUI_AreaData), 
    Area_Dispatcher 
};

