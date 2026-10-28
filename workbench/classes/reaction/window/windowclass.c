/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction window.class - BOOPSI class implementation
*/
#define DEBUG 0

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/layers.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <utility/tagitem.h>
#include <reaction/reaction_prefs.h>
#include <exec/semaphores.h>

/* Fallback definitions in case the installed <gadgets/layout.h> doesn't
 * yet have the new layout-private methods we use to attach the gadget
 * tree to the window. Public header has matching definitions. */
#ifndef LM_ADDTOWINDOW
#define LM_ADDTOWINDOW          (LAYOUT_Dummy + 0x500)
#define LM_REMOVEFROMWINDOW     (LAYOUT_Dummy + 0x501)
struct lpAddToWindow
{
    ULONG               MethodID;
    struct Window      *lpaw_Window;
    struct Requester   *lpaw_Requester;
};
#endif

#include <string.h>

#include "window_intern.h"

#define WindowBase ((struct Library *)(cl->cl_UserData))

#define G(obj)  ((struct Gadget *)(obj))

/******************************************************************************/

/* WA_ tags passed on to the window as they are */
static BOOL window_keep_tag(struct WindowClassData *data, struct TagItem *tag)
{
    UWORD i;

    switch (tag->ti_Tag)
    {
        case WA_CloseGadget:
        case WA_DepthGadget:
        case WA_SizeGadget:
        case WA_DragBar:
            data->wcd_GadgetTags = TRUE;
            /* fall through */
        case WA_Flags:
        case WA_NoCareRefresh:
        case WA_SimpleRefresh:
        case WA_SmartRefresh:
        case WA_SizeBRight:
        case WA_SizeBBottom:
        case WA_GimmeZeroZero:
        case WA_Borderless:
        case WA_Activate:
        case WA_RMBTrap:
        case WA_Backdrop:
        case WA_SuperBitMap:
        case WA_BackFill:
        case WA_HelpGroup:
        case WA_MenuHelp:
        case WA_Zoom:
        case WA_NotifyDepth:
            break;
        default:
            return FALSE;
    }
    for (i = 0; i < data->wcd_NumWinTags; i++)
        if (data->wcd_WinTags[i].ti_Tag == tag->ti_Tag)
            break;
    if (i < 32)
    {
        data->wcd_WinTags[i] = *tag;
        if (i == data->wcd_NumWinTags)
            data->wcd_NumWinTags++;
        data->wcd_WinTags[data->wcd_NumWinTags].ti_Tag = TAG_DONE;
    }
    return TRUE;
}

