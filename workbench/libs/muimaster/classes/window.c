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

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <graphics/gfxmacros.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/commodities.h>
#include <proto/layers.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "support.h"
#include "classes/window.h"
#include "classes/area.h"

#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

static const int __version = 1;
static const int __revision = 1;

static void handle_event(Object *win, struct IntuiMessage *event);

/****************************************************************************/
/** Public functions                                                       **/
/****************************************************************************/

static BOOL SetupRenderInfo(struct MUI_RenderInfo *mri)
{
    int i;

    if (!(mri->mri_Screen = LockPubScreen(NULL))) return FALSE;
    if (!(mri->mri_DrawInfo = GetScreenDrawInfo(mri->mri_Screen)))
    {
	UnlockPubScreen(NULL,mri->mri_Screen);
	return FALSE;
    }

    mri->mri_Colormap     = mri->mri_Screen->ViewPort.ColorMap;
    mri->mri_ScreenWidth  = mri->mri_Screen->Width;
    mri->mri_ScreenHeight = mri->mri_Screen->Height;

    /* TODO: set MUIMRI_TRUECOLOR */
    /* TODO: set MUIMRI_THINFRAMES */

#if 0    
    mri->mri_FocusPixel = MUI_ObtainPen (mri, &__zprefs.active_object_color, 0);
#endif

   
    for (i=0;i<MPEN_COUNT;i++)
	mri->mri_PensStorage[i] = ObtainBestPenA(mri->mri_Colormap, __zprefs.muipens[i].red, __zprefs.muipens[i].green, __zprefs.muipens[i].blue, NULL);
    mri->mri_Pens = mri->mri_PensStorage;
    return TRUE;
}

void CleanupRenderInfo(struct MUI_RenderInfo *mri)
{
    int i;
    for (i=0;i<MPEN_COUNT;i++)
	ReleasePen(mri->mri_Colormap, mri->mri_PensStorage[i]);

    FreeScreenDrawInfo(mri->mri_Screen, mri->mri_DrawInfo);
    mri->mri_DrawInfo = NULL;

    UnlockPubScreen(NULL, mri->mri_Screen);
    mri->mri_Screen = NULL;
}

void ShowRenderInfo(struct MUI_RenderInfo *mri)
{
    mri->mri_RastPort = mri->mri_Window->RPort;
}

void HideRenderInfo(struct MUI_RenderInfo *mri)
{
    mri->mri_RastPort = NULL;
}



ULONG _zune_window_get_default_events (void)
{
    return IDCMP_NEWSIZE      | IDCMP_REFRESHWINDOW
         | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_MENUPICK
         | IDCMP_CLOSEWINDOW  | IDCMP_RAWKEY
         | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW;
}

void _zune_window_change_events (struct MUI_WindowData *data)
{
    struct MinNode *mn;
    struct MUI_EventHandlerNode *ehn;
    ULONG new_events = _zune_window_get_default_events();
    ULONG old_events = data->wd_Events;

    for (mn = data->wd_EHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
        ehn = (struct MUI_EventHandlerNode *)mn;
        new_events |= ehn->ehn_Events;
    }

    /* sba: kill the IDCMP_VANILLAKEY flag. MUI doesn't do this but programs
    ** which use this will behave different if they request for this flag
    ** (also on MUI)
    */
    new_events &= ~IDCMP_VANILLAKEY;

    data->wd_Events = new_events;
    if ((old_events != new_events) && (data->wd_Flags & MUIWF_OPENED))
    {
        ModifyIDCMP(data->wd_RenderInfo.mri_Window, new_events);
    }
}


BOOL DisplayWindow(struct MUI_WindowData *data)
{
    struct Window *win;
    ULONG flags = data->wd_CrtFlags;

/*    ASSERT(data->wd_RenderInfo.mri_Window == NULL);
    ASSERT(data->wd_RenderInfo.mri_WindowObject != NULL);
*/
    if (data->wd_Flags & MUIWF_ACTIVE)
        flags |= WFLG_ACTIVATE;

    if (!(flags & WFLG_SIZEBRIGHT))
        flags |= WFLG_SIZEBBOTTOM;

    /* The following calculations are not very correct, the size and dragbar
    ** are ignored also the current overscan view */
    if (data->wd_X == MUIV_Window_LeftEdge_Centered)
    {
    	data->wd_X = (data->wd_RenderInfo.mri_Screen->Width - data->wd_Width)/2;
    }

    if (data->wd_Y == MUIV_Window_TopEdge_Centered)
    {
    	data->wd_Y = (data->wd_RenderInfo.mri_Screen->Height - data->wd_Height)/2;
    }

    win = OpenWindowTags(NULL,
        WA_Left,         (IPTR)data->wd_X,
        WA_Top,          (IPTR)data->wd_Y,
        WA_Flags,        (IPTR)flags,
        WA_Title,        (IPTR)data->wd_Title,
        WA_CustomScreen, (IPTR)data->wd_RenderInfo.mri_Screen,
        WA_InnerWidth,   (IPTR)data->wd_Width,
        WA_InnerHeight,  (IPTR)data->wd_Height,
        WA_AutoAdjust,   (IPTR)TRUE,
        WA_NewLookMenus, (IPTR)TRUE,
        TAG_DONE);

    if (win)
    {
        int hborders = win->BorderLeft + win->BorderRight;
        int vborders = win->BorderTop  + win->BorderBottom;

        /* recalc window size (which will hopefully equal our requested size) */
        data->wd_Width  = win->GZZWidth;
        data->wd_Height = win->GZZHeight;

        /* set window limits according to window contents */
        WindowLimits(win, data->wd_MinMax.MinWidth  + hborders,
                          data->wd_MinMax.MinHeight + vborders,
                          data->wd_MinMax.MaxWidth  + hborders,
                          data->wd_MinMax.MaxHeight + vborders);

        win->UserData = (char*)data->wd_RenderInfo.mri_WindowObject;
        win->UserPort = data->wd_UserPort; /* Same port for all windows */
        ModifyIDCMP(win, data->wd_Events);

        data->wd_RenderInfo.mri_Window = win;
//	D(bug(" >> &data->wd_RenderInfo=%lx\n", &data->wd_RenderInfo));

        return TRUE;
    }

    return FALSE;
}

