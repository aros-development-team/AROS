/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Panel class implementation
*/

#include "clib/exec_protos.h"
#include "clib/intuition_protos.h"
#include "clib/muimaster_protos.h"
#include "clib/utility_protos.h"
#include "frame.h"
#include "graphics/rastport.h"
#include "inline/graphics.h"
#include "intuition/classes.h"
#include "intuition/classusr.h"
#include "intuition/intuition.h"
#include "libraries/mui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "muimaster_intern.h"
#include "imspec_intern.h"
#include "imspec.h"
#include "classes/area.h"
#include "classes/window.h"
#include "panel.h"
#include "panel_private.h"

#include "panelgroup.h"
#include "prefs.h"
#include "support.h"
#include "textengine.h"
#include "utility/tagitem.h"

#define DEBUG 0
#include <aros/debug.h>

#ifndef MADF_SETUP
#define MADF_SETUP             (1<< 28) /* PRIV - zune-specific */
#endif

/******************************************************************
 * DragHandle Methods
 *****************************************************************/

/*** Instance data **********************************************************/
struct DragHandle_DATA {
    BOOL vertical; /* If handle is positioned to the left, draw lines vertically */
};

/*** DragHandle Methods ****************************************************************/
IPTR DragHandle__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct DragHandle_DATA *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperNewTags(cl,
                                    obj,
                                    NULL,
                                    MUIA_Draggable,
                                    TRUE,
                                    MUIA_InputMode,
                                    MUIV_InputMode_RelVerify,
                                    TAG_MORE,
                                    (IPTR) msg->ops_AttrList,
                                    TAG_DONE);

    if (!obj)
        return 0;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_DragHandle_Vertical: data->vertical = (IPTR) tag->ti_Data; break;
        }
    }

    return (IPTR) obj;
}

IPTR DragHandle__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct DragHandle_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_DragHandle_Vertical: data->vertical = tag->ti_Data; break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR DragHandle__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct DragHandle_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID) {
    case MUIA_DragHandle_Vertical: *msg->opg_Storage = (IPTR) data->vertical; return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR DragHandle__MUIM_CreateDragImage(struct IClass *cl, Object *obj, struct MUIP_CreateDragImage *msg)
{
    struct DragHandle_DATA *data = INST_DATA(cl, obj);

    Object *parent = _parent(obj);

    if (parent) {
        /* Create drag image of the parent object, not the handle */
        return DoMethodA(parent, msg);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR DragHandle__MUIM_DeleteDragImage(struct IClass *cl, Object *obj, struct MUIP_DeleteDragImage *msg)
{
    struct DragHandle_DATA *data = INST_DATA(cl, obj);

    Object *parent = _parent(obj);

    if (parent) {
        /* Delete drag image of the parent object, not the handle */
        return DoMethodA(parent, msg);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR DragHandle__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg) msg);

    struct DragHandle_DATA *data = INST_DATA(cl, obj);
    struct MUI_MinMax *mi = msg->MinMaxInfo;

    /* Set minimum dimensions for the drag handle */
    if (data->vertical) {
        mi->MinWidth = 7; /* Minimum width for vertical lines */
        mi->MinHeight = 50; /* Minimum height to be usable */

        mi->DefWidth = 7;
        mi->DefHeight = 100;

        /* Maximum dimensions - allow stretching vertically but limit horizontal */
        if (mi->MaxHeight < MUI_MAXMAX)
            mi->MaxHeight = MUI_MAXMAX;
        if (mi->MaxWidth < 5)
            mi->MaxWidth = 7;
    } else {
        mi->MinWidth = 50; /* Minimum width to be usable */
        mi->MinHeight = 7; /* Minimum height for horizontal lines */

        mi->DefWidth = 100;
        mi->DefHeight = 7;

        /* Maximum dimensions - allow stretching horizontally but limit vertical */
        if (mi->MaxWidth < MUI_MAXMAX)
            mi->MaxWidth = MUI_MAXMAX;
        if (mi->MaxHeight < 5)
            mi->MaxHeight = 7;
    }

    return 0;
}

IPTR DragHandle__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct DragHandle_DATA *data = INST_DATA(cl, obj);

    IPTR result = DoSuperMethodA(cl, obj, (Msg) msg);

    /* Draw two parallel lines in the middle */
    struct RastPort *rp = _rp(obj);
    ULONG old_pen = GetAPen(rp);

    /* Set pen to halfshadow color for the lines */
    SetAPen(rp, MPEN_HALFSHADOW);
    SetDrMd(rp, JAM1);

    if (data->vertical) {
        /* Calculate positions for two parallel vertical lines */
        WORD center_x = _left(obj) + (_width(obj) / 2);
        WORD line_top = _top(obj);
        WORD line_bottom = _top(obj) + _height(obj);

        /* Space the lines evenly around the center */
        WORD line_spacing = 2;
        WORD line1_x = center_x - line_spacing;
        WORD line2_x = center_x + line_spacing;

        /* Draw first vertical line */
        Move(rp, line1_x, line_top);
        Draw(rp, line1_x, line_bottom);

        /* Draw second vertical line */
        Move(rp, line2_x, line_top);
        Draw(rp, line2_x, line_bottom);
    } else {
        /* Calculate positions for two parallel horizontal lines */
        WORD line_left = _left(obj);
        WORD line_right = _left(obj) + _width(obj);
        WORD center_y = _top(obj) + (_height(obj) / 2);

        /* Space the lines evenly around the center */
        WORD line_spacing = 2;
        WORD line1_y = center_y - line_spacing;
        WORD line2_y = center_y + line_spacing;

        /* Draw first horizontal line */
        Move(rp, line_left, line1_y);
        Draw(rp, line_right, line1_y);

        /* Draw second horizontal line */
        Move(rp, line_left, line2_y);
        Draw(rp, line_right, line2_y);
    }

    /* Restore original pen */
    SetAPen(rp, old_pen);

    return result;
}

IPTR DragHandle__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*** Class initialization ***************************************************/

#if ZUNE_BUILTIN_DRAGHANDLE
BOOPSI_DISPATCHER(IPTR, DragHandle_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID) {
    case OM_NEW: return DragHandle__OM_NEW(cl, obj, (struct opSet *) msg);
    case OM_SET: return DragHandle__OM_SET(cl, obj, (struct opSet *) msg);
    case OM_GET: return DragHandle__OM_GET(cl, obj, (struct opGet *) msg);
    case MUIM_Setup: return DragHandle__MUIM_Setup(cl, obj, (struct MUIP_Setup *) msg);
    case MUIM_Draw: return DragHandle__MUIM_Draw(cl, obj, (struct MUIP_Draw *) msg);
    case MUIM_AskMinMax: return DragHandle__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *) msg);
    case MUIM_CreateDragImage: return DragHandle__MUIM_CreateDragImage(cl, obj, (struct MUIP_CreateDragImage *) msg);
    case MUIM_DeleteDragImage: return DragHandle__MUIM_DeleteDragImage(cl, obj, (struct MUIP_DeleteDragImage *) msg);
    default: return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_DragHandle_desc = { MUIC_DragHandle,
                                                        MUIC_Area,
                                                        sizeof(struct DragHandle_DATA),
                                                        (void *) DragHandle_Dispatcher };
