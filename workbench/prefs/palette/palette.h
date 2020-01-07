/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _PEPALETTE_H
#define _PEPALETTE_H

/*** Identifier base ********************************************************/
#define MUIB_PEPalette                  (TAG_USER | 0x11000000)

#define MUIA_PEPalette_Pens             (MUIB_PEPalette + 1)

extern struct MUI_CustomClass *PEPalette_CLASS;
#define PEPaletteObject BOOPSIOBJMACRO_START(PEPalette_CLASS->mcc_Class)

#endif

