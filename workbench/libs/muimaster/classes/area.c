/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <graphics/gfxmacros.h>
#include <intuition/imageclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

extern struct Library *MUIMasterBase;
#include "muimaster_intern.h"

#include "mui.h"
#include "support.h"
#include "imspec.h"
#include "menu.h"
#include "prefs.h"
#include "font.h"
#include "textengine.h"
#include "bubbleengine.h"

//#define MYDEBUG 1
#include "debug.h"

/*
Area.mui/MUIA_Background            done
Area.mui/MUIA_BottomEdge            done
Area.mui/MUIA_ContextMenu           done
Area.mui/MUIA_ContextMenuTrigger
Area.mui/MUIA_ControlChar           done
Area.mui/MUIA_CycleChain            done
Area.mui/MUIA_Disabled              done
Area.mui/MUIA_Draggable             done
Area.mui/MUIA_Dropable              done
Area.mui/MUIA_ExportID
Area.mui/MUIA_FillArea              done
Area.mui/MUIA_FixHeight             done
Area.mui/MUIA_FixHeightTxt          done
Area.mui/MUIA_FixWidth              done
Area.mui/MUIA_FixWidthTxt           done
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
Area.mui/MUIM_DragDrop              done
Area.mui/MUIM_DragFinish
Area.mui/MUIM_DragQuery             done
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

static const struct MUI_FrameSpec_intern *get_intframe(
    Object *obj,
    struct MUI_AreaData *data);
static void set_inner_sizes (Object *obj, struct MUI_AreaData *data);
static void set_title_sizes (Object *obj, struct MUI_AreaData *data);

static void area_update_msizes(Object *obj, struct MUI_AreaData *data,
			       const struct MUI_FrameSpec_intern *frame,
			       const struct ZuneFrameGfx *zframe);
static void setup_control_char (struct MUI_AreaData *data, Object *obj,
				struct IClass *cl);
static void cleanup_control_char (struct MUI_AreaData *data, Object *obj);

//static void setup_cycle_chain (struct MUI_AreaData *data, Object *obj);
//static void cleanup_cycle_chain (struct MUI_AreaData *data, Object *obj);

#define ZUNE_FOCUS_TYPE_ACTIVE_OBJ 0
#define ZUNE_FOCUS_TYPE_DROP_OBJ   1

static void _zune_focus_new(Object *obj, int type)
{
    Object *parent;
    struct RastPort *rp;
    UWORD oldDrPt;

    //bug("_zune_focus_new 1 %p\n", obj);

    if (NULL == obj || !(_flags(obj) & MADF_CANDRAW))
	return;

    parent = _parent(obj);
    rp = _rp(obj);
    oldDrPt = rp->LinePtrn;

    int x1 = _left(obj);
    int y1 = _top(obj);
    int x2 = _left(obj) + _width(obj) -1;
    int y2 = _top(obj)  + _height(obj) -1;

    if (!parent || parent == _win(obj)) return;

    SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], _pens(obj)[MPEN_SHADOW], JAM2);

    if (type == ZUNE_FOCUS_TYPE_ACTIVE_OBJ)
    {
    	SetDrPt(rp, 0xCCCC);
	x1--; y1--; x2++; y2++;
    }
    else
    {
    	SetDrPt(rp,0xF0F0);
    }

    Move(rp, x1, y1);
    Draw(rp, x2, y1);
    Draw(rp, x2, y2);
    Draw(rp, x1, y2);
    Draw(rp, x1, y1);
    SetDrPt(rp, oldDrPt);
}

static void _zune_focus_destroy(Object *obj, int type)
{
    Object *parent;

    //bug("_zune_focus_destroy 1 %p\n", obj);

    if (NULL == obj || !(_flags(obj) & MADF_CANDRAW))
	return;

    parent = _parent(obj);
    int x1 = _left(obj);
    int y1 = _top(obj);
    int x2 = _left(obj) + _width(obj) - 1;
    int y2 = _top(obj)  + _height(obj) - 1;
    int width; 
    int height;

    if (type == ZUNE_FOCUS_TYPE_ACTIVE_OBJ)
    {
    	if (!parent || parent == _win(obj)) return;

    	x1--; y1--; x2++; y2++;
	width = x2 - x1 + 1;
    	height = y2 - y1 + 1;
    
	DoMethod(parent, MUIM_DrawBackground, x1, y1, width, 1, x1, y1, 0);
	DoMethod(parent, MUIM_DrawBackground, x2, y1, 1, height, x2, y1, 0);
	DoMethod(parent, MUIM_DrawBackground, x1, y2, width, 1, x1, y2, 0);
	DoMethod(parent, MUIM_DrawBackground, x1, y1, 1, height, x1, y1, 0);
    }
    else
    {
    	struct Region 	 *region;
	struct Rectangle  rect;
	APTR	    	  clip = NULL;
	
	region = NewRegion();
	if (region)
	{
	    rect.MinX = _left(obj);
	    rect.MinY = _top(obj);
	    rect.MaxX = _right(obj);
	    rect.MaxY = _top(obj);
	    
	    OrRectRegion(region, &rect);
	    
	    rect.MinX = _right(obj);
	    rect.MinY = _top(obj);
	    rect.MaxX = _right(obj);
	    rect.MaxY = _bottom(obj);
	    
	    OrRectRegion(region, &rect);

	    rect.MinX = _left(obj);
	    rect.MinY = _bottom(obj);
	    rect.MaxX = _right(obj);
	    rect.MaxY = _bottom(obj);
	    
	    OrRectRegion(region, &rect);

	    rect.MinX = _left(obj);
	    rect.MinY = _top(obj);
	    rect.MaxX = _left(obj);
	    rect.MaxY = _bottom(obj);
	    
	    OrRectRegion(region, &rect);
	 
	    clip = MUI_AddClipRegion(muiRenderInfo(obj), region);  
	     
	} /* if (region) */
	 
    	MUI_Redraw(obj, MADF_DRAWOBJECT);
	
	if (region)
	{
	    MUI_RemoveClipRegion(muiRenderInfo(obj), clip);
	}
	
    }

}