void UndisplayWindow(struct MUI_WindowData *data)
{
    struct Window *win = data->wd_RenderInfo.mri_Window;

    if (win != NULL)
    {
        data->wd_RenderInfo.mri_Window = NULL;

        /* store position and size */
        data->wd_X      = win->LeftEdge;
        data->wd_Y      = win->TopEdge;
        data->wd_Width  = win->GZZWidth;
        data->wd_Height = win->GZZHeight;

        ClearMenuStrip(win);

        if (win->UserPort)
        {
            struct IntuiMessage *msg, *succ;

            /* remove all messages pending for this window */
            Forbid();
            for (msg  = (struct IntuiMessage *)win->UserPort->mp_MsgList.lh_Head;
                 (succ = (struct IntuiMessage *)msg->ExecMessage.mn_Node.ln_Succ);
                 msg  = succ)
            {
                if (msg->IDCMPWindow == win)
                {
                    Remove((struct Node *)msg);
                    ReplyMsg((struct Message *)msg);
                }
            }
            win->UserPort = NULL;
            ModifyIDCMP(win, 0);
            Permit();
        }

        CloseWindow(win);
    }
}

void
_zune_window_resize (struct MUI_WindowData *data)
{
    struct Window *win = data->wd_RenderInfo.mri_Window;
    WORD dx = data->wd_Width  - win->Width;
    WORD dy = data->wd_Height - win->Height;

    WindowLimits(win, data->wd_MinMax.MinWidth,
                      data->wd_MinMax.MinHeight,
                      data->wd_MinMax.MaxWidth,
                      data->wd_MinMax.MaxHeight);

    SizeWindow(win, dx, dy);
}

/**************/

void _zune_window_message(struct IntuiMessage *imsg)
{
    struct Window *iWin;
    Object        *oWin;
    struct MUI_WindowData *data;

    iWin = imsg->IDCMPWindow;
    oWin = (Object *)iWin->UserData;

    data = muiWindowData(oWin);

    if (data->wd_DragObject)
    {
    	int finish_drag = 0;

    	if (imsg->Class == IDCMP_MOUSEMOVE)
    	{
    	    if (data->wd_DropObject)
    	    {
		if (imsg->MouseX < _left(data->wd_DropObject) || imsg->MouseX > _right(data->wd_DropObject) || imsg->MouseY < _top(data->wd_DropObject) || imsg->MouseY > _bottom(data->wd_DropObject))
		{
		    /* We have left the object */
		    UndrawDragNDrop(data->wd_dnd);
		    DoMethod(data->wd_DropObject, MUIM_DragFinish,data->wd_DragObject);
		    data->wd_DropObject = NULL;
		}
	    }

    	    if (!data->wd_DropObject)
    	    {
		struct Layer *l;
		Object *dest_wnd = NULL;

		/* Find out if app has an openend window at this position */
		if ((l = WhichLayer(&iWin->WScreen->LayerInfo, iWin->LeftEdge + imsg->MouseX, iWin->TopEdge + imsg->MouseY)))
		{
		    Object                *cstate;
		    Object                *child;
		    struct MinList        *ChildList;

		    get(_app(oWin), MUIA_Application_WindowList, (ULONG *)&(ChildList));
		    cstate = (Object *)ChildList->mlh_Head;
		    while ((child = NextObject(&cstate)))
		    {
			struct Window *wnd;
			get(child, MUIA_Window_Window,(ULONG*)&wnd);
			if (wnd->WLayer == l)
			{
			    data->wd_DropWindow = wnd;
			    dest_wnd = child;
			    break;
			}
		    }
		}

		if (dest_wnd)
		{
		    Object *root;
		    get(dest_wnd, MUIA_Window_RootObject, &root);

		    if (root)
		    {
			if ((data->wd_DropObject = (Object*)DoMethod(root,MUIM_DragQueryExtended,data->wd_DragObject,
		    			imsg->MouseX + iWin->LeftEdge - data->wd_DropWindow->LeftEdge,
		    			imsg->MouseY + iWin->TopEdge - data->wd_DropWindow->TopEdge)))
		    	{
			    UndrawDragNDrop(data->wd_dnd);
			    DoMethod(data->wd_DropObject, MUIM_DragBegin,data->wd_DragObject);
		        }
		    }
		}
	    }

	    if (data->wd_DropObject)
	    {
	    	LONG update = 0;
	    	LONG i;
	    	for (i=0;i<2;i++)
	    	{
		    LONG res = DoMethod(data->wd_DropObject,MUIM_DragReport,data->wd_DragObject,
						imsg->MouseX + iWin->LeftEdge - data->wd_DropWindow->LeftEdge,
						imsg->MouseY + iWin->TopEdge - data->wd_DropWindow->TopEdge,update);
		    switch (res)
		    {
			case    MUIV_DragReport_Abort:
				UndrawDragNDrop(data->wd_dnd);
				DoMethod(data->wd_DropObject, MUIM_DragFinish,data->wd_DragObject);
				data->wd_DropObject = NULL;
				break;

			case    MUIV_DragReport_Continue: break;
			case    MUIV_DragReport_Lock: break; /* NYI */
			case    MUIV_DragReport_Refresh:
				UndrawDragNDrop(data->wd_dnd);
				update = 1;
				break;
		    }
	    	}
	    }
	    DrawDragNDrop(data->wd_dnd, imsg->MouseX + iWin->LeftEdge , imsg->MouseY + iWin->TopEdge);
    	}

    	if (imsg->Class == IDCMP_MOUSEBUTTONS)
    	{
	    if ((imsg->Code == MENUDOWN)  || (imsg->Code == SELECTUP))
	    {
	    	if (imsg->Code == SELECTUP && data->wd_DropObject)
	    	{
		    UndrawDragNDrop(data->wd_dnd);
		    DoMethod(data->wd_DropObject, MUIM_DragFinish, data->wd_DragObject);
		    DoMethod(data->wd_DropObject, MUIM_DragDrop, data->wd_DragObject,
		    		imsg->MouseX + iWin->LeftEdge - data->wd_DropWindow->LeftEdge,
		    		imsg->MouseY + iWin->TopEdge - data->wd_DropWindow->TopEdge);
		    data->wd_DropObject = NULL;
	    	}
		finish_drag = 1;
	    }
	}

	if (imsg->Class == IDCMP_CLOSEWINDOW) finish_drag = 1;

	if (finish_drag)
	{
	    UndrawDragNDrop(data->wd_dnd);
	    if (data->wd_DropObject)
	    {
		DoMethod(data->wd_DropObject, MUIM_DragFinish,data->wd_DragObject);
		data->wd_DropObject = NULL;
	    }
	    DeleteDragNDrop(data->wd_dnd);
	    DoMethod(data->wd_DragObject,MUIM_DeleteDragImage, data->wd_DragImage);
	    muiAreaData(data->wd_DragObject)->mad_Flags &= ~MADF_DRAGGING;
	    data->wd_DragImage = NULL;
	    data->wd_DragObject = NULL;
	    data->wd_DropWindow = NULL;
	    data->wd_dnd = NULL;
    	}
    	return;
    }

    switch (imsg->Class)
    {
	case	IDCMP_NEWSIZE:
		if ((iWin->GZZWidth  != data->wd_Width) || (iWin->GZZHeight != data->wd_Height))
		{
		    data->wd_Width  = iWin->GZZWidth;
		    data->wd_Height = iWin->GZZHeight;
		    DoMethod(data->wd_RootObject, MUIM_Hide);

		    if (data->wd_RenderInfo.mri_Window->Flags & WFLG_SIMPLE_REFRESH)
		    {
		        data->wd_Flags |= MUIWF_RESIZING;
		    } else
		    {
			_width(data->wd_RootObject) = data->wd_Width - (data->wd_innerLeft + data->wd_innerRight);
			_height(data->wd_RootObject) = data->wd_Height - (data->wd_innerBottom + data->wd_innerTop);
			DoMethod(data->wd_RootObject, MUIM_Layout);
			DoMethod(data->wd_RootObject, MUIM_Show);
			MUI_Redraw(data->wd_RootObject, MADF_DRAWALL);
		    }
		}
	  	break;

	case	IDCMP_REFRESHWINDOW:
		if (data->wd_Flags & MUIWF_RESIZING)
		{
		    if (MUI_BeginRefresh(&data->wd_RenderInfo, 0))
		    {
			MUI_EndRefresh(&data->wd_RenderInfo, 0);
		    }

		    data->wd_Flags &= ~MUIWF_RESIZING;
		    _width(data->wd_RootObject) = data->wd_Width - (data->wd_innerLeft + data->wd_innerRight);
		    _height(data->wd_RootObject) = data->wd_Height - (data->wd_innerBottom + data->wd_innerTop);
		    DoMethod(data->wd_RootObject, MUIM_Layout);
		    DoMethod(data->wd_RootObject, MUIM_Show);
		    MUI_Redraw(data->wd_RootObject, MADF_DRAWALL);
		} else
		{
		    if (MUI_BeginRefresh(&data->wd_RenderInfo, 0))
		    {
			MUI_Redraw(data->wd_RootObject, MADF_DRAWALL);
			MUI_EndRefresh(&data->wd_RenderInfo, 0);
		    }
		}
		break;

	case	IDCMP_CLOSEWINDOW:
		set(oWin, MUIA_Window_CloseRequest, TRUE);
		nnset(oWin, MUIA_Window_CloseRequest, FALSE); /* I'm not sure here but zune keeps track of old values inside notifyclass */
		break;
    }

    handle_event(oWin, imsg);
}

