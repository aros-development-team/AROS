/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    PanelGroup implementation
*/

#include <clib/alib_protos.h>
#include <exec/memory.h>
#include <graphics/gfxmacros.h>
#include <intuition/classes.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "libraries/mui.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "panel.h"
#include "panelgroup.h"
#include "panelgroup_private.h"
#include "support.h"



/****** PanelGroup.mui/MUIC_PanelGroup ************
*
*   NAME
*       MUIC_PanelGroup
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       A Group with cached state for recognized Panel and DragHandle
*       children. Group owns children; tracking nodes only borrow their pointers.
*       ScanPanels adds missing nodes but does not remove stale ones or refresh
*       existing states immediately. A balanced Group change pair refreshes a
*       cache marked stale. Direct child/title changes do not invalidate it.
*
*   METHODS
*       Collapse/Expand/Toggle use cached state. They may update that cache
*       before Setup without changing the visible child. AllowMultiple FALSE is
*       not an invariant: externally changed panels and title clicks can bypass
*       it. To refresh, ScanPanels then perform a balanced Group change pair.
*       Keep tracked objects alive; ScanPanels does not reconcile removals.
*
*   BUGS
*       Tracking allocation failures are not reported reliably. Drag
*       reordering expects Handle -> Panel -> PanelGroup; dragging Panel itself
*       is not accepted. Mixed child types can disrupt position calculations.
*       A successful drop result need not mean a move; tracking order remains
*       unchanged. The internal acceptance helper requires a non-NULL object.
*
*/
/****** PanelGroup.mui/MUIA_PanelGroup_AllowMultiple ************
*
*   NAME
*       MUIA_PanelGroup_AllowMultiple
*
*   FUNCTION
*       Permit the expand-all operation and multiple expanded panels in managed
*       operations.
*
*   INPUTS
*       Initialize, set and get, BOOL normalized; default TRUE.
*
*   NOTES
*       Switching to FALSE collapses other cached-collapsible tracked panels
*       only if ExpandedPanel is non-NULL, and only set-up targets are actually
*       written. It does not choose a panel when none is recorded. Single
*       ExpandPanel collapses only the recorded prior panel, not every
*       independently expanded child. Get returns TRUE with the option; it does
*       not verify exclusivity.
*
*/
/****** PanelGroup.mui/MUIA_PanelGroup_ExpandedPanel ************
*
*   NAME
*       MUIA_PanelGroup_ExpandedPanel
*
*   FUNCTION
*       Store the panel designated as expanded in single-panel mode.
*
*   INPUTS
*       Initialize, set and get, borrowed Object pointer; default NULL. Supply a
*       live child Panel or NULL.
*
*   NOTES
*       Construction stores the pointer and can mark layout dirty, but does not
*       enact initial exclusivity. Changed Set in single-panel mode collapses
*       the previous live collapsible object and expands the new one through a
*       setup-only helper. Membership is not validated. Multiple mode only
*       stores the pointer. Repeating the same pointer does nothing locally. Get
*       returns TRUE with the recorded pointer, not a scan of visible children;
*       keep it valid while stored.
*
*/
/****** PanelGroup.mui/MUIA_PanelGroup_CollapseAll ************
*
*   NAME
*       MUIA_PanelGroup_CollapseAll
*
*   FUNCTION
*       Trigger collapse of all eligible tracked panels.
*
*   INPUTS
*       Set-only trigger, BOOL; nonzero invokes the operation, zero does
*       nothing. Not processed by the local constructor; no local getter.
*
*   NOTES
*       Calls MUIM_PanelGroup_CollapsePanel with NULL. Clears the recorded
*       ExpandedPanel even if no node changes. Eligibility and results use
*       cached flags; pre-Setup calls can change the cache without changing
*       children. OM_SET returns the superclass result rather than the
*       operation's changed flag.
*
*/
/****** PanelGroup.mui/MUIA_PanelGroup_ExpandAll ************
*
*   NAME
*       MUIA_PanelGroup_ExpandAll
*
*   FUNCTION
*       Trigger expansion of all eligible tracked panels.
*
*   INPUTS
*       Set-only trigger, BOOL; nonzero invokes the operation, zero does
*       nothing. Not processed by the local constructor; no local getter.
*
*   NOTES
*       Calls MUIM_PanelGroup_ExpandPanel with NULL. Does nothing in
*       single-panel mode. Eligibility and results use cached flags; pre-Setup
*       calls can change the cache without changing children. OM_SET returns the
*       superclass result rather than the operation's changed flag.
*
*/
/****** PanelGroup.mui/MUIA_PanelGroup_DragReordering ************
*
*   NAME
*       MUIA_PanelGroup_DragReordering
*
*   FUNCTION
*       Enable drop acceptance and child dragging for panel reordering.
*
*   INPUTS
*       Initialize, set and get, BOOL normalized; default FALSE.
*
*   NOTES
*       Sets this group Dropable and inherited Draggable on tracked objects;
*       newly tracked objects are made draggable only when enabled. For Panel
*       this controls its DragHandle path, not MUIA_Panel_Draggable. Use a
*       vertical group of Panel children with handles: the drag query expects
*       handle -> Panel -> this group. Mixed non-panel children are not indexed
*       consistently. Get returns TRUE with the option, not proof of successful
*       handle creation or drop.
*
*/
/****** PanelGroup.mui/MUIM_PanelGroup_CollapsePanel ************
*
*   NAME
*       MUIM_PanelGroup_CollapsePanel
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PanelGroup_CollapsePanel, panel);
*
*   FUNCTION
*       Collapse one tracked panel or all eligible tracked panels.
*
*   INPUTS
*       obj - Live PanelGroup with coherent tracking.
*       panel - Borrowed live tracked object, or NULL with the method-specific
*       meaning.
*
*   RESULT
*       TRUE if cached state changed; FALSE otherwise.
*
*   NOTES
*       NULL selects all and clears ExpandedPanel. A specific target must
*       be tracked, cached-collapsible and cached-expanded. Only set-up panels
*       are actually changed; the cache can change before Setup. Direct child
*       changes require the class's cache-refresh sequence.
*
*/
/****** PanelGroup.mui/MUIM_PanelGroup_ExpandPanel ************
*
*   NAME
*       MUIM_PanelGroup_ExpandPanel
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PanelGroup_ExpandPanel, panel);
*
*   FUNCTION
*       Expand one tracked panel or all eligible tracked panels.
*
*   INPUTS
*       obj - Live PanelGroup with coherent tracking.
*       panel - Borrowed live tracked object, or NULL with the method-specific
*       meaning.
*
*   RESULT
*       TRUE if cached state changed; FALSE otherwise.
*
*   NOTES
*       NULL expands all only with AllowMultiple TRUE. A specific target
*       must be tracked, cached-collapsible and cached-collapsed. Single-panel
*       mode collapses only the previously recorded ExpandedPanel, not every
*       independently expanded child. Cache changes can precede Setup; direct
*       child changes require the class's cache-refresh sequence.
*
*/
/****** PanelGroup.mui/MUIM_PanelGroup_TogglePanel ************
*
*   NAME
*       MUIM_PanelGroup_TogglePanel
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PanelGroup_TogglePanel, panel);
*
*   FUNCTION
*       Choose collapse or expansion from a tracked panel's cached state.
*
*   INPUTS
*       obj - Live PanelGroup with coherent tracking.
*       panel - Borrowed live tracked object, or NULL with the method-specific
*       meaning.
*
*   RESULT
*       The selected CollapsePanel/ExpandPanel result, or FALSE for NULL,
*       untracked or cached-noncollapsible target.
*
*   NOTES
*       NULL does not mean all. Uses cached state rather than querying
*       the child. Cache changes can precede Setup; see class synchronization
*       requirements for direct child changes.
*
*/
/****** PanelGroup.mui/MUIM_PanelGroup_GetPanelState ************
*
*   NAME
*       MUIM_PanelGroup_GetPanelState
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PanelGroup_GetPanelState, panel);
*
*   FUNCTION
*       Read a tracked panel's cached collapse state.
*
*   INPUTS
*       obj - Live PanelGroup with coherent tracking.
*       panel - Borrowed live tracked object, or NULL with the method-specific
*       meaning.
*
*   RESULT
*       MUIV_PanelGroup_Panel_Collapsed (0) or MUIV_PanelGroup_Panel_Expanded
*       (1). NULL and untracked targets also return Expanded.
*
*   NOTES
*       Unknown and untracked objects return Expanded, as do expanded
*       panels. No live query or cache refresh occurs. See the class-level
*       synchronization requirements.
*
*/
/****** PanelGroup.mui/MUIM_PanelGroup_ScanPanels ************
*
*   NAME
*       MUIM_PanelGroup_ScanPanels
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_PanelGroup_ScanPanels);
*
*   FUNCTION
*       Add recognized direct children to the tracking list.
*
*   INPUTS
*       obj - Live PanelGroup with stable child list; previously tracked objects
*       must still be alive.
*
*   RESULT
*       FALSE when the child list is unavailable; TRUE after scanning, even if
*       individual tracking-node allocations failed.
*
*   NOTES
*       Recognizes direct Panel and DragHandle children, including
*       subclasses. Adds missing nodes but does not remove departed children or
*       refresh the retained nodes immediately. The count reflects recognized
*       children, not successful allocations. Marks the cache stale; a balanced
*       Group change pair refreshes it. No ownership transfer occurs.
*
*/

