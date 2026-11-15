/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

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

/****** Panel.mui/MUIC_Panel ************
*
*   NAME
*       MUIC_Panel
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       A Group with an optional PanelTitle and drag handle. A non-NULL
*       Title and non-None TitlePosition at construction create the title; later
*       sets cannot create one. Left titles select horizontal grouping unless
*       caller Group tags override it. Group owns all generated children.
*
*   METHODS
*       Collapsed hides content, not the title or handle; expanding makes
*       all content children visible rather than restoring their earlier state.
*       First collapse requires positive geometry or saved dimensions. Without
*       a title, collapsed state is not usable. Title clicks bypass PanelGroup's
*       cached state; coordinate changes through PanelGroup if its state matters.
*
*   BUGS
*       MUIA_Panel_Draggable is not implemented; use inherited
*       MUIA_Draggable. Its mere presence creates a handle, even when FALSE.
*       Changing title position does not reorient Group/handle. Padding has no
*       layout effect. Drag-image allocation failure can leave a partial image;
*       deletion frees the bitmap but not the image structure.
*
*/
/****** Panel.mui/MUIA_Panel_Padding ************
*
*   NAME
*       MUIA_Panel_Padding
*
*   FUNCTION
*       Store the panel padding value.
*
*   INPUTS
*       Initialize, set and get, integer stored as UWORD. Default zero.
*
*   NOTES
*       The field has no layout or drawing consumer. Set requests redraw when
*       changed but does not add spacing. Large values truncate. Get returns
*       TRUE with the stored value; use inherited Group/Area spacing controls
*       for their documented effects.
*
*/
/****** Panel.mui/MUIA_Panel_Title ************
*
*   NAME
*       MUIA_Panel_Title
*
*   FUNCTION
*       Set or read the title text.
*
*   INPUTS
*       Initialize, set and get, terminated STRPTR. Default no title.
*
*   NOTES
*       Construction copies the text through a new PanelTitle only when
*       TitlePosition is not None. Set replaces text in an existing title and
*       otherwise has no local effect. Get returns TRUE with a borrowed pointer
*       to the owned copy, or NULL without a title. Copy it before
*       replacement/disposal; do not free it or pass the same owned pointer back
*       to Set, which frees it before copying. Avoid empty text with left
*       vertical rendering. Allocation failure is not reported as Panel creation
*       or Set failure.
*
*/
/****** Panel.mui/MUIA_Panel_TitlePosition ************
*
*   NAME
*       MUIA_Panel_TitlePosition
*
*   FUNCTION
*       Select the title position used at construction.
*
*   INPUTS
*       Initialize, set and get, ULONG: None (0, default), Top (1), Left (2).
*
*   NOTES
*       Construction uses Left for horizontal grouping and creates a title only
*       with non-NULL Title and non-None position. Get without a title returns
*       None with TRUE. Set only forwards to an existing PanelTitle: it neither
*       removes the title for None nor changes Group/handle orientation.
*       PanelTitle position replacement also has resource-lifecycle defects;
*       recreate the Panel to change its arrangement.
*
*/
/****** Panel.mui/MUIA_Panel_TitleTextPosition ************
*
*   NAME
*       MUIA_Panel_TitleTextPosition
*
*   FUNCTION
*       Align text within the title.
*
*   INPUTS
*       Initialize, set and get, ULONG: Centered (0, default), Left (1), Right
*       (2).
*
*   NOTES
*       Forwarded to an existing PanelTitle. For vertical text, Left means near
*       the top and Right near the bottom. Unknown values use centered drawing.
*       No title means Set has no effect and Get returns Centered with TRUE.
*       Alignment does not change parent Group orientation.
*
*/
/****** Panel.mui/MUIA_Panel_TitleVertical ************
*
*   NAME
*       MUIA_Panel_TitleVertical
*
*   FUNCTION
*       Render left-side title text as stacked characters.
*
*   INPUTS
*       Initialize, set and get, BOOL; default FALSE.
*
*   NOTES
*       Forwarded to an existing PanelTitle. Effective only with Left position;
*       characters are stacked rather than rotated. Use nonempty text, since
*       empty vertical text reaches division by its length. Set invalidates
*       title measurements and requests redraw, but no explicit parent relayout.
*       Get without a title returns FALSE with TRUE.
*
*/
/****** Panel.mui/MUIA_Panel_Collapsible ************
*
*   NAME
*       MUIA_Panel_Collapsible
*
*   FUNCTION
*       Allow title-click toggling.
*
*   INPUTS
*       Initialize, set and get, BOOL; default FALSE.
*
*   NOTES
*       Forwarded to PanelTitle when present. Direct Panel_Collapsed Set is not
*       gated by this option. Changing it after Setup does not allocate missing
*       arrow resources. Without a title Get returns FALSE with TRUE and Set has
*       no local effect. PanelGroup caches this option and does not
*       automatically track later changes.
*
*/
/****** Panel.mui/MUIA_Panel_Collapsed ************
*
*   NAME
*       MUIA_Panel_Collapsed
*
*   FUNCTION
*       Hide or show panel content.
*
*   INPUTS
*       Initialize, set and get, BOOL; default FALSE.
*
*   NOTES
*       The title stores the state. Setup applies initial collapse; changed Set
*       keeps title/handle visible and sets all other children ShowMe to the
*       inverse, then requests group layout and redraw. Expanding does not
*       restore independently hidden children. A first runtime collapse without
*       valid current or saved geometry is rejected locally. Get returns the
*       title state with TRUE, or FALSE without a title. No-title panels cannot
*       reliably reverse a collapse because no state is retained. Direct changes
*       do not refresh PanelGroup caches.
*
*/
/****** Panel.mui/MUIA_Panel_DrawSeparator ************
*
*   NAME
*       MUIA_Panel_DrawSeparator
*
*   FUNCTION
*       Request a line beside the title when expanded.
*
*   INPUTS
*       Initialize and set, BOOL; default TRUE for a Panel-created title. No
*       local Panel getter.
*
*   NOTES
*       Forwarded to PanelTitle_ShowSeparator. The title draws a horizontal line
*       at Top or vertical line at Left only when not collapsed. The title size
*       calculation still includes the separator when collapsed. Without a title
*       there is no local effect.
*
*/
/****** Panel.mui/MUIA_Panel_TitleClickedHook ************
*
*   NAME
*       MUIA_Panel_TitleClickedHook
*
*   FUNCTION
*       Install the callback for title clicks.
*
*   INPUTS
*       Initialize and set, borrowed struct Hook pointer; default NULL. No local
*       Panel getter.
*
*   NOTES
*       Forwarded to an existing title. On SELECTDOWN inside it, PanelTitle
*       first attempts Toggle if collapsible, then calls CallHookPkt(hook,
*       titleObject, NULL), ignoring the hook result and consuming the event.
*       The object argument is PanelTitle, not Panel. Keep hook and callback
*       state alive through use; callbacks must preserve objects needed by the
*       remaining synchronous event path.
*
*/
/****** Panel.mui/MUIA_Panel_Draggable ************
*
*   NAME
*       MUIA_Panel_Draggable
*
*   FUNCTION
*       Identify a declared Panel-specific tag with no local behavior.
*
*   INPUTS
*       Declared in panel.h; no local initialization, Set or Get case.
*
*   NOTES
*       Use inherited MUIA_Draggable for the implemented handle path. Do not
*       infer that this similarly named identifier creates a handle or enables
*       dragging. The inherited constructor path creates a handle on tag
*       presence even for FALSE; see the class limitations.
*
*/
/****** Panel.mui/MUIA_Panel_DrawStateIndicator ************
*
*   NAME
*       MUIA_Panel_DrawStateIndicator
*
*   FUNCTION
*       Choose whether the title draws a collapse arrow.
*
*   INPUTS
*       Initialize only locally, BOOL; default FALSE. No local Panel Set/Get
*       case.
*
*   NOTES
*       Passed to PanelTitle at construction. Arrow drawing also requires
*       Collapsible and available vector resources. Hiding the indicator does
*       not disable clicking or remove reserved arrow space. Setting this Panel
*       tag later has no local implementation.
*
*/
/****** DragHandle.mui/MUIC_DragHandle ************
*
*   NAME
*       MUIC_DragHandle
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       An Area drawing two parallel lines. Vertical is a BOOL despite the
*       header's pointer annotation. Construction defaults to Draggable TRUE
*       and RelVerify. Drag-image creation/deletion is delegated to the current
*       parent (or Area without one); keep that parent alive through both calls.
*
*   NOTES
*       Vertical defaults to a 7-pixel minimum width; horizontal to a
*       7-pixel minimum height. Maximum dimensions are not uniformly capped.
*       Changing Vertical does not trigger relayout or redraw. Drawing uses
*       MPEN_HALFSHADOW directly and leaves JAM1 drawing mode.
*
*/
/****** DragHandle.mui/MUIA_DragHandle_Vertical ************
*
*   NAME
*       MUIA_DragHandle_Vertical
*
*   FUNCTION
*       Choose vertical rather than horizontal handle lines.
*
*   INPUTS
*       Initialize, set and get, BOOL in implementation; default FALSE. The
*       header incorrectly annotates an object pointer.
*
*   NOTES
*       Stored without Boolean normalization. Nonzero draws vertical lines and
*       chooses the vertical size branch. Set does not trigger redraw or layout.
*       Get writes the stored value and returns TRUE. A Panel creates the
*       initial value from the title position available at handle creation.
*
*/
/****** PanelTitle.mui/MUIC_PanelTitle ************
*
*   NAME
*       MUIC_PanelTitle
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       An Area with owned text, optional state arrow and separator.
*       PanelTitle is intended as a Panel child: Toggle writes to its parent,
*       which must synchronize the title's state. Vertical left-side text
*       stacks bytes rather than rotating characters.
*
*   METHODS
*       Setup installs a mouse handler and prepares arrow resources;
*       Cleanup removes the handler, and Dispose frees the title's resources.
*       A SELECTDOWN in the title calls Toggle when collapsible, invokes an
*       optional hook and consumes the event. Collapsible reserves arrow space
*       even when its indicator is hidden. Update-only draws are not handled.
*
*   BUGS
*       Empty vertical text can divide by zero. Text copying and repeated
*       constructor Text tags have allocation/leak limitations. Position changes
*       and repeated Setup can leave incorrect or leaked arrow resources;
*       changing Collapsible after Setup does not allocate arrows. Keep the
*       title's position and collapse configuration fixed for its lifetime.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_Text ************
*
*   NAME
*       MUIA_PanelTitle_Text
*
*   FUNCTION
*       Replace or read the owned title text.
*
*   INPUTS
*       Initialize, set and get, terminated STRPTR; default NULL.
*
*   NOTES
*       Text is copied with StrDup; Get returns a borrowed copy pointer with
*       TRUE. Set frees the old copy before duplicating the input, so first copy
*       an obtained pointer if reusing it. Replacement invalidates measurements
*       and requests full redraw only when set up; no explicit parent relayout.
*       Avoid empty strings for left vertical rendering. Copy failure leaves no
*       text without an independent error result.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_Position ************
*
*   NAME
*       MUIA_PanelTitle_Position
*
*   FUNCTION
*       Select top or left title rendering.
*
*   INPUTS
*       Initialize, set and get, ULONG: Top (1, default), Left (2).
*
*   NOTES
*       Sets size/drawing orientation within the title, not parent Group
*       orientation. Other values are not a supported no-title mode. Runtime Set
*       manages arrows in the wrong order, using the old position before storing
*       the new one; prefer construction-time choice and recreate for another
*       arrangement. Get returns TRUE with the stored value.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_TextPosition ************
*
*   NAME
*       MUIA_PanelTitle_TextPosition
*
*   FUNCTION
*       Align title text along its reading direction.
*
*   INPUTS
*       Initialize, set and get, ULONG: Centered (0, default), Left (1), Right
*       (2).
*
*   NOTES
*       Horizontal Left/Right align near the corresponding edge. Vertical
*       Left/Right align near top/bottom. Unknown values draw centered. Set
*       requests full redraw when changed and set up. Get returns TRUE with the
*       stored value.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_Vertical ************
*
*   NAME
*       MUIA_PanelTitle_Vertical
*
*   FUNCTION
*       Stack characters for a left-side title.
*
*   INPUTS
*       Initialize, set and get, BOOL; default FALSE.
*
*   NOTES
*       Only effective at Left position. Uses bytewise graphics Text calls and
*       measures character A; do not use empty text. Does not rotate glyphs. Set
*       stores the value and requests full redraw when changed and set up; no
*       explicit parent relayout. Get returns TRUE with the stored value.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_Collapsible ************
*
*   NAME
*       MUIA_PanelTitle_Collapsible
*
*   FUNCTION
*       Enable parent-directed toggling and reserve arrow space.
*
*   INPUTS
*       Initialize, set and get, BOOL; default FALSE.
*
*   NOTES
*       Toggle requires a live Panel parent. This flag does not itself change
*       collapsed state. Enabling after Setup does not allocate missing arrow
*       specifications. Set stores the value and requests full redraw when
*       changed and set up; no explicit parent relayout. Get returns TRUE with
*       the stored value.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_Collapsed ************
*
*   NAME
*       MUIA_PanelTitle_Collapsed
*
*   FUNCTION
*       Store the title state used for arrow and separator drawing.
*
*   INPUTS
*       Initialize, set and get, BOOL; default FALSE.
*
*   NOTES
*       Direct Set affects this title only; it does not hide Panel content. Use
*       Panel_Collapsed for content visibility. Toggle relies on parent feedback
*       to update this field. Set stores the value and requests full redraw when
*       changed and set up; no explicit parent relayout. Get returns TRUE with
*       the stored value.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_ShowSeparator ************
*
*   NAME
*       MUIA_PanelTitle_ShowSeparator
*
*   FUNCTION
*       Draw a separator while the title is expanded.
*
*   INPUTS
*       Initialize, set and get, BOOL; default FALSE.
*
*   NOTES
*       Top draws a bottom line; Left draws a side line. Collapsed suppresses
*       drawing but not the size allowance. Set stores the value and requests
*       full redraw when changed and set up; no explicit parent relayout. Get
*       returns TRUE with the stored value.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_ClickHook ************
*
*   NAME
*       MUIA_PanelTitle_ClickHook
*
*   FUNCTION
*       Call a hook after a title press.
*
*   INPUTS
*       Initialize, set and get, borrowed struct Hook pointer; default NULL.
*
*   NOTES
*       For SELECTDOWN inside the object, attempt Toggle first when collapsible,
*       then call CallHookPkt(hook, titleObject, NULL). Return value is ignored;
*       the event is consumed. The hook can run even when no collapse occurred.
*       Keep callback and object state valid through the call; Get returns the
*       borrowed pointer with TRUE.
*
*/
/****** PanelTitle.mui/MUIA_PanelTitle_DrawStateIndicator ************
*
*   NAME
*       MUIA_PanelTitle_DrawStateIndicator
*
*   FUNCTION
*       Enable drawing of the collapse arrow.
*
*   INPUTS
*       Initialize only locally, BOOL; default FALSE. No local Set/Get case.
*
*   NOTES
*       Drawing also requires Collapsible and valid arrow specifications. Top
*       uses right/down for collapsed/expanded; Left uses up/right. Arrow space
*       is reserved whenever Collapsible is set, regardless of this option. No
*       arrow-allocation failure is returned to the caller.
*
*/
/****** PanelTitle.mui/MUIM_PanelTitle_Toggle ************
*
*   NAME
*       MUIM_PanelTitle_Toggle
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PanelTitle_Toggle);
*
*   FUNCTION
*       Request the inverse collapsed state from the parent Panel.
*
*   INPUTS
*       obj - Live PanelTitle attached to a live Panel; preserve both through
*       synchronous Set and notifications.
*
*   RESULT
*       TRUE when Collapsible is nonzero; FALSE otherwise. TRUE reports the
*       request path, not a verified parent-state change.
*
*   NOTES
*       Set parent Panel_Collapsed to the inverse of the stored title state,
*       then request full redraw if set up. The method does not directly change
*       its own state and does not invoke ClickHook. The click handler invokes
*       the hook separately afterward. PanelGroup cached state is not updated
*       automatically.
*
*/

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
/*
 * Create Area with draggable/RelVerify defaults then store Vertical. Return
 * object or zero on superclass failure. Caller tags may override defaults;
 * no local resource allocation. Vertical is BOOL storage despite pointer
 * annotation in header.
 */
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

