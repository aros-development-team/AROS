/*
    Copyright (C) 2011-2026, The AROS Development Team. All rights reserved.

    Desc: Workbook Application Class
*/

#define DEBUG 0
#include <aros/debug.h>

#include <string.h>
#include <limits.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/gadtools.h>
#include <proto/workbench.h>
#include <proto/graphics.h>

#include <intuition/classusr.h>
#include <libraries/gadtools.h>
#include <workbench/handler.h>

#include "workbook_intern.h"
#include "workbook_menu.h"
#include "classes.h"

struct wbApp {
    struct MsgPort *WinPort;
    ULONG           WinMask;   /* Mask of our port(s) */
    struct MsgPort *AppPort;
    ULONG           AppMask;   /* Mask of our port(s) */
    Object         *Root;      /* Background 'root' window */
    struct MinList  Windows; /* Subwindows */
};

/* Open a drawer as a WBWindow and attach it to the application's window list. */
static void wbOpenDrawer(Class *cl, Object *obj, CONST_STRPTR path)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbApp *my = INST_DATA(cl, obj);
    Object *win;

    win = NewObject(WBWindow, NULL,
                        WBWA_UserPort, my->WinPort,
                        WBWA_Path, path,
                        TAG_END);

    if (win)
    {
        struct opMember opmmsg;
        opmmsg.MethodID = OM_ADDMEMBER;
        opmmsg.opam_Object = win;
        DoMethodA(obj, &opmmsg);
    }
}

/* Initialize WBApp instance state and create the message ports required
 * before Workbench registration. Visible Workbench state is created later.
 */
static IPTR WBAppNew(Class *cl, Object *obj, struct opSet *ops)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbApp *my;
    IPTR rc;

    rc = DoSuperMethodA(cl, obj, (Msg)ops);
    if (rc == 0)
        return 0;

    my = INST_DATA(cl, rc);

    NEWLIST(&my->Windows);

    /* Create our Workbench message port */
    my->AppPort = CreatePort(NULL, 0);

    if (my->AppPort == NULL) {
        DoSuperMethod(cl, (Object *)rc, OM_DISPOSE);
        return 0;
    }

    /* Create our Window message port */
    my->WinPort = CreatePort(NULL, 0);

    if (my->WinPort == NULL) {
        DeleteMsgPort(my->AppPort);
        DoSuperMethod(cl, (Object *)rc, OM_DISPOSE);
        return 0;
    }

    my->AppMask = (1UL << my->AppPort->mp_SigBit);
    my->WinMask = (1UL << my->WinPort->mp_SigBit);

    my->Root = NULL;

    return rc;
}

/* Handle OM_DISPOSE for WBApp-owned window state and message ports.
 * The root window may be absent when Workbench registration never succeeded.
 */
static IPTR WBAppDispose(Class *cl, Object *obj, Msg msg)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbApp *my = INST_DATA(cl, obj);
    Object *tstate = (Object *)my->Windows.mlh_Head;
    Object *tmp;

    /* Get rid of the subwindows */
    while ((tmp = NextObject(&tstate))) {
        STACKED ULONG omrmethodID;
        omrmethodID = OM_REMOVE;
        DoMethodA(tmp, &omrmethodID);
        DisposeObject(obj);
    }

    /* Get rid of the main window */
    if (my->Root)
        DisposeObject(my->Root);

    DeleteMsgPort(my->AppPort);
    DeleteMsgPort(my->WinPort);

    return DoSuperMethodA(cl, obj, msg);
}

/* Append a window object to the WBApp subwindow list. */
static IPTR WBAppAddMember(Class *cl, Object *obj, struct opMember *opm)
{
    struct opAddTail omatmsg;
    struct wbApp *my = INST_DATA(cl, obj);

    omatmsg.MethodID = OM_ADDTAIL;
    omatmsg.opat_List = (struct List *)&my->Windows;

    return DoMethodA(opm->opam_Object, &omatmsg);
}

/* Remove a window object from its current list membership. */
static IPTR WBAppRemMember(Class *cl, Object *obj, struct opMember *opm)
{
    STACKED ULONG omrmethodID;
    omrmethodID = OM_REMOVE;
    return DoMethodA(opm->opam_Object, &omrmethodID);
}


/* Return the WBWindow object that owns an Intuition window, or NULL if the
 * window is not managed by this WBApp instance.
 */
static Object *wbLookupWindow(Class *cl, Object *obj, struct Window *win)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbApp *my = INST_DATA(cl, obj);
    Object *ostate = (Object *)my->Windows.mlh_Head;
    Object *owin;
    struct Window *match = NULL;

    /* Is it the root window? */
    GetAttr(WBWA_Window, my->Root, (IPTR *)&match);
    if (match == win)
        return my->Root;

    while ((owin = NextObject(&ostate))) {
        match = NULL;
        GetAttr(WBWA_Window, owin, (IPTR *)&match);
        if (match == win)
            return owin;
    }

    return NULL;
}

