#ifndef GADGETS_AROSPALETTE_H
#   define GADGETS_AROSPALETTE_H

#ifndef LIBRARIES_GADTOOLS_H
#   include <libraries/gadtools.h>
#endif

#define AROSPALETTECLASS "palette.aros"
#define AROSPALETTENAME "Gadgets/arospalette.gadget"

/* [IS] (UBYTE) The number of colors to show specified by depth.
   Palette will show 2^depth colors */
#define AROSA_Palette_Depth		GTPA_Depth

/* [ISG] (UBYTE) Select color of palette */
#define AROSA_Palette_Color		GTPA_Color

/* [ISG] (UBYTE) First color to use in palette */ 
#define AROSA_Palette_ColorOffset	GTPA_ColorOffset

/* [I] (UWORD) Width of indicator box. If specified, color indicator
   box will be placed to the left of palette. */
#define AROSA_Palette_IndicatorWidth	GTPA_IndicatorWidth

/* [I] (UWORD) Height of indicator box. If specified, color indicator
   box will be placed on the top of palette. */
#define AROSA_Palette_IndicatorHeight	GTPA_IndicatorHeight

#define AROSA_Palette_NumColors		GTPA_NumColors

/* [ISG] */
#define AROSA_Palette_ColorTable	GTPA_ColorTable

#endif /* GADGETS_AROSPALETTE#_H */
