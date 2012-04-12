/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef WORKBOOK_MENU_H
#define WORKBOOK_MENU_H

#define Broken NM_ITEMDISABLED |

/* Handy macros */
#define WBMENU_ID_(id, name, cmd, flags, mutex) ((IPTR)id)
#define WBMENU_ITEM_(id, name, cmd, flags, mutex) { NM_ITEM, name, cmd, flags, 0, (APTR)id }
#define WBMENU_SUBITEM_(id, name, cmd, flags, mutex) { NM_SUB, name, cmd, flags, 0, (APTR)id }

#define WBMENU_ID(x)                     WBMENU_ID_(x)
#define WBMENU_TITLE(name)               { NM_TITLE, name }
#define WBMENU_ITEM(x)                   WBMENU_ITEM_(x)
#define WBMENU_BAR                       { NM_ITEM, NM_BARLABEL }
#define WBMENU_SUBTITLE(name)            { NM_ITEM, name }
#define WBMENU_SUBITEM(x)                WBMENU_SUBITEM_(x)
#define WBMENU_SUBBAR                    { NM_SUB, NM_BARLABEL }

#define WBMENU_ITEM_ID(item) ((IPTR)GTMENUITEM_USERDATA(item))

/* Workbench Menu */
#define WBMENU_WB               "Workbook"
#define WBMENU_WB_BACKDROP	0, "Backdrop",  "B", Broken 0, 0
#define WBMENU_WB_EXECUTE	1, "Execute",   "E", Broken 0, 0
#define WBMENU_WB_SHELL		2, "Shell",     "W", 0, 0
#define WBMENU_WB_ABOUT		3, "About...",    0, Broken 0, 0
#define WBMENU_WB_QUIT		4, "Quit",      "Q", 0, 0
#define WBMENU_WB_SHUTDOWN	5, "Shutdown",    0, 0, 0

/* Window Menu */
#define WBMENU_WN               "Window"
#define WBMENU_WN_NEW_DRAWER	20, "New drawer",      "N", Broken 0, 0
#define WBMENU_WN_OPEN_PARENT	21, "Open parent",     "K", 0, 0
#define WBMENU_WN_UPDATE	22, "Update",            0, Broken 0, 0
#define WBMENU_WN_SELECT_ALL	23, "Select contents", "A", Broken 0, 0
#define WBMENU_WN_SELECT_NONE	24, "Unselect all",      0, Broken 0, 0
#define WBMENU_WN__SNAP		"Snapshot"
#define WBMENU_WN__SNAP_WINDOW	40, "Window",            0, Broken 0, 0
#define WBMENU_WN__SNAP_ALL	41, "All",               0, Broken 0, 0
#define WBMENU_WN__SHOW		"Show"
#define WBMENU_WN__SHOW_ICONS	45, "Only icons",      "-", CHECKIT|CHECKED, ~((1 << 0))
#define WBMENU_WN__SHOW_ALL	46, "All files",       "+", CHECKIT, ~((1 << 1))
#define WBMENU_WN__VIEW		"View by"
#define WBMENU_WN__VIEW_ICON	50, "Icon",            "1", Broken CHECKIT|CHECKED, ~((1 << 0))
#define WBMENU_WN__VIEW_DETAILS	51, "Details",           0, Broken CHECKIT, ~((1 << 1))

/* Icon Menu */
#define WBMENU_IC               "Icons"
#define WBMENU_IC_OPEN          60, "Open",            "O", 0, 0
#define WBMENU_IC_COPY          61, "Copy",            "C", Broken 0, 0
#define WBMENU_IC_RENAME        62, "Rename...",       "R", Broken 0, 0
#define WBMENU_IC_INFO          63, "Information...",  "I", 0, 0
#define WBMENU_IC_SNAPSHOT      64, "Snapshot",        "S", Broken 0, 0
#define WBMENU_IC_UNSNAPSHOT    65, "Unsnapshot",      "U", Broken 0, 0
#define WBMENU_IC_LEAVE_OUT     66, "Leave out",       "L", Broken 0, 0
#define WBMENU_IC_PUT_AWAY      67, "Put away",        "P", Broken 0, 0
#define WBMENU_IC_DELETE        70, "Delete...",         0, Broken 0, 0
#define WBMENU_IC_FORMAT        71, "Format...",         0, Broken 0, 0
#define WBMENU_IC_EMPTY_TRASH   72, "Empty trash",       0, Broken 0, 0

#endif /* WORKBOOK_MENU_H */
