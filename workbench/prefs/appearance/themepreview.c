/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Renders a live preview of the selected theme using decortheme.library
    (theme parsing/instantiation) and decorator.library (element
    rendering). The preview holds its own DecorTheme instance, entirely
    independent of the theme the system decorator currently uses.
*/

#define DEBUG 0
#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <intuition/imageclass.h>
#include <graphics/rpattr.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/decorator.h>
#include <proto/decortheme.h>

#include <proto/alib.h>

#include <libraries/decortheme.h>

#include <stdlib.h>
#include <string.h>

#include "locale.h"
#include "themepreview.h"

#define PREVIEW_WIDTH  256
#define PREVIEW_HEIGHT 150

struct Library *DecoratorBase = NULL;
struct Library *DecorThemeBase = NULL;
static LONG tp_LibUsers = 0;

/*** Instance Data **********************************************************/

struct ThemePreview_DATA
{
    char                    *tp_Theme;          /* Theme path (THEMES:<name>) */
    struct Screen           *tp_Screen;         /* Screen we render for (between Setup/Cleanup) */
    struct DecorTheme       *tp_DecorTheme;     /* Our private theme instance */
    struct DecorThemeScreen *tp_ScreenTheme;    /* ... instantiated for tp_Screen */
    struct BitMap           *tp_BM;
    struct RastPort         *tp_Buffer;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct ThemePreview_DATA *data = INST_DATA(CLASS, self)

/*** Rendering **************************************************************/

static void ThemePreview__FreeTheme(struct ThemePreview_DATA *data)
{
    if (data->tp_ScreenTheme)
    {
        DTReleaseScreenTheme(data->tp_ScreenTheme);
        data->tp_ScreenTheme = NULL;
    }
    if (data->tp_DecorTheme)
    {
        DTFreeTheme(data->tp_DecorTheme);
        data->tp_DecorTheme = NULL;
    }
}

static void ThemePreview__DisposeBuffer(struct ThemePreview_DATA *data)
{
    if (data->tp_Buffer)
    {
        FreeRastPort(data->tp_Buffer);
        data->tp_Buffer = NULL;
    }
    if (data->tp_BM)
    {
        FreeBitMap(data->tp_BM);
        data->tp_BM = NULL;
    }
}

static void ThemePreview__FillRect(struct RastPort *rp, LONG x0, LONG y0, LONG x1, LONG y1, ULONG argb)
{
    SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, argb, TAG_DONE);
    RectFill(rp, x0, y0, x1, y1);
}

/* Builds (or rebuilds) the offscreen preview of the currently selected
   theme. Rendering is a mock scene - screen bar, a window with titlebar,
   gadgets and a scrollbar - composed entirely from theme elements. */