/****************************************************************************/
/** Private functions                                                      **/
/****************************************************************************/

static ULONG invoke_event_handler (struct MUI_EventHandlerNode *ehn,
		      struct IntuiMessage *event, ULONG muikey)
{
    ULONG res;

    if (ehn->ehn_Class)
	res = CoerceMethod(ehn->ehn_Class, ehn->ehn_Object, MUIM_HandleEvent, (IPTR)event, muikey);
    else
	res = DoMethod(ehn->ehn_Object, MUIM_HandleEvent, (IPTR)event, muikey);
    return res;
}

#if 0

static void
handle_key(Object *win, struct IntuiMessageKey *event, gint mask)
{
    struct MUI_WindowData *data = muiWindowData(win);
    struct MinNode *mn;
    struct MUI_EventHandlerNode *ehn;
    ULONG res;
    ULONG muikey;

    /* Zune can handle the key itself */
    if (zune_key_translate(win, event, &muikey) == FALSE)
	return;

    /* try ActiveObject */
    if (data->wd_ActiveObject)
    {
	for (mn = data->wd_EHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
	{
	    ehn = (struct MUI_EventHandlerNode *)mn;

	    if ((ehn->ehn_Object == (Object *)data->wd_ActiveObject->data) &&
		(ehn->ehn_Events & mask))
	    {
		res = invoke_event_handler(ehn, (struct IntuiMessage *)event, muikey);
		if (res & MUI_EventHandlerRC_Eat)
		{
		    DoMethod(_app(ehn->ehn_Object), MUIM_Application_PushMethod,
			     GPOINTER_TO_UINT(ehn->ehn_Object), 3,
			     MUIM_HandleEvent, 0, MUIKEY_RELEASE);
		    return;
		}
	    }
	}
    }

    /* try DefaultObject */
    if ((data->wd_DefaultObject)
	&& !((data->wd_ActiveObject)
	     && (data->wd_DefaultObject == (Object *)data->wd_ActiveObject->data)))
    {	    
	for (mn = data->wd_EHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
	{
	    ehn = (struct MUI_EventHandlerNode *)mn;

	    if ((ehn->ehn_Object == data->wd_DefaultObject) &&
		(ehn->ehn_Events & mask))
	    {
		res = invoke_event_handler(ehn, (struct IntuiMessage *)event, muikey);
		if (res & MUI_EventHandlerRC_Eat)
		{
		    DoMethod(_app(ehn->ehn_Object), MUIM_Application_PushMethod,
			     GPOINTER_TO_UINT(ehn->ehn_Object), 3,
			     MUIM_HandleEvent, 0, MUIKEY_RELEASE);
		    return;
		}
	    }
	}
    }

    if ((event->string == NULL)
	|| (strlen(event->string) != 1))
	return;

    /* try Control Chars */
    for (mn = data->wd_CCList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
	ehn = (struct MUI_EventHandlerNode *)mn;

	if ((tolower(ehn->ehn_Events) == tolower(event->string[0])))
	{
	    res = invoke_event_handler(ehn, (struct IntuiMessage *)event, ehn->ehn_Flags);
	    if (res & MUI_EventHandlerRC_Eat)
	    {
		DoMethod(_app(ehn->ehn_Object), MUIM_Application_PushMethod,
			 GPOINTER_TO_UINT(ehn->ehn_Object), 3,
			 MUIM_HandleEvent, 0, MUIKEY_RELEASE);
		return;
	    }
	}
    }
    
}

#endif

/**************************************************************************
 ...
**************************************************************************/
static void handle_event(Object *win, struct IntuiMessage *event)
{
    struct MUI_WindowData *data = muiWindowData(win);
    struct MinNode *mn;
    struct MUI_EventHandlerNode *ehn;
    ULONG res;
    LONG muikey = MUIKEY_NONE;
    ULONG mask = event->Class;

    if (mask == IDCMP_RAWKEY)
    {
	struct InputEvent ievent;
	ievent.ie_NextEvent    = NULL;
	ievent.ie_Class        = IECLASS_RAWKEY;
	ievent.ie_SubClass     = 0;
	ievent.ie_Code         = event->Code;
	ievent.ie_Qualifier    = event->Qualifier;
	ievent.ie_EventAddress = (APTR *) *((ULONG *)(event->IAddress));

	for (muikey=MUIKEY_COUNT-1;muikey >= 0;muikey--) /* 0 == MUIKEY_PRESS */
	{
	    if (__zprefs.muikeys[muikey].ix_well && MatchIX(&ievent,&__zprefs.muikeys[muikey].ix))
		break;
	}
	if (muikey == MUIKEY_PRESS && (event->Code & IECODE_UP_PREFIX)) muikey = MUIKEY_RELEASE;
    }

    /* try ActiveObject */
    if (data->wd_ActiveObject)
    {
	/* sba: I'm not sure if the active object also receives
	** other events than the muikey stuff first. IMO not.
	** Also which method should be used? MUIM_HandleInput or
	** MUIM_HandleEvent. Also note that there is a flag MUI_EHF_ALWAYSKEYS
	** which probably means that all keys events are requested??
	** For now MUIM_HandleEvent is used as this is currently implemented
	** in Area class ;) although I guess it should be MUIM_HandleInput as this
	** was earlier
	*/

	if (muikey != MUIKEY_NONE)
	{
	    res = DoMethod(data->wd_ActiveObject->obj, MUIM_HandleEvent, (IPTR)event, muikey);
	    if (res & MUI_EventHandlerRC_Eat) return;
	}

/*	for (mn = data->wd_EHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
	{
	    ehn = (struct MUI_EventHandlerNode *)mn;

	    if ((ehn->ehn_Object == data->wd_ActiveObject->obj) &&
		(ehn->ehn_Events & mask))
	    {
		res = invoke_event_handler(ehn, (struct IntuiMessage *)event, muikey);
		if (res & MUI_EventHandlerRC_Eat)
		    return;
	    }
	}*/
    }

    /* try DefaultObject */

    /* try Control Chars */
    if (mask == IDCMP_RAWKEY)
    {
    	struct IntuiMessage imsg;
    	ULONG key;

	/* Remove the up prefix as convert key does not convert a upkey event */
    	imsg = *event;
    	imsg.Code &= ~IECODE_UP_PREFIX;
    	key = ConvertKey(&imsg);

    	if (key)
    	{
            for (mn = data->wd_CCList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
            {
		ehn = (struct MUI_EventHandlerNode *)mn;

		if (ehn->ehn_Events == key)
		{
		    LONG muikey = ehn->ehn_Flags;
		    if (event->Code & IECODE_UP_PREFIX)
		    {
			if (muikey == MUIKEY_PRESS) muikey = MUIKEY_RELEASE;
			else muikey = MUIKEY_RELEASE;
		    }

		    if (muikey != MUIKEY_NONE)
		    {
			res = DoMethod(ehn->ehn_Object, MUIM_HandleEvent, (IPTR)event, muikey);
			if (res & MUI_EventHandlerRC_Eat) return;
		    }
		}
	    }
	}
    }
    

    /* try eventhandler */
    for (mn = data->wd_EHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
	ehn = (struct MUI_EventHandlerNode *)mn;

	if (ehn->ehn_Events & mask)
	{
	    res = invoke_event_handler(ehn, event, muikey);
	    if (res & MUI_EventHandlerRC_Eat)
		return;
	}
    }

    /* nobody has eaten the message so we can try ourself */
    switch (muikey)
    {
    	case	MUIKEY_PRESS:break;
	case	MUIKEY_TOGGLE:break;
	case	MUIKEY_UP:break;
	case	MUIKEY_DOWN:break;
	case	MUIKEY_PAGEUP:break;
	case	MUIKEY_PAGEDOWN:break;
	case	MUIKEY_TOP:break;
	case	MUIKEY_BOTTOM:break;
	case	MUIKEY_LEFT:break;
	case	MUIKEY_RIGHT:break;
	case	MUIKEY_WORDLEFT:break;
	case	MUIKEY_WORDRIGHT:break;
	case	MUIKEY_LINESTART:break;
	case	MUIKEY_LINEEND:break;
	case	MUIKEY_GADGET_NEXT:set(win, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);break;
	case	MUIKEY_GADGET_PREV:set(win, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Prev);break;
	case	MUIKEY_GADGET_OFF:set(win, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);break;
	case	MUIKEY_WINDOW_CLOSE:set(win, MUIA_Window_CloseRequest, TRUE);nnset(win, MUIA_Window_CloseRequest, FALSE);break;
	case	MUIKEY_WINDOW_NEXT:break;
	case	MUIKEY_WINDOW_PREV:break;
	case	MUIKEY_HELP:break;
	case	MUIKEY_POPUP:break;
	default: break;
    }
}

/******************************************************************************/
/******************************************************************************/
static ULONG window_Open(struct IClass *cl, Object *obj);
static ULONG window_Close(struct IClass *cl, Object *obj);

/* code for setting MUIA_Window_RootObject */
static void window_change_root_object (struct MUI_WindowData *data, Object *obj,
			   Object *newRoot)
{
    Object *oldRoot = data->wd_RootObject;

    if (!(data->wd_Flags & MUIWF_OPENED))
    {
	if (oldRoot)
	{
	    if (data->wd_ActiveObject && data->wd_ActiveObject->obj == oldRoot)
		set(obj, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
	    DoMethod(oldRoot, MUIM_DisconnectParent);
	}

	data->wd_RootObject = newRoot;
	if (newRoot)
	{
	    /* if window is in App tree, inform child */
	    if (muiNotifyData(obj)->mnd_GlobalInfo)
		DoMethod(newRoot, MUIM_ConnectParent, (IPTR)obj);
	}
    }
}

static struct ObjNode *FindObjNode(struct MUI_WindowData *data, Object *obj)
{
    struct ObjNode *node;
    for (node = (struct ObjNode*)data->wd_CCList.mlh_Head; node->node.mln_Succ; node = (struct ObjNode*)node->node.mln_Succ)
    {
    	if (node->obj == obj)
	{
	    return node;
	}
    }
    return NULL;
}

/* code for setting MUIA_Window_ActiveObject */
static void window_set_active_object (struct MUI_WindowData *data, Object *obj,
			  ULONG newval)
{
    if (data->wd_ActiveObject && (ULONG)data->wd_ActiveObject->obj == newval)
	return;

    switch (newval)
    {
	case MUIV_Window_ActiveObject_None:
	    if (data->wd_ActiveObject)
	    {
		DoMethod(data->wd_ActiveObject->obj, MUIM_GoInactive);
		data->wd_ActiveObject = NULL;
	    }
	    break;

	case MUIV_Window_ActiveObject_Next:
	    if (data->wd_ActiveObject)
	    {
		DoMethod(data->wd_ActiveObject->obj, MUIM_GoInactive);
		data->wd_ActiveObject = (struct ObjNode*)((data->wd_ActiveObject->node.mln_Succ->mln_Succ)?(data->wd_ActiveObject->node.mln_Succ):NULL);
	    }
	    else if (!IsListEmpty((struct List*)&data->wd_CycleChain))
		data->wd_ActiveObject = (struct ObjNode*)data->wd_CycleChain.mlh_Head;
	    break;

	case MUIV_Window_ActiveObject_Prev:
	    if (data->wd_ActiveObject)
	    {
		DoMethod(data->wd_ActiveObject->obj, MUIM_GoInactive);
		data->wd_ActiveObject = (struct ObjNode*)((data->wd_ActiveObject->node.mln_Pred->mln_Pred)?(data->wd_ActiveObject->node.mln_Pred):NULL);
	    }
	    else if (!IsListEmpty((struct List*)&data->wd_CycleChain))
		data->wd_ActiveObject = (struct ObjNode*)data->wd_CycleChain.mlh_TailPred;
	    break;

	default:
	    if (data->wd_ActiveObject)
	    {
		DoMethod(data->wd_ActiveObject->obj, MUIM_GoInactive);
	    }
	    else if (!newval == NULL) return;
	    data->wd_ActiveObject = FindObjNode(data,(Object*)newval);
	    break;
    }

    if (data->wd_ActiveObject)
    {
	DoMethod(data->wd_ActiveObject->obj, MUIM_GoActive);
    }
}


/*
 * calculate real dimensions from programmer requirements.
 * may be overridden by user settings if MUIA_Window_ID is set.
 */
/* MUIV_Window_Height_Screen and MUIV_Window_Height_Visible
 * are not handled yet, as their Width couterparts.
 */
static void window_select_dimensions (struct MUI_WindowData *data)
{
    if (!data->wd_Width)
    {
	if (data->wd_ReqWidth > 0)
	    data->wd_Width = data->wd_ReqWidth;
	else if (data->wd_ReqWidth == MUIV_Window_Width_Default)
	    data->wd_Width = data->wd_MinMax.DefWidth;
	else if (_between(MUIV_Window_Width_MinMax(100),
			  data->wd_ReqWidth,
			  MUIV_Window_Width_MinMax(0)))
	{
	    data->wd_Width = data->wd_MinMax.MinWidth
		- data->wd_ReqWidth
		* (data->wd_MinMax.MaxWidth - data->wd_MinMax.MinWidth);
	}
	else if (_between(MUIV_Window_Width_Screen(100),
			  data->wd_ReqWidth,
			  MUIV_Window_Width_Screen(0)))
	{
	    data->wd_Width = data->wd_RenderInfo.mri_ScreenWidth
		* (- (data->wd_ReqWidth + 200)) / 100;
	}
	else if (_between(MUIV_Window_Width_Visible(100),
			  data->wd_ReqWidth,
			  MUIV_Window_Width_Visible(0)))
	{
	    data->wd_Width = data->wd_RenderInfo.mri_ScreenWidth
		* (- (data->wd_ReqWidth + 100)) / 100;
	}

	if (data->wd_ReqHeight > 0)
	    data->wd_Height = data->wd_ReqHeight;
	else if (data->wd_ReqHeight == MUIV_Window_Height_Default)
	    data->wd_Height = data->wd_MinMax.DefHeight;
	else if (_between(MUIV_Window_Height_MinMax(100),
			  data->wd_ReqHeight,
			  MUIV_Window_Height_MinMax(0)))
	{
	    data->wd_Height = data->wd_MinMax.MinHeight
		- data->wd_ReqHeight
		* (data->wd_MinMax.MaxHeight - data->wd_MinMax.MinHeight);
	}
	else if (_between(MUIV_Window_Height_Screen(100),
			  data->wd_ReqHeight,
			  MUIV_Window_Height_Screen(0)))
	{
	    data->wd_Height = data->wd_RenderInfo.mri_ScreenHeight
		* (- (data->wd_ReqHeight + 200)) / 100;
	}
	else if (_between(MUIV_Window_Height_Visible(100),
			  data->wd_ReqHeight,
			  MUIV_Window_Height_Visible(0)))
	{
	    data->wd_Height = data->wd_RenderInfo.mri_ScreenHeight
		* (- (data->wd_ReqHeight + 100)) / 100;
	}

	/* scaled */
	if (data->wd_ReqWidth == MUIV_Window_Width_Scaled)
	    data->wd_Width = data->wd_Height * data->wd_MinMax.MinWidth
		/ data->wd_MinMax.MinHeight;
	else if (data->wd_ReqHeight == MUIV_Window_Width_Scaled)
	    data->wd_Height = data->wd_Width * data->wd_MinMax.MinHeight
		/ data->wd_MinMax.MinWidth;
    }
    data->wd_Width = CLAMP(data->wd_Width, data->wd_MinMax.MinWidth,
			   data->wd_MinMax.MaxWidth);
    data->wd_Height = CLAMP(data->wd_Height, data->wd_MinMax.MinHeight,
			    data->wd_MinMax.MaxHeight);
}


/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Window_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_WindowData *data;
    struct TagItem *tags,*tag;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return FALSE;

    /* Initial local instance data */
    data = INST_DATA(cl, obj);
    data->wd_RenderInfo.mri_WindowObject = obj;

    NewList((struct List*)&(data->wd_EHList));
    NewList((struct List*)&(data->wd_CCList));
    NewList((struct List*)&(data->wd_CycleChain));

    data->wd_CrtFlags = WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET
                      | WFLG_SIMPLE_REFRESH | WFLG_REPORTMOUSE | WFLG_NEWLOOKMENUS;

    data->wd_Events = _zune_window_get_default_events();
    data->wd_ActiveObject = NULL;
    data->wd_ID = 0;
    data->wd_Title = "";
    data->wd_ReqHeight = MUIV_Window_Height_Default;
    data->wd_ReqWidth = MUIV_Window_Width_Default;
    data->wd_RootObject = NULL;
    data->wd_DefaultObject = NULL;
    data->wd_Flags = 0;
/* alternate dimensions */
/* no change in coordinates */
    data->wd_AltDim.Top = MUIV_Window_AltTopEdge_NoChange;
    data->wd_AltDim.Left = MUIV_Window_AltLeftEdge_NoChange;
/* default to min size */
    data->wd_AltDim.Width = MUIV_Window_AltWidth_MinMax(0);
    data->wd_AltDim.Height = MUIV_Window_AltHeight_MinMax(0);
    data->wd_X = MUIV_Window_LeftEdge_Centered;
    data->wd_Y = MUIV_Window_TopEdge_Centered;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem((struct TagItem **)&tags)); )
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Window_CloseGadget:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_CLOSEGADGET);
	    break;
	case MUIA_Window_SizeGadget:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_SIZEGADGET);
	    break;
	case MUIA_Window_Backdrop:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_BACKDROP);
	    break;
	case MUIA_Window_Borderless:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_BORDERLESS);
	    break;
	case MUIA_Window_DepthGadget:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_DEPTHGADGET);
	    break;
	case MUIA_Window_DragBar:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_DRAGBAR);
	    break;
	case MUIA_Window_SizeRight:
	    _handle_bool_tag(data->wd_CrtFlags, tag->ti_Data, WFLG_SIZEBRIGHT);
	    break;
	case MUIA_Window_Height:
	    data->wd_ReqHeight = (LONG)tag->ti_Data;
	    break;
	case MUIA_Window_Width:
	    data->wd_ReqWidth = (LONG)tag->ti_Data;
	    break;
	case MUIA_Window_ID:
	    set(obj, MUIA_Window_ID, tag->ti_Data);
	    break;
	case MUIA_Window_Title:
	    set(obj, MUIA_Window_Title, tag->ti_Data);
	    break;
	case MUIA_Window_Activate:
	    set(obj, MUIA_Window_Activate, tag->ti_Data);
	    break;
	case MUIA_Window_DefaultObject:
	    set(obj, MUIA_Window_DefaultObject, tag->ti_Data);
	    break;
	case MUIA_Window_RootObject:
	    if (!tag->ti_Data)
	    {
		CoerceMethod(cl, obj, OM_DISPOSE);
		return 0;
	    }
	    set(obj, MUIA_Window_RootObject, tag->ti_Data);
	    break;
	case MUIA_Window_AltHeight:
	    data->wd_AltDim.Height = (WORD)tag->ti_Data;
	    break;
	case MUIA_Window_AltWidth:
	    data->wd_AltDim.Width = (WORD)tag->ti_Data;
	    break;
	case MUIA_Window_AltLeftEdge:
	    data->wd_AltDim.Left = (WORD)tag->ti_Data;
	    break;
	case MUIA_Window_AltTopEdge:
	    data->wd_AltDim.Top = (WORD)tag->ti_Data;
	    break;
	case MUIA_Window_AppWindow:
	    break;
	}
    }

    D(bug("muimaster.library/window.c: Window Object created at 0x%lx\n",obj));

    return (ULONG)obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
