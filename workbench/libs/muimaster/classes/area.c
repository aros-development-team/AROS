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
#include <graphics/gfxmacros.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

extern struct Library *MUIMasterBase;
#include "muimaster_intern.h"

#include "mui.h"
#include "support.h"
#include "imspec.h"
#include "menu.h"

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
#define g_free(x) free(x)
#endif

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
Area.mui/MUIA_ContextMenu           done
Area.mui/MUIA_ContextMenuTrigger
Area.mui/MUIA_ControlChar           done
Area.mui/MUIA_CycleChain            done
Area.mui/MUIA_Disabled
Area.mui/MUIA_Draggable             done
Area.mui/MUIA_Dropable              done
Area.mui/MUIA_ExportID
Area.mui/MUIA_FillArea              done
Area.mui/MUIA_FixHeight             done
Area.mui/MUIA_FixHeightTxt          need text/font
Area.mui/MUIA_FixWidth              done
Area.mui/MUIA_FixWidthTxt           need text/font
Area.mui/MUIA_Font                  done
Area.mui/MUIA_Frame                 done
Area.mui/MUIA_FramePhantomHoriz     done
Area.mui/MUIA_FrameTitle            done
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
Area.mui/MUIA_ShowSelState          done (I only)
Area.mui/MUIA_Timer                 done
Area.mui/MUIA_TopEdge               done
Area.mui/MUIA_VertDisappear
Area.mui/MUIA_VertWeight            done
Area.mui/MUIA_Weight                done
Area.mui/MUIA_Width                 done
Area.mui/MUIA_Window                done
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
Area.mui/MUIM_Draw                  done
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

//static void setup_cycle_chain (struct MUI_AreaData *data, Object *obj);
//static void cleanup_cycle_chain (struct MUI_AreaData *data, Object *obj);


static void _zune_focus_new(Object *obj, int type)
{
    Object *parent = _parent(obj);
    struct RastPort *rp = _rp(obj);
    UWORD oldDrPt = rp->LinePtrn;

    int x1 = _left(obj) - 1;
    int y1 = _top(obj)  - 1;
    int x2 = _left(obj) + _width(obj);
    int y2 = _top(obj)  + _height(obj);

    if (!parent || parent == _win(obj)) return;

    SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], _pens(obj)[MPEN_SHADOW], JAM2);

    if (!type) SetDrPt(rp, 0xCCCC);
    else SetDrPt(rp,0x5555);

    Move(rp, x1, y1);
    Draw(rp, x2, y1);
    Draw(rp, x2, y2);
    Draw(rp, x1, y2);
    Draw(rp, x1, y1);
    SetDrPt(rp, oldDrPt);
}

