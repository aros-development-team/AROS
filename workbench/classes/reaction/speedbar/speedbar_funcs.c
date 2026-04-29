/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction speedbar.gadget - Button node management functions
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <gadgets/speedbar.h>

#include "speedbar_intern.h"

/* Internal structure for speed button nodes */
struct SpeedButtonNode
{
    struct Node sbn_Node;
    UWORD       sbn_Number;
    Object     *sbn_Image;
    STRPTR      sbn_Text;
    ULONG       sbn_Flags;
    LONG        sbn_Disabled;
    LONG        sbn_Toggle;
    LONG        sbn_Selected;
};

#define SBNF_DISABLED  (1 << 0)
#define SBNF_TOGGLE    (1 << 1)
#define SBNF_SELECTED  (1 << 2)

/*****************************************************************************

    NAME */
#include <proto/speedbar.h>

        AROS_LH2(struct Node *, AllocSpeedButtonNodeA,

/*  SYNOPSIS */
        AROS_LHA(UWORD, number, D0),
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct Library *, SpeedBarBase, 6, SpeedBar)

/*  FUNCTION
        Allocates a speed button node for use in a SpeedBar gadget.

    INPUTS
        number - Button identifier number.
        tags   - Tag list controlling node attributes.

    RESULT
        Pointer to the allocated node, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SpeedButtonNode *node;
    struct TagItem *tag, *tstate;

    node = AllocVec(sizeof(struct SpeedButtonNode), MEMF_CLEAR | MEMF_PUBLIC);
    if (!node)
        return NULL;

    node->sbn_Node.ln_Type = NT_USER;
    node->sbn_Number = number;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case SBNA_Image:
                node->sbn_Image = (Object *)tag->ti_Data;
                break;

            case SBNA_Text:
                node->sbn_Text = (STRPTR)tag->ti_Data;
                node->sbn_Node.ln_Name = (STRPTR)tag->ti_Data;
                break;

            case SBNA_Disabled:
                node->sbn_Disabled = (LONG)tag->ti_Data;
                if (tag->ti_Data)
                    node->sbn_Flags |= SBNF_DISABLED;
                else
                    node->sbn_Flags &= ~SBNF_DISABLED;
                break;

            case SBNA_Toggle:
                node->sbn_Toggle = (LONG)tag->ti_Data;
                if (tag->ti_Data)
                    node->sbn_Flags |= SBNF_TOGGLE;
                else
                    node->sbn_Flags &= ~SBNF_TOGGLE;
                break;

            case SBNA_Selected:
                node->sbn_Selected = (LONG)tag->ti_Data;
                if (tag->ti_Data)
                    node->sbn_Flags |= SBNF_SELECTED;
                else
                    node->sbn_Flags &= ~SBNF_SELECTED;
                break;
        }
    }

    return &node->sbn_Node;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH1(void, FreeSpeedButtonNode,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),

/*  LOCATION */
        struct Library *, SpeedBarBase, 7, SpeedBar)

/*  FUNCTION
        Frees a speed button node allocated by AllocSpeedButtonNodeA().

    INPUTS
        node - Node to free, or NULL (safely ignored).

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

        AROS_LH2(void, SetSpeedButtonNodeAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, SpeedBarBase, 8, SpeedBar)

/*  FUNCTION
        Modifies attributes of a speed button node.

    INPUTS
        node - Node to modify.
        tags - Tag list with new attribute values.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SpeedButtonNode *sbn = (struct SpeedButtonNode *)node;
    struct TagItem *tag, *tstate;

    if (!node)
        return;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case SBNA_Image:
                sbn->sbn_Image = (Object *)tag->ti_Data;
                break;

            case SBNA_Text:
                sbn->sbn_Text = (STRPTR)tag->ti_Data;
                sbn->sbn_Node.ln_Name = (STRPTR)tag->ti_Data;
                break;

            case SBNA_Disabled:
                sbn->sbn_Disabled = (LONG)tag->ti_Data;
                if (tag->ti_Data)
                    sbn->sbn_Flags |= SBNF_DISABLED;
                else
                    sbn->sbn_Flags &= ~SBNF_DISABLED;
                break;

            case SBNA_Toggle:
                sbn->sbn_Toggle = (LONG)tag->ti_Data;
                if (tag->ti_Data)
                    sbn->sbn_Flags |= SBNF_TOGGLE;
                else
                    sbn->sbn_Flags &= ~SBNF_TOGGLE;
                break;

            case SBNA_Selected:
                sbn->sbn_Selected = (LONG)tag->ti_Data;
                if (tag->ti_Data)
                    sbn->sbn_Flags |= SBNF_SELECTED;
                else
                    sbn->sbn_Flags &= ~SBNF_SELECTED;
                break;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(void, GetSpeedButtonNodeAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, SpeedBarBase, 9, SpeedBar)

/*  FUNCTION
        Retrieves attributes from a speed button node.

    INPUTS
        node - Node to query.
        tags - Tag list with ti_Data pointing to storage for each value.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct SpeedButtonNode *sbn = (struct SpeedButtonNode *)node;
    struct TagItem *tag, *tstate;

    if (!node)
        return;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case SBNA_Image:
                *(Object **)tag->ti_Data = sbn->sbn_Image;
                break;

            case SBNA_Text:
                *(STRPTR *)tag->ti_Data = sbn->sbn_Text;
                break;

            case SBNA_Disabled:
                *(LONG *)tag->ti_Data = sbn->sbn_Disabled;
                break;

            case SBNA_Toggle:
                *(LONG *)tag->ti_Data = sbn->sbn_Toggle;
                break;

            case SBNA_Selected:
                *(LONG *)tag->ti_Data = sbn->sbn_Selected;
                break;
        }
    }

    AROS_LIBFUNC_EXIT
}