/**************************************************************************
 OM_NEW
**************************************************************************/
static IPTR Area_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_AreaData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return FALSE;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);

    data->mad_Flags = MADF_FILLAREA | MADF_SHOWME | MADF_SHOWSELSTATE | MADF_DROPABLE;
    data->mad_HorizWeight = data->mad_VertWeight = 100;
    data->mad_InputMode = MUIV_InputMode_None;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Background:
		    data->mad_Flags |= MADF_OWNBG;
		    if (data->mad_BackgroundSpec)
		    {
		    	zune_image_spec_free(data->mad_BackgroundSpec);
		    }
		    data->mad_BackgroundSpec = zune_image_spec_duplicate(tag->ti_Data);
		    break;

	    case MUIA_ControlChar:
		data->mad_ControlChar = tag->ti_Data;
		break;
	    case MUIA_CycleChain:
		_handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_CYCLECHAIN);
		break;

	    case MUIA_Disabled:
	    case MUIA_NestedDisabled:
		if (tag->ti_Data)
		{
		    data->mad_DisableCount = 1;
		}
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
	    case MUIA_FixHeightTxt:
	    	data->mad_HardHeightTxt = (STRPTR)tag->ti_Data;
		break;
	    case MUIA_FixWidth:
		data->mad_Flags |= MADF_FIXWIDTH;
		data->mad_HardWidth = tag->ti_Data;
		break;
	    case MUIA_FixWidthTxt:
	    	data->mad_HardWidthTxt = (STRPTR)tag->ti_Data;
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
		data->mad_InnerBottom = CLAMP((IPTR)tag->ti_Data, 0, 32);
		break;
	    case MUIA_InnerLeft:
		data->mad_Flags |= MADF_INNERLEFT;
		data->mad_InnerLeft = CLAMP((IPTR)tag->ti_Data, 0, 32);
		break;
	    case MUIA_InnerRight:
		data->mad_Flags |= MADF_INNERRIGHT;
		data->mad_InnerRight = CLAMP((IPTR)tag->ti_Data, 0, 32);
		break;
	    case MUIA_InnerTop:
		data->mad_Flags |= MADF_INNERTOP;
		data->mad_InnerTop =  CLAMP((IPTR)tag->ti_Data, 0, 32);
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

    /* In Soliton MUIA_Selected was setted to MUIV_InputMode_RelVerify (=1) for MUIA_Input_Mode
    ** MUIV_InputMode_RelVerify which is wrong of course but MUI seems to filter this out
    ** so we have to do it also
    */
    if (data->mad_InputMode == MUIV_InputMode_RelVerify)
    {
    	if (data->mad_Flags & MADF_SELECTED)
	    D(bug("MUIA_Selected was set in OM_NEW, although being in MUIV_InputMode_RelVerify\n"));
    	data->mad_Flags &= ~MADF_SELECTED;
    }

    data->mad_ehn.ehn_Events = 0; /* Will be filled on demand */
    data->mad_ehn.ehn_Priority = -5;
    /* Please also send mui key events to us, no idea if mui handles this like this */
    data->mad_ehn.ehn_Flags    = MUI_EHF_ALWAYSKEYS;
    data->mad_ehn.ehn_Object   = obj;
    data->mad_ehn.ehn_Class    = cl;

    data->mad_hiehn.ehn_Events   = 0;
    data->mad_hiehn.ehn_Priority = -10;
    data->mad_hiehn.ehn_Flags    = MUI_EHF_HANDLEINPUT;
    data->mad_hiehn.ehn_Object   = obj;
    data->mad_hiehn.ehn_Class    = 0;

    return (IPTR)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static IPTR Area_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    zune_image_spec_free(data->mad_BackgroundSpec); /* Safe to call this with NULL */

    return DoSuperMethodA(cl, obj, msg);
}