#define DEBUG 1

#include <aros/debug.h>

#ifndef MADF_SETUP
#define MADF_SETUP (1 << 28) /* PRIV - zune-specific */
#endif

/* Helper functions */
/*
 * Search live tracking list by pointer identity and return borrowed node or
 * NULL. Does not dereference candidate panel or query state; list must
 * remain stable and result expires on node removal/container disposal.
 */
static struct PanelNode *FindPanelNode(struct PanelGroup_DATA *data, Object *panel);
/*
 * Initialize FALSE, query live non-NULL panel Collapsible and return value
 * without checking getter status. NULL yields FALSE. Borrow object for call;
 * no ownership transfer or cache update.
 */
static BOOL IsPanelCollapsible(Object *panel);
/*
 * Initialize FALSE, query live non-NULL panel Collapsed and return value
 * without checking getter status. NULL yields FALSE. Requires valid object
 * pointer; no synchronization or allocation.
 */
static BOOL IsPanelCollapsed(Object *panel);
/*
 * Set requested state only on non-NULL live panel with MADF_SETUP. Return
 * void without confirming transition; pre-Setup is a no- op. Notifications
 * may execute synchronously and must preserve objects.
 */
static void SetPanelCollapsed(Object *panel, BOOL collapsed);
/*
 * Refresh every live tracked pointer using Panel getters, storing
 * collapsible/collapsed flags. Requires no stale removed/disposed pointers
 * and stable list through calls. does not discover membership or enforce
 * exclusivity.
 */
static void UpdatePanelStates(struct IClass *cl, Object *obj);

/**************************************************************************
 Helper function to find a panel in our tracking list
**************************************************************************/
/*
 * Search live tracking list by pointer identity and return borrowed node or
 * NULL. Does not dereference candidate panel or query state; list must
 * remain stable and result expires on node removal/container disposal.
 */
