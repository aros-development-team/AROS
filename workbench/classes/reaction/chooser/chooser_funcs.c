/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction chooser.gadget - Exported node management functions
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <gadgets/chooser.h>
#include <utility/tagitem.h>

#include <string.h>

#include "chooser_intern.h"

/******************************************************************************/

static void choosernode_set(struct ChooserNode *cn, struct TagItem *tags)
{
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case CNA_Text:
                cn->cn_Text = (STRPTR)tag->ti_Data;
                break;
            case CNA_Image:
                cn->cn_Image = (Object *)tag->ti_Data;
                break;
            case CNA_Disabled:
                cn->cn_Disabled = (BOOL)tag->ti_Data;
                break;
            case CNA_Separator:
                cn->cn_Separator = (BOOL)tag->ti_Data;
                break;
            case CNA_UserData:
                cn->cn_UserData = (APTR)tag->ti_Data;
                break;
            case CNA_ReadOnly:
                cn->cn_ReadOnly = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/*****************************************************************************

    NAME */
#include <proto/chooser.h>

        AROS_LH1(struct Node *, AllocChooserNodeA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct Library *, ChooserBase, 6, Chooser)

/*  FUNCTION
        Allocates and initializes a ChooserNode for use with
        the chooser.gadget label list.

    INPUTS
        tags - TagItem array with CNA_* attributes.

    RESULT
        Pointer to the allocated node, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ChooserNode *cn;

    cn = (struct ChooserNode *)AllocVec(sizeof(struct ChooserNode),
                                        MEMF_PUBLIC | MEMF_CLEAR);
    if (cn)
    {
        choosernode_set(cn, tags);
    }

    return (struct Node *)cn;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, FreeChooserNode,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),

/*  LOCATION */
        struct Library *, ChooserBase, 7, Chooser)

/*  FUNCTION
        Frees a ChooserNode previously allocated with AllocChooserNodeA.

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
        AROS_LH2(void, SetChooserNodeAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, ChooserBase, 8, Chooser)

/*  FUNCTION
        Sets attributes on an existing ChooserNode.

    INPUTS
        node - The ChooserNode to modify.
        tags - TagItem array with CNA_* attributes.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (node)
    {
        choosernode_set((struct ChooserNode *)node, tags);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(void, GetChooserNodeAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, ChooserBase, 9, Chooser)

/*  FUNCTION
        Gets attributes from a ChooserNode. Each tag's ti_Data is
        a pointer to an IPTR that receives the attribute value.

    INPUTS
        node - The ChooserNode to query.
        tags - TagItem array with CNA_* attribute IDs. ti_Data
               points to storage for the returned value.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ChooserNode *cn;
    struct TagItem *tag;

    if (!node)
        return;

    cn = (struct ChooserNode *)node;

    while ((tag = NextTagItem(&tags)))
    {
        IPTR *store = (IPTR *)tag->ti_Data;
        if (!store)
            continue;

        switch (tag->ti_Tag)
        {
            case CNA_Text:
                *store = (IPTR)cn->cn_Text;
                break;
            case CNA_Image:
                *store = (IPTR)cn->cn_Image;
                break;
            case CNA_Disabled:
                *store = (IPTR)cn->cn_Disabled;
                break;
            case CNA_Separator:
                *store = (IPTR)cn->cn_Separator;
                break;
            case CNA_UserData:
                *store = (IPTR)cn->cn_UserData;
                break;
            case CNA_ReadOnly:
                *store = (IPTR)cn->cn_ReadOnly;
                break;
        }
    }

    AROS_LIBFUNC_EXIT
}
