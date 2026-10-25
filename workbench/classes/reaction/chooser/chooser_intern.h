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

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct ChooserBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

/* ChooserNode - internal representation of a chooser list item */
struct ChooserNode
{
    struct MinNode      cn_Node;
    STRPTR              cn_Text;        /* CNA_Text */
    Object              *cn_Image;      /* CNA_Image */
    BOOL                cn_Disabled;    /* CNA_Disabled */
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
    BOOL                cd_PopUp;        /* Popup mode */
    BOOL                cd_Hidden;       /* Hidden */

    BOOL                cd_Active;       /* Dropdown is open */
    LONG                cd_NumLabels;    /* Cached count of labels */

    struct Window       *cd_PopWin;      /* the open list, or NULL */
    LONG                cd_Hot;          /* highlighted entry, -1 none */
    WORD                cd_ItemH;        /* height of one entry */
    BOOL                cd_Sticky;       /* opened by a click: the next click picks */
    BOOL                cd_ShowAt;       /* ShowChooser(): open at cd_ShowX/Y */
    WORD                cd_ShowX, cd_ShowY;
    BOOL                cd_HideReq;      /* HideChooser(): leave on the next input */
    struct List         *cd_OwnLabels;   /* list built from CHOOSER_LabelArray */
};

void chooser_popup_close(struct ChooserData *data);

#endif /* CHOOSER_INTERN_H */