static void window_set(Class *cl, Object *o, struct opSet *msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        if (window_keep_tag(data, tag))
            continue;
        switch (tag->ti_Tag)
        {
            case WA_IDCMP:
                data->wcd_IDCMP = tag->ti_Data;
                if (data->wcd_Window)
                    ModifyIDCMP(data->wcd_Window, data->wcd_Window->IDCMPFlags | data->wcd_IDCMP);
                break;

            case WA_Title:
                data->wcd_Title = (STRPTR)tag->ti_Data;
                if (data->wcd_Window)
                    SetWindowTitles(data->wcd_Window, data->wcd_Title, (UBYTE *)~0);
                break;

            case WA_ScreenTitle:
                data->wcd_ScreenTitle = (STRPTR)tag->ti_Data;
                break;

            case WINDOW_MenuStrip:
                data->wcd_MenuStrip = (struct Menu *)tag->ti_Data;
                break;

            case WINDOW_NewMenu:
                data->wcd_NewMenu = (struct NewMenu *)tag->ti_Data;
                break;

            case WINDOW_IconNoDispose:
                data->wcd_IconNoDispose = (BOOL)tag->ti_Data;
                break;

            case WINDOW_Position:
                data->wcd_Position = tag->ti_Data;
                break;

            case WINDOW_Layout:
                data->wcd_Layout = (Object *)tag->ti_Data;
                break;

            case WINDOW_GadgetUserData:
                data->wcd_GadgetUserData = (BOOL)tag->ti_Data;
                break;

            case WINDOW_IDCMPHook:
                data->wcd_IDCMPHook = (struct Hook *)tag->ti_Data;
                break;

            case WINDOW_IDCMPHookBits:
                data->wcd_IDCMPHookBits = tag->ti_Data;
                break;

            case WINDOW_AppPort:
                data->wcd_AppPort = (struct MsgPort *)tag->ti_Data;
                break;

            case WINDOW_AppMsgHook:
                data->wcd_AppMsgHook = (struct Hook *)tag->ti_Data;
                break;

            case WINDOW_SharedPort:
                data->wcd_UserPort = (struct MsgPort *)tag->ti_Data;
                break;

            case WINDOW_Icon:
                data->wcd_Icon = (struct DiskObject *)tag->ti_Data;
                break;

            case WINDOW_LockWidth:
                data->wcd_LockWidth = (BOOL)tag->ti_Data;
                break;

            case WINDOW_LockHeight:
                data->wcd_LockHeight = (BOOL)tag->ti_Data;
                break;

            case WINDOW_UserData:
                data->wcd_UserData = tag->ti_Data;
                break;

            case WINDOW_IconTitle:
                data->wcd_IconTitle = (STRPTR)tag->ti_Data;
                break;

            case WINDOW_IconifyGadget:
                data->wcd_IconifyGadget = (BOOL)tag->ti_Data;
                break;

            /* Standard Intuition window tags */
            case WA_Left:
                data->wcd_Left = (WORD)tag->ti_Data;
                break;

            case WA_Top:
                data->wcd_Top = (WORD)tag->ti_Data;
                break;

            case WA_InnerWidth:
            case WA_Width:
                data->wcd_Width = (WORD)tag->ti_Data;
                data->wcd_OuterWidth = (tag->ti_Tag == WA_Width);
                break;

            case WA_InnerHeight:
            case WA_Height:
                data->wcd_Height = (WORD)tag->ti_Data;
                data->wcd_OuterHeight = (tag->ti_Tag == WA_Height);
                break;

            case WA_PubScreen:
                data->wcd_Screen = (struct Screen *)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR Window__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[Window] OM_NEW: entry\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct WindowClassData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct WindowClassData));

        /* Set defaults */
        data->wcd_Position = WPOS_CENTERSCREEN;

        /* Process initial attributes */
        window_set(cl, (Object *)retval, msg);

        D(bug("[Window] OM_NEW: created obj 0x%p title='%s' position=%ld layout=0x%p\n",
            (APTR)retval, data->wcd_Title ? data->wcd_Title : (STRPTR)"(null)",
            (LONG)data->wcd_Position, data->wcd_Layout));
    }
    else
    {
        D(bug("[Window] OM_NEW: superclass failed\n"));
    }

    return retval;
}

/******************************************************************************/

/* Detach a shared IDCMP port before closing the window, discarding any
 * pending messages that belong to this window (CloseWindowSafely) */
/* WindowLimits() refuses a minimum above the current size, so while a
   ChangeWindowBox() is pending the limits are set again on IDCMP_NEWSIZE */
static void window_limits(struct WindowClassData *data)
{
    if (WindowLimits(data->wcd_Window, data->wcd_MinWidth, data->wcd_MinHeight, ~0, ~0))
        data->wcd_MinWidth = data->wcd_MinHeight = 0;
}

static void window_detach_sharedport(struct WindowClassData *data)
{
    if (data->wcd_UserPort && data->wcd_Window &&
        data->wcd_Window->UserPort == data->wcd_UserPort)
    {
        struct IntuiMessage *imsg;
        struct Node *succ;

        Forbid();

        imsg = (struct IntuiMessage *)data->wcd_UserPort->mp_MsgList.lh_Head;
        while ((succ = imsg->ExecMessage.mn_Node.ln_Succ))
        {
            if (imsg->IDCMPWindow == data->wcd_Window)
            {
                Remove((struct Node *)imsg);
                ReplyMsg((struct Message *)imsg);
            }
            imsg = (struct IntuiMessage *)succ;
        }

        data->wcd_Window->UserPort = NULL;
        ModifyIDCMP(data->wcd_Window, 0);

        Permit();
    }
}

/* Free a menu strip built from WINDOW_NewMenu, if any */
static void window_freemenus(struct WindowClassData *data)
{
    if (data->wcd_MenuCreated)
    {
        if (data->wcd_MenuStrip)
            FreeMenus(data->wcd_MenuStrip);
        data->wcd_MenuStrip = NULL;
        data->wcd_MenuCreated = FALSE;
    }
    if (data->wcd_VisualInfo)
    {
        FreeVisualInfo(data->wcd_VisualInfo);
        data->wcd_VisualInfo = NULL;
    }
}

