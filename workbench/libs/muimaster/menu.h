/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_MENU_H
#define _MUI_MENU_H

struct NewMenu;
struct Window;
struct ZMenu;

struct ZMenu *zune_open_menu(struct Window *wnd, struct NewMenu *newmenu);
void zune_mouse_update(struct ZMenu *menu, int left_down);
/* returns the user data of the selected entry */
APTR zune_close_menu(struct ZMenu *menu);

#endif