static void _zune_focus_destroy(Object *obj)
{
    Object *parent = _parent(obj);

    int x1 = _left(obj) - 1;
    int y1 = _top(obj)  - 1;
    int x2 = _left(obj) + _width(obj);
    int y2 = _top(obj)  + _height(obj);
    int width = x2 - x1 + 1;
    int height = y2 - y1 + 1;

    if (!parent || parent == _win(obj)) return;

    DoMethod(parent, MUIM_DrawBackground, x1, y1, width, 1, 0, 0, 0);
    DoMethod(parent, MUIM_DrawBackground, x2, y1, 1, height, 0, 0, 0);
    DoMethod(parent, MUIM_DrawBackground, x1, y2, width, 1, 0, 0, 0);
    DoMethod(parent, MUIM_DrawBackground, x1, y1, 1, height, 0, 0, 0);
}


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
    data->mad_Background = (struct MUI_ImageSpec *)-1;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Background:
		data->mad_Background = (struct MUI_ImageSpec *)tag->ti_Data;
		break;
	    case MUIA_ControlChar:
		data->mad_ControlChar = tag->ti_Data;
		break;
	    case MUIA_CycleChain:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_CYCLECHAIN);
		break;
	    case MUIA_FillArea:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_FILLAREA);
		break;
	    case MUIA_Draggable:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_DRAGGABLE);
		break;
	    case MUIA_Dropable:
	        _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_DROPABLE);
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
	    case MUIA_ContextMenu:
		data->mad_ContextMenu = (Object*)tag->ti_Data;
		break;
	}
    }

    if (data->mad_Background != (struct MUI_ImageSpec *)-1) /* own bg ? */
	data->mad_Background = zune_image_spec_to_structure((ULONG)data->mad_Background);
    else data->mad_Background = NULL; /* will be filled by parent */

    if ((data->mad_Flags & MADF_SHOWSELSTATE) && (data->mad_InputMode != MUIV_InputMode_None))
	data->mad_SelBack = zune_image_spec_to_structure(MUII_SelectedBack);

    if ((data->mad_Frame != 0) && (data->mad_FrameTitle))
    {
    	char *frame_title = mui_alloc(strlen(data->mad_FrameTitle)+1);
    	if (frame_title) strcpy(frame_title,data->mad_FrameTitle);
    	data->mad_FrameTitle = frame_title;
    }

    data->mad_ehn.ehn_Events = 0; /* Will be filled on demand */
    data->mad_ehn.ehn_Priority = 0;
    data->mad_ehn.ehn_Flags    = 0;
    data->mad_ehn.ehn_Object   = obj;
    data->mad_ehn.ehn_Class    = cl;

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

    if (data->mad_Background)
	zune_imspec_free(data->mad_Background);

    if (data->mad_SelBack)
	zune_imspec_free(data->mad_SelBack);

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

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
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

	    case MUIA_ControlChar:
		if (_flags(obj) & MADF_SETUP)
		    cleanup_control_char(data, obj);
		data->mad_ControlChar = tag->ti_Data;
		if (_flags(obj) & MADF_SETUP)
		    setup_control_char(data, obj, cl);
		break;

	    case MUIA_CycleChain:
