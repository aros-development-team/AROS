/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#include <graphics/gfxmacros.h>
#include <graphics/rastport.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>

#include <zunepriv.h>
#include <renderinfo.h>
#include <window_impl.h>
#include <prefs.h>
#include <imagespec.h>
#include <key.h>
#include <Application.h>
#include <Window.h>
#include <windowdata.h>
#include <muikey.h>
#include <ctype.h>

static void handle_event(Object *win, struct IntuiMessage *event, ULONG mask);

/****************************************************************************/
/** Public functions                                                       **/
/****************************************************************************/

BOOL
_zune_renderinfo_setup (struct MUI_RenderInfo *mri)
{
    mri->mri_Screen = LockPubScreen(NULL);
    g_return_val_if_fail(mri->mri_Screen != NULL, FALSE);
    mri->mri_DrawInfo = GetScreenDrawInfo(mri->mri_Screen);
    g_return_val_if_fail(mri->mri_DrawInfo != NULL, FALSE);

    mri->mri_Pens = mri->mri_Pixels;

    mri->mri_Colormap     = mri->mri_Screen->ViewPort.ColorMap;
    mri->mri_ScreenWidth  = mri->mri_Screen->Width;
    mri->mri_ScreenHeight = mri->mri_Screen->Height;

    /* TODO: set MUIMRI_TRUECOLOR */
    /* TODO: set MUIMRI_THINFRAMES */

#if 0    
    /* use prefs pens as a temp to alloc pixels, copy pixels to mri */
    gdk_colormap_alloc_colors(mri->mri_Colormap, __zprefs.muipens, MPEN_COUNT,
			      FALSE, TRUE, success);

    mri->mri_FocusPixel = MUI_ObtainPen (mri, &__zprefs.active_object_color, 0);
 
    for (i = 0; i < MPEN_COUNT; i++)
    {
	mri->mri_Pens[i] = __zprefs.muipens[i].pixel;
	mri->mri_DrawInfo->dri_Pens[i+2] = mri->mri_Pens[i];
    }
#endif

#warning FIXME: allocate correct pens here
    mri->mri_Pixels[MPEN_SHINE     ] = mri->mri_DrawInfo->dri_Pens[SHINEPEN];
    mri->mri_Pixels[MPEN_HALFSHINE ] = mri->mri_DrawInfo->dri_Pens[SHINEPEN];
    mri->mri_Pixels[MPEN_BACKGROUND] = mri->mri_DrawInfo->dri_Pens[BACKGROUNDPEN];
    mri->mri_Pixels[MPEN_HALFSHADOW] = mri->mri_DrawInfo->dri_Pens[SHADOWPEN];
    mri->mri_Pixels[MPEN_SHADOW    ] = mri->mri_DrawInfo->dri_Pens[SHADOWPEN];
    mri->mri_Pixels[MPEN_TEXT      ] = mri->mri_DrawInfo->dri_Pens[TEXTPEN];
    mri->mri_Pixels[MPEN_FILL      ] = mri->mri_DrawInfo->dri_Pens[FILLPEN];
    mri->mri_Pixels[MPEN_MARK      ] = mri->mri_DrawInfo->dri_Pens[HIGHLIGHTTEXTPEN];

    return TRUE;
}

void
_zune_renderinfo_cleanup (struct MUI_RenderInfo *mri)
{
    FreeScreenDrawInfo(mri->mri_Screen, mri->mri_DrawInfo);
    mri->mri_DrawInfo = NULL;

    UnlockPubScreen(NULL, mri->mri_Screen);
    mri->mri_Screen = NULL;
}

void
_zune_renderinfo_show (struct MUI_RenderInfo *mri)
{
kprintf("*** _zune_renderinfo_show(): setting mri->mri_RastPort\n");
    mri->mri_RastPort = mri->mri_Window->RPort;
}

void
_zune_renderinfo_hide (struct MUI_RenderInfo *mri)
{
    mri->mri_RastPort = NULL;
}



ULONG
_zune_window_get_default_events (void)
{
    return IDCMP_NEWSIZE      | IDCMP_REFRESHWINDOW
         | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_MENUPICK
         | IDCMP_CLOSEWINDOW  | IDCMP_RAWKEY
         | IDCMP_ACTIVEWINDOW | IDCMP_INACTIVEWINDOW | IDCMP_VANILLAKEY;
}

