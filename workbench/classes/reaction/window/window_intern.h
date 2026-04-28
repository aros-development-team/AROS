/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction window.class - Internal definitions
*/

#ifndef WINDOW_INTERN_H
#define WINDOW_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif
#ifndef CLASSES_WINDOW_H
#include <classes/window.h>
#endif

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

/* Method IDs */
#define WM_Dummy        0x6A00
#define WM_OPEN         (WM_Dummy + 1)
#define WM_CLOSE        (WM_Dummy + 2)
#define WM_HANDLEINPUT  (WM_Dummy + 3)
#define WM_ICONIFY      (WM_Dummy + 4)
#define WM_UNICONIFY    (WM_Dummy + 5)
#define WM_NEWPREFS     (WM_Dummy + 6)
#define WM_RETHINK      (WM_Dummy + 7)

/* Window class instance data */
struct WindowClassData
{
    struct Window       *wcd_Window;        /* Intuition Window */
    struct Screen       *wcd_Screen;        /* Screen for window */
    struct MsgPort      *wcd_UserPort;      /* Shared user port, if any */
    struct MsgPort      *wcd_AppPort;       /* AppWindow port */
    Object              *wcd_Layout;        /* Root layout object */
    struct Menu         *wcd_MenuStrip;     /* Window menus */
    struct Hook         *wcd_IDCMPHook;     /* Custom IDCMP hook */
    struct Hook         *wcd_BackfillHook;  /* Background fill hook */
    struct Hook         *wcd_AppMsgHook;    /* AppWindow message hook */
    struct DiskObject   *wcd_Icon;          /* Iconify icon */

    STRPTR              wcd_Title;          /* Window title */
    STRPTR              wcd_ScreenTitle;    /* Screen title */

    ULONG               wcd_Flags;          /* Window flags */
    ULONG               wcd_IDCMPHookBits;  /* IDCMP bits for hook */
    ULONG               wcd_UniqueID;       /* Unique window ID */
    ULONG               wcd_Position;       /* Window position */
    ULONG               wcd_HelpGroup;      /* Help group */
    ULONG               wcd_UserData;       /* User data */

    WORD                wcd_Left;           /* Window left edge */
    WORD                wcd_Top;            /* Window top edge */
    WORD                wcd_Width;          /* Window width */
    WORD                wcd_Height;         /* Window height */

    BOOL                wcd_GadgetUserData; /* Use gadget user data */
    BOOL                wcd_Iconified;      /* Iconified state */
    BOOL                wcd_LockWidth;      /* Lock width */
    BOOL                wcd_LockHeight;     /* Lock height */
    BOOL                wcd_NewLookMenus;   /* New look menus */

    /* IDCMP message processing */
    struct IntuiMessage *wcd_CurrentMsg;    /* Current message being processed */
    ULONG               wcd_MsgClass;       /* Current message class */
    UWORD               wcd_MsgCode;        /* Current message code */
    APTR                wcd_MsgIAddress;    /* Current message IAddress */
};

#endif /* WINDOW_INTERN_H */