IPTR Window__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);

    D(bug("[Window] OM_DISPOSE: obj 0x%p\n", o));

    /* Close window if still open */
    if (data->wcd_Window)
    {
        if (data->wcd_MenuStrip)
            ClearMenuStrip(data->wcd_Window);

        window_detach_sharedport(data);
        CloseWindow(data->wcd_Window);
        data->wcd_Window = NULL;
    }

    window_freemenus(data);

    /* Dispose the root layout */
    if (data->wcd_Layout)
    {
        DisposeObject(data->wcd_Layout);
        data->wcd_Layout = NULL;
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Window__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    D(bug("[Window] OM_SET: obj 0x%p\n", o));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    window_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Window__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case WINDOW_Window:
            *msg->opg_Storage = (IPTR)data->wcd_Window;
            return TRUE;

        case WINDOW_SigMask:
            if (data->wcd_Window)
            {
                if (data->wcd_UserPort)
                    *msg->opg_Storage = 1L << data->wcd_UserPort->mp_SigBit;
                else
                    *msg->opg_Storage = 1L << data->wcd_Window->UserPort->mp_SigBit;
            }
            else
                *msg->opg_Storage = 0;
            return TRUE;

        case WINDOW_UserData:
            *msg->opg_Storage = data->wcd_UserData;
            return TRUE;

        case WINDOW_GadgetUserData:
            *msg->opg_Storage = data->wcd_GadgetUserData;
            return TRUE;

        case WINDOW_Qualifier:
            *msg->opg_Storage = data->wcd_Qualifier;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Window__WM_OPEN(Class *cl, Object *o, Msg msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);
    struct Screen *screen;
    WORD left, top, width, height;
    UWORD layoutMinW = 0, layoutMinH = 0;

    D(bug("[Window] WM_OPEN: obj 0x%p title='%s'\n", o,
        data->wcd_Title ? data->wcd_Title : (STRPTR)"(null)"));

    if (data->wcd_Window)
        return (IPTR)data->wcd_Window;

    screen = data->wcd_Screen;
    if (!screen)
        screen = LockPubScreen(NULL);

    /* Query the layout's preferred minimum size before opening the window.
     * ReAction apps typically pass placeholder WA_Width/WA_Height values
     * and rely on the window auto-sizing to the layout's natural size.
     */
    if (data->wcd_Layout && screen)
    {
        struct GadgetInfo gi;
        struct gpDomain gpd;

        memset(&gi, 0, sizeof(gi));
        gi.gi_Screen = screen;
        gi.gi_Window = NULL;
        gi.gi_RastPort = &screen->RastPort;
        gi.gi_DrInfo = GetScreenDrawInfo(screen);

        memset(&gpd, 0, sizeof(gpd));
        gpd.MethodID  = GM_DOMAIN;
        gpd.gpd_GInfo = &gi;
        gpd.gpd_RPort = &screen->RastPort;
        gpd.gpd_Which = GDOMAIN_MINIMUM;

        if (DoMethodA(data->wcd_Layout, (Msg)&gpd))
        {
            layoutMinW = (UWORD)gpd.gpd_Domain.Width;
            layoutMinH = (UWORD)gpd.gpd_Domain.Height;
            D(bug("[Window] WM_OPEN: layout min size %ldx%ld\n",
                (LONG)layoutMinW, (LONG)layoutMinH));
        }

        if (gi.gi_DrInfo)
            FreeScreenDrawInfo(screen, gi.gi_DrInfo);
    }

    left   = data->wcd_Left;
    top    = data->wcd_Top;
    /* no size given: the layout's own minimum, as ReAction opens it */
    width  = data->wcd_Width  ? data->wcd_Width  : (layoutMinW ? layoutMinW : 400);
    height = data->wcd_Height ? data->wcd_Height : (layoutMinH ? layoutMinH : 300);

    /* Auto-fit to layout content. LockWidth/LockHeight forces the window to
     * exactly the layout's minimum size; otherwise we just ensure the inner
     * area is at least large enough to hold the layout.
     */
    if (layoutMinW && !(data->wcd_Width && data->wcd_OuterWidth))
    {
        if (data->wcd_LockWidth || width < layoutMinW)
            width = layoutMinW;
    }
    if (layoutMinH && !(data->wcd_Height && data->wcd_OuterHeight))
    {
        if (data->wcd_LockHeight || height < layoutMinH)
            height = layoutMinH;
    }

    /* Apply position policy */
    switch (data->wcd_Position)
    {
        case WPOS_CENTERSCREEN:
            left = (screen->Width - width) / 2;
            top  = (screen->Height - height) / 2;
            break;

        case WPOS_CENTERMOUSE:
            left = screen->MouseX - width / 2;
            top  = screen->MouseY - height / 2;
            break;

        case WPOS_FULLSCREEN:
            left   = 0;
            top    = screen->BarHeight + 1;
            width  = screen->Width;
            height = screen->Height - screen->BarHeight - 1;
            break;

        case WPOS_TOPLEFT:
        default:
            break;
    }

    /* Clamp to screen */
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (left + width > screen->Width) left = screen->Width - width;
    if (top + height > screen->Height) top = screen->Height - height;

    /* Default refresh mode from cap_SimpleRefresh prefs */
    BOOL simpleRefresh = TRUE;
    {
        struct UIPrefs *prefs;
        prefs = (struct UIPrefs *)FindSemaphore((STRPTR)RAPREFSSEMAPHORE);
        if (prefs)
        {
            ObtainSemaphoreShared(&prefs->cap_Semaphore);
            simpleRefresh = prefs->cap_SimpleRefresh ? TRUE : FALSE;
            ReleaseSemaphore(&prefs->cap_Semaphore);
        }
    }

    data->wcd_Window = OpenWindowTags(NULL,
        WA_Left,        left,
        WA_Top,         top,
        (data->wcd_Width && data->wcd_OuterWidth) ? WA_Width : WA_InnerWidth, width,
        (data->wcd_Height && data->wcd_OuterHeight) ? WA_Height : WA_InnerHeight, height,
        WA_Title,       (IPTR)data->wcd_Title,
        WA_ScreenTitle, (IPTR)data->wcd_ScreenTitle,
        WA_PubScreen,   (IPTR)screen,
        data->wcd_GadgetTags ? TAG_IGNORE : WA_DragBar,     TRUE,
        data->wcd_GadgetTags ? TAG_IGNORE : WA_DepthGadget, TRUE,
        data->wcd_GadgetTags ? TAG_IGNORE : WA_SizeGadget,  TRUE,
        data->wcd_GadgetTags ? TAG_IGNORE : WA_CloseGadget, TRUE,
        WA_Activate,    TRUE,
        simpleRefresh ? WA_SimpleRefresh : WA_SmartRefresh, TRUE,
        WA_IDCMP,       IDCMP_CLOSEWINDOW | IDCMP_GADGETUP |
                         IDCMP_MENUPICK | IDCMP_REFRESHWINDOW |
                         IDCMP_NEWSIZE | IDCMP_RAWKEY | IDCMP_VANILLAKEY |
                         IDCMP_CHANGEWINDOW | IDCMP_ACTIVEWINDOW |
                         IDCMP_INACTIVEWINDOW | data->wcd_IDCMP,
        WA_NewLookMenus, TRUE,
        WA_SizeBBottom, TRUE,
        /* the real limits need the borders: window_limits() below */
        WA_MinWidth,  1,
        WA_MinHeight, 1,
        WA_MaxWidth,  ~0,
        WA_MaxHeight, ~0,
        TAG_MORE,       (IPTR)data->wcd_WinTags);

    if (!data->wcd_Screen)
        UnlockPubScreen(NULL, screen);

    if (data->wcd_Window)
    {
        struct Window *w = data->wcd_Window;
        WORD minw = layoutMinW ? layoutMinW + w->BorderLeft + w->BorderRight : w->Width;
        WORD minh = layoutMinH ? layoutMinH + w->BorderTop + w->BorderBottom : w->Height;

        /* a size the program gave as the outer size can be below the
         * layout's minimum: grow, and shrink no further than that */
        if (w->Width < minw || w->Height < minh)
        {
            WORD nw = w->Width < minw ? minw : w->Width;
            WORD nh = w->Height < minh ? minh : w->Height;
            WORD nl = w->LeftEdge, nt = w->TopEdge;

            if (nl + nw > w->WScreen->Width) nl = w->WScreen->Width - nw;
            if (nt + nh > w->WScreen->Height) nt = w->WScreen->Height - nh;
            if (nl < 0) nl = 0;
            if (nt < 0) nt = 0;
            ChangeWindowBox(w, nl, nt, nw, nh);
        }
        data->wcd_MinWidth = minw;
        data->wcd_MinHeight = minh;
        window_limits(data);

        /* Route IDCMP through the application's shared port
         * (WINDOW_SharedPort) so its Wait() mask sees our messages */
        if (data->wcd_UserPort && data->wcd_Window->UserPort &&
            data->wcd_Window->UserPort != data->wcd_UserPort)
        {
            ULONG idcmp = data->wcd_Window->IDCMPFlags;

            ModifyIDCMP(data->wcd_Window, 0);
            data->wcd_Window->UserPort = data->wcd_UserPort;
            ModifyIDCMP(data->wcd_Window, idcmp);
        }

        /* Attach layout gadget */
        if (data->wcd_Layout)
        {
            /* Set the layout gadget dimensions to the window inner area */
            SetAttrs(data->wcd_Layout,
                GA_Left,   data->wcd_Window->BorderLeft,
                GA_Top,    data->wcd_Window->BorderTop,
                GA_Width,  data->wcd_Window->Width - data->wcd_Window->BorderLeft - data->wcd_Window->BorderRight,
                GA_Height, data->wcd_Window->Height - data->wcd_Window->BorderTop - data->wcd_Window->BorderBottom,
                TAG_DONE);

            /* Layout flattens itself + descendant gadgets into the window's
             * GList so Intuition can hit-test and dispatch IDCMP_GADGETUP for
             * each child. The layout itself is the head of that chain. */
            DoMethod(data->wcd_Layout, LM_ADDTOWINDOW,
                (IPTR)data->wcd_Window, (IPTR)NULL);
        }

        /* Build a menu strip from a GadTools NewMenu array if no
         * ready-made strip was provided (WINDOW_NewMenu) */
        if (!data->wcd_MenuStrip && data->wcd_NewMenu)
        {
            data->wcd_VisualInfo = GetVisualInfoA(data->wcd_Window->WScreen, NULL);
            if (data->wcd_VisualInfo)
            {
                data->wcd_MenuStrip = CreateMenusA(data->wcd_NewMenu, NULL);
                if (data->wcd_MenuStrip)
                {
                    struct TagItem lmtags[] = { { GTMN_NewLookMenus, TRUE }, { TAG_DONE, 0 } };
                    if (LayoutMenusA(data->wcd_MenuStrip, data->wcd_VisualInfo, lmtags))
                        data->wcd_MenuCreated = TRUE;
                    else
                    {
                        FreeMenus(data->wcd_MenuStrip);
                        data->wcd_MenuStrip = NULL;
                    }
                }
                if (!data->wcd_MenuCreated)
                {
                    FreeVisualInfo(data->wcd_VisualInfo);
                    data->wcd_VisualInfo = NULL;
                }
            }
        }

        /* Attach menu strip */
        if (data->wcd_MenuStrip)
            SetMenuStrip(data->wcd_Window, data->wcd_MenuStrip);
    }

    D(bug("[Window] WM_OPEN: result window=0x%p\n", data->wcd_Window));

    return (IPTR)data->wcd_Window;
}