#endif /* ZUNE_BUILTIN_DRAGHANDLE */

Object *Panel_CreateDragHandle(struct IClass *cl, Object *panel, ULONG title_position)
{
    struct Panel_DATA *data = INST_DATA(cl, panel);

    Object *drag_handle = MUI_NewObject(MUIC_DragHandle,
                                        MUIA_DragHandle_Vertical,
                                        title_position == MUIV_PanelTitle_Position_Left,
                                        TAG_DONE);

    return drag_handle;
}

/******************************************************************
 * PanelTitle Methods
 *****************************************************************/

static void PanelTitle_CalculateTextSize(struct PanelTitle_DATA *data, Object *obj)
{
    if (!data->text || !obj) {
        data->text_width = data->text_height = 0;
        return;
    }

    if (data->vertical && data->position == MUIV_PanelTitle_Position_Left) {
        /* Vertical text: measure single character and multiply by string length */
        struct RastPort *rp = _rp(obj);
        if (rp) {
            struct TextExtent te;
            TextExtent(rp, "A", 1, &te);
            data->text_width = te.te_Width;
            data->text_height = te.te_Height * strlen(data->text);
        } else {
            data->text_width = 8;
            data->text_height = strlen(data->text) * 16;
        }
    } else {
        /* Horizontal text: use ZText for accurate measurement */
        ZText *ztext = zune_text_new(NULL, data->text, ZTEXT_ARG_NONE, 0);
        if (ztext) {
            if (muiRenderInfo(obj) && (_flags(obj) & MADF_SETUP)) {
                zune_text_get_bounds(ztext, obj);
                data->text_width = ztext->width;
                data->text_height = ztext->height;
            }
            zune_text_destroy(ztext);
        } else {
            data->text_width = strlen(data->text) * 8;
            data->text_height = 16;
        }
    }
}

static void PanelTitle_DrawArrow(Object *obj, struct PanelTitle_DATA *data, LONG left, LONG top, UWORD direction)
{
    struct RastPort *rp = _rp(obj);
    if (!rp || !data->collapsible)
        return;

    LONG cx = left;
    LONG cy = top - 2;

    struct MUI_RenderInfo *mri = muiRenderInfo(obj);

    switch (direction) {
    case PANELTITLE_ARROW_DOWN: {
        if (data->down_arrow_spec) {
            zune_imspec_draw(data->down_arrow_spec,
                             mri,
                             cx,
                             cy,
                             PANELTITLE_ARROW_WIDTH,
                             PANELTITLE_ARROW_HEIGHT,
                             0,
                             0,
                             0);
        }
        break;
    }
    case PANELTITLE_ARROW_UP: {
        if (data->up_arrow_spec) {
            zune_imspec_draw(data->up_arrow_spec,
                             mri,
                             cx,
                             cy,
                             PANELTITLE_ARROW_WIDTH,
                             PANELTITLE_ARROW_HEIGHT,
                             0,
                             0,
                             0);
        }
        break;
    }

    case PANELTITLE_ARROW_RIGHT: {
        if (data->right_arrow_spec) {
            zune_imspec_draw(data->right_arrow_spec,
                             mri,
                             cx,
                             cy,
                             PANELTITLE_ARROW_WIDTH,
                             PANELTITLE_ARROW_HEIGHT,
                             0,
                             0,
                             0);
        }
        break;
    }
    }
}

