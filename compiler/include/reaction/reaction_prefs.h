/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible reaction/reaction_prefs.h
          Reaction preferences definitions
*/

#ifndef REACTION_REACTION_PREFS_H
#define REACTION_REACTION_PREFS_H

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

/* Obtain this semaphore while reading the preferences */
#define RAPREFSSEMAPHORE "REACTION-PREFS"

/* This structure is subject to change and is primarily intended
 * for class authors implementing preferences support.
 */

#define ClassActPrefs UIPrefs

struct UIPrefs
{
    struct SignalSemaphore   cap_Semaphore;
    UWORD                    cap_PrefsVersion;   /* set to 1 */
    UWORD                    cap_PrefsSize;      /* size of this struct */
    UBYTE                    cap_BevelType;      /* bevel style (BVT_*) */
    UWORD                    cap_LayoutSpacing;  /* default inter-element gap */
    BOOL                     cap_3DLook;         /* enable 3D appearance */
    UWORD                    cap_LabelPen;       /* pen for label text */
    UBYTE                    cap_LabelPlace;     /* label placement */
    UBYTE                    cap_3DLabel;        /* 3D label rendering */
    ULONG                   *cap_Reserved1;
    BOOL                     cap_SimpleRefresh;  /* use simple-refresh windows */
    UBYTE                    cap_Pattern[256];   /* backfill pattern name */
    ULONG                   *cap_Reserved2;

    BOOL                     cap_3DProp;         /* 3D proportional gadgets */
    BOOL                     cap_Reserved3;

    UBYTE                    cap_GlyphType;      /* glyph style (GLT_*) */
    UBYTE                    cap_Reserved4;

    struct TextAttr         *cap_FallbackAttr;   /* fallback font */
    struct TextAttr         *cap_LabelAttr;      /* label font */
};

/* Bevel types */
#define BVT_GT      0   /* GadTools-style 2:1 bevel */
#define BVT_THIN    1   /* thin 1:1 bevel */
#define BVT_THICK   2   /* 4-colour thick bevel */
#define BVT_XEN     3   /* Xen-inspired 4-colour bevel */
#define BVT_XENTHIN 4   /* Xen-inspired thin 1:1 bevel */

/* Glyph types */
#define GLT_GT    0     /* GadTools style */
#define GLT_FLAT  1     /* flat style */
#define GLT_3D    2     /* 3D style */

#endif /* REACTION_REACTION_PREFS_H */
