/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"


static STRPTR SkipChars(STRPTR v)
{
    STRPTR c;
    c = strstr(v, "=");
    return ++c;
}

static LONG GetInt(STRPTR v)
{
    STRPTR c;
    c = SkipChars(v);
    return (LONG) atol(c);
}

static void GetIntegers(STRPTR v, LONG *v1, LONG *v2)
{
    STRPTR c;
    TEXT va1[32], va2[32];
    LONG cnt;
    c = SkipChars(v);
    if (c)
    {
        cnt = sscanf(c, "%s %s", va1, va2);
        if (cnt == 1)
        {
            *v1 = -1;
            *v2 = atol(va1);
        }
        else if (cnt == 2)
        {
            *v1 = atol(va1);
            *v2 = atol(va2);
        }
    }
}

static void GetTripleIntegers(STRPTR v, LONG *v1, LONG *v2, LONG *v3)
{
    STRPTR ch;
    LONG a, b, c;
    LONG cnt;
    ch = SkipChars(v);
    if (ch)
    {
        cnt = sscanf(ch, "%x %x %d", &a, &b, &c);
        if (cnt == 3)
        {
            *v1 = a;
            *v2 = b;
            *v3 = c;
        }
    }
}

static void GetColors(STRPTR v, LONG *v1, LONG *v2)
{
    STRPTR ch;
    LONG a, b;
    LONG cnt;
    ch = SkipChars(v);
    if (ch)
    {
        cnt = sscanf(ch, "%x %x", &a, &b);
        if (cnt == 2)
        {
            *v1 = a;
            *v2 = b;
        }
    }
}


static BOOL GetBool(STRPTR v, STRPTR id)
{
    if (strstr(v, id)) return TRUE; else return FALSE;
}

static void LoadMenuConfig(STRPTR path, struct DecorConfig * dc)
{
    TEXT    buffer[256];
    STRPTR  line, v;
    BPTR    file;
    BPTR    lock;
    BPTR    olddir = 0;

    dc->MenuIsTiled = FALSE;
    dc->MenuTileLeft = 0;
    dc->MenuTileTop = 0;
    dc->MenuTileRight = 0;
    dc->MenuTileBottom = 0;
    dc->MenuInnerLeft = 0;
    dc->MenuInnerTop = 0;
    dc->MenuInnerRight = 0;
    dc->MenuInnerBottom = 0;

    lock = Lock(path, ACCESS_READ);
    if (lock)
        olddir = CurrentDir(lock);
    else 
        return;

    file = Open("Menu/Config", MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "TileLeft ")) == line)
                {
                    dc->MenuTileLeft = GetInt(v);
                    dc->MenuIsTiled = TRUE;
                }
                else  if ((v = strstr(line, "TileTop ")) == line)
                {
                    dc->MenuTileTop = GetInt(v);
                    dc->MenuIsTiled = TRUE;
                }
                else  if ((v = strstr(line, "TileRight ")) == line)
                {
                    dc->MenuTileRight = GetInt(v);
                    dc->MenuIsTiled = TRUE;
                }
                else  if ((v = strstr(line, "TileBottom ")) == line)
                {
                    dc->MenuTileBottom = GetInt(v);
                    dc->MenuIsTiled = TRUE;
                }
                else if ((v = strstr(line, "InnerLeft ")) == line)
                {
                    dc->MenuInnerLeft = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerTop ")) == line)
                {
                    dc->MenuInnerTop = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerRight ")) == line)
                {
                    dc->MenuInnerRight = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerBottom ")) == line)
                {
                    dc->MenuInnerBottom = GetInt(v);
                }
            }
        }
        while(line);
        Close(file);
    }

    if (olddir) CurrentDir(olddir);
    UnLock(lock);
}

