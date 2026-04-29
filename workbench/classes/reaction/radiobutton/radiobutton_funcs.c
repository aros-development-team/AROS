/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction radiobutton.gadget - Node management functions
*/

#include <proto/exec.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <utility/tagitem.h>
#include <gadgets/radiobutton.h>

#include "radiobutton_intern.h"

/* Internal node structure for radiobutton entries */
struct RadioButtonNode
{
    struct Node rbn_Node;
    STRPTR      rbn_Text;
    ULONG       rbn_Flags;
    LONG        rbn_Selected;
    LONG        rbn_Disabled;
};

#define RBNF_DISABLED  (1 << 0)
#define RBNF_SELECTED  (1 << 1)

/*****************************************************************************

    NAME */
#include <proto/radiobutton.h>

        AROS_LH2(struct Node *, AllocRadioButtonNodeA,

/*  SYNOPSIS */
        AROS_LHA(UWORD, columns, D0),
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct Library *, RadioButtonBase, 6, RadioButton)

/*  FUNCTION
        Allocates a radio button list node.

    INPUTS
        columns - Reserved, pass 0.
        tags    - Tag list controlling node attributes.

    RESULT
        Pointer to the allocated node, or NULL on failure.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RadioButtonNode *node;
    struct TagItem *tag, *tstate;

    node = AllocVec(sizeof(struct RadioButtonNode), MEMF_CLEAR | MEMF_PUBLIC);
    if (!node)
        return NULL;

    node->rbn_Node.ln_Type = NT_USER;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case RBNA_Labels:
                node->rbn_Text = (STRPTR)tag->ti_Data;
                node->rbn_Node.ln_Name = (STRPTR)tag->ti_Data;
                break;

            case RBNA_Disabled:
                node->rbn_Disabled = (LONG)tag->ti_Data;
                if (tag->ti_Data)
                    node->rbn_Flags |= RBNF_DISABLED;
                else
                    node->rbn_Flags &= ~RBNF_DISABLED;
                break;
        }
    }

    return &node->rbn_Node;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH1(void, FreeRadioButtonNode,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),

/*  LOCATION */
        struct Library *, RadioButtonBase, 7, RadioButton)

/*  FUNCTION
        Frees a radio button node allocated by AllocRadioButtonNodeA().

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

        AROS_LH2(void, SetRadioButtonNodeAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, RadioButtonBase, 8, RadioButton)

/*  FUNCTION
        Modifies attributes of a radio button node.

    INPUTS
        node - Node to modify.
        tags - Tag list with new attribute values.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RadioButtonNode *rbn = (struct RadioButtonNode *)node;
    struct TagItem *tag, *tstate;

    if (!node)
        return;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case RBNA_Labels:
                rbn->rbn_Text = (STRPTR)tag->ti_Data;
                rbn->rbn_Node.ln_Name = (STRPTR)tag->ti_Data;
                break;

            case RBNA_Disabled:
                rbn->rbn_Disabled = (LONG)tag->ti_Data;
                if (tag->ti_Data)
                    rbn->rbn_Flags |= RBNF_DISABLED;
                else
                    rbn->rbn_Flags &= ~RBNF_DISABLED;
                break;
        }
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

        AROS_LH2(void, GetRadioButtonNodeAttrsA,

/*  SYNOPSIS */
        AROS_LHA(struct Node *, node, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, RadioButtonBase, 9, RadioButton)

/*  FUNCTION
        Retrieves attributes from a radio button node.

    INPUTS
        node - Node to query.
        tags - Tag list with ti_Data pointing to storage for each value.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct RadioButtonNode *rbn = (struct RadioButtonNode *)node;
    struct TagItem *tag, *tstate;

    if (!node)
        return;

    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
            case RBNA_Labels:
                *(STRPTR *)tag->ti_Data = rbn->rbn_Text;
                break;

            case RBNA_Disabled:
                *(LONG *)tag->ti_Data = rbn->rbn_Disabled;
                break;
        }
    }

    AROS_LIBFUNC_EXIT
}
