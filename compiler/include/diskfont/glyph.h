#ifndef DISKFONT_GLYPH_H
#define DISKFONT_GLYPH_H

/*
    Copyright (C) 2001 AROS - The Amiga Research OS
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

struct GlyphWidthEntry
{
    struct MinNode  gwe_Node;
    UWORD   	    gwe_Code;
    FIXED   	    gwe_Width;
};

#endif /* DISKFONT_GLYPH_H */