/**************************************************************************
 OM_SET
**************************************************************************/
static IPTR Area_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_AreaData *data  = INST_DATA(cl, obj);
    struct TagItem             *tags  = msg->ops_AttrList;
    struct TagItem             *tag;

    int change_disable = 0; /* Has the disable state changed? */

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case    MUIA_Background:
		    if (data->mad_Background)
		    {
			if (_flags(obj) & MADF_CANDRAW)
			{
			    zune_imspec_hide(data->mad_Background);
			}
			if (_flags(obj) & MADF_SETUP)
			{
			    zune_imspec_cleanup(data->mad_Background);
			    data->mad_Background = NULL;
			}
		    }

		    zune_image_spec_free(data->mad_BackgroundSpec);
		    if (tag->ti_Data)
		    {
			data->mad_BackgroundSpec = zune_image_spec_duplicate(tag->ti_Data);
			data->mad_Flags |= MADF_OWNBG;
		    }
		    else
		    {
			data->mad_BackgroundSpec = NULL;
			data->mad_Flags &= ~MADF_OWNBG;
		    }

		    if (_flags(obj) & MADF_SETUP)
		    {
			data->mad_Background =
			    zune_imspec_setup((IPTR)data->mad_BackgroundSpec,
					      muiRenderInfo(obj));
		    }
		    if (_flags(obj) & MADF_CANDRAW)
		    {
			zune_imspec_show(data->mad_Background, obj);
		    }
		    MUI_Redraw(obj, MADF_DRAWOBJECT);
		    break;

	    case    MUIA_FillArea:
		    _handle_bool_tag(data->mad_Flags, tag->ti_Data, MADF_FILLAREA);
		    break;

	    case MUIA_Frame:
		/* this is not documented in MUI but it is possible,
		   and needed to suppress frame for external images */
		data->mad_Frame = tag->ti_Data;
		if (muiGlobalInfo(obj))
		{
		set_inner_sizes(obj, data);
		set_title_sizes(obj, data);
	    	}
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

	    case    MUIA_Disabled:
		    if (tag->ti_Data)
		    {
		    	if (!data->mad_DisableCount)
			{
			    data->mad_DisableCount = 1;
			    change_disable = 1;
			}
		    }
		    else 
		    {
		    	if (data->mad_DisableCount)
		    	{
			    data->mad_DisableCount = 0;
			    change_disable = 1;
			}
		    }
		    break;

	    case    MUIA_NestedDisabled:
		    if (tag->ti_Data)
		    {
		    	if (!data->mad_DisableCount) change_disable = 1;
		    	data->mad_DisableCount++;
		    }   else 
		    {
		    	if (data->mad_DisableCount)
		    	{
			    data->mad_DisableCount--;
			    if (!data->mad_DisableCount) change_disable = 1;
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
		{
		    ULONG oldflags = data->mad_Flags;
		    int recalc = 0;

		    if (tag->ti_Data) data->mad_Flags |= MADF_SHOWME;
		    else data->mad_Flags &= ~MADF_SHOWME;

		    if (oldflags != data->mad_Flags)
		    {
			if (!tag->ti_Data)
			{
			    /* Should be made invisible, so send a MUIM_Hide and then a MUIM_Cleanup to the object if needed,
			    ** as objects with MUIA_ShowMe to false neighter get MUIM_Setup nor MUIM_Show */
			    if (_flags(obj)&MADF_CANDRAW)
			    {
			    	DoHideMethod(obj);
			    	recalc = 1;
			    }
			    if (_flags(obj)&MADF_SETUP) DoMethod(obj,MUIM_Cleanup);
			} else
			{
			    Object *parent = _parent(obj); /* Will be NULL if direct child of a window! */
			    if (parent)
			    {
				if (_flags(parent) & MADF_SETUP) DoSetupMethod(obj,muiRenderInfo(parent));
				if (_flags(parent) & MADF_CANDRAW) DoShowMethod(obj);
			    } else
			    {
				/* Check if window is open... */
			    }
			}

		    	if (recalc)
		    	{
			    DoMethod(_win(obj), MUIM_Window_RecalcDisplay, (IPTR)_parent(obj));
			}
		    }
		}
		break;

	    case    MUIA_Selected:
		/*  D(bug(" Area_Set(%p) : MUIA_Selected val=%ld sss=%d\n", obj, tag->ti_Data, !!(data->mad_Flags & MADF_SHOWSELSTATE))); */
		if (tag->ti_Data) data->mad_Flags |= MADF_SELECTED;
		else data->mad_Flags &= ~MADF_SELECTED;
/*  		if (data->mad_Flags & MADF_SHOWSELSTATE) */
		    MUI_Redraw(obj, MADF_DRAWOBJECT);
/*  		else */
/*  		    MUI_Redraw(obj, MADF_DRAWUPDATE); */
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

    if (change_disable)
    {
	MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


/**************************************************************************
 OM_GET
**************************************************************************/
static IPTR Area_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_AreaData *data = INST_DATA(cl, obj);

    switch(msg->opg_AttrID)
    {
	case MUIA_BottomEdge:
	    STORE = (IPTR)_bottom(obj);
	    return(TRUE);
	case MUIA_ControlChar:
	    STORE = data->mad_ControlChar;
	    return(TRUE);
	case MUIA_CycleChain:
	    STORE = ((data->mad_Flags & MADF_CYCLECHAIN) != 0);
	    return(TRUE);

	case    MUIA_Disabled:
	case MUIA_NestedDisabled:
	    STORE = !!data->mad_DisableCount; /* BOOLEAN */
	    return(TRUE);

	case MUIA_Font:
	    STORE = (IPTR)data->mad_FontPreset;
	    return(TRUE);
	case MUIA_Height:
	    STORE = (IPTR)_height(obj);
	    return(TRUE);
	case MUIA_HorizWeight:
	    STORE = (IPTR)data->mad_HorizWeight;
	    return(TRUE);

	case MUIA_InnerBottom:
	    STORE = (IPTR)data->mad_InnerBottom;
	    return 1;

	case MUIA_InnerLeft:
	    STORE = (IPTR)data->mad_InnerLeft;
	    return 1;

	case MUIA_InnerRight:
	    STORE = (IPTR)data->mad_InnerRight;
	    return 1;

	case MUIA_InnerTop:
	    STORE = (IPTR)data->mad_InnerTop; 

	case MUIA_LeftEdge:
	    STORE = (IPTR)_left(obj);
	    return(TRUE);
	case MUIA_Pressed:
	    STORE = !!(data->mad_Flags & MADF_PRESSED);
	    return(TRUE);
	case MUIA_RightEdge:
	    STORE = (IPTR)_right(obj);
	    return(TRUE);
	case MUIA_Selected:
	    STORE = !!(data->mad_Flags & MADF_SELECTED);
	    return(TRUE);
	case MUIA_ShortHelp:
	    STORE = (IPTR)data->mad_ShortHelp;
	    return(TRUE);
	case MUIA_ShowMe:
	    STORE = (data->mad_Flags & MADF_SHOWME);
	    return(TRUE);
	case MUIA_Timer:
	    return(TRUE);
	case MUIA_TopEdge:
	    STORE = (IPTR)_top(obj);
	    return(TRUE);
	case MUIA_VertWeight:
	    STORE = (IPTR)data->mad_VertWeight;
	    return(TRUE);
	case MUIA_Width:
	    STORE = (IPTR)_width(obj);
	    return(TRUE);
	case MUIA_Window:
	    if (muiAreaData(obj)->mad_RenderInfo)
		STORE = (IPTR)_window(obj);
	    else
		STORE = 0L;
	    return(TRUE);
	case MUIA_WindowObject:
	    if (muiAreaData(obj)->mad_RenderInfo)
		STORE = (IPTR)_win(obj);
	    else
		STORE = 0L;
	    return(TRUE);
	case MUIA_ContextMenu:
	    STORE = (IPTR)data->mad_ContextMenu;
	    return 1;
    }

    return(DoSuperMethodA(cl, obj, (Msg) msg));
#undef STORE
}


/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
static IPTR Area_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    const struct MUI_FrameSpec_intern *frame;

    frame = get_intframe(obj, data);
    zframe = zune_zframe_get(frame);

    area_update_msizes(obj, data, frame, zframe);
    
    msg->MinMaxInfo->MinWidth = _subwidth(obj);
    msg->MinMaxInfo->MinHeight = _subheight(obj);

    msg->MinMaxInfo->MaxWidth = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->MaxHeight = msg->MinMaxInfo->MinHeight;
    msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
    msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;

   /*   D(bug("Area_AskMinMax 0x%lx (%s): Min=%ldx%ld Max=%ldx%ld Def=%ldx%ld\n", obj, data->mad_FrameTitle, */
/*  	  msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight, */
/*  	  msg->MinMaxInfo->MaxWidth, msg->MinMaxInfo->MaxHeight, */
/*  	  msg->MinMaxInfo->DefWidth, msg->MinMaxInfo->DefHeight)); */

    return TRUE;
}

/*
 * Called after MUIM_AskMinMax, to handle fixed and max sizes.
 */
void __area_finish_minmax(Object *obj, struct MUI_MinMax *MinMaxInfo)
{
    struct MUI_AreaData *data = muiAreaData(obj);
    
    if ((_flags(obj) & MADF_FIXHEIGHT) && data->mad_HardHeight)
    {
	int h;

    	h = data->mad_HardHeight + data->mad_subheight;
	
	MinMaxInfo->MinHeight =
	MinMaxInfo->DefHeight = 
	MinMaxInfo->MaxHeight = CLAMP(h, MinMaxInfo->MinHeight, MinMaxInfo->MaxHeight);
    }
    else if (data->mad_HardHeightTxt)
    {
    	ZText *text;
	
	if ((text = zune_text_new(NULL, data->mad_HardHeightTxt, ZTEXT_ARG_NONE, 0)))
	{
	    zune_text_get_bounds(text, obj);
	    
	    MinMaxInfo->MinHeight =
	    MinMaxInfo->DefHeight =
	    MinMaxInfo->MaxHeight = 
	    	CLAMP(text->height + data->mad_subheight, MinMaxInfo->MinHeight, MinMaxInfo->MaxHeight);
	    
	    zune_text_destroy(text);
	}

    }
    else if (_flags(obj) & MADF_MAXHEIGHT)
    {	
	MinMaxInfo->MaxHeight =
	    CLAMP(data->mad_HardHeight + data->mad_subheight,
		  MinMaxInfo->MinHeight,
		  MinMaxInfo->MaxHeight);
    }

    if ((_flags(obj) & MADF_FIXWIDTH) && data->mad_HardWidth)
    {
	int w;

    	w = data->mad_HardWidth + data->mad_subwidth;
	
	MinMaxInfo->MinWidth =
	MinMaxInfo->DefWidth = 
	MinMaxInfo->MaxWidth = CLAMP(w, MinMaxInfo->MinWidth, MinMaxInfo->MaxWidth);
    }
    else if (data->mad_HardWidthTxt)
    {
    	ZText *text;
	
	if ((text = zune_text_new(NULL, data->mad_HardWidthTxt, ZTEXT_ARG_NONE, 0)))
	{
	    zune_text_get_bounds(text, obj);
	    
	    MinMaxInfo->MinWidth =
	    MinMaxInfo->DefWidth =
	    MinMaxInfo->MaxWidth = 
	    	CLAMP(text->width + data->mad_subwidth, MinMaxInfo->MinWidth, MinMaxInfo->MaxWidth);
	    
	    zune_text_destroy(text);
	}

    }
    else if (_flags(obj) & MADF_MAXWIDTH)
    {
	MinMaxInfo->MaxWidth =
	    CLAMP(data->mad_HardWidth + data->mad_subwidth,
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

/*                      <-- _top(obj) (frame title position depends of _top(obj))
 *  ====== Title =====  <-- frame_top (depends of title, if centered/above)
 * |                  | <-- bgtop (depends of frame, bg always begins under frame)
 * |                  |
 * |                  |
 *  ==================  <-- "bgbottom" and "frame_bottom" (frame overwrites bg (1))
 *
 * (1) : needed for phantom frame objects, where no frame overwrites bg, thus bg
 * must go as far as theorical bottom frame border.
 */

/*
 * draw object background if MADF_FILLAREA.
 */
static void Area_Draw__handle_background(Object *obj, struct MUI_AreaData *data, ULONG flags,
					 const struct ZuneFrameGfx *zframe)
{
    struct MUI_ImageSpec_intern *background;
    int bgtop, bgleft, bgw, bgh;

    if (!(data->mad_Flags & MADF_SELECTED) || !(data->mad_Flags & MADF_SHOWSELSTATE))
	background = data->mad_Background;
    else
	background = data->mad_SelBack;

    bgleft = _left(obj);
    bgtop = _top(obj) + data->mad_TitleHeightAbove + zframe->itop;
    bgw = _width(obj);
    bgh = _height(obj) - bgtop + _top(obj);

    if (!background)
    {
	/* This will do the rest, TODO: on MADF_DRAWALL we not really need to draw this */
/*  	D(bug(" Area_Draw(%p):%ld: MUIM_DrawBackground\n", obj, __LINE__)); */
	DoMethod(obj, MUIM_DrawBackground, bgleft, bgtop, bgw, bgh, bgleft, bgtop,
		 data->mad_Flags);
    }
    else
    {
/*  	D(bug(" Area_Draw(%p):%ld: zune_imspec_draw\n", obj, __LINE__)); */
	zune_imspec_draw(background, data->mad_RenderInfo,
			 bgleft, bgtop, bgw, bgh, bgleft, bgtop, 0);
    }

    if (muiGlobalInfo(obj)->mgi_Prefs->window_redraw == WINDOW_REDRAW_WITHOUT_CLEAR)
    {
	if (bgtop > _top(obj) && !(flags & MADF_DRAWALL))
	{
	    /* Fill in the gap produced by the title with the background
	     * of the parent object but only if
	     * the upper object hasn't drawn it already
	     * (which is the case if MADF_DRAWALL is setted) */
	    DoMethod(obj, MUIM_DrawParentBackground, bgleft, _top(obj), bgw, bgtop - _top(obj),
		     bgleft, _top(obj), data->mad_Flags);
	}
    }
}


/*
 * draw object frame + title if not MADF_FRAMEPHANTOM.
 */
static void Area_Draw__handle_frame(Object *obj, struct MUI_AreaData *data,
				    const struct ZuneFrameGfx *zframe)
{
    APTR textdrawclip = (APTR)-1;
    struct Region *region;
    int tx;
    int tw, frame_height, frame_top;
    int addtw;
    struct TextExtent te;
    int nchars;
    int maxtxtwidth;

    /* no frametitle, just draw frame and return */
    if (!data->mad_FrameTitle)
    {
	zframe->draw(muiRenderInfo(obj), _left(obj), _top(obj), _width(obj), _height(obj));
	return;
    }
      
    /* set clipping so that frame is not drawn behind title */

    switch (muiGlobalInfo(obj)->mgi_Prefs->group_title_color)
    {
	case GROUP_TITLE_COLOR_OUTLINE:
	    addtw = 2;
	    break;
	case GROUP_TITLE_COLOR_3D:
	    addtw = 1;
	    break;
	default:
	    addtw = 0;
    }

    maxtxtwidth = _width(obj) - zframe->ileft - zframe->iright - 2 * 5 - addtw;

    nchars = TextFit(_rp(obj), data->mad_FrameTitle,
		     strlen(data->mad_FrameTitle),
		     &te, NULL, 1, maxtxtwidth, _font(obj)->tf_YSize);

    tw = te.te_Width + addtw;
    tx = _left(obj) + (_width(obj) - tw) / 2;

    frame_top = _top(obj) + data->mad_TitleHeightAbove;
    frame_height = _height(obj) - frame_top + _top(obj);

    if ((region = NewRegion()))
    {
	struct Rectangle rect;
	int hspace = _font(obj)->tf_YSize / 8;

	rect.MinX = tx - hspace;
	rect.MinY = _top(obj);
	rect.MaxX = tx + tw - 1 + hspace;
	rect.MaxY = _top(obj) + _font(obj)->tf_YSize - 1; // frame is not thick enough anywhy
	OrRectRegion(region,&rect);

	rect.MinX = _left(obj);
	rect.MinY = _top(obj);
	rect.MaxX = _right(obj);
	rect.MaxY = _bottom(obj);
	XorRectRegion(region,&rect);

	textdrawclip = MUI_AddClipRegion(muiRenderInfo(obj),region);
    }

    zframe->draw(muiRenderInfo(obj), _left(obj), frame_top, _width(obj), frame_height);

    if (region && textdrawclip != (APTR)-1)
    {
	MUI_RemoveClipRegion(muiRenderInfo(obj),textdrawclip);
/*		DisposeRegion(region);*/ /* sba: DisposeRegion happens in MUI_RemoveClipRegion, this seems wrong to me */
    }

    /* Title text drawing */
    SetDrMd(_rp(obj), JAM1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color == GROUP_TITLE_COLOR_3D)
    {
	Move(_rp(obj), tx + 1, _top(obj) + _font(obj)->tf_Baseline + 1);
	Text(_rp(obj), data->mad_FrameTitle, nchars);
	SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline);
	Text(_rp(obj), data->mad_FrameTitle, nchars);
    }
    else if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color == GROUP_TITLE_COLOR_OUTLINE)
    {
	SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);

	tx += addtw / 2;
	Move(_rp(obj), tx + 1, _top(obj) + _font(obj)->tf_Baseline);
	Text(_rp(obj), data->mad_FrameTitle, nchars);
	Move(_rp(obj), tx - 1, _top(obj) + _font(obj)->tf_Baseline);
	Text(_rp(obj), data->mad_FrameTitle, nchars);
	Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline + 1);
	Text(_rp(obj), data->mad_FrameTitle, nchars);
	Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline - 1);
	Text(_rp(obj), data->mad_FrameTitle, nchars);

        SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
	Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline);
	Text(_rp(obj), data->mad_FrameTitle, nchars);
    }
    else
    {
	if (muiGlobalInfo(obj)->mgi_Prefs->group_title_color == GROUP_TITLE_COLOR_HILITE)
	{
	    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
	}
	Move(_rp(obj), tx, _top(obj) + _font(obj)->tf_Baseline);
	Text(_rp(obj), data->mad_FrameTitle, nchars);
    }
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
static IPTR Area_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    struct TextFont *obj_font = NULL;
    //APTR areaclip;