/******************************************************************************/

IPTR Window__WM_CLOSE(Class *cl, Object *o, Msg msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);

    D(bug("[Window] WM_CLOSE: obj 0x%p window=0x%p\n", o, data->wcd_Window));

    if (data->wcd_Window)
    {
        if (data->wcd_Layout)
            DoMethod(data->wcd_Layout, LM_REMOVEFROMWINDOW,
                (IPTR)data->wcd_Window, (IPTR)NULL);

        if (data->wcd_MenuStrip)
            ClearMenuStrip(data->wcd_Window);

        window_detach_sharedport(data);
        CloseWindow(data->wcd_Window);
        data->wcd_Window = NULL;
    }

    /* Recreated on the (possibly different) screen at the next WM_OPEN */
    window_freemenus(data);

    return TRUE;
}

/******************************************************************************/

IPTR Window__WM_HANDLEINPUT(Class *cl, Object *o, Msg msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);
    struct IntuiMessage *imsg;
    ULONG result = WMHI_LASTMSG;
    WORD *code = (WORD *)((IPTR *)msg)[1];

    if (!data->wcd_Window)
        return WMHI_LASTMSG;

    imsg = (struct IntuiMessage *)GetMsg(data->wcd_Window->UserPort);
    if (!imsg)
        return WMHI_LASTMSG;

    D(bug("[Window] WM_HANDLEINPUT: obj 0x%p IDCMP class=0x%08lx code=0x%04lx\n",
        o, (ULONG)imsg->Class, (ULONG)imsg->Code));

    if (code)
        *code = imsg->Code;

    data->wcd_Qualifier = imsg->Qualifier;

    switch (imsg->Class)
    {
        case IDCMP_CLOSEWINDOW:
            result = WMHI_CLOSEWINDOW;
            break;

        case IDCMP_GADGETUP:
        {
            struct Gadget *gad = (struct Gadget *)imsg->IAddress;
            if (data->wcd_GadgetUserData && gad->UserData)
                result = WMHI_GADGETUP | (ULONG)(IPTR)gad->UserData;
            else
                result = WMHI_GADGETUP | gad->GadgetID;
            break;
        }

        case IDCMP_MENUPICK:
            result = WMHI_MENUPICK | (imsg->Code & 0xFFFF);
            break;

        case IDCMP_RAWKEY:
            result = WMHI_RAWKEY | (imsg->Code & 0xFFFF);
            break;

        case IDCMP_VANILLAKEY:
            result = WMHI_VANILLAKEY | (imsg->Code & 0xFFFF);
            break;

        case IDCMP_NEWSIZE:
            result = WMHI_NEWSIZE;
            if (data->wcd_MinWidth)
                window_limits(data);
            /* Rethink layout on resize */
            if (data->wcd_Layout)
            {
                SetAttrs(data->wcd_Layout,
                    GA_Width,  data->wcd_Window->Width - data->wcd_Window->BorderLeft - data->wcd_Window->BorderRight,
                    GA_Height, data->wcd_Window->Height - data->wcd_Window->BorderTop - data->wcd_Window->BorderBottom,
                    TAG_DONE);
                RefreshGList((struct Gadget *)data->wcd_Layout, data->wcd_Window, NULL, -1);
            }
            /* Repaint the borders - after shrinking they contain stale
             * pixels from the previously larger interior */
            RefreshWindowFrame(data->wcd_Window);
            break;

        case IDCMP_REFRESHWINDOW:
            BeginRefresh(data->wcd_Window);
            EndRefresh(data->wcd_Window, TRUE);
            break;

        case IDCMP_ACTIVEWINDOW:
            result = WMHI_ACTIVE;
            break;

        case IDCMP_INACTIVEWINDOW:
            result = WMHI_INACTIVE;
            break;

        case IDCMP_CHANGEWINDOW:
            result = WMHI_CHANGEWINDOW;
            break;

        default:
            /* Check IDCMP hook */
            if (data->wcd_IDCMPHook && (imsg->Class & data->wcd_IDCMPHookBits))
            {
                CallHookPkt(data->wcd_IDCMPHook, o, imsg);
            }
            break;
    }

    ReplyMsg((struct Message *)imsg);
    return result;
}

/******************************************************************************/

IPTR Window__WM_ICONIFY(Class *cl, Object *o, Msg msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);

    if (data->wcd_Window && !data->wcd_Iconified)
    {
        /* Save position */
        data->wcd_Left   = data->wcd_Window->LeftEdge;
        data->wcd_Top    = data->wcd_Window->TopEdge;
        data->wcd_Width  = data->wcd_Window->Width;
        data->wcd_Height = data->wcd_Window->Height;

        /* Close the window */
        DoMethod(o, WM_CLOSE);
        data->wcd_Iconified = TRUE;
    }

    return TRUE;
}

/******************************************************************************/

IPTR Window__WM_RETHINK(Class *cl, Object *o, Msg msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);

    if (data->wcd_Window && data->wcd_Layout)
    {
        SetAttrs(data->wcd_Layout,
            GA_Width,  data->wcd_Window->Width - data->wcd_Window->BorderLeft - data->wcd_Window->BorderRight,
            GA_Height, data->wcd_Window->Height - data->wcd_Window->BorderTop - data->wcd_Window->BorderBottom,
            TAG_DONE);
        RefreshGList((struct Gadget *)data->wcd_Layout, data->wcd_Window, NULL, -1);
    }

    return TRUE;
}
