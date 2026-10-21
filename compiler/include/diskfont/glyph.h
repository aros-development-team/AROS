#ifndef DISKFONT_GLYPH_H
#define DISKFONT_GLYPH_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

/*
 * 32-bit color glyph map for outline engines that can expose color bitmap
 * strikes, such as CBDT/CBLC emoji fonts. cgm_BMModulo is in bytes and each
 * pixel stores ARGB component bytes in the format WritePixelArrayAlpha()
 * consumes.
 */
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

