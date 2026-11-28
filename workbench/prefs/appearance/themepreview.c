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
    ULONG                    tp_Size;           /* Image size to show it in (0 = its default) */
    LONG                     tp_Width;          /* Dimensions of the preview, which grow with */
    LONG                     tp_Height;         /* the image size of the theme */
    struct Screen           *tp_Screen;         /* Screen we render for (between Setup/Cleanup) */
    struct DecorTheme       *tp_DecorTheme;     /* Our private theme instance */
    struct DecorThemeScreen *tp_ScreenTheme;    /* ... instantiated for tp_Screen */
    struct BitMap           *tp_BM;
    struct RastPort         *tp_Buffer;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct ThemePreview_DATA *data = INST_DATA(CLASS, self)

/*** Libraries **************************************************************/

void ThemePreview_OpenLibs(void)
{
    if (tp_LibUsers++ == 0)
    {
        DecoratorBase = OpenLibrary("decorator.library", 0);
        DecorThemeBase = OpenLibrary("decortheme.library", 0);
    }
}

void ThemePreview_CloseLibs(void)
{
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
}

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
    data->tp_Width = 0;
    data->tp_Height = 0;
}

static void ThemePreview__FillRect(struct RastPort *rp, LONG x0, LONG y0, LONG x1, LONG y1, ULONG argb)
{
    SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, argb, TAG_DONE);
    RectFill(rp, x0, y0, x1, y1);
}

/* Draws a bar title the way the decoration does: vertically centred on
   the bar, in the theme's colours, outlined or shadowed if it says so */
static void ThemePreview__Text(struct RastPort *rp, struct TextFont *font, CONST_STRPTR text,
    LONG tx, LONG top, LONG barh, ULONG color, ULONG shadow, BOOL outline, BOOL shadowed)
{
    LONG len = strlen(text);
    LONG tymax = barh - (font->tf_YSize - font->tf_Baseline) - 1;
    LONG ty = (barh + font->tf_Baseline - 1) >> 1;

    if (ty > tymax) ty = tymax;
    ty += top;

    SetDrMd(rp, JAM1);
    if (outline)
    {
        static const BYTE off[8][2] = { {1,0}, {2,0}, {0,0}, {0,1}, {0,2}, {1,2}, {2,1}, {2,2} };
        int i;

        SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, shadow, TAG_DONE);
        for (i = 0; i < 8; i++)
        {
            Move(rp, tx + off[i][0], ty + off[i][1]);
            Text(rp, text, len);
        }
        tx++;
        ty++;
    }
    else if (shadowed)
    {
        SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, shadow, TAG_DONE);
        Move(rp, tx + 1, ty + 1);
        Text(rp, text, len);
    }

    /* the theme's title colours only apply to outlined and shadowed
       titles; a plain title is drawn with the screen's text pen */
    if (!outline && !shadowed)
        color = 0x00000000;

    SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, color, TAG_DONE);
    Move(rp, tx, ty);
    Text(rp, text, len);
}

/* Builds (or rebuilds) the offscreen preview of the currently selected
   theme. Rendering is a mock scene - screen bar, a window with titlebar,
   gadgets and a scrollbar - composed entirely from theme elements, laid
   out with the sizes the theme defines as the decoration does it. */