//		if (data->mad_InputMode == MUIV_InputMode_None)
//		    break;

		if ((!(_flags(obj) & MADF_CYCLECHAIN) && tag->ti_Data)
		    || ((_flags(obj) & MADF_CYCLECHAIN) && !tag->ti_Data))
		{
		    if (_flags(obj) & MADF_SETUP)
		    {
		    	cleanup_control_char(data,obj);
			_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_CYCLECHAIN);
			setup_control_char(data,obj,cl);
		    }   else _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_CYCLECHAIN);
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
		{
		    Object *parent;
		    Object *win;
		    get(obj, MUIA_Parent, &parent);
		    win = _win(obj);

		    if (tag->ti_Data) data->mad_Flags |= MADF_SHOWME;
		    else data->mad_Flags &= ~MADF_SHOWME;

		    if (parent == win || (parent && (_flags(parent)&MADF_CANDRAW)))
			DoMethod(_win(obj), MUIM_Window_RecalcDisplay);
		}
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
		break;

	    case MUIA_VertWeight:
		data->mad_VertWeight = tag->ti_Data;
		break;

	    case MUIA_Draggable:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_DRAGGABLE);
		break;
	    case MUIA_Dropable:
	        _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_DROPABLE);
	        break;
	    case MUIA_ContextMenu:
		data->mad_ContextMenu = (Object*)tag->ti_Data;
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
	case MUIA_ContextMenu:
	    STORE = (ULONG)data->mad_ContextMenu;
	    return 1;
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
	/* Save the orginal font */
	struct TextFont *obj_font = _font(obj);

	_font(obj) = zune_font_get(MUIV_Font_Title);
	zune_text_get_bounds(data->mad_TitleText, obj);

        /* restore the font */
	_font(obj) = obj_font;

	_subheight(obj) = _subheight(obj) - _addtop(obj) + data->mad_TitleText->height + 1;
	_addtop(obj) = data->mad_TitleText->height + 1;

	if (__zprefs.group_title_color == GROUP_TITLE_COLOR_3D)
	{
	    _subheight(obj) += 1;
	    _addtop(obj) += 1;
	}
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

    D(bug("muimaster.library/area.c: Draw Area Object at 0x%lx %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj)));

    if (msg->flags & MADF_DRAWALL)
	msg->flags |= MADF_DRAWOBJECT;

    if (!(msg->flags & MADF_DRAWOBJECT))
    {
	/* dont draw bg/frame, let subclass redraw content only
	** Don't know if MUI handles it like this, or if it
	** simply return without setting MADF_DRAWOBJECT
	**/
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

    /* Background drawing */
    if (data->mad_Flags & MADF_FILLAREA)
    {
	struct MUI_ImageSpec *background;

	if (!(data->mad_Flags & MADF_SELECTED)) background = data->mad_Background;
	else background = data->mad_SelBack;

	/* RECHECK: sba: Orginally there was muiRenderInfo(obj)->mri_ClipRect.XXX used
        ** but this didn't worked, if there are some background problems recheck this
	*/

	if (data->mad_TitleText)
	{
	    zune_draw_image(data->mad_RenderInfo, background,
			    _left(obj), _top(obj), _width(obj), _height(obj),
			    _left(obj), _top(obj), 0);

/*	    int y = MAX(muiRenderInfo(obj)->mri_ClipRect.MinY,
			_top(obj) + zframe->ythickness
			+ data->mad_TitleText->height / 2);


	    zune_draw_image(data->mad_RenderInfo, background,
			    _left(obj), y, _width(obj), _height(obj) -  y),
			    _left(obj), _top(obj) + data->mad_TitleText->height / 2, 0);
*/
	}
	else
	{
	    zune_draw_image(data->mad_RenderInfo, background,
                           _left(obj),_top(obj),_width(obj),_height(obj),
			    _left(obj), _top(obj), 0);
	}
    }


    /* Frame drawing */
    if (!(data->mad_Flags & MADF_FRAMEPHANTOM))
    {
	int state = __zprefs.frames[data->mad_Frame].state;
	if (data->mad_Flags & MADF_SELECTED)
	    state ^= 1;

	/* set clipping so that frame is not drawn behind title */

	if (data->mad_TitleText)
	{
	    APTR textdrawclip;

	    struct TextFont *obj_font = _font(obj);
	    struct Region *region;

	    int x;
	    int width;

	    width = data->mad_TitleText->width;
	    if (__zprefs.group_title_color == GROUP_TITLE_COLOR_3D)
		width += 1;

            switch (__zprefs.group_title_position)
            {
		case GROUP_TITLE_POSITION_RIGHT: x = _mright(obj) - width - 3;  break;
		case GROUP_TITLE_POSITION_CENTERED: x = _mleft(obj) + (_mwidth(obj) - width) / 2; break;
		default: x = _mleft(obj) + 2; /* additional space */
	    }

            if (x < _mleft(obj) + 2) x = _mleft(obj) + 2;

	    if ((region = NewRegion()))
	    {
	    	struct Rectangle rect;
	    	rect.MinX = x - 2;
	    	rect.MinY = _top(obj);
	    	rect.MaxX = MIN(_mright(obj),x + width + 3);
	    	rect.MaxY = rect.MinY + data->mad_TitleText->height - 1; // frame is not thick enough anywhy
		OrRectRegion(region,&rect);

	    	rect.MinX = _left(obj);
	    	rect.MinY = _top(obj);
	    	rect.MaxX = _right(obj);
	    	rect.MaxY = _bottom(obj);
		XorRectRegion(region,&rect);

		textdrawclip = MUI_AddClipRegion(muiRenderInfo(obj),region);
	    }
	    
	    zframe->draw[state](muiRenderInfo(obj), _left(obj),
				_top(obj) + data->mad_TitleText->height / 2,
				_width(obj),
				_height(obj) - data->mad_TitleText->height / 2);

	    if (region)
	    {
	    	MUI_RemoveClipRegion(muiRenderInfo(obj),textdrawclip);
/*		DisposeRegion(region);*/ /* sba: DisposeRegion happens in MUI_RemoveClipRegion, this seems wrong to me */
	    }


	    /* Title text drawing */
	    _font(obj) = zune_font_get(MUIV_Font_Title);

            /* TODO: sba if a TextFit() for zune text is available one could disable the clipping */
	    textdrawclip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj) + 2, _top(obj),
					   _mwidth(obj) - 4, data->mad_TitleText->height);

	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	    if (__zprefs.group_title_color == GROUP_TITLE_COLOR_3D)
	    {
		zune_text_draw(data->mad_TitleText, obj, x + 1, x + width, _top(obj) + 1);
		SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
		zune_text_draw(data->mad_TitleText, obj, x, x + width - 1, _top(obj));
	    }
	    else if (__zprefs.group_title_color == GROUP_TITLE_COLOR_WHITE)
		SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);

	    if (__zprefs.group_title_color != GROUP_TITLE_COLOR_3D)
	    {
		x++;
		zune_text_draw(data->mad_TitleText, obj, x, x + width - 1, _top(obj));
	    }
	    MUI_RemoveClipping(muiRenderInfo(obj), textdrawclip);

	    _font(obj) = obj_font;
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
 MUIM_DrawBackground
**************************************************************************/
static ULONG Area_DrawBackground(struct IClass *cl, Object *obj, struct MUIP_DrawBackground *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    if (!(data->mad_Flags & MADF_CANDRAW)) /* not between show/hide */
	return FALSE;

    zune_draw_image(data->mad_RenderInfo, data->mad_Background,
		    msg->left, msg->top, msg->width, msg->height,
		    msg->xoffset, msg->yoffset, 0);

    return TRUE;
}

/* Perverting the EventHandlerNode structure to specify a shortcut.
 */
static void setup_control_char (struct MUI_AreaData *data, Object *obj, struct IClass *cl)
{
/*    if (data->mad_InputMode != MUIV_InputMode_None) */ /* needed to be commented, because it checks also for the controlchar */
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


static void cleanup_control_char (struct MUI_AreaData *data, Object *obj)
{
    if (data->mad_InputMode != MUIV_InputMode_None)
    {
	DoMethod(_win(obj),
		 MUIM_Window_RemControlCharHandler, (IPTR)&data->mad_ccn);
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

    muiRenderInfo(obj) = msg->RenderInfo;

    area_update_data(data);

    zune_imspec_setup(&data->mad_Background, muiRenderInfo(obj));

    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
	zune_imspec_setup(&data->mad_SelBack, muiRenderInfo(obj));
    }

    if (data->mad_InputMode != MUIV_InputMode_None || data->mad_ContextMenu)
    {
	data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
    }
    setup_control_char (data, obj, cl);
//    setup_cycle_chain (data, obj);

    if (data->mad_FontPreset == MUIV_Font_Inherit)
    {
	if (_parent(obj) != NULL && _parent(obj) != _win(obj)) data->mad_Font = _font(_parent(obj));
	else data->mad_Font = zune_font_get(MUIV_Font_Normal);
    }
    else data->mad_Font = zune_font_get(data->mad_FontPreset);

    if (data->mad_FrameTitle)
    {
	data->mad_TitleText = zune_text_new(NULL, data->mad_FrameTitle,
				    ZTEXT_ARG_NONE, 0);
    }

    if (data->mad_Flags & MADF_ACTIVE)
    {
	set(_win(obj), MUIA_Window_ActiveObject, obj);
    }
    _flags(obj) |= MADF_SETUP;

    data->mad_Timer.ihn_Flags = MUIIHNF_TIMER;
    data->mad_Timer.ihn_Method = MUIM_Timer;
    data->mad_Timer.ihn_Object = obj;

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

    if (data->mad_TitleText)
    {
	zune_text_destroy(data->mad_TitleText);
	data->mad_TitleText = NULL;
    }

//    cleanup_cycle_chain (data, obj);
    cleanup_control_char (data, obj);

    if (data->mad_Timer.ihn_Millis)
    {
	DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->mad_Timer);
	data->mad_Timer.ihn_Millis = 0;
    }

    /* Remove the event handler if it has been added */
    if (data->mad_ehn.ehn_Events)
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);

    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
	zune_imspec_cleanup(&data->mad_SelBack, muiRenderInfo(obj));
    }

    zune_imspec_cleanup(&data->mad_Background, muiRenderInfo(obj));

    muiRenderInfo(obj) = NULL;
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

    zune_imspec_set_scaled_size(data->mad_Background, _width(obj), _height(obj));
    zune_imspec_show(data->mad_Background, obj);

    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
	zune_imspec_set_scaled_size(data->mad_SelBack, _width(obj), _height(obj));
	zune_imspec_show(data->mad_SelBack, obj);
    }

    get(_win(obj), MUIA_Window_ActiveObject, &activeobj);

    if (obj == activeobj)
	_zune_focus_new(obj,0);

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

    zune_imspec_hide(data->mad_Background);
    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
	zune_imspec_hide(data->mad_SelBack);
    }

    if (data->mad_ContextZMenu)
    {
	zune_close_menu(data->mad_ContextZMenu);
	data->mad_ContextZMenu = NULL;
    }

    _flags(obj) &= ~MADF_CANDRAW;
    return TRUE;
}