static void ThemePreview__RenderPreview(struct ThemePreview_DATA *data)
{
    struct DecorThemeScreen *dts;
    struct DecorConfig      *dc;
    struct DecoratorElement *els;
    struct DecorImages      *di;
    struct RastPort         *rp;
    BOOL                     truecolor;
    LONG                     sbarh, barh, gcols;
    LONG                     wx0, wy0, ww, wh, wx1;
    LONG                     x, y, tx;
    ULONG                    col;

    ThemePreview__FreeTheme(data);

    if (!DecorThemeBase || !DecoratorBase || !data->tp_Theme || !data->tp_Screen)
    {
        ThemePreview__DisposeBuffer(data);
        return;
    }

    D(bug("[ThemePreview] %s: loading '%s'\n", __func__, data->tp_Theme));

    data->tp_DecorTheme = DTLoadTheme(data->tp_Theme);
    if (!data->tp_DecorTheme)
    {
        ThemePreview__DisposeBuffer(data);
        return;
    }

    truecolor = (GetBitMapAttr(data->tp_Screen->RastPort.BitMap, BMA_DEPTH) > 8) ? TRUE : FALSE;

    data->tp_ScreenTheme = DTObtainScreenTheme(data->tp_DecorTheme, data->tp_Screen, truecolor);
    if (!data->tp_ScreenTheme)
    {
        ThemePreview__FreeTheme(data);
        ThemePreview__DisposeBuffer(data);
        return;
    }

    if (!data->tp_Buffer)
    {
        if ((data->tp_BM = AllocBitMap(PREVIEW_WIDTH, PREVIEW_HEIGHT, 1, BMF_CLEAR,
                                       data->tp_Screen->RastPort.BitMap)) == NULL)
            return;
        if ((data->tp_Buffer = CreateRastPort()) == NULL)
        {
            ThemePreview__DisposeBuffer(data);
            return;
        }
        data->tp_Buffer->BitMap = data->tp_BM;
    }

    dts = data->tp_ScreenTheme;
    dc = data->tp_DecorTheme->dt_Config;
    els = dts->dts_Elements;
    di = dts->dts_Images;
    rp = data->tp_Buffer;

    gcols = dc->GadgetsThreeState ? 3 : 4;

    sbarh = dc->SBarHeight;
    if (sbarh < 10) sbarh = 10;
    if (sbarh > 28) sbarh = 28;

    barh = dc->BarHeight;
    if (barh < 10) barh = 10;
    if (barh > 30) barh = 30;

    /* Desktop background */
    ThemePreview__FillRect(rp, 0, 0, PREVIEW_WIDTH - 1, PREVIEW_HEIGHT - 1, 0x00AAAAAA);

    /* == Screen bar == */
    DRenderElement(&els[DECOR_ELEM_ScrBarFill], rp, 0, 0, 0, PREVIEW_WIDTH, sbarh, 0);
    DRenderElement(&els[DECOR_ELEM_ScrBarLogo], rp, 0, dc->SLogoOffset,
        (sbarh - di->img_sbarlogo->h) / 2, di->img_sbarlogo->w, di->img_sbarlogo->h, 0);
    if (di->img_sdepth->ok)
    {
        LONG dw = di->img_sdepth->w >> 1;
        DRenderElement(&els[DECOR_ELEM_ScrDepth], rp, IDS_NORMAL,
            PREVIEW_WIDTH - dw - 2, (sbarh - di->img_sdepth->h) / 2, -1, -1, 0);
    }

    /* Screen title */
    col = (dc->STitleColorText != 0xFFFFFFFF) ? dc->STitleColorText : 0x00CCCCCC;
    SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, col, TAG_DONE);
    SetDrMd(rp, JAM1);
    Move(rp, dc->STitleOffset + 2, (sbarh >> 1) + 3);
    Text(rp, "AROS", 4);

    /* == Mock window == */
    wx0 = 14;
    wy0 = sbarh + 10;
    ww = PREVIEW_WIDTH - 28;
    wh = PREVIEW_HEIGHT - wy0 - 10;
    wx1 = wx0 + ww;

    /* Window border background */
    if (di->img_border_normal->ok)
        DRenderElement(&els[DECOR_ELEM_WinBorderNormal], rp, 0, wx0, wy0, ww, wh, 0);
    else
    {
        struct GradientSpec spec;
        spec.gs_xt = 0;
        spec.gs_yt = 0;
        spec.gs_xb = ww;
        spec.gs_yb = wh;
        spec.gs_xp = wx0;
        spec.gs_yp = wy0;
        spec.gs_w = ww;
        spec.gs_h = wh;
        spec.gs_StartRGB = dc->ActivatedGradientColor_s;
        spec.gs_EndRGB = dc->ActivatedGradientColor_e;
        spec.gs_Angle = dc->ActivatedGradientColor_a;
        spec.gs_dx = 0;
        spec.gs_dy = 0;
        DFillPixelArrayGradient(-1, TRUE, rp, &spec);
    }

    /* Window inner area */
    ThemePreview__FillRect(rp,
        wx0 + dc->LeftBorder, wy0 + barh,
        wx1 - 1 - dc->RightBorder, wy0 + wh - 1 - dc->BottomBorder,
        0x00F4F4F4);

    /* Titlebar - composed from the theme's bar section elements.
       Subimage row 0 selects the active state */
    if (di->img_winbar_normal->ok)
    {
        LONG closew = di->img_close->ok ? di->img_close->w / gcols : 0;
        LONG zoomw = di->img_zoom->ok ? di->img_zoom->w / gcols : 0;
        LONG depthw = di->img_depth->ok ? di->img_depth->w / gcols : 0;
        LONG rgadx, titlew;

        x = wx0;
        x = DRenderElement(&els[DECOR_ELEM_WinBarPreGadget], rp, 0, x, wy0, dc->BarPreGadget_s, barh, wx1);
        if (closew)
        {
            DRenderElement(&els[DECOR_ELEM_WinClose], rp, IDS_NORMAL,
                x, wy0 + ((barh - di->img_close->h) / 2), -1, -1, 0);
            x += closew;
        }
        x = DRenderElement(&els[DECOR_ELEM_WinBarJoinGB], rp, 0, x, wy0, dc->BarJoinGB_s, barh, wx1);
        x = DRenderElement(&els[DECOR_ELEM_WinBarJoinBT], rp, 0, x, wy0, dc->BarJoinBT_s, barh, wx1);
        tx = x + 3;

        rgadx = wx1 - dc->BarPostGadget_s - zoomw - depthw;
        titlew = rgadx - x - dc->BarJoinTB_s - dc->BarJoinBG_s;
        if (titlew > 0)
            x = DRenderElement(&els[DECOR_ELEM_WinBarTitleFill], rp, 0, x, wy0, titlew, barh, wx1);
        x = DRenderElement(&els[DECOR_ELEM_WinBarJoinTB], rp, 0, x, wy0, dc->BarJoinTB_s, barh, wx1);
        x = DRenderElement(&els[DECOR_ELEM_WinBarJoinBG], rp, 0, x, wy0, dc->BarJoinBG_s, barh, wx1);
        if (zoomw)
        {
            DRenderElement(&els[DECOR_ELEM_WinZoom], rp, IDS_NORMAL,
                x, wy0 + ((barh - di->img_zoom->h) / 2), -1, -1, 0);
            x += zoomw;
        }
        if (depthw)
        {
            DRenderElement(&els[DECOR_ELEM_WinDepth], rp, IDS_NORMAL,
                x, wy0 + ((barh - di->img_depth->h) / 2), -1, -1, 0);
            x += depthw;
        }
        DRenderElement(&els[DECOR_ELEM_WinBarPostGadget], rp, 0, x, wy0, dc->BarPostGadget_s, barh, wx1);

        /* Window title */
        col = (dc->TitleColorText != 0xFFFFFFFF) ? dc->TitleColorText : 0x00FFFFFF;
        SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, col, TAG_DONE);
        SetDrMd(rp, JAM1);
        Move(rp, tx, wy0 + (barh >> 1) + 3);
        Text(rp, "Preview", 7);
    }

    /* Vertical scrollbar - container with a knob */
    if (di->img_verticalcontainer->ok)
    {
        LONG contw = di->img_verticalcontainer->w >> 1;
        LONG vx = wx1 - contw - 1;
        LONG vy0 = wy0 + barh + 1;
        LONG vy1 = wy0 + wh - 1 - dc->BottomBorder;
        LONG mid = (vy1 - vy0) - dc->ContainerTop_s - dc->ContainerBottom_s;

        y = vy0;
        y = DRenderElement(&els[DECOR_ELEM_VContainerTop], rp, 0, vx, y, 0, dc->ContainerTop_s, 0);
        if (mid > 0)
            y = DRenderElement(&els[DECOR_ELEM_VContainerTile], rp, 0, vx, y, 0, mid, 0);
        DRenderElement(&els[DECOR_ELEM_VContainerBottom], rp, 0, vx, y, 0, dc->ContainerBottom_s, 0);

        if (di->img_verticalknob->ok)
        {
            LONG knobw = di->img_verticalknob->w / 3;
            LONG kx = vx + ((contw - knobw) / 2);
            LONG kh = ((vy1 - vy0) * 2) / 5;
            LONG ktile = kh - dc->KnobTop_s - dc->KnobBottom_s;

            y = vy0 + 2;
            y = DRenderElement(&els[DECOR_ELEM_VKnobTop], rp, 0, kx, y, 0, dc->KnobTop_s, 0);
            if (ktile > 0)
                y = DRenderElement(&els[DECOR_ELEM_VKnobTileTop], rp, 0, kx, y, 0, ktile, 0);
            DRenderElement(&els[DECOR_ELEM_VKnobBottom], rp, 0, kx, y, 0, dc->KnobBottom_s, 0);
        }
    }
}