static void PanelTitle_DrawSeparator(Object *obj, LONG left, LONG bottom, LONG width, LONG top, BOOL horizontal)
{
    struct RastPort *rp = _rp(obj);
    if (!rp)
        return;

    SetAPen(rp, _pens(obj)[MPEN_HALFSHADOW]);
    SetDrMd(rp, JAM1);

    if (horizontal) {
        Move(rp, left, bottom);
        Draw(rp, left + width, bottom);
    } else {
        Move(rp, left + width, bottom);
        Draw(rp, left + width, top);
    }
}

static void PanelTitle_FreeVectorSpecs(struct IClass *cl, Object *obj)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);

    if (data->right_arrow_spec)
        zune_imspec_cleanup(data->right_arrow_spec);
    if (data->up_arrow_spec)
        zune_imspec_cleanup(data->up_arrow_spec);
    if (data->down_arrow_spec)
        zune_imspec_cleanup(data->down_arrow_spec);
}

static void PanelTitle_AllocateVectorSpecs(struct IClass *cl, Object *obj)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);
    struct MUI_RenderInfo *mri = muiRenderInfo(obj);

    if (data->position == MUIV_PanelTitle_Position_Left) {
        data->right_arrow_spec = zune_imspec_setup((IPTR) "1:3", mri);
        data->up_arrow_spec = zune_imspec_setup((IPTR) "1:0", mri);
    } else if (data->position == MUIV_PanelTitle_Position_Top) {
        data->right_arrow_spec = zune_imspec_setup((IPTR) "1:3", mri);
        data->down_arrow_spec = zune_imspec_setup((IPTR) "1:1", mri);
    }
}

/*** Methods ****************************************************************/

IPTR PanelTitle__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct PanelTitle_DATA *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return 0;

    data = INST_DATA(cl, obj);

    /* Initialize defaults */
    data->text = NULL;
    data->position = MUIV_PanelTitle_Position_Top;
    data->text_position = MUIV_PanelTitle_TextPosition_Centered;
    data->vertical = FALSE;
    data->collapsible = FALSE;
    data->collapsed = FALSE;
    data->show_separator = FALSE;
    data->click_hook = NULL;
    data->layout_valid = FALSE;
    data->text_width = data->text_height = 0;
    data->draw_state_indicator = FALSE;

    /* Parse attributes */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_PanelTitle_Text: data->text = StrDup((STRPTR) tag->ti_Data); break;
        case MUIA_PanelTitle_Position: data->position = tag->ti_Data; break;
        case MUIA_PanelTitle_TextPosition: data->text_position = tag->ti_Data; break;
        case MUIA_PanelTitle_Vertical: data->vertical = tag->ti_Data; break;
        case MUIA_PanelTitle_Collapsible: data->collapsible = tag->ti_Data; break;
        case MUIA_PanelTitle_Collapsed: data->collapsed = tag->ti_Data; break;
        case MUIA_PanelTitle_ShowSeparator: data->show_separator = tag->ti_Data; break;
        case MUIA_PanelTitle_ClickHook: data->click_hook = (struct Hook *) tag->ti_Data; break;
        case MUIA_PanelTitle_DrawStateIndicator: data->draw_state_indicator = tag->ti_Data; break;
        }
    }

    return (IPTR) obj;
}

IPTR PanelTitle__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);

    if (data->text) {
        FreeVec(data->text);
        data->text = NULL;
    }

    PanelTitle_FreeVectorSpecs(cl, obj);

    return DoSuperMethodA(cl, obj, msg);
}

IPTR PanelTitle__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;
    BOOL redraw = FALSE;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_PanelTitle_Text:
            if (data->text)
                FreeVec(data->text);
            data->text = StrDup((STRPTR) tag->ti_Data);
            data->layout_valid = FALSE;
            redraw = TRUE;
            break;

        case MUIA_PanelTitle_Position:
            if (data->position != tag->ti_Data) {
                PanelTitle_FreeVectorSpecs(cl, obj);
                PanelTitle_AllocateVectorSpecs(cl, obj);
                data->position = tag->ti_Data;
                data->layout_valid = FALSE;
                redraw = TRUE;
            }
            break;

        case MUIA_PanelTitle_TextPosition:
            if (data->text_position != tag->ti_Data) {
                data->text_position = tag->ti_Data;
                redraw = TRUE;
            }
            break;

        case MUIA_PanelTitle_Vertical:
            if (data->vertical != (BOOL) tag->ti_Data) {
                data->vertical = tag->ti_Data;
                data->layout_valid = FALSE;
                redraw = TRUE;
            }
            break;

        case MUIA_PanelTitle_Collapsible:
            if (data->collapsible != (BOOL) tag->ti_Data) {
                data->collapsible = tag->ti_Data;
                data->layout_valid = FALSE;
                redraw = TRUE;
            }
            break;

        case MUIA_PanelTitle_Collapsed:
            if (data->collapsed != (BOOL) tag->ti_Data) {
                data->collapsed = tag->ti_Data;
                data->layout_valid = FALSE;
                redraw = TRUE;
            }
            break;

        case MUIA_PanelTitle_ShowSeparator:
            if (data->show_separator != (BOOL) tag->ti_Data) {
                data->show_separator = tag->ti_Data;
                redraw = TRUE;
            }
            break;

        case MUIA_PanelTitle_ClickHook: data->click_hook = (struct Hook *) tag->ti_Data; break;
        }
    }

    if (redraw && (_flags(obj) & MADF_SETUP)) {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR PanelTitle__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID) {
    case MUIA_PanelTitle_Text: *msg->opg_Storage = (IPTR) data->text; return TRUE;

    case MUIA_PanelTitle_Position: *msg->opg_Storage = data->position; return TRUE;

    case MUIA_PanelTitle_TextPosition: *msg->opg_Storage = data->text_position; return TRUE;

    case MUIA_PanelTitle_Vertical: *msg->opg_Storage = data->vertical; return TRUE;

    case MUIA_PanelTitle_Collapsible: *msg->opg_Storage = data->collapsible; return TRUE;

    case MUIA_PanelTitle_Collapsed: *msg->opg_Storage = data->collapsed; return TRUE;

    case MUIA_PanelTitle_ShowSeparator: *msg->opg_Storage = data->show_separator; return TRUE;

    case MUIA_PanelTitle_ClickHook: *msg->opg_Storage = (IPTR) data->click_hook; return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR PanelTitle__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    /* Set up event handler for mouse clicks */
    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->ehn);

    /* Calculate initial text size */
    PanelTitle_CalculateTextSize(data, obj);
    data->layout_valid = FALSE;

    if (data->collapsible) {
        PanelTitle_AllocateVectorSpecs(cl, obj);
    }

    return TRUE;
}