/**************************************************************************
 called by parent object.
**************************************************************************/
static ULONG Area_DisconnectParent(struct IClass *cl, Object *obj, struct MUIP_DisconnectParent *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    /* don't be active if added elsewhere */
    data->mad_Flags &= ~MADF_ACTIVE;
    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/**************************************************************************
 Called when gadget activated
**************************************************************************/
static ULONG Area_GoActive(struct IClass *cl, Object *obj, Msg msg)
{
    if (!(_flags(obj) & MADF_ACTIVE))
    {
	if (_flags(obj) & MADF_CANDRAW)
	    _zune_focus_new(obj,0);

	_flags(obj) |= MADF_ACTIVE;
    }
    return TRUE;
}

/**************************************************************************
 Called when gadget deactivated
**************************************************************************/
static ULONG Area_GoInactive(struct IClass *cl, Object *obj, Msg msg)
{
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

/* either lmb or press key */
static void handle_press(struct IClass *cl, Object *obj)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    switch (data->mad_InputMode)
    {
	case MUIV_InputMode_RelVerify:
	    set(obj, MUIA_Timer, ++muiAreaData(obj)->mad_Timeval);
	    if (!data->mad_Timer.ihn_Millis)
	    {
	    	data->mad_Timer.ihn_Millis = 150;
		DoMethod(_app(obj), MUIM_Application_AddInputHandler, &data->mad_Timer);
	    }
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
static void handle_release(struct IClass *cl, Object *obj)
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
    }

    if (data->mad_Timer.ihn_Millis)
    {
	DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->mad_Timer);
    	data->mad_Timer.ihn_Millis = 0;
    }

}

