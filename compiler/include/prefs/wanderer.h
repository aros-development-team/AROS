#ifndef PREFS_WANDERER_H
#define PREFS_WANDERER_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: workbench.h 23386 2005-06-23 08:17:21Z neil $

    Desc: wanderer prefs definitions
    Lang: English
*/

#ifndef GRAPHICS_GFX_H
#include <graphics/gfx.h>
#endif

#define ID_WANDR MAKE_ID('W','A','N','R')


/* The maximum length the path may have. */
#define PATHLENGTHSIZE 256 
#define ICON_TEXT_MAXLEN_DEFAULT    20

struct WandererPrefs
{
    UBYTE wpd_WorkbenchBackground[PATHLENGTHSIZE],
          wpd_DrawerBackground[PATHLENGTHSIZE];
         
    UBYTE wpd_NavigationMethod;  // Are we using the toolbar or not for navigation
    UBYTE wpd_ToolbarEnabled; // Is the toolbar enabled?
    
    UBYTE wpd_IconListMode; // How is it going to be listed
    UBYTE wpd_IconTextMode; // How is the text rendered
    
    ULONG wpd_IconTextMaxLen; // Max length of icon text
};

#define WPD_NAVIGATION_CLASSIC 0
#define WPD_NAVIGATION_ENHANCED 1
#define WPD_ICONLISTMODE_PLAIN 0
#define WPD_ICONLISTMODE_GRID 1
#define WPD_ICONTEXTMODE_OUTLINE 0
#define WPD_ICONTEXTMODE_PLAIN 1
#define WPD_GENERAL 0
#define WPD_APPEARANCE 1
#define WPD_TOOLBAR 2

#endif /* PREFS_WORKBENCH_H */