static struct PanelNode *FindPanelNode(struct PanelGroup_DATA *data, Object *panel)
{
    struct PanelNode *node;
    int count = 0;

    ForeachNode(&data->panel_list, node)
    {
        count++;
        if (node->panel == panel) {
            return node;
        }
    }
    return NULL;
}

/**************************************************************************
 Helper function to check if a panel is collapsible
**************************************************************************/
/*
 * Initialize FALSE, query live non-NULL panel Collapsible and return value
 * without checking getter status. NULL yields FALSE. Borrow object for call;
 * no ownership transfer or cache update.
 */
static BOOL IsPanelCollapsible(Object *panel)
{
    BOOL collapsible = FALSE;

    if (panel) {
        get(panel, MUIA_Panel_Collapsible, &collapsible);
    }
    return collapsible;
}

/**************************************************************************
 Helper function to check if a panel is collapsed
**************************************************************************/
/*
 * Initialize FALSE, query live non-NULL panel Collapsed and return value
 * without checking getter status. NULL yields FALSE. Requires valid object
 * pointer; no synchronization or allocation.
 */
static BOOL IsPanelCollapsed(Object *panel)
{
    BOOL collapsed = FALSE;

    if (panel) {
        get(panel, MUIA_Panel_Collapsed, &collapsed);
    }
    return collapsed;
}

/**************************************************************************
 Helper function to set panel collapsed state
**************************************************************************/
/*
 * Set requested state only on non-NULL live panel with MADF_SETUP. Return
 * void without confirming transition; pre-Setup is a no-op. Notifications
 * may execute synchronously and must preserve objects.
 */
static void SetPanelCollapsed(Object *panel, BOOL collapsed)
{
    if (panel) {
        if (_flags(panel) & MADF_SETUP) {
            set(panel, MUIA_Panel_Collapsed, collapsed);
        }
    }
}

/**************************************************************************
 Update panel states in our tracking list
**************************************************************************/
/*
 * Refresh every live tracked pointer using Panel getters, storing
 * collapsible/collapsed flags. Requires no stale removed/disposed pointers
 * and stable list through calls. does not discover membership or enforce
 * exclusivity.
 */
static void UpdatePanelStates(struct IClass *cl, Object *obj)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct PanelNode *node;

    ForeachNode(&data->panel_list, node)
    {
        if (node->panel) {
            node->collapsed = IsPanelCollapsed(node->panel);
            node->collapsible = IsPanelCollapsible(node->panel);
        }
    }
}

/*
 * Delegate Group, initialize tracking defaults, parse options/pointer, set
 * Dropable and scan direct children. Return object or zero on superclass
 * failure; tracking allocation failures ignored. ExpandedPanel borrowed and
 * initial dirty flag does not itself enforce exclusivity.
 */
IPTR PanelGroup__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct PanelGroup_DATA *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return 0;

    data = INST_DATA(cl, obj);

    /* Initialize default values */
    data->allow_multiple = TRUE;
    data->expanded_panel = NULL;
    data->panel_count = 0;
    data->layout_dirty = FALSE;
    data->states_current = FALSE;
    data->drag_reordering = FALSE;
    data->drop_position = -1;
    data->show_drop_mark = FALSE;

    /* Initialize panel list */
    NewList((struct List *) &data->panel_list);

    /* Parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_PanelGroup_AllowMultiple: data->allow_multiple = tag->ti_Data ? TRUE : FALSE; break;

        case MUIA_PanelGroup_DragReordering:
            if (data->drag_reordering != (tag->ti_Data ? TRUE : FALSE)) {
                data->drag_reordering = tag->ti_Data ? TRUE : FALSE;

                /* Set PanelGroup as dropable when drag reordering is enabled */
                set(obj, MUIA_Dropable, data->drag_reordering);

                /* Update all panels' draggable attribute */
                struct PanelNode *node;
                ForeachNode(&data->panel_list, node)
                {
                    if (node->panel) {
                        set(node->panel, MUIA_Draggable, data->drag_reordering);
                    }
                }
            }
            break;
        case MUIA_PanelGroup_ExpandedPanel:
            data->expanded_panel = (Object *) tag->ti_Data;
            if (!data->allow_multiple && data->expanded_panel) {
                /* In single panel mode, collapse all others */
                data->layout_dirty = TRUE;
            }
            break;
        }
    }

    /* Ensure MUIA_Dropable is set based on final drag_reordering state */
    set(obj, MUIA_Dropable, data->drag_reordering);

    DoMethod(obj, MUIM_PanelGroup_ScanPanels);
    return (IPTR) obj;
}

/*
 * Remove/free tracking nodes then delegate owned child disposal to Group.
 * Requires no active callback/drag/list traversal. Does not dispose borrowed
 * node panels separately or validate stale pointers; return superclass
 * result.
 */