static ULONG event_button(Class *cl, Object *obj, struct IntuiMessage *imsg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    BOOL in = _between(_left(obj), imsg->MouseX, _right(obj))
           && _between(_top(obj),  imsg->MouseY, _bottom(obj));

    switch (imsg->Code)
    {
	case	SELECTDOWN:
		if (in)
		{
//		    set(_win(obj), MUIA_Window_ActiveObject, obj);
		    data->mad_ClickX = imsg->MouseX;
		    data->mad_ClickY = imsg->MouseY;
		    if ((data->mad_InputMode != MUIV_InputMode_Toggle) && (data->mad_Flags & MADF_SELECTED))
			break;
		    nnset(obj,MUIA_Timer,0);
		    handle_press(cl, obj);
		    if (data->mad_InputMode == MUIV_InputMode_RelVerify)
		    {
			if (data->mad_ehn.ehn_Events) DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
			data->mad_ehn.ehn_Events |= IDCMP_MOUSEMOVE | IDCMP_RAWKEY;
	                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
	            }
		    return MUI_EventHandlerRC_Eat;
		}

	case	SELECTUP:
		if (data->mad_ehn.ehn_Events != IDCMP_MOUSEBUTTONS)
		{
		    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
	            data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
		    if (!in) nnset(obj, MUIA_Pressed, FALSE);
		    handle_release(cl, obj);
		    return MUI_EventHandlerRC_Eat;
		}
		break;

	case    MENUDOWN:
		if (in && data->mad_ContextMenu)
		{
		    Object *menuobj = (Object*)DoMethod(obj, MUIM_ContextMenuBuild, imsg->MouseX, imsg->MouseY);
		    if (menuobj)
		    {
			struct NewMenu *newmenu;
			get(menuobj,MUIA_Menuitem_NewMenu,&newmenu);
			if (newmenu)
			{
			    if (data->mad_ContextZMenu) zune_close_menu(data->mad_ContextZMenu);
			    data->mad_ContextZMenu = zune_open_menu(_window(obj),newmenu);
			}

			if (data->mad_ehn.ehn_Events) DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
			data->mad_ehn.ehn_Events |= IDCMP_MOUSEMOVE;
	                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
		    }
	            return MUI_EventHandlerRC_Eat;
        	}
	        break;

	case    MENUUP:
		if (data->mad_ContextZMenu)
		{
		    zune_close_menu(data->mad_ContextZMenu);
		    data->mad_ContextZMenu = NULL;
		    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
	            data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
	            return MUI_EventHandlerRC_Eat;
		}
		break;
    }

    return 0;
}

