/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

struct AppGUIData
{
 UWORD		agd_MaxWidth;
 UWORD		agd_MaxHeight;
 UWORD		agd_BevelBoxOffset;
 struct Window	*agd_Window;
 struct Menu	*agd_Menu;
 struct Gadget	*agd_GadgetList;
};

/* ReadArgs() stuff */
#define ARG_FROM 0
#define ARG_EDIT 1
#define ARG_USE 2
#define ARG_SAVE 3
#define ARG_PUBSCREEN 4

#define NUM_ARGS 5

/* Menu item identifiers */
#define MENU_ID_OPEN		1
#define MENU_ID_SAVE		2
#define MENU_ID_SAVEAS		3
#define MENU_ID_QUIT		4
#define MENU_ID_DEFAULTS	5
#define MENU_ID_LASTSAVED	6
#define MENU_ID_RESTORE		7

/* GUI constants */
#define WINDOW_OFFSET		3

#define MX_INIT_ACTIVE		0 // Default MX choice

#define GADGET_INNER_SPACING	3 // Should be proportional to font size?
#define GADGET_OUTER_SPACING	3 // Should be proportional to font size?

#define MX_CHOOSEFONT		0 
#define BUTTON_GETFONT		1
#define BUTTON_SAVE		2
#define BUTTON_USE		3
#define BUTTON_CANCEL		4
#define GADGET_ID_TEXT		5