void
_zune_window_change_events (struct MUI_WindowData *data)
{
    struct MinNode *mn;
    struct MUI_EventHandlerNode *ehn;
    ULONG new_events = _zune_window_get_default_events();
    ULONG old_events = data->wd_Events;
/*  	g_print("old events= %d  mask = %d\n", data->wd_Events, mask); */

    for (mn = data->wd_EHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
        ehn = (struct MUI_EventHandlerNode *)mn;
        new_events |= ehn->ehn_Events;
    }

    data->wd_Events = new_events;
    if ((old_events != new_events) && (data->wd_Flags & MUIWF_OPENED))
    {
        ModifyIDCMP(data->wd_RenderInfo.mri_Window, new_events);
/*  	g_print("set events\n"); */
    }
}


BOOL
_zune_window_open (struct MUI_WindowData *data)
{
    struct Window *win;
    ULONG flags = data->wd_CrtFlags;

    ASSERT(data->wd_RenderInfo.mri_Window == NULL);
    ASSERT(data->wd_RenderInfo.mri_WindowObject != NULL);

    if (data->wd_Flags & MUIWF_ACTIVE)
        flags |= WFLG_ACTIVATE;

    if (!(flags & WFLG_SIZEBRIGHT))
        flags |= WFLG_SIZEBBOTTOM;

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

        win->UserData = data->wd_RenderInfo.mri_WindowObject;
        win->UserPort = data->wd_UserPort; /* Same port for all windows */
        ModifyIDCMP(win, data->wd_Events);

        data->wd_RenderInfo.mri_Window = win;
kprintf(" >> &data->wd_RenderInfo=%lx\n", &data->wd_RenderInfo);

        return TRUE;
    }

    return FALSE;
}

void
_zune_window_close (struct MUI_WindowData *data)
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
                 succ = (struct IntuiMessage *)msg->ExecMessage.mn_Node.ln_Succ;
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

void
_zune_focus_new (Object *obj)
{
#ifdef _AROS

    struct RastPort *rp = _rp(obj);
    UWORD oldDrPt = rp->LinePtrn;

    int x1 = _left(obj) - 1;
    int y1 = _top(obj)  - 1;
    int x2 = _left(obj) + _width(obj);
    int y2 = _top(obj)  + _height(obj);

kprintf("*** _zune_focus_new()\n");
    SetABPenDrMd(rp, _pens(obj)[MPEN_SHINE], _pens(obj)[MPEN_SHADOW], JAM2);
    SetDrPt(rp, 0xCCCC);
    Move(rp, x1, y1);
    Draw(rp, x2, y1);
    Draw(rp, x2, y2);
    Draw(rp, x2, y1);
    Draw(rp, x1, y1);
    SetDrPt(rp, oldDrPt);

#else

    GdkWindowAttr  attributes;
    gint           attributes_mask = 0;
    WORD           w[4];
    WORD           h[4];
    WORD           x[4];
    WORD           y[4];
    int            i;
    GdkWindow    **focuswin = muiRenderInfo(obj)->mri_FocusWin;

    GdkColor color;
    color.pixel = muiRenderInfo(obj)->mri_FocusPixel;
/*  g_print("focus using pixel %lx\n", color.pixel); */
    i = 0;
    x[i] = _left(obj) - 1;
    y[i] = _top(obj) - 1;
    w[i] = _width(obj) + 2;
    h[i] = 1;
    i++;
    x[i] = _left(obj) - 1;
    y[i] = _top(obj) + _height(obj);
    w[i] = _width(obj) + 2;
    h[i] = 1;
    i++;
    x[i] = _left(obj) - 1;
    y[i] = _top(obj);
    w[i] = 1;
    h[i] = _height(obj);
    i++;
    x[i] = _left(obj) + _width(obj);
    y[i] = _top(obj);
    w[i] = 1;
    h[i] = _height(obj);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.colormap = muiRenderInfo(obj)->mri_Colormap;  
    attributes.event_mask = muiWindowData(obj)->wd_Events;   
    attributes_mask |= GDK_WA_X | GDK_WA_Y;
    attributes_mask |= (GDK_WA_VISUAL | GDK_WA_COLORMAP); 

    for (i = 0; i < 4; i++)
    {
	attributes.width = w[i];
	attributes.height = h[i];
	attributes.x = x[i];
	attributes.y = y[i];

	focuswin[i] = NULL;
	focuswin[i] = gdk_window_new (_window(obj), &attributes, attributes_mask);
	if (!focuswin[i])
	    return;
	gdk_window_set_background(focuswin[i], &color);
	gdk_window_clear(focuswin[i]);
	gdk_window_set_user_data (focuswin[i], _win(obj));
	gdk_window_show (focuswin[i]);    
    }

#endif
}