static ULONG event_motion(Class *cl, Object *obj, struct IntuiMessage *imsg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if ((imsg->Qualifier & IEQUALIFIER_RBUTTON) && data->mad_ContextZMenu)
    {
	zune_mouse_update(data->mad_ContextZMenu, 0);
	return MUI_EventHandlerRC_Eat;
    }

    if (imsg->Qualifier & IEQUALIFIER_LEFTBUTTON)
    {
	BOOL in = _between(_left(obj), imsg->MouseX, _right(obj))
	           && _between(_top(obj),  imsg->MouseY, _bottom(obj));

	if (in)
	{
	    if ((data->mad_Flags & MADF_DRAGGABLE) && ((abs(data->mad_ClickX-imsg->MouseX) >= 3) || (abs(data->mad_ClickY-imsg->MouseY)>=3))) /* should be user configurable */
	    {
		if (data->mad_InputMode == MUIV_InputMode_RelVerify)
		    set(obj, MUIA_Selected, FALSE);
		nnset(obj, MUIA_Pressed, FALSE);

		if (data->mad_ehn.ehn_Events) DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->mad_ehn);
		data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->mad_ehn);
		if (data->mad_Timer.ihn_Millis)
		{
		   DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->mad_Timer);
		   data->mad_Timer.ihn_Millis = 0;
		}

	    	DoMethod(obj,MUIM_DoDrag, data->mad_ClickX - _left(obj), data->mad_ClickY - _top(obj), 0);
		return MUI_EventHandlerRC_Eat;
	    }
	}

	if (data->mad_InputMode == MUIV_InputMode_RelVerify)
	{
	    if (!in && (data->mad_Flags & MADF_SELECTED)) /* going out */
	    {
		set(obj, MUIA_Selected, FALSE);
	    }
	    else if (in && !(data->mad_Flags & MADF_SELECTED)) /* going in */
	    {
	        set(obj, MUIA_Selected, TRUE);
	    }
	}
    }
    return MUI_EventHandlerRC_Eat;
}

