#ifndef _PANELGROUP_PRIVATE_H_
#define _PANELGROUP_PRIVATE_H_

#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

/*** Instance data **********************************************************/
struct PanelGroup_DATA
{
    BOOL  allow_multiple;      /* Allow multiple panels expanded simultaneously */
    Object *expanded_panel;    /* Currently expanded panel (if allow_multiple is TRUE) */

    /* Panel tracking */
    struct MinList panel_list; /* List of managed panels */
    ULONG panel_count;         /* Number of panels in the group */

    /* Runtime state */
    BOOL  layout_dirty;        /* Layout needs refresh */
    BOOL  states_current;      /* Panel states in tracking nodes are current */

    /* Drag and drop support */
    BOOL  drag_reordering;     /* Enable drag reordering of panels */

    LONG  drop_position;       /* Target drop position (-1 = no drop) */
    WORD  drop_mark_y;         /* Y coordinate of drop mark */
    BOOL  show_drop_mark;      /* Show visual drop indicator */
};

/* Panel tracking node */
struct PanelNode
{
    struct MinNode node;
    Object *panel;             /* The panel object */
    BOOL   collapsed;          /* Current state of this panel */
    BOOL   collapsible;        /* Whether this panel can be collapsed */
};

/* Internal method IDs */
#define MUIM_PanelGroup_AddPanel        (TAG_USER | 0x41000201)
#define MUIM_PanelGroup_RemovePanel     (TAG_USER | 0x41000202)

/* Internal method parameter structures */
struct MUIP_PanelGroup_AddPanel {
    STACKED ULONG MethodID;
    STACKED Object *panel;
};

struct MUIP_PanelGroup_RemovePanel {
    STACKED ULONG MethodID;
    STACKED Object *panel;
};



#endif /* _PANELGROUP_PRIVATE_H_ */