/*      D(bug("Area_Draw(0x%lx) %ldx%ldx%ldx%ld\n",obj,_left(obj),_top(obj),_right(obj),_bottom(obj))); */
/*      D(bug(" Area_Draw(%p) msg=0x%08lx flags=0x%08lx\n",obj, msg->flags,_flags(obj))); */

    if (msg->flags & MADF_DRAWALL)
	msg->flags |= MADF_DRAWOBJECT;

    if (!(msg->flags & MADF_DRAWOBJECT))
    {
	/* dont draw bg/frame, let subclass redraw content only
	**/
	return 0;
    }

/* Background cant be drawn without knowing anything about frame, thus some
 * calculations are made before background and frame drawing.
 */
    {
	/* on selected state, will get the opposite frame */
	const struct MUI_FrameSpec_intern *frame;
	int state;

	frame = get_intframe(obj, data);
	state = frame->state;
	if ((data->mad_Flags & MADF_SELECTED) && (data->mad_Flags & MADF_SHOWSELSTATE))
	    state ^= 1;

	zframe = zune_zframe_get_with_state(
	    &muiGlobalInfo(obj)->mgi_Prefs->frames[data->mad_Frame], state);
	/* update innersizes as there are frames which have different inner spacings in selected state */
	area_update_msizes(obj, data, frame, zframe);
    }

    /* Background drawing */
    if (data->mad_Flags & MADF_FILLAREA)
    {
	Area_Draw__handle_background(obj, data, msg->flags, zframe);
    }

    obj_font = _font(obj);
    _font(obj) = zune_font_get(obj, MUIV_Font_Title);
    SetFont(_rp(obj), _font(obj));

    /* Frame and frametitle drawing */
    if (!(data->mad_Flags & MADF_FRAMEPHANTOM))
    {
	Area_Draw__handle_frame(obj, data, zframe);
    }

    _font(obj) = obj_font;
    SetFont(_rp(obj), _font(obj));

