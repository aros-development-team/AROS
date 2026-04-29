/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible classes/window.h
*/

#ifndef CLASSES_WINDOW_H
#define CLASSES_WINDOW_H

/****************************************************************************/

#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

/****************************************************************************/

/*
 * window.class - ClassAct/ReAction compatible window management class
 *
 * Superclass: rootclass
 * Include:    <classes/window.h>
 */

/* Attributes specific to window.class */

#define WINDOW_Dummy            (REACTION_Dummy + 0x25000)

#define WINDOW_Window           (WINDOW_Dummy + 1)  /* struct Window * — the underlying Intuition window */

#define WINDOW_SigMask          (WINDOW_Dummy + 2)  /* ULONG — signal mask for this window */

#define WINDOW_MenuStrip        (WINDOW_Dummy + 4)  /* struct Menu * — menu strip attached to window */

#define WINDOW_Layout           (WINDOW_Dummy + 5)  /* Object * — root layout group */
#define WINDOW_ParentLayout     WINDOW_Layout
#define WINDOW_ParentGroup      WINDOW_Layout

#define WINDOW_UserData         (WINDOW_Dummy + 6)  /* APTR — application-private data */

#define WINDOW_SharedPort       (WINDOW_Dummy + 7)  /* struct MsgPort * — shared IDCMP port (not freed on dispose) */

#define WINDOW_Zoom             (WINDOW_Dummy + 8)  /* BOOL — simulate zoom gadget click */

#define WINDOW_FrontBack        (WINDOW_Dummy + 9)  /* UWORD — WT_FRONT or WT_BACK */

#define WINDOW_Activate         (WINDOW_Dummy + 10) /* BOOL — activate this window */

#define WINDOW_LockWidth        (WINDOW_Dummy + 11) /* BOOL — prevent horizontal resizing */

#define WINDOW_LockHeight       (WINDOW_Dummy + 12) /* BOOL — prevent vertical resizing */

#define WINDOW_AppPort          (WINDOW_Dummy + 13) /* struct MsgPort * — port for AppMessages; required for iconification */

#define WINDOW_Position         (WINDOW_Dummy + 14) /* ULONG — initial window placement (see WPOS_*) */

#define WINDOW_IDCMPHook        (WINDOW_Dummy + 15) /* struct Hook * — custom IDCMP message handler */

#define WINDOW_IDCMPHookBits    (WINDOW_Dummy + 16) /* ULONG — IDCMP flags routed to hook */

#define WINDOW_GadgetUserData   (WINDOW_Dummy + 17) /* UWORD — gadget UserData interpretation mode (see WGUD_*) */
#define WINDOW_InterpretUserData    WINDOW_GadgetUserData

#define WINDOW_MenuUserData     (WINDOW_Dummy + 25) /* UWORD — menu item UserData interpretation mode */

#define WGUD_HOOK    0  /* UserData is a Hook pointer */
#define WGUD_FUNC    1  /* UserData is a function pointer */
#define WGUD_IGNORE  2  /* UserData is application-private */

#define WINDOW_IconTitle        (WINDOW_Dummy + 18) /* STRPTR — title shown when iconified */

#define WINDOW_AppMsgHook       (WINDOW_Dummy + 19) /* struct Hook * — called on AppMessages */

#define WINDOW_Icon             (WINDOW_Dummy + 20) /* struct DiskObject * — custom icon for iconified state */

#define WINDOW_AppWindow        (WINDOW_Dummy + 21) /* BOOL — register as Workbench AppWindow */

#define WINDOW_GadgetHelp       (WINDOW_Dummy + 22) /* BOOL — enable gadget help */

#define WINDOW_IconifyGadget    (WINDOW_Dummy + 23) /* BOOL — add iconify gadget to title bar */

#define WINDOW_TextAttr         (WINDOW_Dummy + 24) /* struct TextAttr * — default window font */

#define WINDOW_BackFillName     (WINDOW_Dummy + 26) /* STRPTR — backfill pattern loaded via datatypes */

#define WINDOW_RefWindow        (WINDOW_Dummy + 41) /* struct Window * — reference for relative positioning */

#define WINDOW_InputEvent       (WINDOW_Dummy + 42) /* struct InputEvent * — valid after WMHI_RAWKEY */

#define WINDOW_HintInfo         (WINDOW_Dummy + 43) /* struct HintInfo * — array of gadget help hints */

#define WINDOW_InterpretIDCMPHook   (WINDOW_Dummy + 46) /* BOOL — process IDCMPHook return values */

#define WINDOW_PreRefreshHook   (WINDOW_Dummy + 48) /* struct Hook * — invoked before RefreshGList() */

#define WINDOW_PostRefreshHook  (WINDOW_Dummy + 49) /* struct Hook * — invoked after RefreshGList() */

