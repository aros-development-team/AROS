/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible images/glyph.h
*/

#ifndef IMAGES_GLYPH_H
#define IMAGES_GLYPH_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif

#define GLYPH_CLASSNAME     "glyph.image"
#define GLYPH_VERSION       44

#define GLYPH_Dummy         (REACTION_Dummy + 0x15000)

#define GLYPH_Glyph        (GLYPH_Dummy+1) /* (WORD) Glyph type, see below */
#define GLYPH_DrawInfo      (GLYPH_Dummy+2) /* Obsolete - do not use */

/* Glyph type constants for GLYPH_Glyph */
#define GLYPH_NONE          0   /* No glyph */
#define GLYPH_DOWNARROW     1   /* Down arrow */
#define GLYPH_UPARROW       2   /* Up arrow */
#define GLYPH_LEFTARROW     3   /* Left arrow */
#define GLYPH_RIGHTARROW    4   /* Right arrow */
#define GLYPH_DROPDOWN      5   /* Dropdown indicator */
#define GLYPH_POPUP         6   /* Popup indicator */
#define GLYPH_CHECKMARK     7   /* Checkmark */
#define GLYPH_POPFONT       8   /* Font popup */
#define GLYPH_POPFILE       9   /* File popup */
#define GLYPH_POPDRAWER     10  /* Drawer popup */
#define GLYPH_POPSCREENMODE 11  /* Screenmode popup */
#define GLYPH_POPTIME       12  /* Time popup */
#define GLYPH_RADIOBUTTON   18  /* Radio button */
#define GLYPH_RETURNARROW   20  /* Return arrow */
#define GLYPH_BDOWNARROW    21  /* Bold down arrow */
#define GLYPH_BUPARROW      22  /* Bold up arrow */
#define GLYPH_BLEFTARROW    23  /* Bold left arrow */
#define GLYPH_BRIGHTARROW   24  /* Bold right arrow */
#define GLYPH_DROPDOWNMENU  25  /* Dropdown menu indicator */
#define GLYPH_CYCLE         26  /* Cycle indicator */

#ifndef GlyphObject
#define GlyphObject     NewObject(NULL, GLYPH_CLASSNAME
#endif
#ifndef GlyphEnd
#define GlyphEnd        TAG_END)
#endif

#endif /* IMAGES_GLYPH_H */