/*    MUI_RemoveClipping(muiRenderInfo(obj), areaclip);*/

    return TRUE;
}

/**************************************************************************
 MUIM_DrawParentBackground
**************************************************************************/
static IPTR Area_DrawParentBackground(struct IClass *cl, Object *obj, struct MUIP_DrawParentBackground *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    Object *parent;

    if (!(data->mad_Flags & MADF_CANDRAW)) /* not between show/hide */
	return FALSE;

    get(obj, MUIA_Parent, &parent);
    if (parent)
    {
    	DoMethod(parent, MUIM_DrawBackground, msg->left, msg->top,
		 msg->width, msg->height, msg->xoffset, msg->yoffset, msg->flags);
    }
    else
    {
	D(bug("Area_DrawParentBackground(%p) : MUIM_Window_DrawBackground\n", obj));
    	DoMethod(_win(obj), MUIM_Window_DrawBackground, msg->left, msg->top,
		 msg->width, msg->height, msg->xoffset, msg->yoffset, msg->flags);
    }
    return TRUE;
}

/**************************************************************************
 MUIM_DrawBackground
**************************************************************************/
static IPTR Area_DrawBackground(struct IClass *cl, Object *obj, struct MUIP_DrawBackground *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    struct MUI_ImageSpec_intern *bg;
    LONG state;

    if (!(data->mad_Flags & MADF_CANDRAW)) /* not between show/hide */
	return FALSE;

    if ((msg->flags & MADF_SELECTED) && (msg->flags & MADF_SHOWSELSTATE) && data->mad_SelBack)
    { 
/*  	D(bug("Area_DrawBackground(%p): selected bg\n", obj)); */
    	bg = data->mad_SelBack;
	state = IDS_SELECTED;
    }
    else
    {
/*  	D(bug("Area_DrawBackground(%p): normal bg\n", obj)); */
	bg = data->mad_Background;
	state = IDS_NORMAL;
    }

    if (!bg)
    {
	Object *parent;
	get(obj, MUIA_Parent, &parent);

	D(bug("Area_DrawBackground(%p) : MUIM_DrawParentBackground\n",
	      obj));
	return DoMethod(obj, MUIM_DrawParentBackground, msg->left, msg->top,
		     msg->width, msg->height, msg->xoffset, msg->yoffset, msg->flags);
    }

/*      D(bug("Area_DrawBackground(%p): draw bg\n", obj)); */
    zune_imspec_draw(bg, data->mad_RenderInfo,
		     msg->left, msg->top, msg->width, msg->height,
		     msg->xoffset, msg->yoffset, state);

    return TRUE;
}

/* Perverting the EventHandlerNode structure to specify a shortcut.
 */
static void setup_control_char (struct MUI_AreaData *data, Object *obj, struct IClass *cl)
{
    if (data->mad_ControlChar != 0 || data->mad_Flags & MADF_CYCLECHAIN)
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
    if (data->mad_ControlChar != 0 || data->mad_Flags & MADF_CYCLECHAIN)
    {
	DoMethod(_win(obj),
		 MUIM_Window_RemControlCharHandler, (IPTR)&data->mad_ccn);
    }
}

static const struct MUI_FrameSpec_intern *get_intframe(
    Object *obj, struct MUI_AreaData *data)
{
    return &muiGlobalInfo(obj)->mgi_Prefs->frames[data->mad_Frame];
}

static void set_inner_sizes (Object *obj, struct MUI_AreaData *data)
{
    const struct MUI_FrameSpec_intern *frame;

    frame = get_intframe(obj, data);
    // Use frame inner spacing when not hardcoded
    if (!(data->mad_Flags & MADF_INNERLEFT))
	data->mad_InnerLeft = frame->innerLeft;
    if (!(data->mad_Flags & MADF_INNERTOP))
	data->mad_InnerTop = frame->innerTop;
    if (!(data->mad_Flags & MADF_INNERRIGHT))
	data->mad_InnerRight = frame->innerRight;
    if (!(data->mad_Flags & MADF_INNERBOTTOM))
	data->mad_InnerBottom = frame->innerBottom;
}


static void set_title_sizes (Object *obj, struct MUI_AreaData *data)
{
    if (data->mad_FrameTitle)
    {
	const struct ZuneFrameGfx *zframe;
	const struct MUI_FrameSpec_intern *frame;

	frame = get_intframe(obj, data);
	zframe = zune_zframe_get(frame);

	_font(obj) = zune_font_get(obj, MUIV_Font_Title);

	switch (muiGlobalInfo(obj)->mgi_Prefs->group_title_position)
	{
	    case GROUP_TITLE_POSITION_ABOVE:
		data->mad_TitleHeightAbove = _font(obj)->tf_Baseline;
		break;
	    case GROUP_TITLE_POSITION_CENTERED:
		data->mad_TitleHeightAbove = _font(obj)->tf_YSize / 2;
		break;
	}

	data->mad_TitleHeightAdd = _font(obj)->tf_YSize - data->mad_InnerTop - zframe->itop;
	data->mad_TitleHeightBelow = data->mad_TitleHeightAdd - data->mad_TitleHeightAbove;
    }
}


