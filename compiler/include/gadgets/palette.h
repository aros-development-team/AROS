/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/palette.h
*/

#ifndef GADGETS_PALETTE_H
#define GADGETS_PALETTE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef REACTION_REACTION_H
#include <reaction/reaction.h>
#endif

#define PALETTE_CLASSNAME   "palette.gadget"
#define PALETTE_VERSION     44

#define PALETTE_Dummy       (REACTION_Dummy + 0x0004000)

#define PALETTE_Colour          (PALETTE_Dummy + 1)  /* Selected colour index */
#define PALETTE_ColourOffset    (PALETTE_Dummy + 2)  /* First colour pen offset */
#define PALETTE_ColourTable     (PALETTE_Dummy + 3)  /* Colour remap table (UWORD *) */
#define PALETTE_NumColours      (PALETTE_Dummy + 4)  /* Number of colours to display */

/* American spelling aliases */
#define PALETTE_Color           PALETTE_Colour
#define PALETTE_ColorOffset     PALETTE_ColourOffset
#define PALETTE_ColorTable      PALETTE_ColourTable
#define PALETTE_NumColors       PALETTE_NumColours

#ifndef PaletteObject
#define PaletteObject   NewObject(NULL, PALETTE_CLASSNAME
#endif
#ifndef PaletteEnd
#define PaletteEnd      TAG_END)
#endif

#endif /* GADGETS_PALETTE_H */
