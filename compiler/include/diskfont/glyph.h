#ifndef DISKFONT_GLYPH_H
#define DISKFONT_GLYPH_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Structures for font engine libraries
    Lang: english
*/

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef  EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef  EXEC_NODES_H
#   include <exec/nodes.h>
#endif

struct GlyphEngine
{
    struct Library  *gle_Library;
    char    	    *gle_Name;
};

typedef LONG FIXED;

struct GlyphMap
{
    UWORD   glm_BMModulo;
    UWORD   glm_BMRows;
    UWORD   glm_BlackLeft;
    UWORD   glm_BlackTop;
    UWORD   glm_BlackWidth;
    UWORD   glm_BlackHeight;
    FIXED   glm_XOrigin;
    FIXED   glm_YOrigin;
    WORD    glm_X0;
    WORD    glm_Y0;
    WORD    glm_X1;
    WORD    glm_Y1;
    FIXED   glm_Width;
    UBYTE  *glm_BitMap;
};

/* A glyph in colour (OT_ColorGlyphMap32): the fields mean the same as the
   GlyphMap's. cgm_BMModulo is in bytes; each pixel is one ULONG of A, R, G,
   B bytes in memory order, as WritePixelArrayAlpha() takes them. Bitmap
   strikes (CBDT, sbix) and COLR v0 layers are rendered; COLR v1 is not.
   A strike's line height is the point size, as an outline font's height
   is; OT_ShearSin and OT_RotateSin apply to outlines only, so a strike
   stays upright. */
struct ColorGlyphMap
{
    ULONG   cgm_BMModulo;
    UWORD   cgm_BMRows;
    UWORD   cgm_BlackLeft;
    UWORD   cgm_BlackTop;
    UWORD   cgm_BlackWidth;
    UWORD   cgm_BlackHeight;
    FIXED   cgm_XOrigin;
    FIXED   cgm_YOrigin;
    WORD    cgm_X0;
    WORD    cgm_Y0;
    WORD    cgm_X1;
    WORD    cgm_Y1;
    FIXED   cgm_Width;
    ULONG  *cgm_ARGBMap;
};

struct GlyphWidthEntry
{
    struct MinNode  gwe_Node;
    UWORD   	    gwe_Code;
    FIXED   	    gwe_Width;
};

#endif /* DISKFONT_GLYPH_H */

