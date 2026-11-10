/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef LINKTEXT_PRIVATE_H
#define LINKTEXT_PRIVATE_H

#include <libraries/mui.h>
#include "linktext_core.h"

struct Linktext_DATA
{
    lt_content *content;
    lt_line *lines;
    ULONG nlines;
    LONG laid_w;                 /* width the lines are for, -1: to do */
    LONG first;                  /* line at the top                    */
    LONG lineh;
    LONG dirty_first, dirty_last;

    struct Hook *hook;
    Object *scroller;
    ULONG want_lines;
    BOOL fix_height;

    BOOL shown, active, motion;
    ULONG sel_anchor, sel_caret; /* equal: no selection                */
    long focus;                  /* link the keyboard is on, -1 none   */

    BOOL pressed, dragging;
    long press_link;
    ULONG press_caret;
    LONG press_x, press_y;

    struct MUI_EventHandlerNode ehn, ehn_move;

    Object *menu, *item_open, *item_copy, *item_text;
    lt_content *menu_content;
    long menu_link;
    char *help;
};

#endif
