/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

    Desc: Graphics function GetRGB32()
    Lang: english
*/
#include <graphics/view.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH4(void, GetRGB32,

/*  SYNOPSIS */
	AROS_LHA(struct ColorMap *, cm        , A0),
	AROS_LHA(ULONG            , firstcolor, D0),
	AROS_LHA(ULONG            , ncolors   , D1),
	AROS_LHA(ULONG *          , table     , A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 150, Graphics)

/*  FUNCTION
	Fill the table with with the 32 bit fractional RGB values from the
	given colormap.

    INPUTS
	cm         - ColorMap structure obtained via GetColorMap()
	firstcolor - the index of first color register to get (starting with 0)
	ncolors    - the number of color registers to examine and write
	             into the table
	table      - a pointer to an array of 32 bit RGB triplets

    RESULT
	-1 	: if no valid entry. (index to high)
	other	: UWORD RGB value, 4 bits per electron gun, right justified

    NOTES
	table should point to an array of at least 3*ncolors longwords.

    EXAMPLE

    BUGS

    SEE ALSO
	GetColorMap() FreeColorMap() SetRGB4() LoadRGB4()
	LoadRGB32() SetRGB32CM() graphics/view.h

    INTERNALS
	This function depends on the ColorMap->ColorTable structure

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  LONG LUT[] = {0x00000000,0x11111111,0x22222222,0x33333333,
		0x44444444,0x55555555,0x66666666,0x77777777,
		0x88888888,0x99999999,0xaaaaaaaa,0xbbbbbbbb,
		0xcccccccc,0xdddddddd,0xeeeeeeee,0xffffffff};

  ULONG i,n;

  n = 0;
  for (i=firstcolor; i< (ncolors+firstcolor); i++ )
  {
    WORD RGBValue = GetRGB4(cm, i);
    table[n++] = LUT[(RGBValue >> 8) & 0xf];
    table[n++] = LUT[(RGBValue >> 4) & 0xf];
    table[n++] = LUT[ RGBValue       & 0xf];
  }

  AROS_LIBFUNC_EXIT
} /* GetRGB32 */