static ULONG Window_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    if ((data->wd_Flags & MUIWF_OPENED))
    {
	set(obj, MUIA_Window_Open, FALSE);
    }
    if (data->wd_RootObject)
	MUI_DisposeObject(data->wd_RootObject);
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
static ULONG Window_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
    struct TagItem        *tags = msg->ops_AttrList;
    struct TagItem        *tag;

    while ((tag = NextTagItem((struct TagItem **)&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	    case MUIA_Window_Activate:
		_handle_bool_tag(data->wd_Flags, tag->ti_Data, MUIWF_ACTIVE);
		break;
	    case MUIA_Window_ActiveObject:
		window_set_active_object(data, obj, tag->ti_Data);
		break;
	    case MUIA_Window_CloseRequest:
		_handle_bool_tag(data->wd_Flags, tag->ti_Data, MUIWF_CLOSEREQUESTED);
		break;
	    case MUIA_Window_DefaultObject:
		data->wd_DefaultObject = (APTR)tag->ti_Data;
		break;
	    case MUIA_Window_ID:
		data->wd_ID = tag->ti_Data;
		break;
	    case MUIA_Window_Open:
		if (tag->ti_Data && !(data->wd_Flags & MUIWF_OPENED))
		    window_Open(cl, obj);
		else if (!tag->ti_Data && (data->wd_Flags & MUIWF_OPENED))
		    window_Close(cl, obj);
		break;
	    case MUIA_Window_RootObject:
		window_change_root_object(data, obj, (Object *)tag->ti_Data);
		break;
	    case MUIA_Window_Title:
		data->wd_Title = (STRPTR)tag->ti_Data;
		break;
	}
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
static ULONG Window_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_WindowData *data = INST_DATA(cl, obj);

    STORE = (ULONG)0;

    switch(msg->opg_AttrID)
    {
	case MUIA_Window_Activate:
	    STORE = (data->wd_Flags & MUIWF_ACTIVE) ? TRUE : FALSE;
	    return(TRUE);
        case MUIA_Window_Window:
            STORE = (data->wd_Flags & MUIWF_OPENED) ? ((ULONG)data->wd_RenderInfo.mri_Window) : FALSE;
            return 1;
	case MUIA_Window_ActiveObject:
	    if (data->wd_ActiveObject)
		STORE = (ULONG)data->wd_ActiveObject->obj;
	    return(TRUE);
	case MUIA_Window_CloseRequest:
	    STORE = (data->wd_Flags & MUIWF_CLOSEREQUESTED) ? TRUE : FALSE;
	    return(TRUE);
	case MUIA_Window_DefaultObject:
	    STORE = (ULONG)data->wd_DefaultObject;
	    return(TRUE);
	case MUIA_Window_Height:
	    STORE = (ULONG)data->wd_Height;
	    return(TRUE);
	case MUIA_Window_ID:
	    STORE = data->wd_ID;
	    return(TRUE);
	case MUIA_Window_Open:
	    STORE = (data->wd_Flags & MUIWF_OPENED) ? TRUE : FALSE;
	    return(TRUE);
	case MUIA_Window_RootObject:
	    STORE = (ULONG)data->wd_RootObject;
	    return(TRUE);
	case MUIA_Window_Title:
	    STORE = (ULONG)data->wd_Title;
	    return(TRUE);
	case MUIA_Window_Width:
	    STORE = (ULONG)data->wd_Width;
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
 Called by Application (parent) object whenever this object is added.
 init GlobalInfo
**************************************************************************/
static ULONG Window_ConnectParent(struct IClass *cl, Object *obj,
		     struct MUIP_ConnectParent *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl,obj,(Msg)msg)) return 0;

    if (data->wd_RootObject)
	DoMethod(data->wd_RootObject, MUIM_ConnectParent, (IPTR)obj);

    return TRUE;
}


/**************************************************************************
 called by parent object
**************************************************************************/
static ULONG Window_DisconnectParent(struct IClass *cl, Object *obj, struct MUIP_DisconnectParent *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    if (data->wd_RootObject)
	DoMethodA(data->wd_RootObject, (Msg)msg);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/*
 * Called before window is opened or resized. It determines its bounds,
 * so you can call window_select_dimensions() to find the final dims.
 */
static void window_minmax (struct MUI_WindowData *data)
{
    /* inquire about sizes */
    DoMethod(data->wd_RootObject, MUIM_AskMinMax, (ULONG)&data->wd_MinMax);
    __area_finish_minmax(data->wd_RootObject, &data->wd_MinMax);

    data->wd_innerLeft   = __zprefs.window_inner_left;
    data->wd_innerRight  = __zprefs.window_inner_right;
    data->wd_innerTop    = __zprefs.window_inner_top;
    data->wd_innerBottom = __zprefs.window_inner_bottom;

    data->wd_MinMax.MinWidth += data->wd_innerLeft + data->wd_innerRight;
    data->wd_MinMax.MaxWidth += data->wd_innerLeft + data->wd_innerRight;
    data->wd_MinMax.DefWidth += data->wd_innerLeft + data->wd_innerRight;
    data->wd_MinMax.MinHeight += data->wd_innerTop + data->wd_innerBottom;
    data->wd_MinMax.MaxHeight += data->wd_innerTop + data->wd_innerBottom;
    data->wd_MinMax.DefHeight += data->wd_innerTop + data->wd_innerBottom;

/*      g_print("Window minmax: min=%dx%d, max=%dx%d, def=%dx%d\n (%dx%d)", */
/*  	    data->wd_MinMax.MinWidth, data->wd_MinMax.MinHeight, */
/*  	    data->wd_MinMax.MaxWidth, data->wd_MinMax.MaxHeight, */
/*  	    data->wd_MinMax.DefWidth, data->wd_MinMax.DefHeight, */
/*  	    data->wd_Width, data->wd_Height); */
}

/*
 * Called after window is opened or resized.
 * An expose event is already queued, it will trigger
 * MUIM_Draw for us when going back to main loop.
 */
static void window_show (struct MUI_WindowData *data)
{
    struct Window *win = data->wd_RenderInfo.mri_Window;

/*      int i; */

    _left(data->wd_RootObject) = data->wd_innerLeft + win->BorderLeft;
    _top(data->wd_RootObject)  = data->wd_innerTop  + win->BorderTop;
    _width(data->wd_RootObject) = data->wd_Width
	- (data->wd_innerLeft + data->wd_innerRight);
    _height(data->wd_RootObject) = data->wd_Height
	- (data->wd_innerBottom + data->wd_innerTop);

    DoMethod(data->wd_RootObject, MUIM_Layout);

    ShowRenderInfo(&data->wd_RenderInfo);

/*  g_print("SHOW\n"); */
/*      for (i = 0; i < MUII_Count; i++) */
/*  	zune_imspec_show(__zprefs.images[i], NULL); */
    DoMethod(data->wd_RootObject, MUIM_Show);
}

static ULONG window_Open(struct IClass *cl, Object *obj)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    if (!data->wd_RootObject)
	return FALSE;

/*  g_print("SETUP\n"); */
    if (!DoMethod(obj, MUIM_Window_Setup))
	return FALSE;
    /* I got display info, so calculate your display dependant data */
    if (!DoMethod(data->wd_RootObject, MUIM_Setup, (IPTR)&data->wd_RenderInfo))
    {
	DoMethod(obj, MUIM_Window_Cleanup);
	return FALSE;
    }
    /* no frame/inner space for root object ! */
    muiAreaData(data->wd_RootObject)->mad_Frame = 0;
    _addleft(data->wd_RootObject) = 0;
    _addtop(data->wd_RootObject) = 0;
    _subwidth(data->wd_RootObject) = 0;
    _subheight(data->wd_RootObject) = 0;

    /* inquire about sizes */
    window_minmax(data);
    window_select_dimensions(data);

    

    data->wd_UserPort = muiGlobalInfo(obj)->mgi_UserPort;

    /* open window here ... */
    if (!DisplayWindow(data))
    {
	/* free display dependant data */
	DoMethod(data->wd_RootObject, MUIM_Cleanup);
	DoMethod(obj, MUIM_Window_Cleanup);
	return FALSE;
    }

    data->wd_Flags |= MUIWF_OPENED;

    window_show(data);

    MUI_Redraw(data->wd_RootObject, MADF_DRAWALL);
    return TRUE;
}

/******************************************************************************/
/******************************************************************************/

static ULONG window_Close(struct IClass *cl, Object *obj)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    /* remove from window */
    DoMethod(data->wd_RootObject, MUIM_Hide);
/*    for (i = 0; i < MUII_Count; i++) */
/*        zune_imspec_hide(__zprefs.images[i]); */
    HideRenderInfo(&data->wd_RenderInfo);

    /* close here ... */
    UndisplayWindow(data);
    data->wd_Flags &= ~MUIWF_OPENED;

    /* free display dependant data */
    DoMethod(data->wd_RootObject, MUIM_Cleanup);
    DoMethod(obj, MUIM_Window_Cleanup);

    return TRUE;
}