IPTR PanelGroup__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct PanelNode *node, *next;

    /* Free all panel tracking nodes */
    ForeachNodeSafe(&data->panel_list, node, next)
    {
        Remove((struct Node *) node);
        mui_free(node);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Process options/triggers in tag order, use setup-only state writes and
 * cached eligibility, optionally balance Group change/redraw, then return
 * Group result. ExpandedPanel membership unchecked; keep borrowed pointer
 * live. Single mode does not guarantee all other panels collapsed.
 */
IPTR PanelGroup__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags, *tag;
    BOOL relayout = FALSE;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_PanelGroup_AllowMultiple:
            if (data->allow_multiple != (tag->ti_Data ? TRUE : FALSE)) {
                data->allow_multiple = tag->ti_Data ? TRUE : FALSE;
                if (!data->allow_multiple && data->expanded_panel) {
                    /* Switch to single panel mode - collapse all except expanded */
                    struct PanelNode *node;
                    ForeachNode(&data->panel_list, node)
                    {
                        if (node->panel != data->expanded_panel && node->collapsible) {
                            SetPanelCollapsed(node->panel, TRUE);
                        }
                    }
                    data->states_current = FALSE; /* Panel states changed externally */
                    relayout = TRUE;
                }
            }
            break;

        case MUIA_PanelGroup_DragReordering:
            if (data->drag_reordering != tag->ti_Data) {
                data->drag_reordering = tag->ti_Data ? TRUE : FALSE;
                D(bug("PanelGroup OM_SET: drag_reordering set to %d\n", data->drag_reordering));

                /* Set PanelGroup as dropable when drag reordering is enabled */
                set(obj, MUIA_Dropable, data->drag_reordering);
                D(bug("PanelGroup OM_SET: setting PanelGroup dropable to %d\n", data->drag_reordering));

                /* Update all panels' draggable attribute */
                struct PanelNode *node;
                ForeachNode(&data->panel_list, node)
                {
                    if (node->panel) {
                        D(bug("PanelGroup OM_SET: setting panel %p draggable to %d\n",
                              node->panel,
                              data->drag_reordering));
                        set(node->panel, MUIA_Draggable, data->drag_reordering);
                        MUI_Redraw(node->panel, MADF_DRAWALL);
                    }
                }
            }
            break;

        case MUIA_PanelGroup_ExpandedPanel:
            if (data->expanded_panel != (Object *) tag->ti_Data) {
                Object *old_panel = data->expanded_panel;
                data->expanded_panel = (Object *) tag->ti_Data;

                if (!data->allow_multiple) {
                    /* Collapse old panel if it exists and is collapsible */
                    if (old_panel && IsPanelCollapsible(old_panel)) {
                        SetPanelCollapsed(old_panel, TRUE);
                    }

                    /* Expand new panel if it exists and is collapsible */
                    if (data->expanded_panel && IsPanelCollapsible(data->expanded_panel)) {
                        SetPanelCollapsed(data->expanded_panel, FALSE);
                    }
                    data->states_current = FALSE; /* Panel states changed externally */
                    relayout = TRUE;
                }
            }
            break;

        case MUIA_PanelGroup_CollapseAll:
            if (tag->ti_Data) {
                DoMethod(obj, MUIM_PanelGroup_CollapsePanel, NULL);
            }
            break;

        case MUIA_PanelGroup_ExpandAll:
            if (tag->ti_Data) {
                DoMethod(obj, MUIM_PanelGroup_ExpandPanel, NULL);
            }
            break;
        }
    }

    if (relayout) {
        DoMethod(obj, MUIM_Group_InitChange);
        DoMethod(obj, MUIM_Group_ExitChange);
        /* Mark objects for display refresh */
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Write AllowMultiple, ExpandedPanel or DragReordering with TRUE; delegate
 * trigger/unknown queries to Group. Requires writable storage; ExpandedPanel
 * is borrowed and may not describe actual visible state. No refresh or
 * allocation.
 */
IPTR PanelGroup__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID) {
    case MUIA_PanelGroup_AllowMultiple: *msg->opg_Storage = data->allow_multiple; return TRUE;
    case MUIA_PanelGroup_ExpandedPanel: *msg->opg_Storage = (IPTR) data->expanded_panel; return TRUE;
    case MUIA_PanelGroup_DragReordering: *msg->opg_Storage = data->drag_reordering; return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Mark layout_dirty TRUE then delegate Group begin-change. Requires live
 * instance and matching ExitChange; does not itself invalidate cached states
 * or modify membership.
 */
IPTR PanelGroup__MUIM_Group_InitChange(struct IClass *cl, Object *obj, Msg msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);

    data->layout_dirty = TRUE;
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Delegate Group then, when dirty and states not current, refresh cached
 * flags; clear dirty and return inherited result. Requires balanced change
 * and all tracked pointers live. Current cache is not re-queried merely
 * because children changed externally.
 */
IPTR PanelGroup__MUIM_Group_ExitChange(struct IClass *cl, Object *obj, Msg msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    IPTR result;

    result = DoSuperMethodA(cl, obj, msg);

    if (data->layout_dirty) {
        /* Only update panel states if they're not current */
        if (!data->states_current) {
            UpdatePanelStates(cl, obj);
            data->states_current = TRUE;
        }
        data->layout_dirty = FALSE;
    }

    return result;
}

/*
 * Compare class ancestry with obtained Panel/DragHandle classes and return
 * BOOL recognition. Requires valid non-NULL object: OCLASS is evaluated
 * before NULL guard. Class references are obtained here without a local
 * matching release; no membership check.
 */
static IPTR Panelgroup__MUIM_Panelgroup_AcceptsPanel(Object *panel)
{
    struct IClass *PanelClass = MUI_GetClass(MUIC_Panel);
    struct IClass *DragHandleClass = MUI_GetClass(MUIC_DragHandle);
    struct IClass *currClass = OCLASS(panel);

    if (!panel || !currClass || !PanelClass) {
        return FALSE;
    }

    while (currClass) {
        if (currClass == PanelClass || currClass == DragHandleClass) {
            return TRUE;
        }
        currClass = currClass->cl_Super;
    }

    return FALSE;
}

/**************************************************************************
 MUIM_PanelGroup_ScanPanels method - Scan for Panel children
**************************************************************************/
/*
 * Scan stable direct child list, add recognized Panel/DragHandle nodes,
 * replace count with recognized total and mark cache stale. Return FALSE
 * only without child list, otherwise TRUE despite add failures. Does not
 * remove stale nodes, reorder existing nodes or immediately refresh them.
 */
IPTR PanelGroup__MUIM_PanelGroup_ScanPanels(struct IClass *cl, Object *obj, Msg msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    Object *child;
    struct MinList *children;
    struct PanelNode *node, *next;
    APTR cstate;
    int panel_count = 0;

    /* Get children list */
    get(obj, MUIA_Group_ChildList, &children);
    if (!children) {
        return FALSE;
    }

    /* Scan children for Panel objects */
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate))) {
        /* Check if this is a Panel object  */
        BOOL is_panel = FALSE;
        if (Panelgroup__MUIM_Panelgroup_AcceptsPanel(child)) {
            panel_count++;
            /* Add new panel  */
            DoMethod(obj, MUIM_PanelGroup_AddPanel, child);
        }
    }

    /* Panel states need refresh after scanning panels */
    data->panel_count = panel_count;
    data->states_current = FALSE;
    return TRUE;
}