IPTR PanelTitle__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR PanelTitle__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);
    IPTR result;

    result = DoSuperMethodA(cl, obj, (Msg) msg);

    if (!data->text) {
        msg->MinMaxInfo->MinWidth = msg->MinMaxInfo->DefWidth = 0;
        msg->MinMaxInfo->MinHeight = msg->MinMaxInfo->DefHeight = 0;
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
        msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
        return result;
    }

    /* Calculate text size if not already done */
    PanelTitle_CalculateTextSize(data, obj);

    UWORD total_width = data->text_width + (PANELTITLE_TEXT_PADDING * 2);
    UWORD total_height = data->text_height + (PANELTITLE_TEXT_PADDING * 2);

    /* Add arrow space if collapsible */
    if (data->collapsible) {
        switch (data->position) {
        case MUIV_PanelTitle_Position_Top: total_width += PANELTITLE_ARROW_WIDTH + PANELTITLE_ARROW_MARGIN; break;
        case MUIV_PanelTitle_Position_Left: total_height += PANELTITLE_ARROW_HEIGHT + PANELTITLE_ARROW_MARGIN; break;
        }
    }

    switch (data->position) {
    case MUIV_PanelTitle_Position_Top:
        msg->MinMaxInfo->MinWidth = msg->MinMaxInfo->DefWidth = total_width;
        msg->MinMaxInfo->MinHeight = msg->MinMaxInfo->DefHeight = total_height + (data->show_separator ? 1 : 0);
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
        msg->MinMaxInfo->MaxHeight = total_height + (data->show_separator ? 1 : 0);
        break;

    case MUIV_PanelTitle_Position_Left:
        msg->MinMaxInfo->MinWidth = msg->MinMaxInfo->DefWidth = total_width + (data->show_separator ? 1 : 0);
        msg->MinMaxInfo->MinHeight = msg->MinMaxInfo->DefHeight = total_height;
        msg->MinMaxInfo->MaxWidth = total_width + (data->show_separator ? 1 : 0);
        msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
        break;
    }

    return result;
}

