/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <exec/types.h>

struct DecorConfig
{
    /* Screen Section */
    LONG    LeftBorder;
    LONG    RightBorder;
    LONG    BottomBorder;
    LONG    SLogoOffset;
    LONG    STitleOffset;
    LONG    SBarHeight;
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
    LONG    MenuInnerLeft;
    LONG    MenuInnerTop;
    LONG    MenuInnerRight;
    LONG    MenuInnerBottom;
};

struct DecorConfig * LoadConfig(STRPTR path);

#endif