/**************************************************************************
 MUIM_PanelGroup_AddPanel method - Add a panel to our tracking
**************************************************************************/
/*
 * Internal MUIM_PanelGroup_AddPanel: borrow panel, allocate tracking node
 * with queried flags, append and optionally set Draggable.
 * NULL/allocationfailure FALSE; duplicate TRUE; successful insertion also
 * FALSE. Does not add Group membership or take panel ownership; keep pointer
 * live until tracking removal.
 */
IPTR PanelGroup__MUIM_PanelGroup_AddPanel(struct IClass *cl, Object *obj, struct MUIP_PanelGroup_AddPanel *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct PanelNode *node;

    if (!msg->panel) {
        return FALSE;
    }

    /* Check if panel is already tracked */
    if (FindPanelNode(data, msg->panel)) {
        return TRUE; /* Already tracked */
    }

    /* Create new tracking node */
    node = mui_alloc_struct(struct PanelNode);
    if (!node) {
        return FALSE;
    }

    node->panel = msg->panel;
    node->collapsed = IsPanelCollapsed(msg->panel);
    node->collapsible = IsPanelCollapsible(msg->panel);

    AddTail((struct List *) &data->panel_list, (struct Node *) node);
    data->panel_count++;

    /* Set panel draggable if drag reordering is enabled */
    if (data->drag_reordering) {
        set(msg->panel, MUIA_Draggable, TRUE);
    }

    /* Panel states need refresh after adding panel */
    data->states_current = FALSE;

    return FALSE;
}

/**************************************************************************
 MUIM_PanelGroup_RemovePanel method - Remove a panel from our tracking
**************************************************************************/
/*
 * Internal MUIM_PanelGroup_RemovePanel: remove/free matching tracking node,
 * clear matching ExpandedPanel, decrement count and mark stale. Return TRUE
 * if found, FALSE otherwise. Does not remove Group membership, dispose panel
 * or undo draggable state.
 */
IPTR PanelGroup__MUIM_PanelGroup_RemovePanel(struct IClass *cl, Object *obj, struct MUIP_PanelGroup_RemovePanel *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct PanelNode *node;

    if (!msg->panel)
        return FALSE;

    node = FindPanelNode(data, msg->panel);
    if (!node)
        return FALSE;

    /* If this was the expanded panel, clear it */
    if (data->expanded_panel == msg->panel)
        data->expanded_panel = NULL;

    Remove((struct Node *) node);
    mui_free(node);
    data->panel_count--;
    data->states_current = FALSE; /* Panel states need refresh after removing panel */

    return TRUE;
}

/**************************************************************************
 MUIM_PanelGroup_CollapsePanel method - Collapse panel(s)
**************************************************************************/
/*
 * Use cached eligibility to collapse specific node or all, update cached
 * flags/ExpandedPanel and balance Group change when changed. Return cached
 * changed BOOL even if pre-Setup write was skipped. Requires live tracked
 * objects and coherent cache; no pre-operation refresh.
 */
IPTR PanelGroup__MUIM_PanelGroup_CollapsePanel(struct IClass *cl,
                                               Object *obj,
                                               struct MUIP_PanelGroup_CollapsePanel *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct PanelNode *node;
    BOOL changed = FALSE;

    if (msg->panel) {
        /* Collapse specific panel */
        node = FindPanelNode(data, msg->panel);
        if (node) {
            if (node->collapsible && !node->collapsed) {
                SetPanelCollapsed(msg->panel, TRUE);
                node->collapsed = TRUE;

                /* Clear expanded panel if this was it */
                if (data->expanded_panel == msg->panel) {
                    data->expanded_panel = NULL;
                }

                changed = TRUE;
                data->states_current = TRUE; /* Node states are current */
            }
        }
    } else {
        /* Collapse all panels */
        ForeachNode(&data->panel_list, node)
        {
            if (node->collapsible && !node->collapsed) {
                SetPanelCollapsed(node->panel, TRUE);
                node->collapsed = TRUE;
                changed = TRUE;
            }
        }
        data->expanded_panel = NULL;
        if (changed) {
            data->states_current = TRUE; /* Node states are current */
        }
    }

    if (changed) {
        DoMethod(obj, MUIM_Group_InitChange);
        DoMethod(obj, MUIM_Group_ExitChange);
    }

    return changed;
}

/**************************************************************************
 MUIM_PanelGroup_ExpandPanel method - Expand panel(s)
**************************************************************************/
/*
 * Use cache to expand specific eligible node, collapsing only recorded old
 * node in single mode; NULL expands all only in multiple mode. Return cached
 * changed BOOL; pre-Setup helper may do nothing. Balance change on cached
 * modifications, preserve live objects through notifications.
 */