IPTR PanelTitle__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);
    struct RastPort *rp;
    IPTR result;

    result = DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & MADF_DRAWOBJECT) || !data->text)
        return result;

    rp = _rp(obj);
    if (!rp)
        return result;

    /* Recalculate layout if needed */
    // if (!data->layout_valid)
    // {
    PanelTitle_CalculateTextSize(data, obj);

    /* Calculate arrow position if collapsible */
    if (data->collapsible) {
        switch (data->position) {
        case MUIV_PanelTitle_Position_Top:
            data->arrow_left = _left(obj) + PANELTITLE_TEXT_PADDING;
            data->arrow_top = _top(obj) + (_height(obj) - PANELTITLE_ARROW_HEIGHT) / 2;
            break;

        case MUIV_PanelTitle_Position_Left:
            data->arrow_left = _left(obj) + (_width(obj) - PANELTITLE_ARROW_WIDTH) / 2;
            data->arrow_top = _bottom(obj) - PANELTITLE_ARROW_HEIGHT - PANELTITLE_TEXT_PADDING;
            break;
        }
    }

    data->layout_valid = TRUE;
    // }

    /* Draw arrow first if collapsible */
    if (data->collapsible) {
        UWORD arrow_direction;

        switch (data->position) {
        case MUIV_PanelTitle_Position_Top:
            arrow_direction = data->collapsed ? PANELTITLE_ARROW_RIGHT : PANELTITLE_ARROW_DOWN;
            break;
        case MUIV_PanelTitle_Position_Left:
            arrow_direction = data->collapsed ? PANELTITLE_ARROW_UP : PANELTITLE_ARROW_RIGHT;
            break;
        default: arrow_direction = PANELTITLE_ARROW_DOWN; break;
        }

        if (data->draw_state_indicator) {
           PanelTitle_DrawArrow(obj, data, data->arrow_left, data->arrow_top, arrow_direction);
        }
    }

    /* Set up text rendering */
    SetAPen(rp, _pens(obj)[MPEN_SHINE]);
    SetDrMd(rp, JAM1);

    /* Draw text */
    if (data->vertical && data->position == MUIV_PanelTitle_Position_Left) {
        /* Render text vertically */
        LONG char_x, char_y;
        WORD char_height, char_width;

        char_height = data->text_height / strlen(data->text);
        char_width = data->text_width;

        /* Calculate starting position based on text position */
        switch (data->text_position) {
        case MUIV_PanelTitle_TextPosition_Left: char_y = _top(obj) + PANELTITLE_TEXT_PADDING; break;
        case MUIV_PanelTitle_TextPosition_Right:
            char_y = _bottom(obj) - (char_height * strlen(data->text)) - PANELTITLE_TEXT_PADDING;
            break;
        case MUIV_PanelTitle_TextPosition_Centered:
        default: {
            LONG available_height = _height(obj) - (PANELTITLE_TEXT_PADDING * 2);
            LONG text_offset = (available_height - data->text_height) / 2;
            char_y = _top(obj) + PANELTITLE_TEXT_PADDING + text_offset;
        } break;
        }

        char_x = _left(obj) + (_width(obj) / 2 - data->text_width / 2);

        /* Draw each character vertically */
        for (LONG i = 0; i < strlen(data->text); i++) {
            char single_char[2] = { data->text[i], '\0' };
            Move(rp, char_x, char_y + char_height);
            Text(rp, single_char, 1);
            char_y += char_height;
        }
    } else {
        /* Render text horizontally */
        if (data->text) {
            LONG text_left, text_right;

            /* Calculate text positioning based on text_position */
            switch (data->text_position) {
            case MUIV_PanelTitle_TextPosition_Left:
                text_left = _left(obj) + PANELTITLE_TEXT_PADDING;
                if (data->collapsible && data->position == MUIV_PanelTitle_Position_Top)
                    text_left += PANELTITLE_ARROW_WIDTH + PANELTITLE_ARROW_MARGIN;
                text_right = text_left + data->text_width;
                break;

            case MUIV_PanelTitle_TextPosition_Right:
                text_right = _right(obj) - PANELTITLE_TEXT_PADDING;
                text_left = text_right - data->text_width;
                break;

            case MUIV_PanelTitle_TextPosition_Centered:
            default: {
                LONG available_width = _width(obj) - (PANELTITLE_TEXT_PADDING * 2);
                LONG arrow_space = 0;
                if (data->collapsible && data->position == MUIV_PanelTitle_Position_Top)
                    arrow_space = PANELTITLE_ARROW_WIDTH + PANELTITLE_ARROW_MARGIN;

                LONG text_offset = (available_width - arrow_space - data->text_width) / 2;
                text_left = _left(obj) + PANELTITLE_TEXT_PADDING + arrow_space + text_offset;
                text_right = text_left + data->text_width;
            } break;
            }

            /* Create ZText object for horizontal text drawing */
            ZText *ztext = zune_text_new(NULL, data->text, ZTEXT_ARG_NONE, 0);
            if (ztext) {
                zune_text_draw(ztext, obj, text_left, text_right, _top(obj) + (_height(obj) - data->text_height) / 2);
                zune_text_destroy(ztext);
            }
        }
    }

    /* Draw separator if requested, but not when collapsed */
    if (data->show_separator && !data->collapsed) {
        PanelTitle_DrawSeparator(obj,
                                 _left(obj),
                                 _bottom(obj),
                                 _width(obj),
                                 _top(obj),
                                 data->position == MUIV_PanelTitle_Position_Top);
    }

    return result;
}

IPTR PanelTitle__MUIM_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);

    if (msg->imsg && msg->imsg->Class == IDCMP_MOUSEBUTTONS && msg->imsg->Code == SELECTDOWN) {
        WORD x = msg->imsg->MouseX;
        WORD y = msg->imsg->MouseY;

        if (_isinobject(obj, x, y)) {

            /* Toggle collapsed state if collapsible */
            if (data->collapsible) {
                DoMethod(obj, MUIM_PanelTitle_Toggle);
            }

            /* Call click hook if set */
            if (data->click_hook) {
                CallHookPkt(data->click_hook, obj, NULL);
            }

            return MUI_EventHandlerRC_Eat;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR PanelTitle__MUIM_PanelTitle_Toggle(struct IClass *cl, Object *obj, struct MUIP_PanelTitle_Toggle *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);

    if (data->collapsible) {
        set(_parent(obj), MUIA_Panel_Collapsed, !data->collapsed);
        if (_flags(obj) & MADF_SETUP) {
            MUI_Redraw(obj, MADF_DRAWOBJECT);
        }

        return TRUE;
    }

    return FALSE;
}

/*** Class dispatcher *******************************************************/

#if ZUNE_BUILTIN_PANELTITLE
BOOPSI_DISPATCHER(IPTR, PanelTitle_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID) {
    case OM_NEW: return PanelTitle__OM_NEW(cl, obj, (struct opSet *) msg);
    case OM_DISPOSE: return PanelTitle__OM_DISPOSE(cl, obj, msg);
    case OM_SET: return PanelTitle__OM_SET(cl, obj, (struct opSet *) msg);
    case OM_GET: return PanelTitle__OM_GET(cl, obj, (struct opGet *) msg);
    case MUIM_Setup: return PanelTitle__MUIM_Setup(cl, obj, (struct MUIP_Setup *) msg);
    case MUIM_Cleanup: return PanelTitle__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *) msg);
    case MUIM_AskMinMax: return PanelTitle__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *) msg);
    case MUIM_Draw: return PanelTitle__MUIM_Draw(cl, obj, (struct MUIP_Draw *) msg);
    case MUIM_HandleEvent: return PanelTitle__MUIM_HandleEvent(cl, obj, (struct MUIP_HandleEvent *) msg);
    case MUIM_PanelTitle_Toggle:
        return PanelTitle__MUIM_PanelTitle_Toggle(cl, obj, (struct MUIP_PanelTitle_Toggle *) msg);
    default: return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