void
_zune_focus_destroy (Object *obj)
{
kprintf("*** _zune_focus_destroy()\n");

#ifdef _AROS

#warning FIXME: focus destroy (redraw object)

#else

    GdkWindow    **focuswin = muiRenderInfo(obj)->mri_FocusWin;
    int i;

    for (i = 0; i < 4; i++)
    {
	if (focuswin[i])
	    gdk_window_destroy(focuswin[i]);
	else
	    break;
	focuswin[i] = NULL;
    }

#endif
}

/**************/

void
_zune_window_message(struct IntuiMessage *imsg)
{
    struct Window *iWin;
    Object        *oWin;
//    Object *grab_object;
    struct MUI_WindowData *data;

    iWin = imsg->IDCMPWindow;
    oWin = (Object *)iWin->UserData;

    data = muiWindowData(oWin);
    ASSERT(data->wd_RenderInfo.mri_Window == iWin);

    switch (imsg->Class)
    {
    case IDCMP_NEWSIZE:
kprintf("NEWSIZE\n");
        if ((iWin->GZZWidth  != data->wd_Width)
         || (iWin->GZZHeight != data->wd_Height))
        {
            data->wd_Width  = iWin->GZZWidth;
            data->wd_Height = iWin->GZZHeight;
kprintf("NEWSIZE: hide root object\n");
            DoMethod(data->wd_RootObject, MUIM_Hide);
            
            _width(data->wd_RootObject) = data->wd_Width
                - (data->wd_innerLeft + data->wd_innerRight);
            _height(data->wd_RootObject) = data->wd_Height
                - (data->wd_innerBottom + data->wd_innerTop);
kprintf("NEWSIZE: layout root object\n");
            DoMethod(data->wd_RootObject, MUIM_Layout);
kprintf("NEWSIZE: show root object\n");
            DoMethod(data->wd_RootObject, MUIM_Show);

            MUI_Redraw(data->wd_RootObject, MADF_DRAWALL);
        }
        break;

    case IDCMP_REFRESHWINDOW:
kprintf("REFRESHWINDOW: MUI_BeginRefresh()\n");
        if (MUI_BeginRefresh(&data->wd_RenderInfo, 0))
        {
          /* redraw root object of window */
kprintf("REFRESHWINDOW: MUI_Redraw()\n");
          MUI_Redraw(data->wd_RootObject, MADF_DRAWALL);

kprintf("REFRESHWINDOW: MUI_EndRefresh()\n");
          MUI_EndRefresh(&data->wd_RenderInfo, 0);
        }
        break;

    case IDCMP_MOUSEBUTTONS:
        handle_event(oWin, imsg, imsg->Class);
        break;

    case IDCMP_MENUPICK:
        break;

    case IDCMP_CLOSEWINDOW:
kprintf("CLOSEWINDOW: MUIA_Window_CloseRequest = TRUE\n");
        set(oWin, MUIA_Window_CloseRequest, TRUE);
        break;

    case IDCMP_RAWKEY:
        break;

    case IDCMP_ACTIVEWINDOW:
kprintf("ACTIVEWINDOW\n");
        break;

    case IDCMP_INACTIVEWINDOW:
kprintf("INACTIVEWINDOW\n");
        break;

    case IDCMP_VANILLAKEY:
kprintf("VANILLAKEY $%02lx (%c)\n", imsg->Code, (imsg->Code & 0x60) ? imsg->Code : '?');
        switch (imsg->Code)
        {
        case '\t':
            if (imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
            {
kprintf(" -> MUIKEY_GADGET_PREV\n");
                set(oWin, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Prev);
            }
            else
            {
kprintf(" -> MUIKEY_GADGET_NEXT\n");
                set(oWin, MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);
            }
            break;

        case '\033': /* ESC */
kprintf(" -> MUIKEY_WINDOW_CLOSE\n");
            set(oWin, MUIA_Window_CloseRequest, TRUE);
            break;
        }
          
        break;
    }

#if 0
    /* If there is a grab in effect...
     */
    grab_object = win;

    /* Not all events get sent to the grabbing widget.
     * The delete, destroy, expose, focus change and resize
     *  events still get sent to the event widget because
     *  1) these events have no meaning for the grabbing widget
     *  and 2) redirecting these events to the grabbing widget
     *  could cause the display to be messed up.
     * 
     * Drag events are also not redirected, since it isn't
     *  clear what the semantics of that would be.
     */
    switch (event->type)
    {
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	    /* We treat button 4-5 specially, assume we have
	     * a MS-style scrollwheel mouse, and try to find
	     * a plausible widget to scroll. We also trap
	     * button 4-5 double and triple clicks here, since
	     * they will be generated if the user scrolls quickly.
	     */
	    break;

	case GDK_KEY_PRESS:
	    handle_key(win, (struct IntuiMessageKey *)event, GDK_KEY_PRESS_MASK);
	    break; 
	case GDK_KEY_RELEASE:
	    handle_key(win, (struct IntuiMessageKey *)event, GDK_KEY_RELEASE_MASK);
	    break;
	    /* key snoopers here */

	    /* else fall through */
	case GDK_MOTION_NOTIFY:
	    handle_event(win, event, GDK_POINTER_MOTION_MASK |
			 GDK_BUTTON_MOTION_MASK | GDK_BUTTON1_MOTION_MASK |
			 GDK_BUTTON2_MOTION_MASK | GDK_BUTTON3_MOTION_MASK);
	    break;
	case GDK_BUTTON_RELEASE:
	    handle_event(win, event, GDK_BUTTON_RELEASE_MASK);
	    break;
	case GDK_PROXIMITY_IN:
	    handle_event(win, event, GDK_PROXIMITY_IN_MASK);
	    break;
	case GDK_PROXIMITY_OUT:
	    handle_event(win, event, GDK_PROXIMITY_OUT_MASK);
	    break;
      

	    /*
	     * perhaps code to highlight active gadget here
	     */
	case GDK_ENTER_NOTIFY:
	    break;      
	case GDK_LEAVE_NOTIFY:
	    break;
      


	case GDK_DRAG_STATUS:
	case GDK_DROP_FINISHED:
	    break;
	case GDK_DRAG_ENTER:
	case GDK_DRAG_LEAVE:
	case GDK_DRAG_MOTION:
	case GDK_DROP_START:
	    break;
    }
#endif  
}

/****************************************************************************/
/** Private functions                                                      **/
/****************************************************************************/

static ULONG
invoke_event_handler (struct MUI_EventHandlerNode *ehn,
		      struct IntuiMessage *event, ULONG muikey)
{
    ULONG res;

    if (ehn->ehn_Class)
	res = CoerceMethod(ehn->ehn_Class, ehn->ehn_Object,
			   MUIM_HandleEvent, GPOINTER_TO_UINT(event), muikey);
    else
	res = DoMethod(ehn->ehn_Object,
		       MUIM_HandleEvent, GPOINTER_TO_UINT(event), muikey);
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

/* handle non-key events
 * event : event to handle
 * mask : event mask to match corresponding to this event
 */
static void
handle_event(Object *win, struct IntuiMessage *event, ULONG mask)
{
    struct MUI_WindowData *data = muiWindowData(win);
    struct MinNode *mn;
    struct MUI_EventHandlerNode *ehn;
    ULONG res;

    for (mn = data->wd_EHList.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
	ehn = (struct MUI_EventHandlerNode *)mn;

	if (ehn->ehn_Events & mask)
	{
	    res = invoke_event_handler(ehn, event, MUIKEY_NONE);
	    if (res & MUI_EventHandlerRC_Eat)
		break;
	}
    }
}