static ULONG
mRecalcDisplay(struct IClass *cl, Object *obj,
	             struct MUIP_Window_RecalcDisplay *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
/*      int i; */

/*  g_print("recalc\n"); */
/*  g_print("HIDE\n"); */
    DoMethod(data->wd_RootObject, MUIM_Hide);
/*      for (i = 0; i < MUII_Count; i++) */
/*  	zune_imspec_hide(__zprefs.images[i]); */
    HideRenderInfo(&data->wd_RenderInfo);

    /* inquire about sizes */
    window_minmax(data);
    /* resize window ? */
    window_select_dimensions(data);
    _zune_window_resize(data);
    window_show(data);
//kprintf("Window->RecalcDisplay calling MUI_Redraw()...\n");
    MUI_Redraw(data->wd_RootObject, MADF_DRAWOBJECT);
    return TRUE;
}


/**************************************************************************
 ...
**************************************************************************/
static ULONG Window_AddEventHandler(struct IClass *cl, Object *obj,
                 struct MUIP_Window_AddEventHandler *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    D(bug("muimaster.library/window.c: Add Eventhandler\n"));

    Enqueue((struct List *)&data->wd_EHList, (struct Node *)msg->ehnode);
    _zune_window_change_events(data);
    return TRUE;
}

/**************************************************************************
 ...
**************************************************************************/
static ULONG Window_RemEventHandler(struct IClass *cl, Object *obj,
                 struct MUIP_Window_RemEventHandler *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    D(bug("muimaster.library/window.c: Rem Eventhandler\n"));

    Remove((struct Node *)msg->ehnode);
    _zune_window_change_events(data);
    return TRUE;
}

