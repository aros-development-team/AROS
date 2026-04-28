/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction window.class - BOOPSI class implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <utility/tagitem.h>

#include <string.h>

#include "window_intern.h"

#define WindowBase ((struct Library *)(cl->cl_UserData))

#define G(obj)  ((struct Gadget *)(obj))

/******************************************************************************/

static void window_set(Class *cl, Object *o, struct opSet *msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case WINDOW_Title:
                data->wcd_Title = (STRPTR)tag->ti_Data;
                if (data->wcd_Window)
                    SetWindowTitles(data->wcd_Window, data->wcd_Title, (UBYTE *)~0);
                break;

            case WINDOW_ScreenTitle:
                data->wcd_ScreenTitle = (STRPTR)tag->ti_Data;
                break;

            case WINDOW_MenuStrip:
                data->wcd_MenuStrip = (struct Menu *)tag->ti_Data;
                break;

            case WINDOW_Position:
                data->wcd_Position = tag->ti_Data;
                break;

            case WINDOW_UniqueID:
                data->wcd_UniqueID = tag->ti_Data;
                break;

            case WINDOW_Flags:
                data->wcd_Flags = tag->ti_Data;
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

            case WINDOW_BackfillHook:
                data->wcd_BackfillHook = (struct Hook *)tag->ti_Data;
                break;

            case WINDOW_SharedPort:
                data->wcd_UserPort = (struct MsgPort *)tag->ti_Data;
                break;

            case WINDOW_Icon:
                data->wcd_Icon = (struct DiskObject *)tag->ti_Data;
                break;

            case WINDOW_Iconified:
                data->wcd_Iconified = (BOOL)tag->ti_Data;
                break;

            case WINDOW_HelpGroup:
                data->wcd_HelpGroup = tag->ti_Data;
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

            case WINDOW_NewLookMenus:
                data->wcd_NewLookMenus = (BOOL)tag->ti_Data;
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
                break;

            case WA_InnerHeight:
            case WA_Height:
                data->wcd_Height = (WORD)tag->ti_Data;
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

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct WindowClassData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct WindowClassData));

        /* Set defaults */
        data->wcd_Position = WPOS_TOPLEFT;

        /* Process initial attributes */
        window_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Window__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);

    /* Close window if still open */
    if (data->wcd_Window)
    {
        if (data->wcd_MenuStrip)
            ClearMenuStrip(data->wcd_Window);

        CloseWindow(data->wcd_Window);
        data->wcd_Window = NULL;
    }

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
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
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

        case WINDOW_Title:
            *msg->opg_Storage = (IPTR)data->wcd_Title;
            return TRUE;

        case WINDOW_UniqueID:
            *msg->opg_Storage = data->wcd_UniqueID;
            return TRUE;

        case WINDOW_GadgetUserData:
            *msg->opg_Storage = data->wcd_GadgetUserData;
            return TRUE;

        case WINDOW_Iconified:
            *msg->opg_Storage = data->wcd_Iconified;
            return TRUE;

        case WINDOW_InnerWidth:
            if (data->wcd_Window)
                *msg->opg_Storage = data->wcd_Window->Width
                    - data->wcd_Window->BorderLeft - data->wcd_Window->BorderRight;
            else
                *msg->opg_Storage = 0;
            return TRUE;

        case WINDOW_InnerHeight:
            if (data->wcd_Window)
                *msg->opg_Storage = data->wcd_Window->Height
                    - data->wcd_Window->BorderTop - data->wcd_Window->BorderBottom;
            else
                *msg->opg_Storage = 0;
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

    if (data->wcd_Window)
        return (IPTR)data->wcd_Window;

    screen = data->wcd_Screen;
    if (!screen)
        screen = LockPubScreen(NULL);

    left   = data->wcd_Left;
    top    = data->wcd_Top;
    width  = data->wcd_Width  ? data->wcd_Width  : 400;
    height = data->wcd_Height ? data->wcd_Height : 300;

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

    data->wcd_Window = OpenWindowTags(NULL,
        WA_Left,        left,
        WA_Top,         top,
        WA_InnerWidth,  width,
        WA_InnerHeight, height,
        WA_Title,       (IPTR)data->wcd_Title,
        WA_ScreenTitle, (IPTR)data->wcd_ScreenTitle,
        WA_PubScreen,   (IPTR)screen,
        WA_DragBar,     TRUE,
        WA_DepthGadget, TRUE,
        WA_SizeGadget,  TRUE,
        WA_CloseGadget, TRUE,
        WA_Activate,    TRUE,
        WA_SimpleRefresh, TRUE,
        WA_IDCMP,       IDCMP_CLOSEWINDOW | IDCMP_GADGETUP |
                         IDCMP_MENUPICK | IDCMP_REFRESHWINDOW |
                         IDCMP_NEWSIZE | IDCMP_RAWKEY | IDCMP_VANILLAKEY |
                         IDCMP_CHANGEWINDOW | IDCMP_ACTIVEWINDOW |
                         IDCMP_INACTIVEWINDOW,
        WA_NewLookMenus, data->wcd_NewLookMenus,
        WA_SizeBBottom, TRUE,
        TAG_DONE);

    if (!data->wcd_Screen)
        UnlockPubScreen(NULL, screen);

    if (data->wcd_Window)
    {
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

            AddGList(data->wcd_Window, (struct Gadget *)data->wcd_Layout, -1, -1, NULL);
            RefreshGList((struct Gadget *)data->wcd_Layout, data->wcd_Window, NULL, -1);
        }

        /* Attach menu strip */
        if (data->wcd_MenuStrip)
            SetMenuStrip(data->wcd_Window, data->wcd_MenuStrip);
    }

    return (IPTR)data->wcd_Window;
}

/******************************************************************************/

IPTR Window__WM_CLOSE(Class *cl, Object *o, Msg msg)
{
    struct WindowClassData *data = INST_DATA(cl, o);

    if (data->wcd_Window)
    {
        if (data->wcd_Layout)
            RemoveGList(data->wcd_Window, (struct Gadget *)data->wcd_Layout, -1);

        if (data->wcd_MenuStrip)
            ClearMenuStrip(data->wcd_Window);

        CloseWindow(data->wcd_Window);
        data->wcd_Window = NULL;
    }

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

    if (code)
        *code = imsg->Code;

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
            /* Rethink layout on resize */
            if (data->wcd_Layout)
            {
                SetAttrs(data->wcd_Layout,
                    GA_Width,  data->wcd_Window->Width - data->wcd_Window->BorderLeft - data->wcd_Window->BorderRight,
                    GA_Height, data->wcd_Window->Height - data->wcd_Window->BorderTop - data->wcd_Window->BorderBottom,
                    TAG_DONE);
                RefreshGList((struct Gadget *)data->wcd_Layout, data->wcd_Window, NULL, -1);
            }
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
