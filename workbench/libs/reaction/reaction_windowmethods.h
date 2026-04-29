/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - Window method IDs for window.class
          Private copy for library source files that can't use
          the installed public header.
*/

#ifndef REACTION_WINDOW_METHODS_H
#define REACTION_WINDOW_METHODS_H

/* Window.class method IDs (matching AmigaOS 3.9 NDK) */
#define WM_HANDLEINPUT  (0x570001L)
#define WM_OPEN         (0x570002L)
#define WM_CLOSE        (0x570003L)
#define WM_NEWPREFS     (0x570004L)
#define WM_ICONIFY      (0x570005L)
#define WM_RETHINK      (0x570006L)
#define WM_SNAPSHOT     (0x570007L)
#define WM_UNICONIFY    WM_OPEN

#endif /* REACTION_WINDOW_METHODS_H */