/**************************************************************************
 Note that this is MUIM_Window_Setup, not MUIM_Setup
**************************************************************************/
static ULONG Window_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
    if (!SetupRenderInfo(&data->wd_RenderInfo))
	return FALSE;

    return TRUE;
}

/**************************************************************************

**************************************************************************/
static ULONG Window_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    if (data->wd_dnd)
    {
    	DeleteDragNDrop(data->wd_dnd);
    	data->wd_dnd = NULL;
    }

    CleanupRenderInfo(&data->wd_RenderInfo);
    return TRUE;
}


/**************************************************************************
 This adds the the control char handler and also do the MUIA_CycleChain
 stuff. Orginal MUI does this in an other way.
**************************************************************************/
static ULONG Window_AddControlCharHandler(struct IClass *cl, Object *obj, struct MUIP_Window_AddControlCharHandler *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);

    Enqueue((struct List *)&data->wd_CCList, (struct Node *)msg->ccnode);

    /* Due to the lack of an better idea ... */
    if (muiAreaData(msg->ccnode->ehn_Object)->mad_Flags & MADF_CYCLECHAIN)
    {
	struct ObjNode *node = mui_alloc_struct(struct ObjNode);
	if (node)
	{
	    node->obj = msg->ccnode->ehn_Object;
	    AddTail((struct List *)&data->wd_CycleChain,(struct Node*)node);
	}
    }
    return TRUE;
}

