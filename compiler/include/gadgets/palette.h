/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible gadgets/palette.h
*/

#ifndef GADGETS_PALETTE_H
#define GADGETS_PALETTE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define PALETTE_CLASSNAME   "gadgets/palette.gadget"
#define PALETTE_VERSION     44

#define PALETTE_Dummy       (TAG_USER + 0xD0000)

#define PALETTE_Color           (PALETTE_Dummy + 0x0001)
#define PALETTE_ColorOffset     (PALETTE_Dummy + 0x0002)
#define PALETTE_NumColors       (PALETTE_Dummy + 0x0003)
#define PALETTE_ColorsPerRow    (PALETTE_Dummy + 0x0004)

#define PaletteObject   NewObject(NULL, PALETTE_CLASSNAME
#define PaletteEnd      TAG_END)

#endif /* GADGETS_PALETTE_H */