IPTR PanelGroup__MUIM_PanelGroup_ExpandPanel(struct IClass *cl, Object *obj, struct MUIP_PanelGroup_ExpandPanel *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct PanelNode *node;
    BOOL changed = FALSE;

    if (msg->panel) {
        /* Expand specific panel */
        node = FindPanelNode(data, msg->panel);
        if (node) {
            if (node->collapsible && node->collapsed) {
                /* If single panel mode, collapse others first */
                if (!data->allow_multiple && data->expanded_panel && data->expanded_panel != msg->panel) {

                    /* Directly collapse the old panel without triggering relayout */
                    struct PanelNode *old_node = FindPanelNode(data, data->expanded_panel);
                    if (old_node && old_node->collapsible && !old_node->collapsed) {
                        SetPanelCollapsed(data->expanded_panel, TRUE);
                        old_node->collapsed = TRUE;
                        changed = TRUE;
                    }
                }

                SetPanelCollapsed(msg->panel, FALSE);
                node->collapsed = FALSE;

                if (!data->allow_multiple) {
                    data->expanded_panel = msg->panel;
                }

                changed = TRUE;
                data->states_current = TRUE; /* Node states are current */
            }
        }
    } else {
        /* Expand all panels (only if multiple allowed) */
        if (data->allow_multiple) {
            ForeachNode(&data->panel_list, node)
            {
                if (node->collapsible && node->collapsed) {
                    SetPanelCollapsed(node->panel, FALSE);
                    node->collapsed = FALSE;
                    changed = TRUE;
                }
            }
            if (changed) {
                data->states_current = TRUE; /* Node states are current */
            }
        }
    }

    if (changed) {
        DoMethod(obj, MUIM_Group_InitChange);
        DoMethod(obj, MUIM_Group_ExitChange);
    }

    return changed;
}

/**************************************************************************
 MUIM_PanelGroup_TogglePanel method - Toggle panel state
**************************************************************************/
/*
 * Reject NULL/untracked/cached-noncollapsible; otherwise dispatch
 * expand/collapse from cached state and return that result. Requires live
 * coherent tracking; no query of current child state or ownership change.
 */
IPTR PanelGroup__MUIM_PanelGroup_TogglePanel(struct IClass *cl, Object *obj, struct MUIP_PanelGroup_TogglePanel *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct PanelNode *node;

    if (!msg->panel) {
        return FALSE;
    }

    node = FindPanelNode(data, msg->panel);

    if (!node) {
        return FALSE;
    }

    if (!node->collapsible) {
        return FALSE;
    }

    if (node->collapsed) {
        return DoMethod(obj, MUIM_PanelGroup_ExpandPanel, msg->panel);
    } else {
        return DoMethod(obj, MUIM_PanelGroup_CollapsePanel, msg->panel);
    }
}

/**************************************************************************
 MUIM_PanelGroup_GetPanelState method - Get panel collapse state
**************************************************************************/
/*
 * Return cached Collapsed0 or Expanded1; NULL/untracked also Expanded1.
 * Requires stable list, no state refresh or child dereference. Result does
 * not distinguish unknown from expanded.
 */
IPTR PanelGroup__MUIM_PanelGroup_GetPanelState(struct IClass *cl,
                                               Object *obj,
                                               struct MUIP_PanelGroup_GetPanelState *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    struct PanelNode *node;

    if (!msg->panel)
        return MUIV_PanelGroup_Panel_Expanded;

    node = FindPanelNode(data, msg->panel);
    if (!node)
        return MUIV_PanelGroup_Panel_Expanded;

    return node->collapsed ? MUIV_PanelGroup_Panel_Collapsed : MUIV_PanelGroup_Panel_Expanded;
}

/**************************************************************************
 Helper function to find panel index in group
**************************************************************************/
/*
 * Search current child list, increment index only for recognized
 * Panel/DragHandle; return index at pointer match or -1. Requires valid live
 * children; no allocation. This helper is defined but not called by the
 * local drop path.
 */
static LONG FindPanelIndex(struct IClass *cl, Object *obj, Object *panel)
{
    struct MinList *children;
    Object *child;
    APTR cstate;
    LONG index = 0;

    get(obj, MUIA_Group_ChildList, &children);
    if (!children)
        return -1;

    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate))) {
        if (child == panel)
            return index;
        if (Panelgroup__MUIM_Panelgroup_AcceptsPanel(child)) {
            index++;
        }
    }
    return -1;
}

/**************************************************************************
 Helper function to calculate drop position from coordinates
**************************************************************************/
/*
 * Compare pointer Y with midpoints of recognized children and return
 * insertion index; X unused. Requires stable child geometry. Return type
 * UWORD converts missing-list -1 to65535; caller must not infer a signed
 * failure. No resource ownership.
 */
static UWORD CalculateDropPosition(struct IClass *cl, Object *obj, WORD x, WORD y)
{
    struct MinList *children;
    Object *child;
    UWORD index = 0;

    // Convert screen coordinates to object-relative coordinates
    WORD rel_x = x - _left(obj);
    WORD rel_y = y - _top(obj);

    get(obj, MUIA_Group_ChildList, &children);
    if (!children)
        return -1;

    APTR cstate = children->mlh_Head;
    while ((child = NextObject(&cstate))) {
        if (Panelgroup__MUIM_Panelgroup_AcceptsPanel(child)) {
            // Use object-relative coordinates for comparison
            WORD child_top = _top(child) - _top(obj);
            WORD child_middle = child_top + _height(child) / 2;

            if (rel_y < child_middle) {
                return index; /* Drop before this panel */
            }
            index++;
        }
    }
    return index; /* Drop after last panel */
}