/**************************************************************************
 ...
**************************************************************************/
static ULONG Area_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if (data->mad_InputMode == MUIV_InputMode_None && !data->mad_ContextMenu) return 0;

    if (msg->muikey != MUIKEY_NONE)
    {
	switch (msg->muikey)
	{
	    case    MUIKEY_PRESS:
		    if (data->mad_Flags & MADF_SELECTED) break;
		    handle_press(cl, obj);
		    if (data->mad_ehn.ehn_Events) DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->mad_ehn);
		    data->mad_ehn.ehn_Events |= IDCMP_RAWKEY;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->mad_ehn);
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_TOGGLE:
		    if (data->mad_InputMode == MUIV_InputMode_Toggle) set(obj, MUIA_Selected, !(data->mad_Flags & MADF_SELECTED));
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_RELEASE:
		    handle_release(cl, obj);
		    if (data->mad_ehn.ehn_Events) DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->mad_ehn);
		    data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->mad_ehn);
		    return MUI_EventHandlerRC_Eat;
	}
	return 0;
    }

    if (msg->imsg)
    {
	switch (msg->imsg->Class)
	{
	    case IDCMP_MOUSEBUTTONS: return event_button(cl, obj, msg->imsg);
	    case IDCMP_MOUSEMOVE: return event_motion(cl, obj, msg->imsg);
	    case IDCMP_RAWKEY:
	    {
	        if (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
	        {
		    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
		    data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
		    nnset(obj, MUIA_Pressed, FALSE); /* aborted */
		    handle_release(cl,obj);
		}
		return MUI_EventHandlerRC_Eat;
	    }
	    break;
	}
    }
    return 0;
}

/**************************************************************************
 ...
**************************************************************************/
static ULONG Area_HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleInput *msg)
{
    /* Actually a dummy, but real MUI does handle here the input stuff which Zune
    ** has in Area_HandleEvent. For compatibility we should do this too
    **/
    return 0;
}

/**************************************************************************
 Trivial; custom classes may override this to get dynamic menus.
**************************************************************************/
static ULONG Area_ContextMenuBuild(struct IClass *cl, Object *obj, struct MUIP_ContextMenuBuild *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    return (ULONG)data->mad_ContextMenu; /* a Menustrip object */
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
    	char selected = (data->mad_Flags & MADF_SELECTED)?1:0;
	DoMethod(msg->dataspace, MUIM_Dataspace_Add, &selected, sizeof(char),id);
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
    	char *selected = (char*)DoMethod(msg->dataspace, MUIM_Dataspace_Find, id);

	if (selected)
	{
	    if (*selected) data->mad_Flags |= MADF_SELECTED;
	    else data->mad_Flags &= ~MADF_SELECTED;
	}
    }
    return 0;
}

/**************************************************************************
 MUIM_Timer
**************************************************************************/
static ULONG Area_Timer(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    if (data->mad_Timer.ihn_Millis)
	DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->mad_Timer);
    data->mad_Timer.ihn_Millis = 50;
    DoMethod(_app(obj), MUIM_Application_AddInputHandler, &data->mad_Timer);

    if (data->mad_Flags & MADF_SELECTED)
	set(obj, MUIA_Timer, ++muiAreaData(obj)->mad_Timeval);
    return 0;
}

/**************************************************************************
 MUIM_DoDrag
**************************************************************************/
static ULONG Area_DoDrag(struct IClass *cl, Object *obj, struct MUIP_DoDrag *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    DoMethod(_win(obj), MUIM_Window_DragObject, obj, msg->touchx, msg->touchy, msg->flags);
    return 0;
}

/**************************************************************************
 MUIM_CreateDragImage
**************************************************************************/
static ULONG Area_CreateDragImage(struct IClass *cl, Object *obj, struct MUIP_CreateDragImage *msg)
{
    struct MUI_DragImage *img = (struct MUI_DragImage *)AllocVec(sizeof(struct MUIP_CreateDragImage),MEMF_CLEAR);
    if (img)
    {
    	struct ZuneFrameGfx *zframe;
	LONG depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap,BMA_DEPTH);

	zframe = zune_zframe_get(&__zprefs.frames[MUIV_Frame_Drag]);

    	img->width = _width(obj) + 2*zframe->xthickness;
    	img->height = _height(obj) + 2*zframe->ythickness;

    	if ((img->bm = AllocBitMap(img->width,img->height,depth,BMF_MINPLANES,_screen(obj)->RastPort.BitMap)))
    	{
    	    /* Render the stuff now */
    	    struct RastPort *rp_save = muiRenderInfo(obj)->mri_RastPort;
    	    struct RastPort temprp;
    	    InitRastPort(&temprp);
    	    temprp.BitMap = img->bm;
    	    ClipBlit(_rp(obj),_left(obj),_top(obj),&temprp,zframe->xthickness,zframe->ythickness,_width(obj),_height(obj),0xc0);

	    muiRenderInfo(obj)->mri_RastPort = &temprp;
	    zframe->draw[0](muiRenderInfo(obj), 0, 0, img->width, img->height);
	    muiRenderInfo(obj)->mri_RastPort = rp_save;
    	}

    	img->touchx = msg->touchx;
    	img->touchy = msg->touchy;
    	img->flags = 0;
    }
    return (ULONG)img;
}

