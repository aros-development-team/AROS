/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible classes/window.h
*/

#ifndef CLASSES_WINDOW_H
#define CLASSES_WINDOW_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

/*
 * window.class - ClassAct/ReAction compatible window management class
 *
 * Superclass: rootclass
 * Include:    <classes/window.h>
 */

/* Class name and version */
#define WINDOW_CLASSNAME    "window.class"
#define WINDOW_VERSION      44

/* Tag base */
#define WINDOW_Dummy        (TAG_USER + 0x10000)

/* Attributes */

/* (I..) Window title string */
#define WINDOW_Title            (WINDOW_Dummy + 0x0001)
/* (I..) Screen title when window active */
#define WINDOW_ScreenTitle      (WINDOW_Dummy + 0x0002)
/* (I..) Pointer to menu strip */
#define WINDOW_MenuStrip        (WINDOW_Dummy + 0x0003)
/* (I..) Window position, see WPOS_* */
#define WINDOW_Position         (WINDOW_Dummy + 0x0004)
/* (I..) Unique window ID for snapshots */
#define WINDOW_UniqueID         (WINDOW_Dummy + 0x0005)
/* (I..) Window flags */
#define WINDOW_Flags            (WINDOW_Dummy + 0x0006)
/* (I..) Pointer to root layout object */
#define WINDOW_Layout           (WINDOW_Dummy + 0x0007)
/* (I..) Pointer to root layout (alias) */
#define WINDOW_ParentGroup      WINDOW_Layout
/* (.G.) Pointer to the intuition Window */
#define WINDOW_Window           (WINDOW_Dummy + 0x0008)
/* (.G.) Signal mask for Wait() */
#define WINDOW_SigMask          (WINDOW_Dummy + 0x0009)
/* (ISG) Use gadget user data for IDs */
#define WINDOW_GadgetUserData   (WINDOW_Dummy + 0x000A)
/* (I..) IDCMPHook for custom processing */
#define WINDOW_IDCMPHook        (WINDOW_Dummy + 0x000B)
/* (I..) IDCMP flags to route to hook */
#define WINDOW_IDCMPHookBits    (WINDOW_Dummy + 0x000C)
/* (I..) Pointer to AppWindow port */
#define WINDOW_AppPort          (WINDOW_Dummy + 0x000D)
/* (I..) AppWindow message ID */
#define WINDOW_AppMsgHook       (WINDOW_Dummy + 0x000E)
/* (I..) Backfill hook */
#define WINDOW_BackfillHook     (WINDOW_Dummy + 0x000F)
/* (I..) Shared user port */
#define WINDOW_SharedPort       (WINDOW_Dummy + 0x0010)
/* (I..) Pointer to window icon for iconification */
#define WINDOW_Icon             (WINDOW_Dummy + 0x0011)
/* (I..) Iconify the window */
#define WINDOW_Iconified        (WINDOW_Dummy + 0x0012)
/* (I..) Help text group */
#define WINDOW_HelpGroup        (WINDOW_Dummy + 0x0013)
/* (I..) CharSet */
#define WINDOW_CharSet          (WINDOW_Dummy + 0x0014)
/* (I..) Lock width at current size */
#define WINDOW_LockWidth        (WINDOW_Dummy + 0x0015)
/* (I..) Lock height at current size */
#define WINDOW_LockHeight       (WINDOW_Dummy + 0x0016)
/* (I..) User data value */
#define WINDOW_UserData         (WINDOW_Dummy + 0x0017)
/* (I..) Intuition lock state */
#define WINDOW_InterpretIDCMPHook (WINDOW_Dummy + 0x0018)
/* (I..) IDCMP refresh notification */
#define WINDOW_RefreshWindow    (WINDOW_Dummy + 0x0019)
/* (.G.) Inner width of window */
#define WINDOW_InnerWidth       (WINDOW_Dummy + 0x001A)
/* (.G.) Inner height of window */
#define WINDOW_InnerHeight      (WINDOW_Dummy + 0x001B)
/* (I..) Horizontal gadget object */
#define WINDOW_HorizProp        (WINDOW_Dummy + 0x001C)
/* (I..) Vertical gadget object */
#define WINDOW_VertProp         (WINDOW_Dummy + 0x001D)
/* (I..) Popup menu for window */
#define WINDOW_PopupGadget      (WINDOW_Dummy + 0x001E)
/* (I..) Use new menu look */
#define WINDOW_NewLookMenus     (WINDOW_Dummy + 0x001F)

/* Window position values for WINDOW_Position */
#define WPOS_CENTERSCREEN       0
#define WPOS_CENTERMOUSE        1
#define WPOS_TOPLEFT            2
#define WPOS_CENTERWINDOW       3
#define WPOS_FULLSCREEN         4

/* Window HandleInput message classes */
#define WMHI_LASTMSG        (0UL)
#define WMHI_CLOSEWINDOW    (1UL << 16)
#define WMHI_NOMORE         (2UL << 16)
#define WMHI_ACTIVE         (3UL << 16)
#define WMHI_INACTIVE       (4UL << 16)
#define WMHI_GADGETUP       (5UL << 16)
#define WMHI_MENUPICK       (6UL << 16)
#define WMHI_MENUHELP       (7UL << 16)
#define WMHI_GADGETHELP     (8UL << 16)
#define WMHI_ICONIFY        (9UL << 16)
#define WMHI_UNICONIFY      (10UL << 16)
#define WMHI_RAWKEY         (11UL << 16)
#define WMHI_NEWSIZE        (12UL << 16)
#define WMHI_CHANGEWINDOW   (13UL << 16)
#define WMHI_JUMPSCREEN     (14UL << 16)
#define WMHI_SNAPSHOT       (15UL << 16)
#define WMHI_VANILLAKEY     (16UL << 16)

/* Masks for extracting information from HandleInput result */
#define WMHI_CLASSMASK      0xFFFF0000
#define WMHI_KEYMASK        0x0000FFFF
#define WMHI_GADGETMASK     0x0000FFFF
#define WMHI_MENUMASK       0x0000FFFF

/* Window.class method IDs */
#define WM_Dummy            0x6A00

#define WM_OPEN             (WM_Dummy + 1)   /* Open the window */
#define WM_CLOSE            (WM_Dummy + 2)   /* Close the window */
#define WM_HANDLEINPUT      (WM_Dummy + 3)   /* Process input events */
#define WM_ICONIFY          (WM_Dummy + 4)   /* Iconify the window */
#define WM_UNICONIFY        (WM_Dummy + 5)   /* Uniconify the window */
#define WM_NEWPREFS         (WM_Dummy + 6)   /* Preferences changed */
#define WM_RETHINK          (WM_Dummy + 7)   /* Rethink layout */
#define WM_SNAPSHOT         (WM_Dummy + 8)   /* Snapshot window position */

/* Object creation macros */
#define WindowObject        NewObject(NULL, WINDOW_CLASSNAME
#define WindowEnd           TAG_END)
#define EndWindow           TAG_END)

#endif /* CLASSES_WINDOW_H */