/**************************************************************************
 Class descriptor
**************************************************************************/
const struct __MUIBuiltinClass _MUI_PanelTitle_desc = { MUIC_PanelTitle,
                                                        MUIC_Area,
                                                        sizeof(struct PanelTitle_DATA),
                                                        (void *) PanelTitle_Dispatcher };
#endif

/******************************************************************
 *
 * Panel Methods
 *
 *****************************************************************/

IPTR Panel__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Panel_DATA *data;
    struct TagItem *tags, *tag;

    IPTR tag_value = (IPTR) GetTagData(MUIA_Panel_TitlePosition, FALSE, msg->ops_AttrList);

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
                                    MUIA_Group_Horiz, tag_value == MUIV_PanelTitle_Position_Left,
                                    TAG_MORE, (IPTR) msg->ops_AttrList, TAG_DONE);
    if (!obj)
        return 0;

    data = INST_DATA(cl, obj);

    data->title_obj = NULL;
    data->layout_dirty = TRUE;
    data->expanded_width = 0;
    data->expanded_height = 0;
    data->drag_active = FALSE;

    STRPTR title_text = NULL;
    ULONG title_text_position = MUIV_Panel_Title_Text_Centered;
    ULONG title_position = MUIV_Panel_Title_None;
    BOOL title_vertical = FALSE;
    BOOL collapsible = FALSE;
    BOOL collapsed = FALSE;
    BOOL show_separator = TRUE;
    BOOL draw_state_indicator = FALSE;
    struct Hook *title_clicked_hook = NULL;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_Panel_Padding: data->padding = tag->ti_Data; break;
        case MUIA_Panel_Title: title_text = (STRPTR) tag->ti_Data; break;
        case MUIA_Panel_TitleTextPosition: title_text_position = tag->ti_Data; break;
        case MUIA_Panel_TitlePosition: title_position = tag->ti_Data; break;
        case MUIA_Panel_TitleVertical: title_vertical = tag->ti_Data; break;
        case MUIA_Panel_Collapsible: collapsible = tag->ti_Data; break;
        case MUIA_Panel_Collapsed: collapsed = tag->ti_Data; break;
        case MUIA_Panel_DrawSeparator: show_separator = tag->ti_Data; break;
        case MUIA_Panel_DrawStateIndicator: draw_state_indicator = tag->ti_Data; break;
        case MUIA_Panel_TitleClickedHook: title_clicked_hook = (struct Hook *) tag->ti_Data; break;
        case MUIA_Draggable: {
            Object *drag_handle = Panel_CreateDragHandle(cl, obj, title_position);
            if (drag_handle) {
                data->drag_handle = drag_handle;

                DoMethod(obj, OM_ADDMEMBER, data->drag_handle);
            }
        }
        }
    }

    /* Create PanelTitle object if title is specified */
    if (title_text && title_position != MUIV_Panel_Title_None) {
        data->title_obj = MUI_NewObject(MUIC_PanelTitle,
            MUIA_PanelTitle_Text, title_text,
            MUIA_PanelTitle_Position, title_position,
            MUIA_PanelTitle_TextPosition, title_text_position,
            MUIA_PanelTitle_Vertical, title_vertical,
            MUIA_PanelTitle_Collapsible, collapsible,
            MUIA_PanelTitle_Collapsed, collapsed,
            MUIA_PanelTitle_ShowSeparator, show_separator,
            MUIA_PanelTitle_DrawStateIndicator, draw_state_indicator,
            MUIA_PanelTitle_ClickHook, title_clicked_hook, TAG_DONE);

        if (data->title_obj) {
            /* Add title object as child */
            DoMethod(obj, OM_ADDMEMBER, data->title_obj);
        }
    }

    return (IPTR) obj;
}

void Panel__Sort_Children(struct IClass *cl, Object *obj)
{
    struct Panel_DATA *data = INST_DATA(cl, obj);

    /* Reorder children to ensure title and drag handle appear first */
    struct MinList *children = NULL;
    get(obj, MUIA_Group_ChildList, &children);

    /* Create a temporary array to hold the reordered children */
    LONG child_count = 0;
    Object *child;
    APTR cstate = children->mlh_Head;
    while ((child = NextObject((APTR *) &cstate))) {
        child_count++;
    }

    if (child_count > 0) {
        Object **child_array = (Object **) AllocVec(sizeof(Object *) * (child_count + 2), MEMF_CLEAR);
        if (child_array) {
            LONG i = 1;

            /* First pass: Add title and drag handle to the beginning */
            if (data->drag_handle) {
                child_array[i++] = data->drag_handle;
            }
            if (data->title_obj) {
                child_array[i++] = data->title_obj;
            }

            /* Second pass: Add all other children in their original order */
            cstate = children->mlh_Head;
            while ((child = NextObject((APTR *) &cstate))) {
                if (child != data->title_obj && child != data->drag_handle) {
                    child_array[i++] = child;
                }
            }

            child_array[i] = NULL;
            child_array[0] = (Object *) MUIM_Group_Sort;

            /* Apply the new order */
            DoMethodA(obj, child_array);

            FreeVec(child_array);
        }
    }
}

