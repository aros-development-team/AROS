/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __ZUNE_PEN_H__
#define __ZUNE_PEN_H__

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#include <gdk/gdktypes.h>

struct MUI_RenderInfo;
struct MUI_PenSpec;

/* MUI's draw pens */

typedef enum {
    MPEN_SHINE      = 0,
    MPEN_HALFSHINE  = 1,
    MPEN_BACKGROUND = 2,
    MPEN_HALFSHADOW = 3,
    MPEN_SHADOW     = 4,
    MPEN_TEXT       = 5,
    MPEN_FILL       = 6,
    MPEN_MARK       = 7,
    MPEN_COUNT      = 8,
} MPen;

typedef struct {
    MPen fg;
    MPen bg;
} MPenCouple;

typedef enum {
    PST_MUI,
    PST_CMAP,
    PST_RGB,
} PenSpecType;

struct MUI_PenSpec {
    union {
	char buf[32]; /* constraint PenSpec size */
	struct {
	    PenSpecType ps_Type;
	    union {
		MPen     mui;   /* MUI pen number */
		ULONG    cmap;  /* colormap entry */
		struct {
		    GdkColor rgb;
		    STRPTR   text;
		} c;
	    } v;
	} s;
    } u;
};

#define ps_penType u.s.ps_Type
#define ps_rgbColor u.s.v.c.rgb
#define ps_rgbText u.s.v.c.text
#define ps_mui u.s.v.mui
#define ps_cmap u.s.v.cmap

#ifndef _AROS

/* set drawing color with the result from MUI_ObtainPen */
void
SetAPen (GdkGC *gc, gulong pen);

void
SetBPen (GdkGC *gc, gulong pen);

#endif

/* dump a penspec */
/* as usual, returns ptr to static storage */
STRPTR
zune_penspec_to_string(struct MUI_PenSpec *spec);

/* create a penspec */
/* it's legal to pass a NULL str param here */
void
zune_string_to_penspec(STRPTR str, struct MUI_PenSpec *spec);

/* dispose pen content, matching zune_string_to_penspec */
void zune_penspec_destroy_content (struct MUI_PenSpec *spec);

/* prepare penspec for use with a given mri (window) */
void
zune_penspec_setup (struct MUI_RenderInfo *mri, struct MUI_PenSpec *spec);

void
zune_penspec_cleanup (struct MUI_RenderInfo *mri, struct MUI_PenSpec *spec);

/* gdk color stream IO */

STRPTR
zune_gdkcolor_to_string(GdkColor *rgb);

void
zune_string_to_gdkcolor (STRPTR str, GdkColor *rgb);

#endif