#define WINDOW_AppWindowPtr     (WINDOW_Dummy + 50) /* struct AppWindow * — gettable pointer from AddAppWindow() */

/* Private */
#define WINDOW_VertProp         (WINDOW_Dummy + 27) /* BOOL — enable vertical border scroller */

#define WINDOW_VertObject       (WINDOW_Dummy + 28) /* Object * — vertical scroller (get only) */

#define WINDOW_HorizProp        (WINDOW_Dummy + 29) /* BOOL — enable horizontal border scroller */

#define WINDOW_HorizObject      (WINDOW_Dummy + 30) /* Object * — horizontal scroller (get only) */

/* Legacy aliases */
#define WINDOW_Title            WA_Title
#define WINDOW_ScreenTitle      WA_ScreenTitle
#define WINDOW_Flags            WA_Flags

/****************************************************************************/

/* WM_HANDLEINPUT return codes */
#define WMHI_LASTMSG            (0L)        /* queue empty */
#define WMHI_IGNORE             (~0L)       /* discard this event */
#define WMHI_GADGETMASK         (0xffff)    /* extract gadget ID */
#define WMHI_MENUMASK           (0xffff)    /* extract menu ID */
#define WMHI_KEYMASK            (0xff)      /* extract key code */
#define WMHI_CLASSMASK          (0xffff0000)/* extract event class */
#define WMHI_CLOSEWINDOW        (1<<16)     /* close gadget hit */
#define WMHI_GADGETUP           (2<<16)     /* gadget released */
#define WMHI_INACTIVE           (3<<16)     /* window deactivated */
#define WMHI_ACTIVE             (4<<16)     /* window activated */
#define WMHI_NEWSIZE            (5<<16)     /* window resized */
#define WMHI_MENUPICK           (6<<16)     /* menu selection */
#define WMHI_MENUHELP           (7<<16)     /* help on menu item */
#define WMHI_GADGETHELP         (8<<16)     /* gadget help triggered */
#define WMHI_ICONIFY            (9<<16)     /* iconify requested */
#define WMHI_UNICONIFY          (10<<16)    /* restore from icon */
#define WMHI_RAWKEY             (11<<16)    /* raw key event */
#define WMHI_VANILLAKEY         (12<<16)    /* translated key event */
#define WMHI_CHANGEWINDOW       (13<<16)    /* window moved or depth-changed */
#define WMHI_INTUITICK          (14<<16)    /* periodic tick (~10/sec) */
#define WMHI_MOUSEMOVE          (15<<16)    /* pointer movement */
#define WMHI_MOUSEBUTTONS       (16<<16)    /* mouse button event */
#define WMHI_DISPOSEDWINDOW     (17<<16)    /* window disposed via hook */

/****************************************************************************/

#define WMF_ZOOMED              (0x0001)    /* window currently zoomed */
#define WMF_ZIPWINDOW           (0x0002)    /* resize toggled zoom */

/****************************************************************************/

/* Possible WINDOW_FrontBack values */
#define WT_FRONT   TRUE
#define WT_BACK    FALSE

/* WINDOW_Position values */
#define WPOS_CENTERSCREEN       (1L)    /* centered on screen */
#define WPOS_CENTERMOUSE        (2L)    /* centered at pointer */
#define WPOS_TOPLEFT            (3L)    /* screen top-left corner */
#define WPOS_CENTERWINDOW       (4L)    /* centered over ref window */
#define WPOS_FULLSCREEN         (5L)    /* fill entire screen */

/****************************************************************************/

/* Window methods */
#define WM_HANDLEINPUT  (0x570001L)

/* Message structure for WM_HANDLEINPUT */
struct wmHandle
{
    ULONG MethodID;         /* method ID */
    WORD *wmh_Code;         /* returned gadget code */
};

#define WM_OPEN         (0x570002L)
#define WM_CLOSE        (0x570003L)
#define WM_NEWPREFS     (0x570004L)
#define WM_ICONIFY      (0x570005L)
#define WM_RETHINK      (0x570006L)

#define WM_UNICONIFY    WM_OPEN     /* restore is equivalent to open */
/* AROS extension */
#define WM_SNAPSHOT     (0x570007L)

/****************************************************************************/

/* Gadget help hint entry for WINDOW_HintInfo */
struct HintInfo
{
    WORD    hi_GadgetID;    /* target gadget; -1 terminates array */
    WORD    hi_Code;        /* required code match; -1 for any */
    STRPTR  hi_Text;        /* hint text to display */
    ULONG   hi_Flags;       /* reserved, must be 0 */
};

/****************************************************************************/

/* Object creation macros */
#ifndef WindowObject
#define WindowObject        NewObject(NULL, "window.class"
#endif
#ifndef WindowEnd
#define WindowEnd           TAG_END)
#endif
#ifndef EndWindow
#define EndWindow           TAG_END)
#endif

#endif /* CLASSES_WINDOW_H */