/**************************************************************************
 
**************************************************************************/
static ULONG Window_RemControlCharHandler(struct IClass *cl, Object *obj, struct MUIP_Window_RemControlCharHandler *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
    struct ObjNode     *node = FindObjNode(data,msg->ccnode->ehn_Object);

    Remove((struct Node *)msg->ccnode);
    if (node)
    {
    	/* Remove from the chain list */
	Remove((struct Node *)node);
	mui_free(node);
    }

    return TRUE;
}

/**************************************************************************
 
**************************************************************************/
static ULONG Window_DragObject(struct IClass *cl, Object *obj, struct MUIP_Window_DragObject *msg)
{
    struct MUI_WindowData *data = INST_DATA(cl, obj);
    if (msg->obj)
    {
	struct DragNDrop *dnd;
	struct MUI_DragImage *di;
	struct BitMapNode *bmn;

	if (!(dnd = CreateDragNDropA(NULL))) return NULL;
	if (!(di = (struct MUI_DragImage*)DoMethod(msg->obj,MUIM_CreateDragImage,-msg->touchx,-msg->touchy,msg->flags)))
	{
	    DeleteDragNDrop(dnd);
	    return 0;
	}
	if (!di->bm)
	{
	    DoMethod(msg->obj,MUIM_DeleteDragImage, di);
	    DeleteDragNDrop(dnd);
	    return 0;
	}

	if (!(bmn = CreateBitMapNode(
		GUI_BitMap, di->bm,
		GUI_LeftOffset, di->touchx,
		GUI_TopOffset, di->touchy,
		GUI_Width, di->width,
		GUI_Height, di->height,
		TAG_DONE)))
	{
	    DoMethod(msg->obj,MUIM_DeleteDragImage, di);
	    DeleteDragNDrop(dnd);
	    return 0;
	}

	AttachBitMapNode(dnd,bmn);

	if (!PrepareDragNDrop(dnd, data->wd_RenderInfo.mri_Screen))
	{
	    DoMethod(msg->obj,MUIM_DeleteDragImage, di);
	    DeleteDragNDrop(dnd);
	    return 0;
	}

	muiAreaData(msg->obj)->mad_Flags |= MADF_DRAGGING;

	data->wd_DragObject = msg->obj;
	data->wd_dnd = dnd;
	data->wd_DragImage = di;
	return 1;
    }
    return 0;
}