/**************************************************************************
 First method to be called after an OM_NEW, it is the place
 for all initializations depending on the environment, but not
 on the gadget size/position. Matched by MUIM_Cleanup.
**************************************************************************/
static IPTR Area_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    const struct MUI_FrameSpec_intern *frame;

    muiRenderInfo(obj) = msg->RenderInfo;

    if (data->mad_Frame)
    {
	/* no frame allowed for root object (see Area.doc) */
	IPTR rootobj;
	get(_win(obj), MUIA_Window_RootObject, &rootobj);
	if ((Object*)rootobj == obj)
	{
	    data->mad_Frame = MUIV_Frame_None;
	    data->mad_FrameTitle = NULL;
	}
    }

    set_inner_sizes(obj, data);
    set_title_sizes(obj, data);

    frame = get_intframe(obj, data);
    zframe = zune_zframe_get(frame);

    area_update_msizes(obj, data, frame, zframe);

    if (data->mad_Flags & MADF_OWNBG)
    {
	data->mad_Background = zune_imspec_setup((IPTR)data->mad_BackgroundSpec,
						 muiRenderInfo(obj));
    }

    if ((data->mad_Flags & MADF_SHOWSELSTATE) &&
	(data->mad_InputMode != MUIV_InputMode_None))
    {
	data->mad_SelBack = zune_imspec_setup(MUII_SelectedBack, muiRenderInfo(obj));
    }

    if (data->mad_InputMode != MUIV_InputMode_None || data->mad_ContextMenu)
    {
	data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
    }

    /* Those are filled by RequestIDCMP() */
    if (data->mad_hiehn.ehn_Events)
	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_hiehn);

    setup_control_char (data, obj, cl);
//    setup_cycle_chain (data, obj);

    if (data->mad_FontPreset == MUIV_Font_Inherit)
    {
	if (_parent(obj) != NULL && _parent(obj) != _win(obj))
	    data->mad_Font = _font(_parent(obj));
	else
	{
	    D(bug("Area_Setup %p: getting normal font\n", obj));
	    data->mad_Font = zune_font_get(obj, MUIV_Font_Normal);
	    D(bug("Area_Setup %p: got normal font %p\n", obj, data->mad_Font));
	}
    }
    else
    {
	data->mad_Font = zune_font_get(obj, data->mad_FontPreset);
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
static IPTR Area_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    _flags(obj) &= ~MADF_SETUP;

//    cleanup_cycle_chain (data, obj);
    cleanup_control_char (data, obj);

    if (data->mad_Timer.ihn_Millis)
    {
	DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->mad_Timer);
	data->mad_Timer.ihn_Millis = 0;
    }

    /* Remove the handler if it is added */
    if (data->mad_hiehn.ehn_Events)
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_hiehn);

    /* Remove the event handler if it has been added */
    if (data->mad_ehn.ehn_Events)
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);

    D(bug("Area cleanup %p active=%p\n", obj,
	  (Object *)XGET(_win(obj), MUIA_Window_ActiveObject)));
    if (obj == (Object *)XGET(_win(obj), MUIA_Window_ActiveObject))
    {
	D(bug("we are active, unset us\n"));
	set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
    }

    /* It's save to call the following function with NULL */
    if ((data->mad_Flags & MADF_SHOWSELSTATE) &&
	(data->mad_InputMode != MUIV_InputMode_None))
    {
	zune_imspec_cleanup(data->mad_SelBack);
	data->mad_SelBack = NULL;
    }
    if (data->mad_Flags & MADF_OWNBG)
    {
	zune_imspec_cleanup(data->mad_Background);
	data->mad_Background = NULL;
    }

    muiRenderInfo(obj) = NULL;

    return TRUE;
}


/**************************************************************************
 Called after the window is open and the area layouted, but before
 any drawing. Matched by one MUIM_Hide.
 Good place to init things depending on gadget size/position.
**************************************************************************/
static IPTR Area_Show(struct IClass *cl, Object *obj, struct MUIP_Show *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    zune_imspec_show(data->mad_Background, obj);

    if (data->mad_Flags & MADF_SHOWSELSTATE
	&& data->mad_InputMode != MUIV_InputMode_None)
    {
	zune_imspec_show(data->mad_SelBack, obj);
    }

    return TRUE;
}

/**************************************************************************
 Called when the window is about to be closed, to match MUIM_Show.
**************************************************************************/
static IPTR Area_Hide(struct IClass *cl, Object *obj, struct MUIP_Hide *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

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

    return TRUE;
}

/**************************************************************************
 Called when gadget activated
**************************************************************************/
static IPTR Area_GoActive(struct IClass *cl, Object *obj, Msg msg)
{
    //bug("Area_GoActive %p\n", obj);
    if (_flags(obj) & MADF_CANDRAW)
	_zune_focus_new(obj, ZUNE_FOCUS_TYPE_ACTIVE_OBJ);
    return TRUE;
}

/**************************************************************************
 Called when gadget deactivated
**************************************************************************/
static IPTR Area_GoInactive(struct IClass *cl, Object *obj, Msg msg)
{
    //bug("Area_GoInactive %p\n", obj);
    if (_flags(obj) & MADF_CANDRAW)
	_zune_focus_destroy(obj, ZUNE_FOCUS_TYPE_ACTIVE_OBJ);
    return TRUE;
}

/**************************************************************************
 This one or derived methods wont be called if short help is
 not set in area instdata. So set this to a dummy val if overriding
**************************************************************************/
static IPTR Area_CreateShortHelp(struct IClass *cl, Object *obj, struct MUIP_CreateShortHelp *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    return (IPTR)data->mad_ShortHelp;
}

/**************************************************************************
 ...
**************************************************************************/
static IPTR Area_DeleteShortHelp(struct IClass *cl, Object *obj, struct MUIP_DeleteShortHelp *msg)
{
    return TRUE;
}

/**************************************************************************
 ...
**************************************************************************/
static IPTR Area_CreateBubble(struct IClass *cl, Object *obj, struct MUIP_CreateBubble *msg)
{
    return (IPTR)zune_bubble_create(obj, msg->x, msg->y, msg->txt, msg->flags);
}

/**************************************************************************
 ...
**************************************************************************/
static IPTR Area_DeleteBubble(struct IClass *cl, Object *obj, struct MUIP_DeleteBubble *msg)
{
    zune_bubble_delete(obj, msg->bubble);
    
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
	    	data->mad_Timer.ihn_Millis = 300;
		DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->mad_Timer);
	    }
	    SetAttrs(obj, MUIA_Selected, TRUE, MUIA_Pressed, TRUE, TAG_DONE);
	    break;

	case MUIV_InputMode_Immediate:
	{
	    IPTR selected;

	    get(obj, MUIA_Selected, &selected);
	    if (selected)
	    {
/*  		D(bug("handle_press(%p) : nnset MUIA_Selected FALSE\n", obj)); */
		nnset(obj, MUIA_Selected, FALSE);
	    }
/*  	    D(bug("handle_press(%p) : set MUIA_Selected TRUE\n", obj)); */
	    set(obj, MUIA_Selected, TRUE);
/*  	    D(bug("handle_press(%p) : done\n", obj)); */
	    break;
	}
	case MUIV_InputMode_Toggle:
	    // although undocumented, MUI sets MUIA_Pressed too
	    SetAttrs(obj, MUIA_Selected, !(data->mad_Flags & MADF_SELECTED),
		     MUIA_Pressed, !(data->mad_Flags & MADF_PRESSED));
	    break;
    }
}