IPTR Panel__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Panel_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;
    BOOL redraw = FALSE;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {

        case MUIA_Panel_Padding:
            if (data->padding != tag->ti_Data) {
                data->padding = tag->ti_Data;
                redraw = TRUE;
            }
            break;

        /* Forward title-related attributes to PanelTitle object */
        case MUIA_Panel_Title:
            if (data->title_obj) {
                set(data->title_obj, MUIA_PanelTitle_Text, tag->ti_Data);
            }
            break;
        case MUIA_Panel_TitlePosition:
            if (data->title_obj) {
                set(data->title_obj, MUIA_PanelTitle_Position, tag->ti_Data);
            }
            break;
        case MUIA_Panel_TitleTextPosition:
            if (data->title_obj) {
                set(data->title_obj, MUIA_PanelTitle_TextPosition, tag->ti_Data);
            }
            break;
        case MUIA_Panel_TitleVertical:
            if (data->title_obj) {
                set(data->title_obj, MUIA_PanelTitle_Vertical, tag->ti_Data);
            }
            break;
        case MUIA_Panel_DrawSeparator:
            if (data->title_obj) {
                set(data->title_obj, MUIA_PanelTitle_ShowSeparator, tag->ti_Data);
            }
            break;

        case MUIA_Panel_Collapsible:
            if (data->title_obj) {
                set(data->title_obj, MUIA_PanelTitle_Collapsible, tag->ti_Data);
            }
            break;

        case MUIA_Panel_Collapsed: {
            BOOL current_collapsed = FALSE;
            if (data->title_obj) {
                get(data->title_obj, MUIA_PanelTitle_Collapsed, &current_collapsed);
            }

            if (current_collapsed != (BOOL)tag->ti_Data) {
                BOOL was_expanded = !current_collapsed;
                BOOL will_be_collapsed = tag->ti_Data;

                /* If transitioning from expanded to collapsed, store current dimensions */
                if (was_expanded && will_be_collapsed) {
                    ULONG current_width = _width(obj);
                    ULONG current_height = _height(obj);
                    if (current_width > 0 && current_height > 0) {
                        /* Only update if we haven't stored valid dimensions yet, or
                         * if current dimensions are significantly different (user resized) */
                        if (data->expanded_width == 0 || data->expanded_height == 0 ||
                            abs((int) current_width - (int) data->expanded_width) > 10 ||
                            abs((int) current_height - (int) data->expanded_height) > 10) {
                            data->expanded_width = current_width;
                            data->expanded_height = current_height;
                        }
                    } else if (data->expanded_width == 0 || data->expanded_height == 0) {
                        if (data->title_obj) {
                            set(data->title_obj, MUIA_PanelTitle_Collapsed, FALSE);
                        }
                        return DoSuperMethodA(cl, obj, (Msg) msg);
                    }
                }

                /* Update title object's collapsed state */
                if (data->title_obj) {
                    set(data->title_obj, MUIA_PanelTitle_Collapsed, tag->ti_Data);
                }

                /* Hide/show children based on collapsed state, but keep title visible */
                struct MinList *children = NULL;
                get(obj, MUIA_Group_ChildList, &children);
                if (children) {
                    Object *child;
                    APTR cstate = children->mlh_Head;
                    while ((child = NextObject((APTR *) &cstate))) {
                        /* Keep title and drag handle visible even when collapsed */
                        if (child != data->title_obj && child != data->drag_handle) {
                            set(child, MUIA_ShowMe, !(BOOL)tag->ti_Data);
                        }
                    }
                }

                /* Trigger relayout */
                DoMethod(obj, MUIM_Group_InitChange);
                DoMethod(obj, MUIM_Group_ExitChange);
                redraw = TRUE;
            }
            break;
        }

        case MUIA_Draggable: {
            if (tag->ti_Data) {
                if (!data->drag_handle) {
                    ULONG title_position = MUIV_Panel_Title_None;
                    if (data->title_obj) {
                        get(data->title_obj, MUIA_PanelTitle_Position, &title_position);
                    }
                    data->drag_handle = Panel_CreateDragHandle(cl, obj, title_position);
                    if (data->drag_handle) {
                        DoMethod(obj, OM_ADDMEMBER, data->drag_handle);
                    }
                }
            } else {
                if (data->drag_handle) {
                    DoMethod(obj, OM_REMMEMBER, data->drag_handle);
                    MUI_DisposeObject(data->drag_handle);
                    data->drag_handle = NULL;
                }
            }

            /* Trigger relayout */
            DoMethod(obj, MUIM_Group_InitChange);
            DoMethod(obj, MUIM_Group_ExitChange);
            redraw = TRUE;
            break;
        }
        case MUIA_Panel_TitleClickedHook:
            if (data->title_obj) {
                set(data->title_obj, MUIA_PanelTitle_ClickHook, tag->ti_Data);
            }
            break;
        }
    }

    if (redraw) {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Panel__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Panel_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID) {
    case MUIA_Panel_Padding: *msg->opg_Storage = data->padding; return TRUE;

    /* Forward title-related attributes to PanelTitle object */
    case MUIA_Panel_Title:
        if (data->title_obj) {
            get(data->title_obj, MUIA_PanelTitle_Text, msg->opg_Storage);
        } else {
            *msg->opg_Storage = 0;
        }
        return TRUE;
    case MUIA_Panel_TitlePosition:
        if (data->title_obj) {
            get(data->title_obj, MUIA_PanelTitle_Position, msg->opg_Storage);
        } else {
            *msg->opg_Storage = MUIV_Panel_Title_None;
        }
        return TRUE;
    case MUIA_Panel_TitleTextPosition:
        if (data->title_obj) {
            get(data->title_obj, MUIA_PanelTitle_TextPosition, msg->opg_Storage);
        } else {
            *msg->opg_Storage = MUIV_Panel_Title_Text_Centered;
        }
        return TRUE;
    case MUIA_Panel_TitleVertical:
        if (data->title_obj) {
            get(data->title_obj, MUIA_PanelTitle_Vertical, msg->opg_Storage);
        } else {
            *msg->opg_Storage = FALSE;
        }
        return TRUE;

    case MUIA_Panel_Collapsible:
        if (data->title_obj) {
            get(data->title_obj, MUIA_PanelTitle_Collapsible, msg->opg_Storage);
        } else {
            *msg->opg_Storage = FALSE;
        }
        return TRUE;
    case MUIA_Panel_Collapsed:
        if (data->title_obj) {
            get(data->title_obj, MUIA_PanelTitle_Collapsed, msg->opg_Storage);
        } else {
            *msg->opg_Storage = FALSE;
        }
        return TRUE;
    case MUIA_Draggable: return data->drag_handle != NULL;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Panel__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Panel_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    Panel__Sort_Children(cl, obj);

    /* Set initial child visibility based on collapsed state */
    BOOL collapsed = FALSE;
    if (data->title_obj) {
        get(data->title_obj, MUIA_PanelTitle_Collapsed, &collapsed);
    }
    if (collapsed) {
        struct MinList *children = NULL;
        get(obj, MUIA_Group_ChildList, &children);
        if (children) {
            Object *child;
            APTR cstate = children->mlh_Head;
            while ((child = NextObject((APTR *) &cstate))) {
                /* Keep title and drag handle visible even when collapsed */
                if (child != data->title_obj && child != data->drag_handle) {
                    set(child, MUIA_ShowMe, FALSE);
                }
            }
        }
    }

    return TRUE;
}