/*** Methods ****************************************************************/
Object *ThemePreview__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    D(bug("[ThemePreview] %s()\n", __PRETTY_FUNCTION__));

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_Frame, MUIV_Frame_ReadList,
        MUIA_FrameTitle, _(MSG_PREVIEW),

        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;
        STRPTR theme;

        if (tp_LibUsers++ == 0)
        {
            DecoratorBase = OpenLibrary("decorator.library", 0);
            DecorThemeBase = OpenLibrary("decortheme.library", 0);
        }

        theme = (STRPTR)GetTagData(MUIA_ThemePreview_Theme, (IPTR)NULL, message->ops_AttrList);
        if (theme)
        {
            data->tp_Theme = AllocVec(strlen(theme) + 1, MEMF_ANY);
            if (data->tp_Theme)
                strcpy(data->tp_Theme, theme);
        }
    }

    return self;
}

IPTR ThemePreview__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    D(bug("[ThemePreview] %s()\n", __PRETTY_FUNCTION__));

    ThemePreview__FreeTheme(data);
    ThemePreview__DisposeBuffer(data);
    FreeVec(data->tp_Theme);

    if (--tp_LibUsers == 0)
    {
        if (DecorThemeBase)
        {
            CloseLibrary(DecorThemeBase);
            DecorThemeBase = NULL;
        }
        if (DecoratorBase)
        {
            CloseLibrary(DecoratorBase);
            DecoratorBase = NULL;
        }
    }

    return DoSuperMethodA(CLASS, self, message);
}

