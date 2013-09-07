/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <proto/alib.h>

#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif

#include <stdlib.h>

#include "locale.h"
#include "themepreview.h"

#if (0)
static    CONST_STRPTR PREVIEWFILE_BASE = "THEME:preview";
static    CONST_STRPTR PREVIEWFILE_WIN = "THEME:preview.win";
static    CONST_STRPTR PREVIEWFILE_ZUNE = "THEME:preview.zune";
static    CONST_STRPTR PREVIEWFILE_WAND = "THEME:preview.wand";
#endif

#define PREVIEW_WIDTH  256
#define PREVIEW_HEIGHT 150

/*** Instance Data **********************************************************/

struct ThemePreview_DATA
{
    char *tp_Theme;
    struct RastPort *tp_Buffer;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct ThemePreview_DATA *data = INST_DATA(CLASS, self)

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

#if (0)
    if (self)
    {
        SETUP_INST_DATA;
    }
#endif

    return self;
}

IPTR ThemePreview__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
#if (0)
    SETUP_INST_DATA;
#endif

    D(bug("[ThemePreview] %s()\n", __PRETTY_FUNCTION__));

    return DoSuperMethodA(CLASS, self, message);
}

IPTR ThemePreview__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
#if (0)
    SETUP_INST_DATA;
#endif
    struct TagItem *tags, *tag;

    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_ThemePreview_Theme:
            {
                D(bug("[ThemePreview] %s: Theme '%s'\n", __PRETTY_FUNCTION__, tag->ti_Data));
                // TODO: Composite preview if available..
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
        // Render Preview..
        clip = MUI_AddClipping(muiRenderInfo(self), _mleft(self), _mtop(self), _mright(self) - _mleft(self) + 1, _mbottom(self) - _mtop(self) + 1);
        BltBitMapRastPort(data->tp_Buffer->BitMap,
                  _mleft(self), _mtop(self),
                  _rp(self),
                  0, 0,
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
ZUNE_CUSTOMCLASS_6
(
    ThemePreview, NULL, MUIC_Area, NULL,
    MUIM_Draw,      struct MUIP_Draw *,
    MUIM_AskMinMax, struct MUIP_AskMinMax *,
    OM_GET,         struct opGet *,
    OM_SET,         struct opSet *,
    OM_NEW,         struct opSet *,
    OM_DISPOSE,     Msg
);

