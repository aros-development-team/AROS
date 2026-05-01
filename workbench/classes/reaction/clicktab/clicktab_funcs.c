/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction clicktab.gadget - Exported node management functions
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <gadgets/clicktab.h>
#include <utility/tagitem.h>

#include <string.h>

#include "clicktab_intern.h"

/******************************************************************************/

static void clicktabnode_set(struct ClickTabNode *tn, struct TagItem *tags)
{
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case TNA_Text:
                tn->tn_Text = (STRPTR)tag->ti_Data;
                break;
            case TNA_Number:
                tn->tn_Number = (LONG)tag->ti_Data;
                break;
            case TNA_Disabled:
                tn->tn_Disabled = (BOOL)tag->ti_Data;
                break;
            case TNA_Image:
                tn->tn_Image = (Object *)tag->ti_Data;
                break;
            case TNA_SelImage:
                tn->tn_SelImage = (Object *)tag->ti_Data;
                break;
            case TNA_UserData:
                tn->tn_UserData = (APTR)tag->ti_Data;
                break;
        }
    }
}

/*****************************************************************************

    NAME */
#include <proto/clicktab.h>

        AROS_LH1(struct Node *, AllocClickTabNodeA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct Library *, ClickTabBase, 6, ClickTab)

/*  FUNCTION
        Allocates and initializes a ClickTabNode for use with
        the clicktab.gadget label list.

    INPUTS
        tags - TagItem array with TNA_* attributes.

    RESULT
        Pointer to the allocated node, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ClickTabNode *tn;

    tn = (struct ClickTabNode *)AllocVec(sizeof(struct ClickTabNode),
                                         MEMF_PUBLIC | MEMF_CLEAR);
    if (tn)
    {
        clicktabnode_set(tn, tags);
    }

    return (struct Node *)tn;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeClickTabNode,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),

/*  LOCATION */
        struct Library *, ClickTabBase, 7, ClickTab)

/*  FUNCTION
        Frees a ClickTabNode previously allocated with AllocClickTabNodeA.

    INPUTS
        node - The node to free.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (node)
    {
        FreeVec(node);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, SetClickTabNodeAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, ClickTabBase, 8, ClickTab)

/*  FUNCTION
        Sets attributes on an existing ClickTabNode.

    INPUTS
        node - The ClickTabNode to modify.
        tags - TagItem array with TNA_* attributes.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (node)
    {
        clicktabnode_set((struct ClickTabNode *)node, tags);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, GetClickTabNodeAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, ClickTabBase, 9, ClickTab)

/*  FUNCTION
        Gets attributes from a ClickTabNode. Each tag's ti_Data is
        a pointer to an IPTR that receives the attribute value.

    INPUTS
        node - The ClickTabNode to query.
        tags - TagItem array with TNA_* attribute IDs. ti_Data
               points to storage for the returned value.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ClickTabNode *tn;
    struct TagItem *tag;

    if (!node)
        return;

    tn = (struct ClickTabNode *)node;

    while ((tag = NextTagItem(&tags)))
    {
        IPTR *store = (IPTR *)tag->ti_Data;
        if (!store)
            continue;

        switch (tag->ti_Tag)
        {
            case TNA_Text:
                *store = (IPTR)tn->tn_Text;
                break;
            case TNA_Number:
                *store = (IPTR)tn->tn_Number;
                break;
            case TNA_Disabled:
                *store = (IPTR)tn->tn_Disabled;
                break;
            case TNA_Image:
                *store = (IPTR)tn->tn_Image;
                break;
            case TNA_SelImage:
                *store = (IPTR)tn->tn_SelImage;
                break;
            case TNA_UserData:
                *store = (IPTR)tn->tn_UserData;
                break;
        }
    }

    AROS_LIBFUNC_EXIT
}