static void LoadSystemConfig(STRPTR path, struct DecorConfig * dc)
{
    TEXT    buffer[256];
    STRPTR  line, v;
    BPTR    file;
    BPTR    lock;
    BPTR    olddir = 0;

    dc->BarJoinTB_o = 0;
    dc->BarJoinTB_s = 0;
    dc->BarPreGadget_o = 0;
    dc->BarPreGadget_s = 0;
    dc->BarPre_o = 0;
    dc->BarPre_s = 0;
    dc->BarLGadgetFill_o = 0;
    dc->BarLGadgetFill_s = 0;
    dc->BarJoinGB_o = 0;
    dc->BarJoinGB_s = 0;
    dc->BarLFill_o = 0;
    dc->BarLFill_s = 0;
    dc->BarJoinBT_o = 0;
    dc->BarJoinBT_s = 0;
    dc->BarTitleFill_o = 0;
    dc->BarTitleFill_s = 0;
    dc->BarRFill_o = 0;
    dc->BarRFill_s = 0;
    dc->BarJoinBG_o = 0;
    dc->BarJoinBG_s = 0;
    dc->BarRGadgetFill_o = 0;
    dc->BarRGadgetFill_s = 0;
    dc->BarPostGadget_o = 0;
    dc->BarPostGadget_s = 0;
    dc->BarPost_o = 0;
    dc->BarPost_s = 0;
    dc->ContainerTop_o = 0;
    dc->ContainerTop_s = 0;
    dc->ContainerVertTile_o = 0;
    dc->ContainerVertTile_s = 0;
    dc->ContainerBottom_o = 0;
    dc->ContainerBottom_s = 0;
    dc->KnobTop_o = 0;
    dc->KnobTop_s = 0;
    dc->KnobTileTop_o = 0;
    dc->KnobTileTop_s = 0;
    dc->KnobVertGripper_o = 0;
    dc->KnobVertGripper_s = 0;
    dc->KnobTileBottom_o = 0;
    dc->KnobTileBottom_s = 0;
    dc->KnobBottom_o = 0;
    dc->KnobBottom_s = 0;
    dc->ContainerLeft_o = 0;
    dc->ContainerLeft_s = 0;
    dc->ContainerHorTile_o = 0;
    dc->ContainerHorTile_s = 0;
    dc->ContainerRight_o = 0;
    dc->ContainerRight_s = 0;
    dc->KnobLeft_o = 0;
    dc->KnobLeft_s = 0;
    dc->KnobTileLeft_o = 0;
    dc->KnobTileLeft_s = 0;
    dc->KnobHorGripper_o = 0;
    dc->KnobHorGripper_s = 0;
    dc->KnobTileRight_o = 0;
    dc->KnobTileRight_s = 0;
    dc->KnobRight_o = 0;
    dc->KnobRight_s = 0;
    dc->GadgetsThreeState = FALSE;
    dc->BarRounded = FALSE;
    dc->TitleOutline = FALSE;
    dc->TitleShadow = FALSE;
    dc->FillTitleBar = FALSE;
    dc->BarMasking = FALSE;
    dc->CloseGadgetOnRight = FALSE;
    dc->UseGradients = FALSE;
    dc->BarVertical = FALSE;
    dc->RightBorderGadgets = 0;
    dc->HorizScrollerHeight = 0;
    dc->ScrollerInnerSpacing = 0;
    dc->BottomBorderGadgets = 0;
    dc->RightBorderNoGadgets = 0;
    dc->BottomBorderNoGadgets = 0;
    dc->BarHeight = 0;
    dc->SizeAddX = 0;
    dc->SizeAddY = 0;
    dc->UpDownAddX = 0;
    dc->UpDownAddY = 0;
    dc->LeftRightAddX = 0;
    dc->LeftRightAddY = 0;
    dc->ActivatedGradientColor_s = 0xAAAAAAAA;
    dc->ActivatedGradientColor_e = 0xEEEEEEFF;
    dc->ActivatedGradientColor_a = 0;
    dc->DeactivatedGradientColor_s = 0x66666666;
    dc->DeactivatedGradientColor_e = 0xAAAAAABB;
    dc->DeactivatedGradientColor_a = 0;
    dc->ShadeValues_l = 320;
    dc->ShadeValues_m = 240;
    dc->ShadeValues_d = 128;
    dc->BaseColors_a = 0;
    dc->BaseColors_d = 0;
    dc->TitleColorText = 0x00CCCCCC;
    dc->TitleColorShadow = 0x00444444;
    dc->LeftBorder = 4;
    dc->RightBorder = 4;
    dc->BottomBorder = 4;
    dc->SLogoOffset = 0;
    dc->STitleOffset = 0;
    dc->SBarHeight = 0;
    dc->STitleOutline = FALSE;
    dc->STitleShadow = FALSE;
    dc->LUTBaseColors_a = 0x00CCCCCC;
    dc->LUTBaseColors_d = 0x00888888;
    dc->STitleColorText = 0x00CCCCCC;
    dc->STitleColorShadow = 0x00444444;
    
    
    
    lock = Lock(path, ACCESS_READ);
    if (lock)
    {
        olddir = CurrentDir(lock);
    }
    else return;

    file = Open("System/Config", MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "NoInactiveSelected ")) == line) {
                    dc->GadgetsThreeState = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarRounded ")) == line) {
                    dc->BarRounded = GetBool(v, "Yes");
                } else if ((v = strstr(line, "WindowTitleMode ")) == line) {
                    dc->TitleOutline = GetBool(v, "Outline");
                    dc->TitleShadow = GetBool(v, "Shadow");
                } else if ((v = strstr(line, "FillTitleBar ")) == line) {
                    dc->FillTitleBar = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarMasking ")) == line) {
                    dc->BarMasking = GetBool(v, "Yes");
                } else if ((v = strstr(line, "CloseRight ")) == line) {
                    dc->CloseGadgetOnRight = GetBool(v, "Yes");
                } else if ((v = strstr(line, "UseGradients ")) == line) {
                    dc->UseGradients = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarLayout ")) == line) {
                    dc->BarVertical = GetBool(v, "Vertical");
                } else  if ((v = strstr(line, "RightBorderGads ")) == line) {
                    dc->RightBorderGadgets = GetInt(v);
                } else  if ((v = strstr(line, "HorScrollerHeight ")) == line) {
                    dc->HorizScrollerHeight = GetInt(v);
                } else  if ((v = strstr(line, "ScrollerInnerSpacing ")) == line) {
                    dc->ScrollerInnerSpacing = GetInt(v);
                } else  if ((v = strstr(line, "BottomBorderGads ")) == line) {
                    dc->BottomBorderGadgets = GetInt(v);
                } else  if ((v = strstr(line, "RightBorderNoGads ")) == line) {
                    dc->RightBorderNoGadgets = GetInt(v);
                } else  if ((v = strstr(line, "BottomBorderNoGads ")) == line) {
                    dc->BottomBorderNoGadgets = GetInt(v);
                } else  if ((v = strstr(line, "BarHeight ")) == line) {
                    dc->BarHeight = GetInt(v); //screen, window
                } else  if ((v = strstr(line, "BarJoinTB ")) == line) {
                    GetIntegers(v, &dc->BarJoinTB_o, &dc->BarJoinTB_s);
                } else  if ((v = strstr(line, "BarPreGadget ")) == line) {
                    GetIntegers(v, &dc->BarPreGadget_o, &dc->BarPreGadget_s);
                } else  if ((v = strstr(line, "BarPre ")) == line) {
                    GetIntegers(v, &dc->BarPre_o, &dc->BarPre_s);
                } else  if ((v = strstr(line, "BarLGadgetFill ")) == line) {
                    GetIntegers(v, &dc->BarLGadgetFill_o, &dc->BarLGadgetFill_s);
                } else  if ((v = strstr(line, "BarJoinGB ")) == line) {
                    GetIntegers(v, &dc->BarJoinGB_o, &dc->BarJoinGB_s);
                } else  if ((v = strstr(line, "BarLFill ")) == line) {
                    GetIntegers(v, &dc->BarLFill_o, &dc->BarLFill_s);
                } else  if ((v = strstr(line, "BarJoinBT ")) == line) {
                    GetIntegers(v, &dc->BarJoinBT_o, &dc->BarJoinBT_s);
                } else  if ((v = strstr(line, "BarTitleFill ")) == line) {
                    GetIntegers(v, &dc->BarTitleFill_o, &dc->BarTitleFill_s);
                } else  if ((v = strstr(line, "BarRFill ")) == line) {
                    GetIntegers(v, &dc->BarRFill_o, &dc->BarRFill_s);
                } else  if ((v = strstr(line, "BarJoinBG ")) == line) {
                    GetIntegers(v, &dc->BarJoinBG_o, &dc->BarJoinBG_s);
                } else  if ((v = strstr(line, "BarRGadgetFill ")) == line) {
                    GetIntegers(v, &dc->BarRGadgetFill_o, &dc->BarRGadgetFill_s);
                } else  if ((v = strstr(line, "BarPostGadget ")) == line) {
                    GetIntegers(v, &dc->BarPostGadget_o, &dc->BarPostGadget_s);
                } else  if ((v = strstr(line, "BarPost ")) == line) {
                    GetIntegers(v, &dc->BarPost_o, &dc->BarPost_s);
                } else  if ((v = strstr(line, "ContainerTop ")) == line) {
                    GetIntegers(v, &dc->ContainerTop_o, &dc->ContainerTop_s);
                } else  if ((v = strstr(line, "ContainerVertTile ")) == line) {
                    GetIntegers(v, &dc->ContainerVertTile_o, &dc->ContainerVertTile_s);
                } else  if ((v = strstr(line, "KnobTop ")) == line) {
                    GetIntegers(v, &dc->KnobTop_o, &dc->KnobTop_s);
                } else  if ((v = strstr(line, "KnobTileTop ")) == line) {
                    GetIntegers(v, &dc->KnobTileTop_o, &dc->KnobTileTop_s);
                } else  if ((v = strstr(line, "KnobVertGripper ")) == line) {
                    GetIntegers(v, &dc->KnobVertGripper_o, &dc->KnobVertGripper_s);
                } else  if ((v = strstr(line, "KnobTileBottom ")) == line) {
                    GetIntegers(v, &dc->KnobTileBottom_o, &dc->KnobTileBottom_s);
                } else  if ((v = strstr(line, "KnobBottom ")) == line) {
                    GetIntegers(v, &dc->KnobBottom_o, &dc->KnobBottom_s);
                } else  if ((v = strstr(line, "ContainerBottom ")) == line) {
                    GetIntegers(v, &dc->ContainerBottom_o, &dc->ContainerBottom_s);
                } else  if ((v = strstr(line, "ContainerLeft ")) == line) {
                    GetIntegers(v, &dc->ContainerLeft_o, &dc->ContainerLeft_s);
                } else  if ((v = strstr(line, "ContainerHorTile ")) == line) {
                    GetIntegers(v, &dc->ContainerHorTile_o, &dc->ContainerHorTile_s);
                } else  if ((v = strstr(line, "KnobLeft ")) == line) {
                    GetIntegers(v, &dc->KnobLeft_o, &dc->KnobLeft_s);
                } else  if ((v = strstr(line, "KnobTileLeft ")) == line) {
                    GetIntegers(v, &dc->KnobTileLeft_o, &dc->KnobTileLeft_s);
                } else  if ((v = strstr(line, "KnobHorGripper ")) == line) {
                    GetIntegers(v, &dc->KnobHorGripper_o, &dc->KnobHorGripper_s);
                } else  if ((v = strstr(line, "KnobTileRight ")) == line) {
                    GetIntegers(v, &dc->KnobTileRight_o, &dc->KnobTileRight_s);
                } else  if ((v = strstr(line, "KnobRight ")) == line) {
                    GetIntegers(v, &dc->KnobRight_o, &dc->KnobRight_s);
                } else  if ((v = strstr(line, "ContainerRight ")) == line) {
                    GetIntegers(v, &dc->ContainerRight_o, &dc->ContainerRight_s);
                } else  if ((v = strstr(line, "AddSize ")) == line) {
                    GetIntegers(v, &dc->SizeAddX, &dc->SizeAddY);
                } else  if ((v = strstr(line, "AddUpDown ")) == line) {
                    GetIntegers(v, &dc->UpDownAddX, &dc->UpDownAddY);
                } else  if ((v = strstr(line, "AddLeftRight ")) == line) {
                    GetIntegers(v, &dc->LeftRightAddX, &dc->LeftRightAddY);
                } else  if ((v = strstr(line, "ActivatedGradient ")) == line) {
                    GetTripleIntegers(v, &dc->ActivatedGradientColor_s, &dc->ActivatedGradientColor_e, &dc->ActivatedGradientColor_a);
                } else  if ((v = strstr(line, "DeactivatedGradient ")) == line) {
                    GetTripleIntegers(v, &dc->DeactivatedGradientColor_s, &dc->DeactivatedGradientColor_e, &dc->DeactivatedGradientColor_a);
                } else  if ((v = strstr(line, "ShadeValues ")) == line) {
                    GetTripleIntegers(v, &dc->ShadeValues_l, &dc->ShadeValues_m, &dc->ShadeValues_d);
                } else  if ((v = strstr(line, "BaseColors ")) == line) {
                    GetColors(v, &dc->BaseColors_a, &dc->BaseColors_d);
                } else  if ((v = strstr(line, "WindowTitleColors ")) == line) {
                    GetColors(v, &dc->TitleColorText, &dc->TitleColorShadow);
                } else if ((v = strstr(line, "LeftBorder ")) == line) {
                    dc->LeftBorder = GetInt(v);
                } else  if ((v = strstr(line, "RightBorder ")) == line) {
                    dc->RightBorder = GetInt(v);
                } else  if ((v = strstr(line, "BottomBorder ")) == line) {
                    dc->BottomBorder = GetInt(v);
                } else  if ((v = strstr(line, "LogoOffset ")) == line) {
                    dc->SLogoOffset = GetInt(v);
                } else  if ((v = strstr(line, "TitleOffset ")) == line) {
                    dc->STitleOffset = GetInt(v);
                } else  if ((v = strstr(line, "SBarHeight ")) == line) {
                    dc->SBarHeight = GetInt(v);
                } else  if ((v = strstr(line, "LUTBaseColors ")) == line) {
                    GetColors(v, &dc->LUTBaseColors_a, &dc->LUTBaseColors_d);
                } else  if ((v = strstr(line, "ScreenTitleColors ")) == line) {
                    GetColors(v, &dc->STitleColorText, &dc->STitleColorShadow);
                } else if ((v = strstr(line, "ScreenTitleMode ")) == line) {
                    dc->STitleOutline = GetBool(v, "Outline");
                    dc->STitleShadow = GetBool(v, "Shadow");
                }
            }
        }
        while(line);
        Close(file);
    }

    if (olddir) CurrentDir(olddir);
    UnLock(lock);
}

struct DecorConfig * LoadConfig(STRPTR path)
{
    struct DecorConfig * dc = AllocVec(sizeof(struct DecorConfig), MEMF_ANY | MEMF_CLEAR);
    
    LoadMenuConfig(path, dc);
    
    LoadSystemConfig(path, dc);
    
    return dc;
};
