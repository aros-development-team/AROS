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
/* Returns either select Menu or selected MenuItem. Fields are exclusive. */
struct zlm
{
    struct Menu         *menu;
    struct MenuItem     *item;
};
void zune_leave_menu(struct ZMenu *zmenu, struct zlm *res);
void zune_close_menu(struct ZMenu *menu);
struct Menu *zune_get_menu_pointer(struct ZMenu *menu);

#endif