/*
 * Store Vertical without normalization or redraw/layout request, then return
 * Area setter result. no pointer ownership or allocation.
 */
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

/*
 * Write Vertical to valid output storage and return TRUE; delegate unknown
 * queries to Area. returned value is stored BOOL, not an object pointer.
 */
IPTR DragHandle__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct DragHandle_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID) {
    case MUIA_DragHandle_Vertical: *msg->opg_Storage = (IPTR) data->vertical; return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Delegate the borrowed drag-image request to current parent if present,
 * otherwise Area, returning its result. Requires stable live parent through
 * creation/deletion; no local image allocation or coordinate conversion.
 */
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

/*
 * Delegate image deletion to current parent if present, otherwise Area.
 * Requires matching creator/parent and valid image message; no local cleanup
 * or ownership repair.
 */
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

/*
 * Delegate Area then overwrite minima/defaults from Vertical, increase
 * selected maxima and return zero. Requires writable size record; does not
 * uniformly constrain the thin dimension to seven pixels.
 */
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

/*
 * Draw parallel lines after Area regardless of flags using valid RastPort.
 * Use MPEN_HALFSHADOW as literal pen, restore old pen but retain JAM1 mode;
 * return superclass result. Requires drawable setup geometry; no allocation
 * or error propagation.
 */
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

/*
 * Delegate directly to Area Setup. Requires valid lifecycle/setup message;
 * this class acquires no additional setup resources and does not validate
 * the parent drag-image path.
 */
IPTR DragHandle__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*** Class initialization ***************************************************/

#if ZUNE_BUILTIN_DRAGHANDLE
/*
 * Dispatch ABI-correct local lifecycle and class messages to their handlers;
 * delegate remaining messages to the declared superclass. Requires live
 * object except construction and normal resource lifecycle; adds no
 * validation or synchronization.
 */
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

/*
 * Create a new DragHandle with Vertical derived from supplied title
 * position; return owned object or NULL. Caller must add it to an owning
 * Group or dispose it. Requires live Panel instance; no insertion performed
 * here.
 */
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

/*
 * Measure borrowed owned text using ZText horizontally or character-A
 * metrics times byte length vertically. Requires live instance and suitable
 * render context; NULL text/object clears dimensions. Return void, store
 * UWORD sizes; successful ZText before Setup may leave old dimensions.
 * Destroy temporary ZText; no overflow validation.
 */
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

/*
 * Draw an available stored vector specification for supplied direction if
 * collapsible and RastPort valid. Requires live render info and valid
 * specification lifetime; missing spec draws nothing. Return void, no
 * allocation or pen-state restoration guarantee.
 */
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

/*
 * Draw horizontal or vertical line with setup pens and JAM1 if RastPort
 * exists. Requires valid geometry/render context. Return void, leave
 * selected pen/draw mode changed and retain no resources.
 */
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

/*
 * Clean up each non-NULL arrow specification using zune_imspec_cleanup.
 * Requires live owned specifications not previously cleaned; pointers are
 * not cleared. repeated use without replacement can act on stale pointers.
 */
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

/*
 * Allocate right plus up/ down arrow specifications for stored Left/Top
 * using live render info. Return void, no allocation status propagation or
 * cleanup of overwritten pointers. Caller must maintain ownership and
 * correct setup/position lifecycle.
 */
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

/*
 * Delegate Area, initialize defaults and duplicate Text constructor values,
 * borrow ClickHook. Return object or zero only on superclass failure;
 * duplicate Text tags leak prior copy and copy failure is not propagated.
 * Keep initial geometry/orientation coherent.
 */
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

/*
 * Free owned text, clean arrow specifications then return Area disposal
 * result. Requires handler removed through Cleanup and valid non-stale
 * specs; does not repair pointers left by flawed position transitions.
 */
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

/*
 * Replace copied text or update supported options; redraw full when needed
 * and set up, then return Area result. Text self-alias is unsafe because old
 * copy is freed first. Position frees/allocates specs using old position
 * before storing new; no DrawStateIndicator setter or explicit parent
 * relayout.
 */
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

/*
 * Return text pointer/options/hook with TRUE into writable storage; delegate
 * DrawStateIndicator and unknown queries. Text is borrowed until
 * replacement/disposal and hook remains caller-owned. Does not allocate or
 * synchronize parent collapse state.
 */
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

/*
 * Require Area success, register mouse-button handler, measure text and
 * allocate arrows if collapsible; return TRUE without checking
 * registration/spec allocation. Requires live Window/render context.
 * Repeated Setup can overwrite retained specs because Cleanup does not
 * release them.
 */
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

/*
 * Remove event handler from live Window then return Area Cleanup result.
 * Requires corresponding Setup and no active callback. Retains arrow
 * specifications for Dispose; no pointer reset or release here.
 */
IPTR PanelTitle__MUIM_Cleanup(struct IClass *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct PanelTitle_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Delegate Area then replace title size bounds from text, padding, arrow
 * space and separator. Requires writable output and render context for
 * measurement. No text yields flexible zero minima; narrow dimension
 * arithmetic unchecked. Return superclass result.
 */
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

/*
 * After Area, draw only full-object/non-NULL-text path. Measure text, draw
 * optional arrow and text then expanded separator; left vertical empty text
 * divides by zero. Requires live specs/render context and nonempty vertical
 * text. Return superclass result; temporary ZText released, pen/mode left
 * changed.
 */
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

/*
 * On SELECTDOWN inside title, request parent toggle if collapsible, call
 * borrowed hook with title and NULL message, then return Eat. Requires live
 * objects through synchronous callbacks; hook result ignored. Other events
 * delegate Area; no local resources retained.
 */
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

/*
 * If collapsible, set live parent Panel_Collapsed to inverse stored title
 * state, redraw if set up and return TRUE; otherwise FALSE. Does not itself
 * change title cache or invoke hook; TRUE does not prove parent accepted
 * transition.
 */
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
/*
 * Dispatch ABI-correct local lifecycle and class messages to their handlers;
 * delegate remaining messages to the declared superclass. Requires live
 * object except construction and normal resource lifecycle; adds no
 * validation or synchronization.
 */
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

/*
 * Create Group orientation from initial TitlePosition, parse fields, create
 * handles on every inherited Draggable tag regardless value and optional
 * title afterward. Return object despite child allocation failure. Group
 * owns inserted children; title copied, hook borrowed. Handle orientation
 * depends on tag order.
 */
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

/*
 * Read live child list, allocate temporary method array, order recorded
 * handle/title first and other children afterward, send Group_Sort and free
 * array. Requires recorded children belong to group and stable list. Return
 * void, allocation failure leaves order unchanged; no NULL child-list guard.
 */
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

/*
 * Forward supported title tags, store unused Padding, manage inherited
 * Draggable handle and change content ShowMe on changed collapse. Requires
 * stable children and valid geometry; first collapse can delegate early when
 * geometry unavailable. Expansion forces all content visible; return
 * superclass result, no rollback or container cache update.
 */
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

/*
 * Return Padding/title-related state with TRUE, supplying defaults without
 * title. Inherited Draggable branch returns handle presence without writing
 * output; caller must not treat storage as initialized. Other queries
 * delegate Group; text remains borrowed.
 */
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

/*
 * Require Group success, sort generated children first and hide content when
 * initial title state collapsed. Return TRUE without checking sorting;
 * requires valid owned child list. Does not save initial expanded dimensions
 * or notify PanelGroup cache.
 */
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

/*
 * Delegate the balanced change-begin message to Group. Requires normal
 * live-object lifecycle and matching ExitChange; no additional local state
 * or allocation.
 */
IPTR Panel__MUIM_Group_InitChange(struct IClass *cl, Object *obj, Msg msg)
{
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Delegate Group end-change then sort handle/title first, returning
 * inherited result. Requires balanced change operation, stable child list
 * and valid recorded generated children. Temporary allocation failure is not
 * reported.
 */
IPTR Panel__MUIM_Group_ExitChange(struct IClass *cl, Object *obj, Msg msg)
{

    struct Panel_DATA *data = INST_DATA(cl, obj);

    IPTR result = DoSuperMethodA(cl, obj, msg);

    Panel__Sort_Children(cl, obj);

    return result;
}

/*
 * Allocate cleared image structure and optional friend bitmap, copy object
 * rectangle and store touch coordinates. Requires drawable object/screen.
 * Return NULL only for structure allocation failure; non-NULL can contain
 * NULL bitmap. Returned resources need matching deletion; bitmap-copy errors
 * unreported.
 */
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

/*
 * Free the supplied drag-image bitmap and return zero. Requires valid image
 * pointer and matching bitmap; allocated image structure itself is not
 * freed, producing a leak in the local create/delete pair.
 */
IPTR Panel__MUIM_DeleteDragImage(struct IClass *cl, Object *obj, struct MUIP_DeleteDragImage *msg)
{
    FreeBitMap(msg->di->bm);
    return 0;
}

/*** Class initialization ***************************************************/

#if ZUNE_BUILTIN_PANEL
/*
 * Dispatch ABI-correct local lifecycle and class messages to their handlers;
 * delegate remaining messages to the declared superclass. Requires live
 * object except construction and normal resource lifecycle; adds no
 * validation or synchronization.
 */
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
