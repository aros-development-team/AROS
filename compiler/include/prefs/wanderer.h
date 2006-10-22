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

struct WandererPrefs
{
    UBYTE wpd_WorkbenchBackground[PATHLENGTHSIZE],
          wpd_DrawerBackground[PATHLENGTHSIZE];
         
    UBYTE wpd_NavigationMethod;  
    UBYTE wpd_ToolbarEnabled;   
    
    UBYTE wpd_IconListMode;
    UBYTE wpd_IconTextMode;
    
};

#define WPD_NAVIGATION_CLASSIC 0
#define WPD_NAVIGATION_ENHANCED 1


#endif /* PREFS_WORKBENCH_H */
