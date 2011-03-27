/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef WINDOWDECORCLASS_H
#define WINDOWDECORCLASS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>

#include "newimage.h"

struct windecor_data /* TODO: make this private, add public class creation function */
{
    struct scrdecor_data *sd;
    struct DrawInfo *dri;
    struct Screen   *scr;
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

    BOOL             outline;
    BOOL             shadow;
    BOOL             barmasking;
    BOOL             closeright;
    BOOL             threestate;
    BOOL             barvert;
    BOOL             usegradients;
    BOOL             rounded;
    BOOL             filltitlebar;

    UWORD            winbarheight;
    UWORD            txt_align;

    LONG             BarJoinTB_o;
    LONG             BarJoinTB_s;
    LONG             BarPreGadget_o;
    LONG             BarPreGadget_s;
    LONG             BarPre_o;
    LONG             BarPre_s;
    LONG             BarLGadgetFill_o;
    LONG             BarLGadgetFill_s;
    LONG             BarJoinGB_o;
    LONG             BarJoinGB_s;
    LONG             BarLFill_o;
    LONG             BarLFill_s;
    LONG             BarJoinBT_o;
    LONG             BarJoinBT_s;
    LONG             BarTitleFill_o;
    LONG             BarTitleFill_s;
    LONG             BarRFill_o;
    LONG             BarRFill_s;
    LONG             BarJoinBG_o;
    LONG             BarJoinBG_s;
    LONG             BarRGadgetFill_o;
    LONG             BarRGadgetFill_s;
    LONG             BarPostGadget_o;
    LONG             BarPostGadget_s;
    LONG             BarPost_o;
    LONG             BarPost_s;

    LONG             ContainerTop_o, ContainerTop_s;
    LONG             ContainerVertTile_o, ContainerVertTile_s;
    LONG             ContainerBottom_o, ContainerBottom_s;
    LONG             KnobTop_o, KnobTop_s;
    LONG             KnobTileTop_o, KnobTileTop_s;
    LONG             KnobVertGripper_o, KnobVertGripper_s;
    LONG             KnobTileBottom_o, KnobTileBottom_s;
    LONG             KnobBottom_o, KnobBottom_s;
    LONG             ContainerLeft_o, ContainerLeft_s;
    LONG             ContainerHorTile_o, ContainerHorTile_s;
    LONG             ContainerRight_o, ContainerRight_s;
    LONG             KnobLeft_o, KnobLeft_s;
    LONG             KnobTileLeft_o, KnobTileLeft_s;
    LONG             KnobHorGripper_o, KnobHorGripper_s;
    LONG             KnobTileRight_o, KnobTileRight_s;
    LONG             KnobRight_o, KnobRight_s;
    LONG             sizeaddx, sizeaddy;
    LONG             updownaddx, updownaddy;
    LONG             leftrightaddx, leftrightaddy;
    LONG             rightbordergads, bottombordergads;
    LONG             rightbordernogads, bottombordernogads;
    LONG             horscrollerheight;
    LONG             scrollerinnerspacing;
    LONG             a_arc, d_arc;
    LONG             a_col_s, a_col_e;
    LONG             d_col_s, d_col_e;
    LONG             b_col_a, b_col_d;
    LONG             light, middle, dark;
    LONG             text_col, shadow_col;
};

struct  WindowData
{
    struct NewImage *ni;

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

    struct  RastPort    *rp;
    UWORD               w, h;
    LONG                ActivePen;
    LONG                DeactivePen;

    WORD   closewidth, depthwidth, zoomwidth;
    BOOL   truecolor;
};

#define WDA_Configuration   0x30002
#define WDA_ScreenData      0x30003

IPTR WinDecor_Dispatcher(struct IClass *cl, Object *obj, Msg msg); /* TODO: make this private, add public class creation function */
#endif