static void ThemePreview__RenderPreview(struct ThemePreview_DATA *data)
{
    struct DecorThemeScreen *dts;
    struct DecorConfig      *dc;
    struct DecoratorElement *els;
    struct DecorImages      *di;
    struct RastPort         *rp;
    BOOL                     truecolor;
    struct TextFont         *font;
    LONG                     sbarh, barh, gcols;
    LONG                     bl, br, bb, sdepthw = 0;
    LONG                     pw, ph;
    LONG                     wx0, wy0, ww, wh, wx1, wy1;
    LONG                     x, y, tx;

    ThemePreview__FreeTheme(data);

    if (!DecorThemeBase || !DecoratorBase || !data->tp_Theme || !data->tp_Screen)
    {
        ThemePreview__DisposeBuffer(data);
        return;
    }

    D(bug("[ThemePreview] %s: loading '%s'\n", __func__, data->tp_Theme));

    data->tp_DecorTheme = DTLoadThemeSize(data->tp_Theme, data->tp_Size);
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

    dts = data->tp_ScreenTheme;
    dc = data->tp_DecorTheme->dt_Config;

    /* The scene is drawn at the scale of the theme: a theme shown in a
       size larger than the standard one gets a preview that much larger. */
    pw = PREVIEW_WIDTH;
    ph = PREVIEW_HEIGHT;
    if (dc->ImageSize > DECORTHEME_DEFSIZE)
    {
        pw = (PREVIEW_WIDTH * dc->ImageSize) / DECORTHEME_DEFSIZE;
        ph = (PREVIEW_HEIGHT * dc->ImageSize) / DECORTHEME_DEFSIZE;
    }
    if (data->tp_Buffer && ((pw != data->tp_Width) || (ph != data->tp_Height)))
        ThemePreview__DisposeBuffer(data);

    if (!data->tp_Buffer)
    {
        if ((data->tp_BM = AllocBitMap(pw, ph, 1, BMF_CLEAR,
                                       data->tp_Screen->RastPort.BitMap)) == NULL)
            return;
        if ((data->tp_Buffer = CreateRastPort()) == NULL)
        {
            ThemePreview__DisposeBuffer(data);
            return;
        }
        data->tp_Buffer->BitMap = data->tp_BM;
        data->tp_Width = pw;
        data->tp_Height = ph;
    }

    els = dts->dts_Elements;
    di = dts->dts_Images;
    rp = data->tp_Buffer;

    gcols = dc->GadgetsThreeState ? 3 : 4;
    font = data->tp_Screen->RastPort.Font;
    SetFont(rp, font);

    /* The sizes are the ones the theme defines, as the decoration uses
       them: the bars have the heights and the borders the widths given
       in its configuration, and gadgets the size of their images. */
    sbarh = dc->SBarHeight;
    if (sbarh < 1) sbarh = font->tf_YSize + 2;
    barh = dc->BarHeight;
    if (barh < 1) barh = font->tf_YSize + 2;

    /* Desktop background */
    ThemePreview__FillRect(rp, 0, 0, pw - 1, ph - 1, 0x00AAAAAA);

    /* == Screen bar == */
    if (di->img_sdepth->ok)
        sdepthw = di->img_sdepth->w >> 1;
    if (di->img_stitlebar->ok)
    {
        LONG gadx = pw - sdepthw - dc->SBarGadPost_s - dc->SBarGadPre_s;

        /* the bar ends in a section for the gadgets if the theme has one */
        if ((dc->SBarGadPre_s > 0) && (gadx > 0))
        {
            DRenderElement(&els[DECOR_ELEM_ScrBarFill], rp, 0, 0, 0, gadx, sbarh, 0);
            x = DRenderElement(&els[DECOR_ELEM_ScrBarGadPre], rp, 0, gadx, 0, dc->SBarGadPre_s, sbarh, 0);
            if (sdepthw > 0)
                DRenderElement(&els[DECOR_ELEM_ScrBarGadFill], rp, 0,
                    gadx + dc->SBarGadPre_s, 0, sdepthw, sbarh, 0);
            if (dc->SBarGadPost_s > 0)
                DRenderElement(&els[DECOR_ELEM_ScrBarGadPost], rp, 0,
                    pw - dc->SBarGadPost_s, 0, dc->SBarGadPost_s, sbarh, 0);
        }
        else
            DRenderElement(&els[DECOR_ELEM_ScrBarFill], rp, 0, 0, 0, pw, sbarh, 0);
    }
    if (di->img_sbarlogo->ok)
        DRenderElement(&els[DECOR_ELEM_ScrBarLogo], rp, 0, dc->SLogoOffset,
            (sbarh - di->img_sbarlogo->h) / 2, di->img_sbarlogo->w, di->img_sbarlogo->h, 0);
    if (sdepthw > 0)
        DRenderElement(&els[DECOR_ELEM_ScrDepth], rp, IDS_NORMAL,
            pw - sdepthw - dc->SBarGadPost_s,
            (sbarh - di->img_sdepth->h) >> 1, -1, -1, 0);

    /* Screen title */
    ThemePreview__Text(rp, font, "AROS", dc->STitleOffset, 0, sbarh,
        dc->STitleColorText, dc->STitleColorShadow,
        truecolor && dc->STitleOutline, truecolor && dc->STitleShadow);

    /* == Mock window ==
       An active window with close, zoom and depth gadgets, and with a
       scroller, arrows and the size gadget in its right border. */
    bl = dc->LeftBorder;
    bb = dc->BottomBorder;
    br = (dc->RightBorderGadgets > 0) ? dc->RightBorderGadgets : dc->RightBorder;

    wx0 = pw / 18;
    wy0 = sbarh + (ph / 18);
    ww = pw - (2 * wx0);
    wh = ph - wy0 - (ph / 18);
    if (wh < barh + bb + 8)
        wh = barh + bb + 8;
    wx1 = wx0 + ww;
    wy1 = wy0 + wh;

    /* Borders (and the title bar area, when the theme fills it) */
    if (di->img_border_normal->ok && !dc->UseGradients)
    {
        DHorizVertRepeatDecorImage(di->img_border_normal, 0x00cccccc, 0, 0, rp, wx0, wy0, ww, wh);
    }
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
    if ((wx1 - br > wx0 + bl) && (wy1 - bb > wy0 + barh))
        ThemePreview__FillRect(rp, wx0 + bl, wy0 + barh, wx1 - 1 - br, wy1 - 1 - bb, 0x00F4F4F4);

    /* Titlebar - the theme's bar sections in the order the decoration
       lays them out. Subimage row 0 selects the active state */
    if (di->img_winbar_normal->ok)
    {
        LONG closew = di->img_close->ok ? di->img_close->w / gcols : 0;
        LONG zoomw = di->img_zoom->ok ? di->img_zoom->w / gcols : 0;
        LONG depthw = di->img_depth->ok ? di->img_depth->w / gcols : 0;
        LONG lgadw = dc->CloseGadgetOnRight ? 0 : closew;
        LONG rgadw = zoomw + depthw + (dc->CloseGadgetOnRight ? closew : 0);
        LONG xr0 = wx1 - dc->BarPostGadget_s - rgadw;
        LONG textw = TextLength(rp, "Preview", 7);
        LONG gx;

        x = wx0;
        if (lgadw > 0)
        {
            x = DRenderElement(&els[DECOR_ELEM_WinBarPreGadget], rp, 0, x, wy0, dc->BarPreGadget_s, barh, wx1);
            gx = x;
            x = DRenderElement(&els[DECOR_ELEM_WinBarLGadgetFill], rp, 0, x, wy0, lgadw, barh, wx1);
            DRenderElement(&els[DECOR_ELEM_WinClose], rp, IDS_NORMAL,
                gx, wy0 + ((barh - di->img_close->h) / 2), -1, -1, 0);
        }
        else
            x = DRenderElement(&els[DECOR_ELEM_WinBarPre], rp, 0, x, wy0, dc->BarPreGadget_s, barh, wx1);

        x = DRenderElement(&els[DECOR_ELEM_WinBarJoinGB], rp, 0, x, wy0, dc->BarJoinGB_s, barh, wx1);
        x = DRenderElement(&els[DECOR_ELEM_WinBarJoinBT], rp, 0, x, wy0, dc->BarJoinBT_s, barh, wx1);
        tx = x;
        if (x + textw + dc->BarJoinTB_s + dc->BarJoinBG_s > xr0)
            textw = xr0 - x - dc->BarJoinTB_s - dc->BarJoinBG_s;
        if (textw > 0)
            x = DRenderElement(&els[DECOR_ELEM_WinBarTitleFill], rp, 0, x, wy0, textw, barh, wx1);
        x = DRenderElement(&els[DECOR_ELEM_WinBarJoinTB], rp, 0, x, wy0, dc->BarJoinTB_s, barh, wx1);

        /* the part between the title and the gadgets on the right */
        if (xr0 - x - dc->BarJoinBG_s > 0)
            x = DRenderElement(&els[DECOR_ELEM_WinBarRFill], rp, 0, x, wy0, xr0 - x - dc->BarJoinBG_s, barh, wx1);
        x = DRenderElement(&els[DECOR_ELEM_WinBarJoinBG], rp, 0, x, wy0, dc->BarJoinBG_s, barh, wx1);
        gx = x;
        if (rgadw > 0)
        {
            x = DRenderElement(&els[DECOR_ELEM_WinBarRGadgetFill], rp, 0, x, wy0, rgadw, barh, wx1);
            DRenderElement(&els[DECOR_ELEM_WinBarPostGadget], rp, 0, x, wy0, dc->BarPostGadget_s, barh, wx1);
        }
        else
            DRenderElement(&els[DECOR_ELEM_WinBarPost], rp, 0, x, wy0, dc->BarPost_s, barh, wx1);

        /* zoom, close (for themes having it on the right) and depth */
        if (zoomw)
        {
            DRenderElement(&els[DECOR_ELEM_WinZoom], rp, IDS_NORMAL,
                gx, wy0 + ((barh - di->img_zoom->h) / 2), -1, -1, 0);
            gx += zoomw;
        }
        if (dc->CloseGadgetOnRight && closew)
        {
            DRenderElement(&els[DECOR_ELEM_WinClose], rp, IDS_NORMAL,
                gx, wy0 + ((barh - di->img_close->h) / 2), -1, -1, 0);
            gx += closew;
        }
        if (depthw)
            DRenderElement(&els[DECOR_ELEM_WinDepth], rp, IDS_NORMAL,
                gx, wy0 + ((barh - di->img_depth->h) / 2), -1, -1, 0);

        /* Window title */
        if (textw > 0)
            ThemePreview__Text(rp, font, "Preview", tx, wy0, barh,
                dc->TitleColorText, dc->TitleColorShadow,
                truecolor && dc->TitleOutline, truecolor && dc->TitleShadow);
    }

    /* Right border: scroller, arrows and size gadget, each of the size
       of its image and centred in the border */
    if (br > 0)
    {
        LONG bx = wx1 - br;
        LONG y1 = wy1;
        LONG y0 = wy0 + barh;

        if (di->img_size->ok)
        {
            LONG w = di->img_size->w / gcols, h = di->img_size->h;

            y1 -= (dc->BottomBorderGadgets > h) ? dc->BottomBorderGadgets : h;
            DDrawStatefulGadgetImageToRP(rp, di->img_size, IDS_NORMAL,
                bx + ((br - w) / 2), y1 + (((wy1 - y1) - h) / 2));
        }
        if (di->img_down->ok)
        {
            LONG w = di->img_down->w / gcols, h = di->img_down->h + dc->UpDownAddY;

            y1 -= h;
            DDrawStatefulGadgetImageToRP(rp, di->img_down, IDS_NORMAL,
                bx + ((br - w) / 2), y1 + (dc->UpDownAddY / 2));
        }
        if (di->img_up->ok)
        {
            LONG w = di->img_up->w / gcols, h = di->img_up->h + dc->UpDownAddY;

            y1 -= h;
            DDrawStatefulGadgetImageToRP(rp, di->img_up, IDS_NORMAL,
                bx + ((br - w) / 2), y1 + (dc->UpDownAddY / 2));
        }

        /* in a window too low for all of it the scroller is left out */
        if (di->img_verticalcontainer->ok && (y1 - y0 > dc->ContainerTop_s + dc->ContainerBottom_s))
        {
            LONG contw = di->img_verticalcontainer->w >> 1;
            LONG vx = bx + ((br - contw) / 2);
            LONG mid = (y1 - y0) - dc->ContainerTop_s - dc->ContainerBottom_s;

            y = y0;
            y = DRenderElement(&els[DECOR_ELEM_VContainerTop], rp, 0, vx, y, 0, dc->ContainerTop_s, 0);
            if (mid > 0)
                y = DRenderElement(&els[DECOR_ELEM_VContainerTile], rp, 0, vx, y, 0, mid, 0);
            DRenderElement(&els[DECOR_ELEM_VContainerBottom], rp, 0, vx, y, 0, dc->ContainerBottom_s, 0);

            if (di->img_verticalknob->ok)
            {
                LONG knobw = di->img_verticalknob->w / 3;
                LONG kx = vx + ((contw - knobw) / 2);
                LONG kh = ((y1 - y0) * 2) / 5;
                LONG ktile = kh - dc->KnobTop_s - dc->KnobBottom_s - dc->KnobVertGripper_s;

                y = y0 + 2;
                y = DRenderElement(&els[DECOR_ELEM_VKnobTop], rp, 0, kx, y, 0, dc->KnobTop_s, 0);
                if (ktile > 1)
                    y = DRenderElement(&els[DECOR_ELEM_VKnobTileTop], rp, 0, kx, y, 0, ktile / 2, 0);
                if (dc->KnobVertGripper_s > 0)
                    y = DRenderElement(&els[DECOR_ELEM_VKnobGripper], rp, 0, kx, y, 0, dc->KnobVertGripper_s, 0);
                if (ktile > 1)
                    y = DRenderElement(&els[DECOR_ELEM_VKnobTileBottom], rp, 0, kx, y, 0, ktile - (ktile / 2), 0);
                DRenderElement(&els[DECOR_ELEM_VKnobBottom], rp, 0, kx, y, 0, dc->KnobBottom_s, 0);
            }
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

        ThemePreview_OpenLibs();

        data->tp_Size = GetTagData(MUIA_ThemePreview_Size, 0, message->ops_AttrList);

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

    ThemePreview_CloseLibs();

    return DoSuperMethodA(CLASS, self, message);
}

IPTR ThemePreview__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tags, *tag;
    BOOL changed = FALSE;

    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_ThemePreview_Size:
                if (data->tp_Size != (ULONG)tag->ti_Data)
                {
                    data->tp_Size = (ULONG)tag->ti_Data;
                    changed = TRUE;
                }
                break;

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
                changed = TRUE;
                break;
            }
        }
    }

    if (changed && data->tp_Screen)
    {
        LONG width = data->tp_Width, height = data->tp_Height;

        ThemePreview__RenderPreview(data);

        if ((data->tp_Width != width) || (data->tp_Height != height))
        {
            /* the preview needs another amount of room */
            Object *parent = NULL;

            GET(self, MUIA_Parent, &parent);
            if (parent && DoMethod(parent, MUIM_Group_InitChange))
                DoMethod(parent, MUIM_Group_ExitChange);
        }
        MUI_Redraw(self, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(CLASS, self, message);
}

IPTR ThemePreview__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_INST_DATA;

    switch (message->opg_AttrID)
    {
        case MUIA_ThemePreview_Size:
            *message->opg_Storage = (IPTR)data->tp_Size;
            return TRUE;

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
    SETUP_INST_DATA;
    ULONG       rc = DoSuperMethodA(CLASS, self, (Msg) message);
    LONG        pw = data->tp_Width ? data->tp_Width : PREVIEW_WIDTH;
    LONG        ph = data->tp_Height ? data->tp_Height : PREVIEW_HEIGHT;

    D(bug("[ThemePreview] %s()\n", __PRETTY_FUNCTION__));

    message->MinMaxInfo->MinWidth  += pw;
    message->MinMaxInfo->MinHeight += ph;

    message->MinMaxInfo->DefWidth  += pw;
    message->MinMaxInfo->DefHeight += ph;

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
        LONG px = _mleft(self) + (((_mright(self) - _mleft(self) + 1) - data->tp_Width) / 2);
        LONG py = _mtop(self) + (((_mbottom(self) - _mtop(self) + 1) - data->tp_Height) / 2);

        if (px < _mleft(self)) px = _mleft(self);
        if (py < _mtop(self)) py = _mtop(self);

        clip = MUI_AddClipping(muiRenderInfo(self), _mleft(self), _mtop(self), _mright(self) - _mleft(self) + 1, _mbottom(self) - _mtop(self) + 1);
        BltBitMapRastPort(data->tp_Buffer->BitMap,
                  0, 0,
                  _rp(self),
                  px, py,
                  data->tp_Width, data->tp_Height,
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
