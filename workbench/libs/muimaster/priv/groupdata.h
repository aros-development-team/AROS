#ifndef __GROUPDATA_H__
#define __GROUPDATA_H__

#include <areadata.h>

struct MUI_GroupData
{
    Object      *family;
    struct Hook *layout_hook;
    ULONG        flags;
#define GROUP_HORIZ       (1<<1)
#define GROUP_SAME_WIDTH  (1<<2)
#define GROUP_SAME_HEIGHT (1<<3)
#define GROUP_CHANGING    (1<<4)
#define GROUP_PAGEMODE    (1<<5)
    ULONG        columns;
    ULONG        rows;
    LONG         active_page;
    ULONG        horiz_spacing;
    ULONG        vert_spacing;
    ULONG        num_childs;
    ULONG        horiz_weight_sum;
    ULONG        vert_weight_sum;
};


/* Hook message for custom layout */

struct MUI_LayoutMsg
{
    ULONG                  lm_Type;     /* type of message (see defines below)    */
    struct MinList        *lm_Children; /* list of this groups children */
    struct MUI_MinMax      lm_MinMax;   /* results for MUILM_MINMAX               */
    struct
    {
	LONG Width;
	LONG Height;
	ULONG priv5;
	ULONG priv6;
    } lm_Layout;   /* size (and result) for MUILM_LAYOUT                       */
};

#define MUILM_MINMAX    1  /* MUI wants you to calc your min & max sizes */
#define MUILM_LAYOUT    2  /* MUI wants you to layout your children      */

#define MUILM_UNKNOWN  -1  /* return this if your hook doesn't implement lm_Type */

#endif
