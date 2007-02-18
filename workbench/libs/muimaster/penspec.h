/*
    Copyright  2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef MUI_PENSPEC_H
#define MUI_PENSPEC_H

#ifndef LIBRARIES_MUI_H
#include "mui.h"
#endif

struct MUI_PenSpec_intern
{
    PenSpecType p_type;
    struct MUI_RenderInfo *p_mri;
    ULONG       p_pen;  /* actual graphics pen, only valid between setup/cleanup */
    BOOL        p_is_allocated;
    union {
	LONG                p_mui;
	LONG                p_cmap;
	LONG                p_sys;
	struct MUI_RGBcolor p_rgb;
    } u;
};

#define p_rgb u.p_rgb
#define p_mui u.p_mui
#define p_cmap u.p_cmap
#define p_sys u.p_sys

/* From ASCII to internal representation */
BOOL zune_pen_spec_to_intern (const struct MUI_PenSpec *spec,
			      struct MUI_PenSpec_intern *intern);
BOOL zune_pen_string_to_intern (CONST_STRPTR spec,
				struct MUI_PenSpec_intern *intern);
/* From internal representation to ASCII */
BOOL zune_pen_intern_to_spec (const struct MUI_PenSpec_intern *intern,
			      struct MUI_PenSpec *spec);
void zune_penspec_fill_muipen(struct MUI_PenSpec_intern *psi, LONG muipen);
void zune_penspec_fill_rgb(struct MUI_PenSpec_intern *psi, ULONG r, ULONG g, ULONG b);

BOOL zune_penspec_setup(struct MUI_PenSpec_intern *pen, struct MUI_RenderInfo *mri);
BOOL zune_penspec_cleanup(struct MUI_PenSpec_intern *pen);

void zune_penspec_draw(struct MUI_PenSpec_intern *psi, struct MUI_RenderInfo *mri,
		       LONG left, LONG top, LONG right, LONG bottom);
void zune_penspec_drawdirect(struct MUI_PenSpec_intern *psi, struct RastPort *rp, struct MUI_RenderInfo *mri,
		       LONG left, LONG top, LONG right, LONG bottom);

#endif /* MUI_PENSPEC_H */
