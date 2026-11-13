/*
    Copyright (C) 2024-2026, The AROS Development Team. All rights reserved.

    Desc: decortheme.library - builds the decorator.library element set
          described by a theme configuration
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>

#include <string.h>

#include <libraries/decortheme.h>
#include "decortheme_intern.h"

/* ========== Element Initialization ========== */

static void InitElement(struct DecoratorElement *elem, struct DecorImage *img,
                       UWORD type, UWORD flags, LONG offset, LONG size,
                       UWORD subcols, UWORD subrows)
{
    elem->de_Image = img;
    elem->de_Type = type;
    elem->de_Flags = flags;
    elem->de_SrcOffset = offset;
    elem->de_SrcSize = size;
    elem->de_SubImageCols = subcols;
    elem->de_SubImageRows = subrows;
    elem->de_PadX = 0;
    elem->de_PadY = 0;
}

static void InitGadgetElement(struct DecoratorElement *elem, struct DecorImage *img,
                             UWORD flags, UWORD subcols, UWORD subrows)
{
    InitElement(elem, img, DE_TYPE_STATEFUL_GADGET, flags | DEF_TITLE_GADGET,
               0, 0, subcols, subrows);
}

static void InitBarElement(struct DecoratorElement *elem, struct DecorImage *img,
                          LONG offset, LONG size)
{
    InitElement(elem, img, DE_TYPE_TILED_TITLE, 0, offset, size, 1, 2);
}

static void InitScrBarElement(struct DecoratorElement *elem, struct DecorImage *img,
                             LONG offset, LONG size, LONG srcheight)
{
    InitElement(elem, img, DE_TYPE_SCALED_TILED_H, 0, offset, size, 1, 1);
    elem->de_SrcHeight = srcheight;
}

