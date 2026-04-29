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

#define PALETTE_Color           (PALETTE_Dummy + 0x0001) /* Selected color index */
#define PALETTE_ColorOffset     (PALETTE_Dummy + 0x0002) /* First color offset */
#define PALETTE_NumColors       (PALETTE_Dummy + 0x0003) /* Colors to display */
#define PALETTE_ColorsPerRow    (PALETTE_Dummy + 0x0004) /* Grid columns */

#ifndef PaletteObject
#define PaletteObject   NewObject(NULL, PALETTE_CLASSNAME
#endif
#ifndef PaletteEnd
#define PaletteEnd      TAG_END)
#endif

#endif /* GADGETS_PALETTE_H */