/******************************************************************************/
/******************************************************************************/

#ifndef _AROS
__asm IPTR Window_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Window_Dispatcher,
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
	case OM_NEW: return Window_New(cl, obj, (struct opSet *) msg);
	case OM_DISPOSE: return Window_Dispose(cl, obj, msg);
	case OM_SET: return Window_Set(cl, obj, (struct opSet *)msg);
	case OM_GET: return Window_Get(cl, obj, (struct opGet *)msg);
	case MUIM_Window_AddEventHandler: return Window_AddEventHandler(cl, obj, (APTR)msg);
	case MUIM_Window_RemEventHandler: return Window_RemEventHandler(cl, obj, (APTR)msg);
	case MUIM_ConnectParent: return Window_ConnectParent(cl, obj, (APTR)msg);
	case MUIM_DisconnectParent: return Window_DisconnectParent(cl, obj, (APTR)msg);
	case MUIM_Window_RecalcDisplay :
	    return(mRecalcDisplay(cl, obj, (APTR)msg));
	case MUIM_Window_Setup: return Window_Setup(cl, obj, (APTR)msg);
	case MUIM_Window_Cleanup: return Window_Cleanup(cl, obj, (APTR)msg);
	case MUIM_Window_AddControlCharHandler: return Window_AddControlCharHandler(cl, obj, (APTR)msg);
	case MUIM_Window_RemControlCharHandler: return Window_RemControlCharHandler(cl, obj, (APTR)msg);
	case MUIM_Window_DragObject: return Window_DragObject(cl, obj, (APTR)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}


/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Window_desc = {
    MUIC_Window, 
    MUIC_Notify, 
    sizeof(struct MUI_WindowData), 
    Window_Dispatcher 
};