void InitThemeElements(struct DecoratorElement *elements, struct DecorConfig *dc, struct DecorImages *di)
{
    UWORD gflags = dc->GadgetsThreeState ? DEF_THREE_STATE : 0;

    memset(elements, 0, sizeof(struct DecoratorElement) * DECOR_NUM_ELEMENTS);

    /* Window titlebar section elements - all use img_winbar_normal */
    InitBarElement(&elements[DECOR_ELEM_WinBarPreGadget], di->img_winbar_normal,
                   dc->BarPreGadget_o, dc->BarPreGadget_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarPre], di->img_winbar_normal,
                   dc->BarPre_o, dc->BarPre_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarLGadgetFill], di->img_winbar_normal,
                   dc->BarLGadgetFill_o, dc->BarLGadgetFill_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarJoinGB], di->img_winbar_normal,
                   dc->BarJoinGB_o, dc->BarJoinGB_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarLFill], di->img_winbar_normal,
                   dc->BarLFill_o, dc->BarLFill_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarJoinBT], di->img_winbar_normal,
                   dc->BarJoinBT_o, dc->BarJoinBT_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarTitleFill], di->img_winbar_normal,
                   dc->BarTitleFill_o, dc->BarTitleFill_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarJoinTB], di->img_winbar_normal,
                   dc->BarJoinTB_o, dc->BarJoinTB_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarRFill], di->img_winbar_normal,
                   dc->BarRFill_o, dc->BarRFill_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarJoinBG], di->img_winbar_normal,
                   dc->BarJoinBG_o, dc->BarJoinBG_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarRGadgetFill], di->img_winbar_normal,
                   dc->BarRGadgetFill_o, dc->BarRGadgetFill_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarPostGadget], di->img_winbar_normal,
                   dc->BarPostGadget_o, dc->BarPostGadget_s);
    InitBarElement(&elements[DECOR_ELEM_WinBarPost], di->img_winbar_normal,
                   dc->BarPost_o, dc->BarPost_s);

    /* Window gadget elements */
    InitGadgetElement(&elements[DECOR_ELEM_WinClose], di->img_close, gflags, dc->GadgetsThreeState ? 3 : 4, 1);
    InitGadgetElement(&elements[DECOR_ELEM_WinDepth], di->img_depth, gflags, dc->GadgetsThreeState ? 3 : 4, 1);
    InitGadgetElement(&elements[DECOR_ELEM_WinZoom], di->img_zoom, gflags, dc->GadgetsThreeState ? 3 : 4, 1);
    InitGadgetElement(&elements[DECOR_ELEM_WinMUI], di->img_mui, gflags, dc->GadgetsThreeState ? 3 : 4, 1);
    InitGadgetElement(&elements[DECOR_ELEM_WinPopup], di->img_popup, gflags, dc->GadgetsThreeState ? 3 : 4, 1);
    InitGadgetElement(&elements[DECOR_ELEM_WinSnapshot], di->img_snapshot, gflags, dc->GadgetsThreeState ? 3 : 4, 1);
    InitGadgetElement(&elements[DECOR_ELEM_WinIconify], di->img_iconify, gflags, dc->GadgetsThreeState ? 3 : 4, 1);
    InitGadgetElement(&elements[DECOR_ELEM_WinLock], di->img_lock, gflags, dc->GadgetsThreeState ? 3 : 4, 1);

    /* Size gadget - not a title gadget */
    InitElement(&elements[DECOR_ELEM_WinSize], di->img_size, DE_TYPE_STATEFUL_GADGET,
               gflags, 0, 0, dc->GadgetsThreeState ? 3 : 4, 1);
    elements[DECOR_ELEM_WinSize].de_PadX = (dc->RightBorderGadgets - (di->img_size ? (dc->GadgetsThreeState ? di->img_size->w / 3 : di->img_size->w >> 2) : 0)) / 2;
    elements[DECOR_ELEM_WinSize].de_PadY = (dc->BottomBorderGadgets - (di->img_size ? di->img_size->h : 0)) / 2;

    /* Arrow gadgets - not title gadgets */
    InitElement(&elements[DECOR_ELEM_WinUp], di->img_up, DE_TYPE_STATEFUL_GADGET,
               gflags, 0, 0, dc->GadgetsThreeState ? 3 : 4, 1);
    elements[DECOR_ELEM_WinUp].de_PadX = (dc->RightBorderGadgets - (di->img_up ? (dc->GadgetsThreeState ? di->img_up->w / 3 : di->img_up->w >> 2) : 0)) / 2;
    elements[DECOR_ELEM_WinUp].de_PadY = dc->UpDownAddY / 2;

    InitElement(&elements[DECOR_ELEM_WinDown], di->img_down, DE_TYPE_STATEFUL_GADGET,
               gflags, 0, 0, dc->GadgetsThreeState ? 3 : 4, 1);
    elements[DECOR_ELEM_WinDown].de_PadX = (dc->RightBorderGadgets - (di->img_down ? (dc->GadgetsThreeState ? di->img_down->w / 3 : di->img_down->w >> 2) : 0)) / 2;
    elements[DECOR_ELEM_WinDown].de_PadY = dc->UpDownAddY / 2;

    InitElement(&elements[DECOR_ELEM_WinLeft], di->img_left, DE_TYPE_STATEFUL_GADGET,
               gflags, 0, 0, dc->GadgetsThreeState ? 3 : 4, 1);
    elements[DECOR_ELEM_WinLeft].de_PadX = dc->LeftRightAddX / 2;
    elements[DECOR_ELEM_WinLeft].de_PadY = (dc->BottomBorderGadgets - (di->img_left ? di->img_left->h : 0)) / 2;

    InitElement(&elements[DECOR_ELEM_WinRight], di->img_right, DE_TYPE_STATEFUL_GADGET,
               gflags, 0, 0, dc->GadgetsThreeState ? 3 : 4, 1);
    elements[DECOR_ELEM_WinRight].de_PadX = dc->LeftRightAddX / 2;
    elements[DECOR_ELEM_WinRight].de_PadY = (dc->BottomBorderGadgets - (di->img_right ? di->img_right->h : 0)) / 2;

    /* Vertical scrollbar elements */
    InitElement(&elements[DECOR_ELEM_VContainerTop], di->img_verticalcontainer,
               DE_TYPE_TILED_VERTICAL, 0, dc->ContainerTop_o, dc->ContainerTop_s, 2, 1);
    InitElement(&elements[DECOR_ELEM_VContainerTile], di->img_verticalcontainer,
               DE_TYPE_TILED_VERTICAL, 0, dc->ContainerVertTile_o, dc->ContainerVertTile_s, 2, 1);
    InitElement(&elements[DECOR_ELEM_VContainerBottom], di->img_verticalcontainer,
               DE_TYPE_TILED_VERTICAL, 0, dc->ContainerBottom_o, dc->ContainerBottom_s, 2, 1);
    InitElement(&elements[DECOR_ELEM_VKnobTop], di->img_verticalknob,
               DE_TYPE_TILED_VERTICAL, 0, dc->KnobTop_o, dc->KnobTop_s, 3, 1);
    InitElement(&elements[DECOR_ELEM_VKnobTileTop], di->img_verticalknob,
               DE_TYPE_TILED_VERTICAL, 0, dc->KnobTileTop_o, dc->KnobTileTop_s, 3, 1);
    InitElement(&elements[DECOR_ELEM_VKnobGripper], di->img_verticalknob,
               DE_TYPE_TILED_VERTICAL, DEF_HAS_GRIPPER, dc->KnobVertGripper_o, dc->KnobVertGripper_s, 3, 1);
    InitElement(&elements[DECOR_ELEM_VKnobTileBottom], di->img_verticalknob,
               DE_TYPE_TILED_VERTICAL, 0, dc->KnobTileBottom_o, dc->KnobTileBottom_s, 3, 1);
    InitElement(&elements[DECOR_ELEM_VKnobBottom], di->img_verticalknob,
               DE_TYPE_TILED_VERTICAL, 0, dc->KnobBottom_o, dc->KnobBottom_s, 3, 1);

    /* Horizontal scrollbar elements */
    InitElement(&elements[DECOR_ELEM_HContainerLeft], di->img_horizontalcontainer,
               DE_TYPE_TILED_HORIZONTAL, 0, dc->ContainerLeft_o, dc->ContainerLeft_s, 1, 2);
    InitElement(&elements[DECOR_ELEM_HContainerTile], di->img_horizontalcontainer,
               DE_TYPE_TILED_HORIZONTAL, 0, dc->ContainerHorTile_o, dc->ContainerHorTile_s, 1, 2);
    InitElement(&elements[DECOR_ELEM_HContainerRight], di->img_horizontalcontainer,
               DE_TYPE_TILED_HORIZONTAL, 0, dc->ContainerRight_o, dc->ContainerRight_s, 1, 2);
    InitElement(&elements[DECOR_ELEM_HKnobLeft], di->img_horizontalknob,
               DE_TYPE_TILED_HORIZONTAL, 0, dc->KnobLeft_o, dc->KnobLeft_s, 1, 3);
    InitElement(&elements[DECOR_ELEM_HKnobTileLeft], di->img_horizontalknob,
               DE_TYPE_TILED_HORIZONTAL, 0, dc->KnobTileLeft_o, dc->KnobTileLeft_s, 1, 3);
    InitElement(&elements[DECOR_ELEM_HKnobGripper], di->img_horizontalknob,
               DE_TYPE_TILED_HORIZONTAL, DEF_HAS_GRIPPER, dc->KnobHorGripper_o, dc->KnobHorGripper_s, 1, 3);
    InitElement(&elements[DECOR_ELEM_HKnobTileRight], di->img_horizontalknob,
               DE_TYPE_TILED_HORIZONTAL, 0, dc->KnobTileRight_o, dc->KnobTileRight_s, 1, 3);
    InitElement(&elements[DECOR_ELEM_HKnobRight], di->img_horizontalknob,
               DE_TYPE_TILED_HORIZONTAL, 0, dc->KnobRight_o, dc->KnobRight_s, 1, 3);

    /* Screen bar elements */
    {
        LONG filllen = 1;

        if (di->img_stitlebar)
        {
            if ((dc->SBarChildPre_s > 0) && (dc->SBarChildPre_s < di->img_stitlebar->w))
                filllen = dc->SBarChildPre_o;
            else if ((dc->SBarGadPre_s > 0) && (dc->SBarGadPre_s < di->img_stitlebar->w))
                filllen = dc->SBarGadPre_o;
            else
                filllen = di->img_stitlebar->w;
        }
        if (filllen == 0)
            filllen = 1;

        InitScrBarElement(&elements[DECOR_ELEM_ScrBarFill], di->img_stitlebar, 0, filllen, dc->SBarHeight);
    }
    InitScrBarElement(&elements[DECOR_ELEM_ScrBarGadPre], di->img_stitlebar,
                     dc->SBarGadPre_o, dc->SBarGadPre_s, dc->SBarHeight);
    InitScrBarElement(&elements[DECOR_ELEM_ScrBarGadFill], di->img_stitlebar,
                     dc->SBarGadFill_o, dc->SBarGadFill_s, dc->SBarHeight);
    InitScrBarElement(&elements[DECOR_ELEM_ScrBarGadPost], di->img_stitlebar,
                     dc->SBarGadPost_o, dc->SBarGadPost_s, dc->SBarHeight);
    InitScrBarElement(&elements[DECOR_ELEM_ScrBarChildPre], di->img_stitlebar,
                     dc->SBarChildPre_o, dc->SBarChildPre_s, dc->SBarHeight);
    InitScrBarElement(&elements[DECOR_ELEM_ScrBarChildFill], di->img_stitlebar,
                     dc->SBarChildFill_o, dc->SBarChildFill_s, dc->SBarHeight);
    InitScrBarElement(&elements[DECOR_ELEM_ScrBarChildPost], di->img_stitlebar,
                     dc->SBarChildPost_o, dc->SBarChildPost_s, dc->SBarHeight);

    /* Screen depth gadget */
    InitElement(&elements[DECOR_ELEM_ScrDepth], di->img_sdepth,
               DE_TYPE_STATEFUL_GADGET, DEF_SCALABLE, 0, 0, 2, 1);

    /* Screen bar logo - tiled horizontal so it draws with alpha */
    InitElement(&elements[DECOR_ELEM_ScrBarLogo], di->img_sbarlogo,
               DE_TYPE_TILED_HORIZONTAL, 0, 0,
               di->img_sbarlogo ? di->img_sbarlogo->w : 0, 1, 1);

    /* Menu elements */
    InitElement(&elements[DECOR_ELEM_MenuBackground], di->img_menu,
               DE_TYPE_TILED_BOTH, 0, 0, 0, 1, 1);
    InitElement(&elements[DECOR_ELEM_MenuAmigaKey], di->img_amigakey,
               DE_TYPE_STATEFUL_GADGET, 0, 0, 0, 1, 1);
    InitElement(&elements[DECOR_ELEM_MenuCheck], di->img_menucheck,
               DE_TYPE_STATEFUL_GADGET, 0, 0, 0, 1, 1);
    InitElement(&elements[DECOR_ELEM_MenuSubMenu], di->img_submenu,
               DE_TYPE_STATEFUL_GADGET, 0, 0, 0, 1, 1);

    /* Border/background elements */
    InitElement(&elements[DECOR_ELEM_WinBorderNormal], di->img_border_normal,
               DE_TYPE_TILED_BOTH, 0, 0, 0, 1, 1);
    InitElement(&elements[DECOR_ELEM_WinBorderDeactivated], di->img_border_deactivated,
               DE_TYPE_TILED_BOTH, 0, 0, 0, 1, 1);
    InitBarElement(&elements[DECOR_ELEM_WinBarNormal], di->img_winbar_normal, 0, 0);
}
