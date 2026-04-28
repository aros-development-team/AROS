/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - Window method IDs for window.class
*/

#ifndef REACTION_WINDOW_METHODS_H
#define REACTION_WINDOW_METHODS_H

/* Window.class method IDs */
#define WM_Dummy        0x6A00

#define WM_OPEN         (WM_Dummy + 1)   /* Open the window */
#define WM_CLOSE        (WM_Dummy + 2)   /* Close the window */
#define WM_HANDLEINPUT  (WM_Dummy + 3)   /* Process input events */
#define WM_ICONIFY      (WM_Dummy + 4)   /* Iconify the window */
#define WM_UNICONIFY    (WM_Dummy + 5)   /* Uniconify the window */
#define WM_NEWPREFS     (WM_Dummy + 6)   /* Preferences changed */
#define WM_RETHINK      (WM_Dummy + 7)   /* Rethink layout */
#define WM_SNAPSHOT     (WM_Dummy + 8)   /* Snapshot window position */

#endif /* REACTION_WINDOW_METHODS_H */