/* Forward a refresh event to the WBWindow object that owns the window. */
static void wbRefreshWindow(Class *cl, Object *obj, struct Window *win)
{
    Object *owin;

    if ((owin = wbLookupWindow(cl, obj, win))) {
        STACKED ULONG wbrefmethodID;
        wbrefmethodID = WBWM_REFRESH;
        DoMethodA(owin, &wbrefmethodID);
    }
}

/* Forward a resize event to the WBWindow object that owns the window. */
static void wbNewSizeWindow(Class *cl, Object *obj, struct Window *win)
{
    Object *owin;

    if ((owin = wbLookupWindow(cl, obj, win))) {
        STACKED ULONG wbnewsmethodID;
        wbnewsmethodID = WBWM_NEWSIZE;
        DoMethodA(owin, &wbnewsmethodID);
    }
}

/* Detach and dispose the managed subwindow matching an Intuition window. */
static void wbCloseWindow(Class *cl, Object *obj, struct Window *win)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    Object *owin;

    if ((owin = wbLookupWindow(cl, obj, win))) {
        struct opMember opmmsg;
        opmmsg.MethodID = OM_REMMEMBER;
        opmmsg.opam_Object = owin;
        DoMethodA(obj, &opmmsg);
        DisposeObject(owin);
    }
}

/* Process an Intuition menu-selection chain. Window-specific commands are
 * offered to the owning WBWindow first; TRUE is returned when Quit is chosen.
 */
static BOOL wbMenuPick(Class *cl, Object *obj, struct Window *win, UWORD menuNumber)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    Object *owin;
    struct MenuItem *item;
    BOOL quit = FALSE;

    owin = wbLookupWindow(cl, obj, win);

    D(bug("Menu: %x\n", menuNumber));
    while (menuNumber != MENUNULL) {
        BOOL handled = FALSE;

        item = ItemAddress(win->MenuStrip, menuNumber);

        /* Let the window have first opportunity */
        if (owin)
        {
            struct wbwm_MenuPick wbmpmsg;
            wbmpmsg.MethodID = WBWM_MENUPICK;
            wbmpmsg.wbwmp_MenuItem = item;
            wbmpmsg.wbwmp_MenuNumber = menuNumber;
            handled = DoMethodA(owin, &wbmpmsg);
        }

        if (!handled) {
            switch (WBMENU_ITEM_ID(item)) {
            case WBMENU_ID(WBMENU_WB_QUIT):
                quit = TRUE;
                break;
            case WBMENU_ID(WBMENU_WB_SHUTDOWN):
                /* TODO: Ask if the user wants a shutdown or reboot */
                ShutdownA(SD_ACTION_POWEROFF);
                /* Can't power off. Try to reboot (cold preferred, warm fallback). */
                ShutdownA(SD_ACTION_REBOOT);
                break;
            }
        }

        menuNumber = item->NextSelect;
    }

    return quit;
}

/* Forward an Intuition tick to the WBWindow object that owns the window. */
static void wbIntuiTick(Class *cl, Object *obj, struct Window *win)
{
    Object *owin;

    if ((owin = wbLookupWindow(cl, obj, win))) {
        STACKED ULONG wbintuitmethodID;
        wbintuitmethodID = WBWM_INTUITICK;
        DoMethodA(owin, &wbintuitmethodID);
    }
}

/* Send WBWM_HIDE to every tracked subwindow. */
static void wbHideAllWindows(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbApp *my = INST_DATA(cl, obj);
    Object *ostate = (Object *)my->Windows.mlh_Head;
    Object *owin;

    while ((owin = NextObject(&ostate))) {
        STACKED ULONG wbhidemethodid;
        wbhidemethodid = WBWM_HIDE;
        DoMethodA(owin, &wbhidemethodid);
    }
}

/* Send WBWM_SHOW to every tracked subwindow. */
static void wbShowAllWindows(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbApp *my = INST_DATA(cl, obj);
    Object *ostate = (Object *)my->Windows.mlh_Head;
    Object *owin;

    while ((owin = NextObject(&ostate))) {
        STACKED ULONG wbshowmethodid;
        wbshowmethodid = WBWM_SHOW;
        DoMethodA(owin, &wbshowmethodid);
    }
}

/* Remove and dispose every tracked subwindow. */
static void wbCloseAllWindows(Class *cl, Object *obj)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbApp *my = INST_DATA(cl, obj);
    Object *ostate = (Object *)my->Windows.mlh_Head;
    Object *owin;

    while ((owin = NextObject(&ostate))) {
        struct opMember opmmsg;
        opmmsg.MethodID = OM_REMMEMBER;
        opmmsg.opam_Object = owin;
        DoMethodA(obj, &opmmsg);
        DisposeObject(owin);
    }
}

