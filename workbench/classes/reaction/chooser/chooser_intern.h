/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction chooser.gadget - Internal definitions
*/

#ifndef CHOOSER_INTERN_H
#define CHOOSER_INTERN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/chooser.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#define G(obj)  ((struct Gadget *)(obj))

/* ChooserNode - internal representation of a chooser list item */
struct ChooserNode
{
    struct MinNode      cn_Node;
    STRPTR              cn_Text;        /* CNA_Text */
    Object              *cn_Image;      /* CNA_Image */
    BOOL                cn_Disabled;    /* CNA_Disabled */
    BOOL                cn_Selected;    /* CNA_Selected */
    BOOL                cn_Separator;   /* CNA_Separator */
    BOOL                cn_ReadOnly;    /* CNA_ReadOnly */
    APTR                cn_UserData;    /* CNA_UserData */
};

/* Chooser gadget instance data */
struct ChooserData
{
    struct List         *cd_Labels;      /* List of ChooserNode items */
    LONG                cd_Selected;     /* Currently selected item index */
    LONG                cd_MaxLabels;    /* Maximum visible items */
    STRPTR              cd_Title;        /* Title string */

    BOOL                cd_DropDown;     /* Dropdown mode */
    BOOL                cd_AutoFit;      /* Auto-fit width */
    BOOL                cd_ReadOnly;     /* Read only mode */
    BOOL                cd_PopUp;        /* Popup mode */
    BOOL                cd_Hidden;       /* Hidden */

    BOOL                cd_Active;       /* Dropdown is open */
    LONG                cd_NumLabels;    /* Cached count of labels */
};

#endif /* CHOOSER_INTERN_H */
