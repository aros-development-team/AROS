/*
    Copyright  2011-2012, The AROS Development Team.
    $Id$
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <exec/types.h>

struct DecorConfig
{
    STRPTR  ThemePath;

    /* Screen Section */
    LONG    LeftBorder;
    LONG    RightBorder;
    LONG    BottomBorder;
    LONG    SLogoOffset;
    LONG    STitleOffset;
    LONG    SBarHeight;
    LONG    SBarChildPre_o;
    LONG    SBarChildPre_s;
    LONG    SBarChildFill_o;
    LONG    SBarChildFill_s;
    LONG    SBarChildPost_o;
    LONG    SBarChildPost_s;
    LONG    SBarGadPre_o;
    LONG    SBarGadPre_s;
    LONG    SBarGadFill_o;
    LONG    SBarGadFill_s;
    LONG    SBarGadPost_o;
    LONG    SBarGadPost_s;
    BOOL    STitleOutline;
    BOOL    STitleShadow;
    LONG    LUTBaseColors_a;
    LONG    LUTBaseColors_d;
    LONG    STitleColorText;
    LONG    STitleColorShadow;

    /* Window Section */
    BOOL    BarRounded;
    BOOL    BarVertical;
    LONG    BarHeight;
    LONG    BarJoinTB_o;
    LONG    BarJoinTB_s;
    LONG    BarPreGadget_o;
    LONG    BarPreGadget_s;
    LONG    BarPre_o;
    LONG    BarPre_s;
    LONG    BarLGadgetFill_o;
    LONG    BarLGadgetFill_s;
    LONG    BarJoinGB_o;
    LONG    BarJoinGB_s;
    LONG    BarLFill_o;
    LONG    BarLFill_s;
    LONG    BarJoinBT_o;
    LONG    BarJoinBT_s;
    LONG    BarTitleFill_o;
    LONG    BarTitleFill_s;
    LONG    BarRFill_o;
    LONG    BarRFill_s;
    LONG    BarJoinBG_o;
    LONG    BarJoinBG_s;
    LONG    BarRGadgetFill_o;
    LONG    BarRGadgetFill_s;
    LONG    BarPostGadget_o;
    LONG    BarPostGadget_s;
    LONG    BarPost_o;
    LONG    BarPost_s;
    LONG    ContainerTop_o;
    LONG    ContainerTop_s;
    LONG    ContainerVertTile_o;
    LONG    ContainerVertTile_s;
    LONG    ContainerBottom_o;
    LONG    ContainerBottom_s;
    LONG    KnobTop_o;
    LONG    KnobTop_s;
    LONG    KnobTileTop_o;
    LONG    KnobTileTop_s;
    LONG    KnobVertGripper_o;
    LONG    KnobVertGripper_s;
    LONG    KnobTileBottom_o;
    LONG    KnobTileBottom_s;
    LONG    KnobBottom_o;
    LONG    KnobBottom_s;
    LONG    ContainerLeft_o;
    LONG    ContainerLeft_s;
    LONG    ContainerHorTile_o;
    LONG    ContainerHorTile_s;
    LONG    ContainerRight_o;
    LONG    ContainerRight_s;
    LONG    KnobLeft_o;
    LONG    KnobLeft_s;
    LONG    KnobTileLeft_o;
    LONG    KnobTileLeft_s;
    LONG    KnobHorGripper_o;
    LONG    KnobHorGripper_s;
    LONG    KnobTileRight_o;
    LONG    KnobTileRight_s;
    LONG    KnobRight_o;
    LONG    KnobRight_s;
    BOOL    GadgetsThreeState;
    BOOL    TitleOutline;
    BOOL    TitleShadow;
    BOOL    FillTitleBar;
    LONG    TitleColorText;
    LONG    TitleColorShadow;
    BOOL    BarMasking;
    BOOL    CloseGadgetOnRight;
    BOOL    UseGradients;
    LONG    BottomBorderGadgets; /* size with gadgets */
    LONG    RightBorderGadgets; /* size with gadgets */
    LONG    UpDownAddY;
    LONG    LeftRightAddX;
    LONG    ActivatedGradientColor_s;
    LONG    ActivatedGradientColor_e;
    LONG    ActivatedGradientColor_a;
    LONG    DeactivatedGradientColor_s;
    LONG    DeactivatedGradientColor_e;
    LONG    DeactivatedGradientColor_a;
    LONG    ShadeValues_l; /* light */
    LONG    ShadeValues_m; /* middle */
    LONG    ShadeValues_d; /* dark */
    LONG    BaseColors_a;
    LONG    BaseColors_d;
    
    /* Window Section - not used */
    LONG    SizeAddX;
    LONG    SizeAddY;
    LONG    UpDownAddX;
    LONG    LeftRightAddY;
    LONG    BottomBorderNoGadgets; /* size without gadgets */
    LONG    RightBorderNoGadgets; /* size without gadgets */
    LONG    HorizScrollerHeight;
    LONG    ScrollerInnerSpacing;

    /* Menu Section */
    BOOL    MenuIsTiled;
    LONG    MenuTileLeft;
    LONG    MenuTileTop;
    LONG    MenuTileRight;
    LONG    MenuTileBottom;
    ULONG   MenuHighlightTint;
    
    /* Menu Section - not used */
    LONG    MenuInnerLeft;
    LONG    MenuInnerTop;
    LONG    MenuInnerRight;
    LONG    MenuInnerBottom;
};

struct DecorConfig * LoadConfig(STRPTR path);
void FreeConfig(struct DecorConfig * dc);

/* Holds all images used for screen, window and menu decoration */
struct DecorImages
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
};

struct DecorImages * NewImages();
struct DecorImages * LoadImages(struct DecorConfig * dc);
void FreeImages(struct DecorImages * dc);
#endif
