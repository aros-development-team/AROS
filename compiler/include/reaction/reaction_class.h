/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible reaction/reaction_class.h
          Definitions for Reaction class authors
*/

#ifndef REACTION_REACTION_CLASS_H
#define REACTION_REACTION_CLASS_H

#ifndef INTUITION_CGHOOKS_H
#include <intuition/cghooks.h>
#endif

/* PRIVATE */
struct SpecialPens
{
    WORD sp_Version;    /* structure version (currently 0) */
    LONG sp_DarkPen;    /* dark pen for extended bevel rendering */
    LONG sp_LightPen;   /* light pen for extended bevel rendering */
    /* this structure may be extended in future versions */
};

/*****************************************************************************
 * Custom method supported by some Reaction gadgets
 */
#define GM_CLIPRECT  (0x550001L)

/* Passes a clipping rectangle to a gadget for correct rendering
 * within virtual groups (e.g. scrolling layout containers).
 */

struct gpClipRect
{
    ULONG                MethodID;       /* GM_CLIPRECT */
    struct GadgetInfo   *gpc_GInfo;      /* gadget context */
    struct Rectangle    *gpc_ClipRect;   /* clip bounds */
    ULONG                gpc_Flags;      /* reserved flags */
};

/* GM_CLIPRECT return values */
#define GMC_VISIBLE         2  /* gadget fully inside clip */
#define GMC_PARTIAL         1  /* gadget partially clipped */
#define GMC_INVISIBLE       0  /* gadget entirely outside clip */

/**************************************************************************/

#endif /* REACTION_REACTION_CLASS_H */