IPTR ThemePreview__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tags, *tag;

    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_ThemePreview_Theme:
            {
                STRPTR theme = (STRPTR)tag->ti_Data;

                D(bug("[ThemePreview] %s: Theme '%s'\n", __PRETTY_FUNCTION__, theme ? theme : (STRPTR)"(none)"));

                /* Ignore if unchanged */
                if (theme && data->tp_Theme && !strcmp(theme, data->tp_Theme))
                    break;
                if (!theme && !data->tp_Theme)
                    break;

                FreeVec(data->tp_Theme);
                data->tp_Theme = NULL;
                if (theme)
                {
                    data->tp_Theme = AllocVec(strlen(theme) + 1, MEMF_ANY);
                    if (data->tp_Theme)
                        strcpy(data->tp_Theme, theme);
                }

                if (data->tp_Screen)
                {
                    ThemePreview__RenderPreview(data);
                    MUI_Redraw(self, MADF_DRAWOBJECT);
                }
                break;
            }
        }
    }

    return DoSuperMethodA(CLASS, self, message);
}

IPTR ThemePreview__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_INST_DATA;

    switch (message->opg_AttrID)
    {
        case MUIA_ThemePreview_Theme:
            *message->opg_Storage = (IPTR)data->tp_Theme;
            break;
        default:
            return DoSuperMethodA(CLASS, self, (Msg)message);
    }

    return TRUE;
}

IPTR ThemePreview__MUIM_Setup(struct IClass *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    if (!DoSuperMethodA(CLASS, self, message))
        return FALSE;

    data->tp_Screen = _screen(self);
    ThemePreview__RenderPreview(data);

    return TRUE;
}

IPTR ThemePreview__MUIM_Cleanup(struct IClass *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    ThemePreview__FreeTheme(data);
    ThemePreview__DisposeBuffer(data);
    data->tp_Screen = NULL;

    return DoSuperMethodA(CLASS, self, message);
}

IPTR ThemePreview__MUIM_AskMinMax(struct IClass *CLASS, Object *self, struct MUIP_AskMinMax *message)
{
    ULONG       rc = DoSuperMethodA(CLASS, self, (Msg) message);

    D(bug("[ThemePreview] %s()\n", __PRETTY_FUNCTION__));

    message->MinMaxInfo->MinWidth  += PREVIEW_WIDTH;
    message->MinMaxInfo->MinHeight += PREVIEW_HEIGHT;

    message->MinMaxInfo->DefWidth  += PREVIEW_WIDTH;
    message->MinMaxInfo->DefHeight += PREVIEW_HEIGHT;

    message->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    message->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return rc;
}

IPTR ThemePreview__MUIM_Draw(struct IClass *CLASS, Object *self, struct MUIP_Draw *message)
{
    SETUP_INST_DATA;

    APTR        clip;

    D(bug("[ThemePreview] %s()\n", __PRETTY_FUNCTION__));

    DoSuperMethodA(CLASS, self, (Msg)message);

    if (data->tp_Buffer)
    {
        /* Centre the preview in the object area */
        LONG px = _mleft(self) + (((_mright(self) - _mleft(self) + 1) - PREVIEW_WIDTH) / 2);
        LONG py = _mtop(self) + (((_mbottom(self) - _mtop(self) + 1) - PREVIEW_HEIGHT) / 2);

        if (px < _mleft(self)) px = _mleft(self);
        if (py < _mtop(self)) py = _mtop(self);

        clip = MUI_AddClipping(muiRenderInfo(self), _mleft(self), _mtop(self), _mright(self) - _mleft(self) + 1, _mbottom(self) - _mtop(self) + 1);
        BltBitMapRastPort(data->tp_Buffer->BitMap,
                  0, 0,
                  _rp(self),
                  px, py,
                  PREVIEW_WIDTH, PREVIEW_HEIGHT,
                  0xC0);
        MUI_RemoveClipping(muiRenderInfo(self), clip);
    }
    else
    {
        char *unavailableStr = (char *)_(MSG_NOPREVIEW);
        int len = strlen(unavailableStr);
        struct TextExtent       te;

        len = TextFit(_rp(self), unavailableStr, len, &te, NULL, 1, _mright(self) - _mleft(self) + 1, _mbottom(self) - _mtop(self) + 1);

        SetDrMd(_rp(self), JAM1);
        Move(_rp(self), _mleft(self) + ((_mright(self) - _mleft(self) + 1) >> 1) - (te.te_Width >> 1), _mtop(self) + ((_mbottom(self) - _mtop(self) + 1) >> 1) - (te.te_Height >> 1));
        Text(_rp(self), unavailableStr, len);
    }
    return 0;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_8
(
    ThemePreview, NULL, MUIC_Area, NULL,
    MUIM_Draw,      struct MUIP_Draw *,
    MUIM_AskMinMax, struct MUIP_AskMinMax *,
    MUIM_Setup,     Msg,
    MUIM_Cleanup,   Msg,
    OM_GET,         struct opGet *,
    OM_SET,         struct opSet *,
    OM_NEW,         struct opSet *,
    OM_DISPOSE,     Msg
);
