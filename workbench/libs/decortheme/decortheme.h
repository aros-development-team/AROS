/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: decortheme.library public header

    decortheme.library parses theme configurations and builds the
    decorator.library element sets used to render themed system
    components. Multiple themes may be loaded concurrently, by any
    number of clients - every theme is a self contained DecorTheme
    instance and the library keeps no global theme state.
*/

#ifndef LIBRARIES_DECORTHEME_H
#define LIBRARIES_DECORTHEME_H

#include <exec/types.h>
#include <libraries/decorator.h>

/* ========== Theme Configuration ========== */

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
    LONG    SBarFrame;
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
    LONG    WinFrameStyle;
    BOOL    BarRounded;
    BOOL    BarVertical;
    LONG    BarHeight;
    LONG    BarFrame;
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
    LONG    BottomBorderGadgets;
    LONG    RightBorderGadgets;
    LONG    UpDownAddY;
    LONG    LeftRightAddX;
    LONG    ActivatedGradientColor_s;
    LONG    ActivatedGradientColor_e;
    LONG    ActivatedGradientColor_a;
    LONG    DeactivatedGradientColor_s;
    LONG    DeactivatedGradientColor_e;
    LONG    DeactivatedGradientColor_a;
    LONG    ShadeValues_l;
    LONG    ShadeValues_m;
    LONG    ShadeValues_d;
    LONG    BaseColors_a;
    LONG    BaseColors_d;

    /* Window Section - not used */
    LONG    SizeAddX;
    LONG    SizeAddY;
    LONG    UpDownAddX;
    LONG    LeftRightAddY;
    LONG    BottomBorderNoGadgets;
    LONG    RightBorderNoGadgets;
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

/* Holds all images used for screen, window and menu decoration */
struct DecorImages
{
    struct DecorImage *img_sdepth;
    struct DecorImage *img_sbarlogo;
    struct DecorImage *img_stitlebar;

    struct DecorImage *img_size;
    struct DecorImage *img_close;
    struct DecorImage *img_depth;
    struct DecorImage *img_zoom;
    struct DecorImage *img_up;
    struct DecorImage *img_down;
    struct DecorImage *img_left;
    struct DecorImage *img_right;
    struct DecorImage *img_mui;
    struct DecorImage *img_popup;
    struct DecorImage *img_snapshot;
    struct DecorImage *img_iconify;
    struct DecorImage *img_lock;
    struct DecorImage *img_winbar_normal;
    struct DecorImage *img_border_normal;
    struct DecorImage *img_border_deactivated;
    struct DecorImage *img_verticalcontainer;
    struct DecorImage *img_verticalknob;
    struct DecorImage *img_horizontalcontainer;
    struct DecorImage *img_horizontalknob;

    struct DecorImage *img_menu;
    struct DecorImage *img_amigakey;
    struct DecorImage *img_menucheck;
    struct DecorImage *img_submenu;
};

/* ========== Element Identifiers ========== */
/* Window titlebar elements */
#define DECOR_ELEM_WinBarPreGadget      0
#define DECOR_ELEM_WinBarPre            1
#define DECOR_ELEM_WinBarLGadgetFill    2
#define DECOR_ELEM_WinBarJoinGB         3
#define DECOR_ELEM_WinBarLFill          4
#define DECOR_ELEM_WinBarJoinBT         5
#define DECOR_ELEM_WinBarTitleFill      6
#define DECOR_ELEM_WinBarJoinTB         7
#define DECOR_ELEM_WinBarRFill          8
#define DECOR_ELEM_WinBarJoinBG         9
#define DECOR_ELEM_WinBarRGadgetFill    10
#define DECOR_ELEM_WinBarPostGadget     11
#define DECOR_ELEM_WinBarPost           12

/* Window gadget elements */
#define DECOR_ELEM_WinClose             13
#define DECOR_ELEM_WinDepth             14
#define DECOR_ELEM_WinZoom              15
#define DECOR_ELEM_WinSize              16
#define DECOR_ELEM_WinMUI               17
#define DECOR_ELEM_WinPopup             18
#define DECOR_ELEM_WinSnapshot          19
#define DECOR_ELEM_WinIconify           20
#define DECOR_ELEM_WinLock              21
#define DECOR_ELEM_WinUp                22
#define DECOR_ELEM_WinDown              23
#define DECOR_ELEM_WinLeft              24
#define DECOR_ELEM_WinRight             25

/* Vertical scrollbar elements */
#define DECOR_ELEM_VContainerTop        26
#define DECOR_ELEM_VContainerTile       27
#define DECOR_ELEM_VContainerBottom     28
#define DECOR_ELEM_VKnobTop             29
#define DECOR_ELEM_VKnobTileTop         30
#define DECOR_ELEM_VKnobGripper         31
#define DECOR_ELEM_VKnobTileBottom      32
#define DECOR_ELEM_VKnobBottom          33

/* Horizontal scrollbar elements */
#define DECOR_ELEM_HContainerLeft       34
#define DECOR_ELEM_HContainerTile       35
#define DECOR_ELEM_HContainerRight      36
#define DECOR_ELEM_HKnobLeft            37
#define DECOR_ELEM_HKnobTileLeft        38
#define DECOR_ELEM_HKnobGripper         39
#define DECOR_ELEM_HKnobTileRight       40
#define DECOR_ELEM_HKnobRight           41

/* Screen bar elements */
#define DECOR_ELEM_ScrBarFill           42
#define DECOR_ELEM_ScrBarGadPre         43
#define DECOR_ELEM_ScrBarGadFill        44
#define DECOR_ELEM_ScrBarGadPost        45
#define DECOR_ELEM_ScrBarChildPre       46
#define DECOR_ELEM_ScrBarChildFill      47
#define DECOR_ELEM_ScrBarChildPost      48
#define DECOR_ELEM_ScrDepth             49
#define DECOR_ELEM_ScrBarLogo           50

/* Menu elements */
#define DECOR_ELEM_MenuBackground       51
#define DECOR_ELEM_MenuAmigaKey         52
#define DECOR_ELEM_MenuCheck            53
#define DECOR_ELEM_MenuSubMenu          54

/* Border/background elements */
#define DECOR_ELEM_WinBorderNormal      55
#define DECOR_ELEM_WinBorderDeactivated 56
#define DECOR_ELEM_WinBarNormal         57

#define DECOR_NUM_ELEMENTS              58

/* ========== Theme Instance ========== */
/* A fully loaded theme: configuration, images and the element
   descriptors built from them. Instances are independent of each
   other - any number may exist in the system at the same time. */

struct DecorTheme
{
    struct DecorConfig      *dt_Config;
    struct DecorImages      *dt_Images;
    struct DecoratorElement  dt_Elements[DECOR_NUM_ELEMENTS];
};

/* A theme instantiated for a particular screen: image copies matched
   to the screen's depth/attributes and element descriptors over those
   copies. Obtained with DTObtainScreenTheme(); the source DecorTheme
   must outlive all screen instances obtained from it. */

struct DecorThemeScreen
{
    struct DecorTheme       *dts_Theme;     /* Source theme (not owned) */
    struct Screen           *dts_Screen;
    BOOL                     dts_TrueColor;
    struct DecorImages      *dts_Images;    /* Screen matched image copies (owned) */
    struct DecoratorElement  dts_Elements[DECOR_NUM_ELEMENTS];
};

#endif /* LIBRARIES_DECORTHEME_H */
