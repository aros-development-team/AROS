/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function BitMapScale()
    Lang: english
*/
#include <graphics/scale.h>
#include <proto/exec.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, BitMapScale,

/*  SYNOPSIS */
	AROS_LHA(struct BitScaleArgs *, bitScaleArgs, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 113, Graphics)

/*  FUNCTION
	Scale a source bit map to a destination bit map other than
	the source bit map.

    INPUTS
	Pass a BitScaleArgs structure filled with the following args
	to this function:
	  bsa_SrcX, bsa_SrcY - upper left coordinate in source bitmap
	  bsa_SrcWidth, bsa_SrcHeight - Width and Height or source bitmap
	  bsa_DestX, bsa_DestY - upper left coordinate in destination
	                         bitmap
	  bsa_DestWidth, bsa_DestHeight - this function will set these
	        values. Use the bsa_???Factor for scaling
	  bsa_XSrcFactor:bsa_XDestFactor - Set these to get approximately
                the same ratio as bsa_XSrcWidth:bsa_XDestWidth, but
	        usually not exactly the same number.
	  bsa_YSrcFactor:bsa_YDestFactor - Set these to get approximately
	        the same ratio as bsa_YSrcWidth:YDestWidth, but
	        usually not exactly the same number.
	  bsa_SrcBitMap - pointer to source bitmap to be scaled
	  bsa_DestBitMap - pointer to destination bitmap which will
	                   will hold the scaled bitmap. Make sure it's
	                   big enough!
	  bsa_Flags - reserved for future use. Set it to zero!
	  bsa_XDDA, bsa_YDDA - for future use.
	  bsa_Reserved1, bsa_Reserved2 - for future use.

    RESULT
	  bsa_DestWidth and bsa_DestHeight will be set by this function

    NOTES
	- Overlapping source and destination bitmaps are not supported
	- Make sure that you provide enough memory for the destination
	  bitmap to hold the result
	- In the destination bitmap only the area where the scaled
	  source bitmap is put into is changed. A frame of the old
	  bitmap is left.

    EXAMPLE

    BUGS

    SEE ALSO
	ScalerDiv() graphics/scales.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)


  if (bitScaleArgs->bsa_SrcBitMap->Pad  != 0  || 
      bitScaleArgs->bsa_DestBitMap->Pad != 0 || 
      bitScaleArgs->bsa_SrcBitMap->Flags  & BMF_AROS_HIDD || 
      bitScaleArgs->bsa_DestBitMap->Flags & BMF_AROS_HIDD)
  {
    driver_BitMapScale(bitScaleArgs, GfxBase);
  }
  else
  {
    /*
     * Algorithm for plain Amiga bitmaps.
     */
    /* 
     * Unfortunately it's not possible to use 16/32 bit copying on bitmaps with this
     * algorithm as there might be an odd number of bits per line in a bitmap and
     * this creates problems when accessing the 2nd, 4th and so on line.
     */

  /*
    #define DEF_USIZE ULONG
    #define DEF_SIZE LONG
    #define DEF_NUMBITSMINUS1 31
    #define DEF_MASK 31
    #define DEF_READMASK 0x80000000
    #define DEF_SHIFTY 2
    #define DEF_SHIFTX 5
  */

  /*  The following lines are necessary for BYTE copying and have to be used right
   *  now!!
   */

    #define DEF_USIZE UBYTE
    #define DEF_SIZE BYTE
    #define DEF_NUMBITSMINUS1 7
    #define DEF_ANDMASK 7
    #define DEF_READMASK 0x80
    #define DEF_SHIFTY 0
    #define DEF_SHIFTX 3

    UWORD * LinePattern;
    bitScaleArgs -> bsa_DestWidth = ScalerDiv(bitScaleArgs -> bsa_SrcWidth,
                                              bitScaleArgs -> bsa_XDestFactor,
                                              bitScaleArgs -> bsa_XSrcFactor);

    bitScaleArgs -> bsa_DestHeight= ScalerDiv(bitScaleArgs -> bsa_SrcHeight,
                                              bitScaleArgs -> bsa_YDestFactor,
                                              bitScaleArgs -> bsa_YSrcFactor);

    /* first of all lets allocate DestHeight words of memory so we can
       precalculate which original line goes to which destination lines */

    if (NULL ==(LinePattern = (UWORD *) AllocMem(2*bitScaleArgs ->bsa_DestHeight, 0)))
      return;

    {
      UWORD DestHeight = bitScaleArgs -> bsa_DestHeight;
      UWORD ys = bitScaleArgs -> bsa_SrcY;
      ULONG count = 0;
      ULONG dyd = bitScaleArgs -> bsa_DestHeight;
      ULONG dys = bitScaleArgs -> bsa_SrcHeight;
      LONG accuys = dyd;
      LONG accuyd = - (dys >> 1);
      while (count < DestHeight)
      {
        accuyd += dys;
        while (accuyd > accuys )
        {
          ys++;
          accuys += dyd;
        }
        LinePattern[count] = ys;
        count++;
      }
    }



    /* now let's go for the real thing: scaling */
    {
      UWORD DestWidth = bitScaleArgs -> bsa_DestWidth + bitScaleArgs -> bsa_DestX;
      ULONG xs = bitScaleArgs -> bsa_SrcX;
      ULONG count = bitScaleArgs -> bsa_DestX;
      ULONG dxd = bitScaleArgs -> bsa_DestWidth;
      ULONG dxs = bitScaleArgs -> bsa_SrcWidth;
      LONG accuxs = dxd;
      LONG accuxd = - (dxs >> 1);
      DEF_USIZE ReadMask;
      ULONG possible_columns, columncounter;
      ULONG this_x; 

      while (count < DestWidth)
      {
        accuxd += dxs;
        while (accuxd > accuxs )
        {
          xs++;
          accuxs += dxd;
        }

        /* instead of copying column by column we can *maybe* even
           copy more than one column at a time - we'll have to see */

        if ((count & DEF_ANDMASK) > (xs & DEF_ANDMASK))
          possible_columns =  DEF_NUMBITSMINUS1 - (count & DEF_ANDMASK);
        else
          possible_columns =  DEF_NUMBITSMINUS1 - (xs & DEF_ANDMASK);

        columncounter = 1; /* one row, that's for sure!*/
        this_x = xs; /* in counter we find the x-coord of the current source pixels  */
        {
          LONG accuxd_tmp = accuxd;
          LONG accuxs_tmp = accuxs;
          ULONG next_x = xs;
          ULONG count2 = count + 1;

          while (possible_columns > 0 && count2 < DestWidth)
          {
             /* where's the next x-source-coordinate going to be? */
            accuxd_tmp += dxs;
            while (accuxd_tmp > accuxs_tmp )
            {
              next_x++;
              accuxs_tmp += dxd;
            }

            if (this_x + 1 == next_x)
            {
              /* it's the immediately following coordinate */
              columncounter++;
              this_x++;
              count2++;
            }
            else
            {
              /* we're copying more than on column then we have to change
               * accuxd and accuxs
               */
              if (columncounter != 1)
              {
                accuxd = accuxd_tmp;
                accuxs = accuxs_tmp;
              }
              break; /* the next column is not the neighbouring one */
            }

            /* determine how many more columns we can copy */
            possible_columns--;
          } /* while */
        }


        /* let's generate a mask that's columncounter bits wide */
        ReadMask = DEF_READMASK;
        ReadMask = (DEF_SIZE)ReadMask >> (columncounter - 1);
        /* let's adjust this mask to the correct position */
        ReadMask = ReadMask >> (xs & DEF_ANDMASK);
        /* The leftmost set bit is xs & DEF_MASK away from the highest bit */

        /* now that we have generated the read-mask we can copy all the columns
         * that need copying in all bitmaps.
         */

        {
          ULONG i,y;
          ULONG ind;
          LONG preshift = (xs & DEF_ANDMASK) - (count & DEF_ANDMASK);
          ULONG shift;
          ULONG AndMask;
          struct BitMap * SrcBitMap  = bitScaleArgs -> bsa_SrcBitMap;
          struct BitMap * DestBitMap = bitScaleArgs -> bsa_DestBitMap;

          if (preshift > 0)
          {
            shift = preshift;
            AndMask = (ReadMask << shift) ^ (DEF_SIZE)(-1);
          }
          else
          {
            shift = -preshift;
            AndMask = (ReadMask >> shift) ^ (DEF_SIZE)(-1);
          }

          /* treat all the Bitmaps after another */
          for (i = 0; i < DestBitMap -> Depth; i++)
          {
            for (y = 0; y < bitScaleArgs -> bsa_DestHeight; y++)
            {
              DEF_USIZE CopyData;
              ind = LinePattern[y] * (SrcBitMap -> BytesPerRow >> DEF_SHIFTY) + /* y-Coord */
                    (xs >> DEF_SHIFTX); /* x-Coord */
              CopyData = ((DEF_USIZE *)SrcBitMap -> Planes[i])[ind];
              CopyData = CopyData & ReadMask;

              if (preshift > 0)
                CopyData = CopyData << shift;
              else
                CopyData = CopyData >> shift;

              /* ind correctly calculates the destination Address for the CopyData */
              ind = y * ((bitScaleArgs -> bsa_DestY + DestBitMap -> BytesPerRow) >> DEF_SHIFTY) + /* y-Coord */
                    (count >> DEF_SHIFTX); /* x-Coord */
              /*  Leave a previous picture in the bitmap untouched except for in the
               *  area where the scaled picture goes into
               */
              ((DEF_USIZE *)DestBitMap ->Planes[i])[ind] =
                (((DEF_USIZE *)DestBitMap ->Planes[i])[ind] & AndMask) | CopyData;
              /*
               kprintf("Dest: %x\n\n",(LONG)((DEF_USIZE *)DestBitMap ->Planes[i])[ind]);
              */
            } /* for () */
          } /* for () */
        }
        xs = this_x;

        /* go to next x-coordinate */
        count += columncounter;
      } /* while */
    }

    /* let's get rid of the allocated memory */
    FreeMem(LinePattern, bitScaleArgs -> bsa_DestHeight);

    #undef DEF_USIZE 
    #undef DEF_SIZE
    #undef DEF_NUMBITSMINUS1
    #undef DEF_ANDMASK
    #undef DEF_READMASK
    #undef DEF_SHIFTY
    #undef DEF_SHIFTX
  }

  AROS_LIBFUNC_EXIT
} /* BitMapScale */
