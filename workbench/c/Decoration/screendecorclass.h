/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef SCREENDECORCLASS_H
#define SCREENDECORCLASS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>

#include "newimage.h"

struct scrdecor_data
{

    struct NewImage *img_sdepth;
    struct NewImage *img_sbarlogo;
    struct NewImage *img_stitlebar;

    struct NewImage *img_size;
    struct NewImage *img_close;
    struct NewImage *img_depth;
    struct NewImage *img_zoom;
    struct NewImage *img_up;
    struct NewImage *img_down;
    struct NewImage *img_left;
    struct NewImage *img_right;
    struct NewImage *img_mui;
    struct NewImage *img_popup;
    struct NewImage *img_snapshot;
    struct NewImage *img_iconify;
    struct NewImage *img_lock;
    struct NewImage *img_winbar_normal;
    struct NewImage *img_border_normal;
    struct NewImage *img_border_deactivated;
    struct NewImage *img_verticalcontainer;
    struct NewImage *img_verticalknob;
    struct NewImage *img_horizontalcontainer;
    struct NewImage *img_horizontalknob;

    struct NewImage *img_menu;
    struct NewImage *img_amigakey;
    struct NewImage *img_menucheck;
    struct NewImage *img_submenu;

    UWORD            sbarheight;
    UWORD            slogo_off;
    UWORD            stitle_off;
    UWORD            winbarheight;

    BOOL             outline;
    BOOL             shadow;

    LONG             leftborder, bottomborder, rightborder;
    LONG             lut_col_a, lut_col_d;
    LONG             text_col, shadow_col;
};

struct ScreenData
{
    struct NewImage img_sdepth;
    struct NewImage img_sbarlogo;
    struct NewImage img_stitlebar;

    struct NewImage img_size;
    struct NewImage img_close;
    struct NewImage img_depth;
    struct NewImage img_zoom;
    struct NewImage img_up;
    struct NewImage img_down;
    struct NewImage img_left;
    struct NewImage img_right;
    struct NewImage img_mui;
    struct NewImage img_popup;
    struct NewImage img_snapshot;
    struct NewImage img_iconify;
    struct NewImage img_lock;
    struct NewImage img_winbar_normal;
    struct NewImage img_border_normal;
    struct NewImage img_border_deactivated;
    struct NewImage img_verticalcontainer;
    struct NewImage img_verticalknob;
    struct NewImage img_horizontalcontainer;
    struct NewImage img_horizontalknob;

    struct NewImage img_menu;
    struct NewImage img_amigakey;
    struct NewImage img_menucheck;
    struct NewImage img_submenu;
    LONG            ActivePen;
    LONG            DeactivePen;
    BOOL            truecolor;

};

#define SDA_Configuration   0x20002
#define SDA_ScreenData      0x20003

IPTR ScrDecor_Dispatcher(struct IClass *cl, Object *obj, Msg msg);
#endif
