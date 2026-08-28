/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction clicktab.gadget - Internal definitions
*/

#ifndef CLICKTAB_INTERN_H
#define CLICKTAB_INTERN_H

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <gadgets/clicktab.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include LC_LIBDEFS_FILE

#include <exec/libraries.h>

/* Module library base with stored class pointer */
struct ClickTabBase_intern
{
    struct Library lib;
    Class *rc_Class;
};

#define G(obj)  ((struct Gadget *)(obj))

/* ClickTabNode - internal representation of a single tab */
struct ClickTabNode
{
    struct MinNode      tn_Node;
    STRPTR              tn_Text;        /* TNA_Text */
    LONG                tn_Number;      /* TNA_Number */
    BOOL                tn_Disabled;    /* TNA_Disabled */
    BOOL                tn_CloseGadget; /* TNA_CloseGadget */
    Object              *tn_Image;      /* TNA_Image */
    Object              *tn_SelImage;   /* TNA_SelImage */
    APTR                tn_UserData;    /* TNA_UserData */
    STRPTR              tn_HintInfo;    /* TNA_HintInfo */
};

/* ClickTab gadget instance data */
struct ClickTabData
{
    struct List         *td_Labels;      /* List of ClickTabNode items */
    LONG                td_Current;      /* Currently selected tab index */
    LONG                td_NumTabs;      /* Total number of tabs */
    Object              *td_PageGroup;   /* Page group object */
    struct Node         *td_NodeClosed;  /* CLICKTAB_NodeClosed (get) */
    Object              *td_CloseImage;  /* CLICKTAB_CloseImage */
    Object              *td_FlagImage;   /* CLICKTAB_FlagImage */
    BOOL                td_LabelTruncate;/* CLICKTAB_LabelTruncate */
    BOOL                td_AutoFit;      /* CLICKTAB_AutoFit */
    BOOL                td_EvenSize;     /* CLICKTAB_EvenSize */

    /* Runtime state */
    LONG                td_HoverTab;     /* Tab under mouse (-1 = none) */
    UWORD               *td_TabWidths;   /* Cached tab pixel widths */
    UWORD               td_TabHeight;    /* Computed tab height */

    /* Internal labels list when caller supplies a NULL-terminated
       STRPTR[] via GA_Text. Retains ownership; freed in OM_DISPOSE. */
    struct List         *td_AutoLabels;
};

#endif /* CLICKTAB_INTERN_H */