IPTR Panel__MUIM_Group_InitChange(struct IClass *cl, Object *obj, Msg msg)
{
    return DoSuperMethodA(cl, obj, msg);
}

IPTR Panel__MUIM_Group_ExitChange(struct IClass *cl, Object *obj, Msg msg)
{

    struct Panel_DATA *data = INST_DATA(cl, obj);

    IPTR result = DoSuperMethodA(cl, obj, msg);

    Panel__Sort_Children(cl, obj);

    return result;
}

IPTR Panel__MUIM_CreateDragImage(struct IClass *cl, Object *obj, struct MUIP_CreateDragImage *msg)
{
    struct MUI_DragImage *img = (struct MUI_DragImage *) AllocVec(sizeof(struct MUI_DragImage), MEMF_CLEAR);
    if (img) {
        IPTR depth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH);

        // Use object size without any frame additions
        img->width = _width(obj);
        img->height = _height(obj);

        if ((img->bm = AllocBitMap(img->width, img->height, depth, BMF_MINPLANES, _screen(obj)->RastPort.BitMap))) {
            ClipBlit(_rp(obj),
                     _left(obj),
                     _top(obj),
                     &((struct RastPort) { .BitMap = img->bm }),
                     0,
                     0,
                     _width(obj),
                     _height(obj),
                     0xc0);
        }
        img->touchx = msg->touchx;
        img->touchy = msg->touchy;
        img->flags = 0;
    }
    return (IPTR) img;
}

IPTR Panel__MUIM_DeleteDragImage(struct IClass *cl, Object *obj, struct MUIP_DeleteDragImage *msg)
{
    FreeBitMap(msg->di->bm);
    return 0;
}

/*** Class initialization ***************************************************/

#if ZUNE_BUILTIN_PANEL
BOOPSI_DISPATCHER(IPTR, Panel_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID) {
    case OM_NEW: return Panel__OM_NEW(cl, obj, (struct opSet *) msg);
    case OM_SET: return Panel__OM_SET(cl, obj, (struct opSet *) msg);
    case OM_GET: return Panel__OM_GET(cl, obj, (struct opGet *) msg);
    case MUIM_Setup: return Panel__MUIM_Setup(cl, obj, (struct MUIP_Setup *) msg);
    case MUIM_Group_InitChange: return Panel__MUIM_Group_InitChange(cl, obj, msg);
    case MUIM_Group_ExitChange: return Panel__MUIM_Group_ExitChange(cl, obj, msg);
    case MUIM_CreateDragImage: return Panel__MUIM_CreateDragImage(cl, obj, (struct MUIP_CreateDragImage *) msg);
    case MUIM_DeleteDragImage: return Panel__MUIM_DeleteDragImage(cl, obj, (struct MUIP_DeleteDragImage *) msg);
    default: return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Panel_desc = { MUIC_Panel,
                                                   MUIC_Group,
                                                   sizeof(struct Panel_DATA),
                                                   (void *) Panel_Dispatcher };
#endif /* ZUNE_BUILTIN_PANEL */