/**************************************************************************
 MUIM_DeleteDragImage
**************************************************************************/
static ULONG Area_DeleteDragImage(struct IClass *cl, Object *obj, struct MUIP_DeleteDragImage *msg)
{
    if (msg->di)
    {
	if (msg->di->bm) FreeBitMap(msg->di->bm);
	FreeVec(msg->di);
    }
    return NULL;
}

/**************************************************************************
 MUIM_DragQueryExtended
**************************************************************************/
static IPTR Area_DragQueryExtended(struct IClass *cl, Object *obj, struct MUIP_DragQueryExtended *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    if (data->mad_Flags & MADF_DROPABLE)
    {
	if (_left(obj) <= msg->x && msg->x <= _right(obj) && _top(obj) <= msg->y && msg->y <= _bottom(obj))
	{
	    if (DoMethod(obj,MUIM_DragQuery,msg->obj) == MUIV_DragQuery_Accept)
		return (IPTR)obj;
	}
    }
    return NULL;
}

/**************************************************************************
 MUIM_DragBegin
**************************************************************************/
ULONG Area_DragBegin(struct IClass *cl, Object *obj, struct MUIP_DragBegin *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    _zune_focus_new(obj,1);
    return 0;
}

/**************************************************************************
 MUIM_DragFinish
**************************************************************************/
ULONG Area_DragFinish(struct IClass *cl, Object *obj, struct MUIP_DragFinish *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    _zune_focus_destroy(obj);
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
	case MUIM_DisconnectParent: return Area_DisconnectParent(cl, obj, (APTR)msg);
	case MUIM_GoActive: return Area_GoActive(cl, obj, (APTR)msg);
	case MUIM_GoInactive: return Area_GoInactive(cl, obj, (APTR)msg);
	case MUIM_Layout: return 1;
	case MUIM_CreateShortHelp: return Area_CreateShortHelp(cl, obj, (APTR)msg);
	case MUIM_DeleteShortHelp: return Area_DeleteShortHelp(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Area_HandleEvent(cl, obj, (APTR)msg);
	case MUIM_ContextMenuBuild: return Area_ContextMenuBuild(cl, obj, (APTR)msg);
	case MUIM_Timer: return Area_Timer(cl,obj,msg);
	case MUIM_DragQuery: return MUIV_DragQuery_Refuse;
	case MUIM_DragBegin: return Area_DragBegin(cl,obj,(APTR)msg);
	case MUIM_DragDrop: return FALSE;
	case MUIM_DragFinish: return Area_DragFinish(cl,obj,(APTR)msg);
	case MUIM_DragReport: return MUIV_DragReport_Continue; /* or MUIV_DragReport_Abort? */
	case MUIM_DoDrag: return Area_DoDrag(cl, obj, (APTR)msg);
	case MUIM_CreateDragImage: return Area_CreateDragImage(cl, obj, (APTR)msg);
	case MUIM_DeleteDragImage: return Area_DeleteDragImage(cl, obj, (APTR)msg);
	case MUIM_DragQueryExtended: return Area_DragQueryExtended(cl, obj, (APTR)msg);
	case MUIM_HandleInput: return Area_HandleInput(cl, obj, (APTR)msg);

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