/* either lmb or release key */
static void handle_release(struct IClass *cl, Object *obj, int cancel)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    if (data->mad_InputMode == MUIV_InputMode_RelVerify)
    {
	if (data->mad_Flags & MADF_SELECTED)
	{
	    if (cancel)
		nnset(obj, MUIA_Pressed, FALSE);
	    else
		set(obj, MUIA_Pressed, FALSE);
	    
	    set(obj, MUIA_Selected, FALSE);
	}
    }

    if (data->mad_Timer.ihn_Millis)
    {
	DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->mad_Timer);
    	data->mad_Timer.ihn_Millis = 0;
    }

}

static IPTR event_button(Class *cl, Object *obj, struct IntuiMessage *imsg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    BOOL in = _between(_left(obj), imsg->MouseX, _right(obj))
           && _between(_top(obj),  imsg->MouseY, _bottom(obj));

    switch (imsg->Code)
    {
	case	SELECTDOWN:
		if (data->mad_InputMode == MUIV_InputMode_None)
		    break;

		if (in)
		{
//		    set(_win(obj), MUIA_Window_ActiveObject, obj);
		    data->mad_ClickX = imsg->MouseX;
		    data->mad_ClickY = imsg->MouseY;

		    if ((data->mad_InputMode != MUIV_InputMode_Toggle) && (data->mad_Flags & MADF_SELECTED))
			break;
		    nnset(obj,MUIA_Timer,0);
		    if (data->mad_InputMode == MUIV_InputMode_RelVerify)
		    {
			if (data->mad_ehn.ehn_Events) DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
			data->mad_ehn.ehn_Events |= IDCMP_MOUSEMOVE | IDCMP_RAWKEY;
	                DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
	            }
		    handle_press(cl, obj);
		    return MUI_EventHandlerRC_Eat;
		}

	case	SELECTUP:
		if (data->mad_InputMode == MUIV_InputMode_None)
		    break;

		if (data->mad_ehn.ehn_Events != IDCMP_MOUSEBUTTONS)
		{
		    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
	            data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
		    if (!in) nnset(obj, MUIA_Pressed, FALSE);
		    handle_release(cl, obj, FALSE /*cancel*/ );
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

static IPTR event_motion(Class *cl, Object *obj, struct IntuiMessage *imsg)
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

		if (data->mad_ehn.ehn_Events) DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
		data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
		if (data->mad_Timer.ihn_Millis)
		{
		   DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->mad_Timer);
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
static IPTR Area_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);

    //bug("Area_HandleEvent [%p] imsg=%p muikey=%ld\n", obj, msg->imsg, msg->muikey);
    if (data->mad_DisableCount) return 0;
    if (data->mad_InputMode == MUIV_InputMode_None && !data->mad_ContextMenu) return 0;

    if (msg->muikey != MUIKEY_NONE)
    {
	switch (msg->muikey)
	{
	    case    MUIKEY_PRESS:
		    if (data->mad_Flags & MADF_SELECTED)
			break;
		    if (data->mad_ehn.ehn_Events)
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
		    data->mad_ehn.ehn_Events |= IDCMP_RAWKEY;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
		    handle_press(cl, obj);
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_TOGGLE:
		    if (data->mad_InputMode == MUIV_InputMode_Toggle)
			set(obj, MUIA_Selected, !(data->mad_Flags & MADF_SELECTED));
		    return MUI_EventHandlerRC_Eat;

	    case    MUIKEY_RELEASE:
		    if (data->mad_ehn.ehn_Events)
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
		    data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
		    handle_release(cl, obj, FALSE /* cancel */);
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
		unsigned char code;
		UWORD msg_code;
		/* Remove the up prefix as convert key does not convert a upkey event */
		msg_code = msg->imsg->Code;
		msg->imsg->Code &= ~IECODE_UP_PREFIX;
		code = ConvertKey(msg->imsg);
		msg->imsg->Code = msg_code;

		if (code != 0 && code == data->mad_ControlChar)
		{
		    if (msg->imsg->Code & IECODE_UP_PREFIX)
		    {
			msg->muikey = MUIKEY_RELEASE;
		    }
		    else
		    {
			msg->muikey = MUIKEY_PRESS;
		    }
		    msg->imsg = NULL;
		    return Area_HandleEvent(cl, obj, msg);
		}

	        if (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
	        {
		    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->mad_ehn);
		    data->mad_ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
		    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->mad_ehn);
		    handle_release(cl,obj, TRUE /*cancel */);
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
static IPTR Area_HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleInput *msg)
{
    /* Actually a dummy, but real MUI does handle here the input stuff which Zune
    ** has in Area_HandleEvent. For compatibility we should do this too
    **/
    //bug("Area_HandleEvent [%p] imsg=%p muikey=%ld\b", obj, msg->imsg, msg->muikey);
    return 0;
}

/**************************************************************************
 Trivial; custom classes may override this to get dynamic menus.
**************************************************************************/
static IPTR Area_ContextMenuBuild(struct IClass *cl, Object *obj, struct MUIP_ContextMenuBuild *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    return (IPTR)data->mad_ContextMenu; /* a Menustrip object */
}


/**************************************************************************
 MUIM_Export : to export an objects "contents" to a dataspace object.
**************************************************************************/
static IPTR Area_Export(struct IClass *cl, Object *obj, struct MUIP_Export *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    ULONG id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
    	char selected = (data->mad_Flags & MADF_SELECTED)?1:0;
	DoMethod(msg->dataspace, MUIM_Dataspace_Add, (IPTR)&selected, sizeof(char),(IPTR)id);
    }
    return 0;
}


/**************************************************************************
 MUIM_Import : to import an objects "contents" from a dataspace object.
**************************************************************************/
static IPTR Area_Import(struct IClass *cl, Object *obj, struct MUIP_Import *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    ULONG id;
    //BOOL val = FALSE;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
    	char *selected = (char*)DoMethod(msg->dataspace, MUIM_Dataspace_Find, (IPTR)id);

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
static IPTR Area_Timer(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    if (data->mad_Timer.ihn_Millis)
	DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->mad_Timer);
    data->mad_Timer.ihn_Millis = 50;
    DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->mad_Timer);

    if (data->mad_Flags & MADF_SELECTED)
	set(obj, MUIA_Timer, ++muiAreaData(obj)->mad_Timeval);
    return 0;
}

/**************************************************************************
 MUIM_DoDrag
**************************************************************************/
static IPTR Area_DoDrag(struct IClass *cl, Object *obj, struct MUIP_DoDrag *msg)
{
    //struct MUI_AreaData *data = INST_DATA(cl, obj);
    DoMethod(_win(obj), MUIM_Window_DragObject, (IPTR)obj, msg->touchx, msg->touchy, msg->flags);
    return 0;
}

/**************************************************************************
 MUIM_CreateDragImage
**************************************************************************/
static IPTR Area_CreateDragImage(struct IClass *cl, Object *obj, struct MUIP_CreateDragImage *msg)
{
    struct MUI_DragImage *img = (struct MUI_DragImage *)AllocVec(sizeof(struct MUIP_CreateDragImage),MEMF_CLEAR);
    if (img)
    {
    	const struct ZuneFrameGfx *zframe;
	LONG depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap,BMA_DEPTH);

	zframe = zune_zframe_get(&muiGlobalInfo(obj)->mgi_Prefs->frames[MUIV_Frame_Drag]);

    	img->width = _width(obj) + zframe->ileft + zframe->iright;
    	img->height = _height(obj) + zframe->itop + zframe->ibottom;

    	if ((img->bm = AllocBitMap(img->width,img->height,depth,BMF_MINPLANES,_screen(obj)->RastPort.BitMap)))
    	{
    	    /* Render the stuff now */
    	    struct RastPort *rp_save = muiRenderInfo(obj)->mri_RastPort;
    	    struct RastPort temprp;
    	    InitRastPort(&temprp);
    	    temprp.BitMap = img->bm;
    	    ClipBlit(_rp(obj),_left(obj),_top(obj),&temprp,zframe->ileft,zframe->itop,_width(obj),_height(obj),0xc0);

	    muiRenderInfo(obj)->mri_RastPort = &temprp;
	    zframe->draw(muiRenderInfo(obj), 0, 0, img->width, img->height);
	    muiRenderInfo(obj)->mri_RastPort = rp_save;
	    
            DeinitRastPort(&temprp);
    	}

    	img->touchx = msg->touchx;
    	img->touchy = msg->touchy;
    	img->flags = 0;
    }
    return (IPTR)img;
}