/**************************************************************************
 MUIM_DragQuery - Check if we accept the drag
**************************************************************************/
/*
 * Refuse disabled/self, recognize live dragged object, then accept only when
 * its parent has this group as parent. Requires non-NULL candidate and valid
 * parent chain; helper dereferences before NULL check. Intended
 * handle->Panel->group path, not direct Panel drag.
 */
IPTR PanelGroup__MUIM_DragQuery(struct IClass *cl, Object *obj, struct MUIP_DragQuery *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);

    if (!data->drag_reordering) {
        return MUIV_DragQuery_Refuse;
    }

    /* refuse to be dropped on ourself */
    if (msg->obj == obj)
        return MUIV_DragQuery_Refuse;

    /* Accept only if the dragged object is a Panel in our group */
    BOOL accepts_panel = Panelgroup__MUIM_Panelgroup_AcceptsPanel(msg->obj);

    if (msg->obj && accepts_panel) {
        Object *parent, *panel;

        panel = _parent(msg->obj);
        get(panel, MUIA_Parent, &parent);

        if (parent == obj) {
            return MUIV_DragQuery_Accept;
        }
    }

    return MUIV_DragQuery_Refuse;
}

/*
 * Compute/store insertion index, derive drop mark from all-child positions
 * and request update; always return Refresh. Requires stable live child
 * geometry/render lifecycle. Recognized-only position and all-child feedback
 * may disagree; missing-list65535 still treated nonnegative.
 */
IPTR PanelGroup__MUIM_DragReport(struct IClass *cl, Object *obj, struct MUIP_DragReport *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    LONG new_position;

    new_position = CalculateDropPosition(cl, obj, msg->x, msg->y);

    if (new_position != data->drop_position) {
        data->drop_position = new_position;
        data->show_drop_mark = (new_position >= 0);

        /* Calculate drop mark Y position */
        if (data->show_drop_mark) {
            struct MinList *children;
            Object *child;
            UWORD panel_index = 0;

            get(obj, MUIA_Group_ChildList, &children);
            if (children) {
                // Default to drop at the end
                data->drop_mark_y = _bottom(obj);
                APTR cstate = children->mlh_Head;
                while ((child = NextObject(&cstate))) {
                    if (panel_index == new_position) {
                        data->drop_mark_y = _bottom(child) + 1;
                        break;
                    }
                    panel_index++;
                }
            }
        }
        MUI_Redraw(obj, MADF_DRAWUPDATE);
    }

    return MUIV_DragReport_Refresh;
}

/*
 * Move dragged object parent using temporary all-child Group_Sort array and
 * stored drop position, then balance change and free array. Requires valid
 * handle/parent and child list. Always TRUE including no-op/allocation
 * failure; does not reorder tracking list or revalidate drag policy.
 */
IPTR PanelGroup__MUIM_DragDrop(struct IClass *cl, Object *obj, struct MUIP_DragDrop *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    LONG drop_pos;
    Object *panel;
    struct MinList *children;

    drop_pos = data->drop_position;
    panel = _parent(msg->obj);
    get(obj, MUIA_Group_ChildList, &children);

    if (!children || drop_pos < 0) {
        return TRUE;
    }

    /* Single pass optimization - find current position and build sorted array in one go */
    Object **sorted_objects;
    UWORD total_children = 0;
    LONG current_pos = -1;
    Object *child;
    APTR cstate;

    /* First, count total children and find current panel position */
    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate))) {
        if (child == panel) {
            current_pos = total_children;
        }
        total_children++;
    }

    /* Only proceed if panel was found and position changed */
    if (current_pos < 0 || drop_pos == current_pos) {
        return TRUE;
    }

    /* Allocate array for all children (plus method ID and NULL terminator) */
    sorted_objects = AllocVec(sizeof(Object *) * (total_children + 2), MEMF_CLEAR);
    if (!sorted_objects) {
        return TRUE;
    }

    /* Build reordered array */
    UWORD i = 1; /* Start at 1, index 0 is for method ID */
    UWORD src_index = 0;

    cstate = children->mlh_Head;
    while ((child = NextObject(&cstate))) {
        if (src_index == drop_pos) {
            /* Insert moved panel at drop position */
            sorted_objects[i++] = panel;
        }

        if (child != panel) {
            /* Add all other children in their current order */
            sorted_objects[i++] = child;
        }

        src_index++;
    }

    /* If drop position is at end, add moved panel last */
    if (drop_pos >= total_children) {
        sorted_objects[i++] = panel;
    }

    /* Apply the new order using MUIM_Group_Sort */
    sorted_objects[0] = (Object *) MUIM_Group_Sort;
    DoMethodA(obj, (struct Msg *) sorted_objects);

    FreeVec(sorted_objects);

    DoMethod(obj, MUIM_Group_InitChange);
    DoMethod(obj, MUIM_Group_ExitChange);

    return TRUE;
}

/*
 * Reset drop position/mark, request full redraw then delegate Group finish
 * and return result. Requires live instance and valid drag lifecycle; no
 * temporary array retained or ownership transfer.
 */
IPTR PanelGroup__MUIM_DragFinish(struct IClass *cl, Object *obj, struct MUIP_DragFinish *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);

    /* Clear drop state */
    data->drop_position = -1;
    data->show_drop_mark = FALSE;

    // clear all drawing artifacts
    MUI_Redraw(obj, MADF_DRAWALL);

    return DoSuperMethodA(cl, obj, (struct Msg *) msg);
}