/* Acquire the single Workbench-handler registration, create the root window
 * only after registration succeeds, and service Workbench and Intuition events
 * until Quit. Returns FALSE if registration or root-window creation fails.
 */
static IPTR WBAppWorkbench(Class *cl, Object *obj, Msg msg)
{
    struct WorkbookBase *wb = (APTR)cl->cl_UserData;
    struct wbApp *my = INST_DATA(cl, obj);
    BOOL done = FALSE;

    if (RegisterWorkbench(my->AppPort)) {
        /* Do not create visible Workbench state until registration succeeds. */
        my->Root = NewObject(WBWindow, NULL,
                             WBWA_Path, NULL,
                             WBWA_UserPort, my->WinPort,
                             TAG_END);

        if (my->Root == NULL) {
            UnregisterWorkbench(my->AppPort);
            return FALSE;
        }

        CurrentDir(BNULL);

        while (!done) {
            ULONG mask;

            mask = Wait(my->AppMask | my->WinMask);

            if (mask & my->AppMask) {
                struct WBHandlerMessage *wbhm;

                while ((wbhm = (APTR)GetMsg(my->AppPort)) != NULL) {
                    switch (wbhm->wbhm_Type) {
                    case WBHM_TYPE_SHOW:
                        wbShowAllWindows(cl, obj);
                        break;
                    case WBHM_TYPE_HIDE:
                        wbHideAllWindows(cl, obj);
                        break;
                    case WBHM_TYPE_OPEN:
                        wbOpenDrawer(cl, obj, wbhm->wbhm_Data.Open.Name);
                        break;
                    case WBHM_TYPE_UPDATE:
                        break;
                    }
                    ReplyMsg((APTR)wbhm);
                }
            }

            if (mask & my->WinMask) {
                struct IntuiMessage *im;

                while ((im = GT_GetIMsg(my->WinPort)) != NULL) {
                    D(bug("im=%p, Class=%d, Code=%d\n", im, im->Class, im->Code));
                    switch (im->Class) {
                    case IDCMP_CLOSEWINDOW:
                        wbCloseWindow(cl, obj, im->IDCMPWindow);
                        break;
                    case IDCMP_REFRESHWINDOW:
                        wbRefreshWindow(cl, obj, im->IDCMPWindow);
                        break;
                    case IDCMP_NEWSIZE:
                        wbNewSizeWindow(cl, obj, im->IDCMPWindow);
                        break;
                    case IDCMP_MENUPICK:
                        done = wbMenuPick(cl, obj, im->IDCMPWindow, im->Code);
                        break;
                    case IDCMP_INTUITICKS:
                        wbIntuiTick(cl, obj, im->IDCMPWindow);
                        break;
                    }
                    GT_ReplyIMsg(im);
                }
            }
        }

        wbCloseAllWindows(cl, obj);

        UnregisterWorkbench(my->AppPort);
        return TRUE;
    }

    {
        struct EasyStruct es =
        {
            sizeof(struct EasyStruct),
            0,
            "Workbook",
            "Another Workbench application is already active.\n"
            "Quit it before starting Workbook.",
            "OK"
        };

        EasyRequestArgs(NULL, &es, NULL, NULL);
    }

    return FALSE;
}

/* Dispatch WBApp methods to their handlers and delegate unknown methods to
 * the superclass.
 */
static IPTR dispatcher(Class *cl, Object *obj, Msg msg)
{
    IPTR rc = 0;

    switch (msg->MethodID) {
    case OM_NEW:       rc = WBAppNew(cl, obj, (APTR)msg); break;
    case OM_DISPOSE:   rc = WBAppDispose(cl, obj, (APTR)msg); break;
    case OM_ADDMEMBER: rc = WBAppAddMember(cl, obj, (APTR)msg); break;
    case OM_REMMEMBER: rc = WBAppRemMember(cl, obj, (APTR)msg); break;
    case WBAM_WORKBENCH:rc = WBAppWorkbench(cl, obj, (APTR)msg); break;
    default:           rc = DoSuperMethodA(cl, obj, msg); break;
    }

    return rc;
}

/* Create the private WBApp BOOPSI class and bind it to the Workbook state.
 * Returns NULL if the class cannot be created.
 */
Class *WBApp_MakeClass(struct WorkbookBase *wb)
{
    Class *cl;

    cl = MakeClass( NULL, "rootclass", NULL,
                    sizeof(struct wbApp),
                    0);
    if (cl != NULL) {
        cl->cl_Dispatcher.h_Entry = HookEntry;
        cl->cl_Dispatcher.h_SubEntry = dispatcher;
        cl->cl_Dispatcher.h_Data = NULL;
        cl->cl_UserData = (IPTR)wb;
    }

    return cl;
}