/**************************************************************************
 MUIM_DeleteDragImage
**************************************************************************/
static IPTR Area_DeleteDragImage(struct IClass *cl, Object *obj, struct MUIP_DeleteDragImage *msg)
{
    if (msg->di)
    {
	if (msg->di->bm) FreeBitMap(msg->di->bm);
	FreeVec(msg->di);
    }
    return 0;
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
	    if (DoMethod(obj,MUIM_DragQuery,(IPTR)msg->obj) == MUIV_DragQuery_Accept)
		return (IPTR)obj;
	}
    }
    return 0;
}

/**************************************************************************
 MUIM_DragBegin
**************************************************************************/
static IPTR Area_DragBegin(struct IClass *cl, Object *obj, struct MUIP_DragBegin *msg)
{
    //struct MUI_AreaData *data = INST_DATA(cl, obj);
    _zune_focus_new(obj, ZUNE_FOCUS_TYPE_DROP_OBJ);
    return 0;
}

/**************************************************************************
 MUIM_DragFinish
**************************************************************************/
static IPTR Area_DragFinish(struct IClass *cl, Object *obj, struct MUIP_DragFinish *msg)
{
    //struct MUI_AreaData *data = INST_DATA(cl, obj);
    _zune_focus_destroy(obj, ZUNE_FOCUS_TYPE_DROP_OBJ);
    return 0;
}


/*
 * Calculates addleft, addtop, subwidth, subheight from current settings.
 * If frame phantom, ignore horizontal frame components.
 * Depends on inner sizes and frame
 */
static void area_update_msizes(Object *obj, struct MUI_AreaData *data,
			       const struct MUI_FrameSpec_intern *frame,
			       const struct ZuneFrameGfx *zframe)
{

/*      if (XGET(obj, MUIA_UserData) == 42) */
/*      { */
/*  	D(bug("area_update_msizes(%p) : ileft=%ld itop=%ld\n", obj, zframe->ileft, zframe->itop)); */
/*      } */

    data->mad_addleft = data->mad_InnerLeft + zframe->ileft;
    data->mad_subwidth = data->mad_addleft + data->mad_InnerRight + zframe->iright;
    data->mad_addtop = data->mad_InnerTop + data->mad_TitleHeightAdd + zframe->itop;
    data->mad_subheight = data->mad_addtop + data->mad_InnerBottom + zframe->ibottom;

    if (data->mad_Flags & MADF_FRAMEPHANTOM)
    {
	data->mad_addleft = 0;
	data->mad_subwidth = 0;
    }

// clamping ... maybe ?

/*      D(bug("area_update_msizes(%x,%d) => addleft/top=%d/%d, subwidth/height=%d/%d\n", */
/*  	  obj, data->mad_Frame, data->mad_addleft, data->mad_addtop, data->mad_subwidth, data->mad_subheight)); */
}

/**************************************************************************
 MUIM_UpdateInnerSizes - Updates the innersizes of an object. You actually
 should only call this method if the dimensions of an object would not be
 affected, otherwise the results are unexpected
**************************************************************************/
static IPTR Area_UpdateInnerSizes(struct IClass *cl, Object *obj, struct MUIP_UpdateInnerSizes *msg)
{
    struct MUI_AreaData *data = INST_DATA(cl, obj);
    const struct ZuneFrameGfx *zframe;
    const struct MUI_FrameSpec_intern *frame;

    if (_flags(obj) & MADF_SETUP)
    {
	frame = get_intframe(obj, data);
	zframe = zune_zframe_get(frame);
        area_update_msizes(obj, data, frame, zframe);
    }
    return 1;
}

static IPTR Area_FindAreaObject(struct IClass *cl, Object *obj,
				struct MUIP_FindAreaObject *msg)
{
    if (msg->obj == obj)
	return (IPTR)obj;
    else
	return (IPTR)NULL;
}

BOOPSI_DISPATCHER(IPTR, Area_Dispatcher, cl, obj, msg)
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
	case MUIM_DrawParentBackground: return Area_DrawParentBackground(cl, obj, (APTR)msg);
	case MUIM_Setup: return Area_Setup(cl, obj, (APTR)msg);
	case MUIM_Cleanup: return Area_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Show: return Area_Show(cl, obj, (APTR)msg);
	case MUIM_Hide: return Area_Hide(cl, obj, (APTR)msg);
	case MUIM_GoActive: return Area_GoActive(cl, obj, (APTR)msg);
	case MUIM_GoInactive: return Area_GoInactive(cl, obj, (APTR)msg);
	case MUIM_Layout: return 1;
	case MUIM_CreateShortHelp: return Area_CreateShortHelp(cl, obj, (APTR)msg);
	case MUIM_DeleteShortHelp: return Area_DeleteShortHelp(cl, obj, (APTR)msg);
    	case MUIM_CreateBubble: return Area_CreateBubble(cl, obj, (APTR)msg);
	case MUIM_DeleteBubble: return Area_DeleteBubble(cl, obj, (APTR)msg);
	case MUIM_HandleEvent: return Area_HandleEvent(cl, obj, (APTR)msg);
	case MUIM_ContextMenuBuild: return Area_ContextMenuBuild(cl, obj, (APTR)msg);
	case MUIM_Timer: return Area_Timer(cl,obj,msg);
	case MUIM_UpdateInnerSizes: return Area_UpdateInnerSizes(cl,obj,(APTR)msg);
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
	case MUIM_FindAreaObject: return Area_FindAreaObject(cl, obj, (APTR)msg);

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
    (void*)Area_Dispatcher 
};