/**************************************************************************
 MUIM_Draw - Draw the panelgroup with drop marks if needed
**************************************************************************/
/*
 * Draw optional dashed drop mark before Group drawing, saving/restoring
 * pen/mode/pattern, then return superclass result. Requires valid render
 * info even before RastPort test. Background/children may overdraw mark; no
 * local draw-flag filter or resource allocation.
 */
IPTR PanelGroup__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct PanelGroup_DATA *data = INST_DATA(cl, obj);
    IPTR result;

    /* Draw drop mark if showing */
    if (data->show_drop_mark) { // && (msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) {
        struct RastPort *rp = muiRenderInfo(obj)->mri_RastPort;
        if (rp) {
            UWORD old_pen = GetAPen(rp);
            UWORD old_drmd = GetDrMd(rp);
            UWORD old_drpt = rp->LinePtrn;

            /* Fallback if allocation failed */
            SetAPen(rp, _pens(obj)[MPEN_SHINE]);

            /* Set up drop mark appearance */
            SetDrMd(rp, JAM1);
            SetDrPt(rp, 0xAAAA); /* Dashed line pattern */

            /* Draw horizontal drop line */
            Move(rp, _mleft(obj), data->drop_mark_y);
            Draw(rp, _mright(obj), data->drop_mark_y);

            /* Draw small arrows at ends to make drop line more visible */
            Move(rp, _mleft(obj), data->drop_mark_y - 2);
            Draw(rp, _mleft(obj) + 4, data->drop_mark_y);
            Draw(rp, _mleft(obj), data->drop_mark_y + 2);

            Move(rp, _mright(obj), data->drop_mark_y - 2);
            Draw(rp, _mright(obj) - 4, data->drop_mark_y);
            Draw(rp, _mright(obj), data->drop_mark_y + 2);

            /* Restore original pen and draw mode */
            SetAPen(rp, old_pen);
            SetDrMd(rp, old_drmd);
            SetDrPt(rp, old_drpt);
        }
    }

    /* Call superclass to draw background and children*/
    result = DoSuperMethodA(cl, obj, (Msg) msg);

    return result;
}

/**************************************************************************
 Main dispatcher
**************************************************************************/
#if ZUNE_BUILTIN_PANELGROUP
/*
 * Dispatch ABI-correct local lifecycle and class messages to their handlers;
 * delegate remaining messages to the declared superclass. Requires live
 * object except construction and normal resource lifecycle; adds no
 * validation or synchronization.
 */
BOOPSI_DISPATCHER(IPTR, PanelGroup_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID) {
    case OM_NEW: return PanelGroup__OM_NEW(cl, obj, (struct opSet *) msg);
    case OM_DISPOSE: return PanelGroup__OM_DISPOSE(cl, obj, msg);
    case OM_SET: return PanelGroup__OM_SET(cl, obj, (struct opSet *) msg);
    case OM_GET: return PanelGroup__OM_GET(cl, obj, (struct opGet *) msg);
    case MUIM_Draw: return PanelGroup__MUIM_Draw(cl, obj, (struct MUIP_Draw *) msg);
    case MUIM_Group_InitChange: return PanelGroup__MUIM_Group_InitChange(cl, obj, msg);
    case MUIM_Group_ExitChange: return PanelGroup__MUIM_Group_ExitChange(cl, obj, msg);
    case MUIM_PanelGroup_AddPanel:
        return PanelGroup__MUIM_PanelGroup_AddPanel(cl, obj, (struct MUIP_PanelGroup_AddPanel *) msg);
    case MUIM_PanelGroup_RemovePanel:
        return PanelGroup__MUIM_PanelGroup_RemovePanel(cl, obj, (struct MUIP_PanelGroup_RemovePanel *) msg);
    case MUIM_PanelGroup_CollapsePanel:
        return PanelGroup__MUIM_PanelGroup_CollapsePanel(cl, obj, (struct MUIP_PanelGroup_CollapsePanel *) msg);
    case MUIM_PanelGroup_ExpandPanel:
        return PanelGroup__MUIM_PanelGroup_ExpandPanel(cl, obj, (struct MUIP_PanelGroup_ExpandPanel *) msg);
    case MUIM_PanelGroup_TogglePanel:
        return PanelGroup__MUIM_PanelGroup_TogglePanel(cl, obj, (struct MUIP_PanelGroup_TogglePanel *) msg);
    case MUIM_PanelGroup_GetPanelState:
        return PanelGroup__MUIM_PanelGroup_GetPanelState(cl, obj, (struct MUIP_PanelGroup_GetPanelState *) msg);
    case MUIM_PanelGroup_ScanPanels: return PanelGroup__MUIM_PanelGroup_ScanPanels(cl, obj, msg);
    case MUIM_DragQuery: return PanelGroup__MUIM_DragQuery(cl, obj, (struct MUIP_DragQuery *) msg);
    case MUIM_DragReport: return PanelGroup__MUIM_DragReport(cl, obj, (struct MUIP_DragReport *) msg);
    case MUIM_DragDrop: return PanelGroup__MUIM_DragDrop(cl, obj, (struct MUIP_DragDrop *) msg);
    case MUIM_DragFinish: return PanelGroup__MUIM_DragFinish(cl, obj, (struct MUIP_DragFinish *) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/**************************************************************************
 Class descriptor
**************************************************************************/
const struct __MUIBuiltinClass _MUI_PanelGroup_desc = { MUIC_PanelGroup,
                                                        MUIC_Group,
                                                        sizeof(struct PanelGroup_DATA),
                                                        (void *) PanelGroup_Dispatcher };
#endif /* ZUNE_BUILTIN_PANELGROUP */
