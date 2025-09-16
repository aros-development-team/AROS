#ifndef _PANEL_PRIVATE_H_
#define _PANEL_PRIVATE_H_

#include <exec/types.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

/*
 * Panel
 */

/*** Instance data **********************************************************/
struct Panel_DATA {
    UWORD padding; /* Internal padding */
    Object *title_obj; /* PanelTitle object */

    /* Runtime state */
    BOOL layout_dirty; /* Layout needs refresh */
    ULONG expanded_width; /* Store width when expanded to preserve it when collapsed */
    ULONG expanded_height; /* Store height when expanded to preserve it when collapsed */

    /* Drag and drop support */
    BOOL drag_active; /* Currently being dragged */
    Object *drag_handle;
};

/*
 * PanelTitle
 */

/*** Private constants *****************************************************/
#define PANELTITLE_TEXT_PADDING         4    /* Padding around text within title area */
#define PANELTITLE_ARROW_WIDTH          6    /* Arrow width for collapsible titles */
#define PANELTITLE_ARROW_HEIGHT         6    /* Arrow height for collapsible titles */
#define PANELTITLE_ARROW_MARGIN         4    /* Margin around arrow */

/* Arrow direction constants */
#define PANELTITLE_ARROW_UP             0
#define PANELTITLE_ARROW_DOWN           1
#define PANELTITLE_ARROW_LEFT           2
#define PANELTITLE_ARROW_RIGHT          3

/*** Instance data **********************************************************/
struct PanelTitle_DATA
{
    STRPTR text;                    /* Title text */
    ULONG position;                 /* Title position (top, left) */
    ULONG text_position;            /* Text position within title area */
    BOOL vertical;                  /* Render text vertically */
    BOOL collapsible;               /* Allow toggling collapsed state */
    BOOL collapsed;                 /* Current collapsed state */
    BOOL show_separator;            /* Draw separator line */
    BOOL draw_state_indicator;      /* Draw collapsed or expanded arrow indicator */

    /* Event handling */
    struct MUI_EventHandlerNode ehn;         /* Event handler for clicks */
    struct Hook *click_hook;                 /* Hook to call when clicked */
    struct MUI_ImageSpec_intern *right_arrow_spec; /* Vector arrow spec */
    struct MUI_ImageSpec_intern *up_arrow_spec;    /* Vector arrow spec */
    struct MUI_ImageSpec_intern *down_arrow_spec;  /* Vector arrow spec */

    /* Rendering state */
    UWORD text_width;               /* Calculated text width */
    UWORD text_height;              /* Calculated text height */

    /* Layout cache */
    BOOL layout_valid;              /* Whether cached layout is valid */
    LONG arrow_left, arrow_top;     /* Arrow position cache */
};

#endif /* _PANEL_PRIVATE_H_ */
