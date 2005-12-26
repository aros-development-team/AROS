/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function BitMapScale()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/scale.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "objcache.h"

VOID HIDD_BM_BitMapScale(OOP_Object *, OOP_Object *, OOP_Object *, struct BitScaleArgs *, OOP_Object *);

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


  if (bitScaleArgs->bsa_SrcBitMap->pad  != 0  || 
      bitScaleArgs->bsa_DestBitMap->pad != 0 || 
      bitScaleArgs->bsa_SrcBitMap->Flags  & BMF_AROS_HIDD || 
      bitScaleArgs->bsa_DestBitMap->Flags & BMF_AROS_HIDD)
  {
    ULONG srcflags = 0;
    ULONG dstflags = 0;

    BOOL src_colmap_set = FALSE;
    BOOL dst_colmap_set = FALSE;
    BOOL success = TRUE;
    BOOL colmaps_ok = TRUE;

    OOP_Object *srcbm_obj;
    OOP_Object *dstbm_obj;
    OOP_Object *tmp_gc;
    

    srcbm_obj = OBTAIN_HIDD_BM(bitScaleArgs->bsa_SrcBitMap);
    dstbm_obj = OBTAIN_HIDD_BM(bitScaleArgs->bsa_DestBitMap);
    tmp_gc = obtain_cache_object(SDD(GfxBase)->gc_cache, GfxBase);

    /* We must lock any HIDD_BM_SetColorMap calls */
    LOCK_BLIT
    if (srcbm_obj && dstbm_obj && tmp_gc)
    {
        /* Try to get a CLUT for the bitmaps */
        if (IS_HIDD_BM(bitScaleArgs->bsa_SrcBitMap)) {
            if (NULL != HIDD_BM_COLMAP(bitScaleArgs->bsa_SrcBitMap))
    	        srcflags |= FLG_HASCOLMAP;
    	    srcflags |= GET_COLMOD_FLAGS(bitScaleArgs->bsa_SrcBitMap);
        } else {
             /* Amiga BM */
             srcflags |= FLG_PALETTE;
        }

        if (IS_HIDD_BM(bitScaleArgs->bsa_DestBitMap)) {
            if (NULL != HIDD_BM_COLMAP(bitScaleArgs->bsa_DestBitMap))
               dstflags |= FLG_HASCOLMAP;
    	    dstflags |= GET_COLMOD_FLAGS(bitScaleArgs->bsa_DestBitMap);
        } else {
            /* Amiga BM */
            dstflags |= FLG_PALETTE;
        }
    	

        if (    (srcflags == FLG_PALETTE || srcflags == FLG_STATICPALETTE)) {
            /* palettized with no colmap. Neew to get a colmap from dest*/
            if (dstflags == FLG_TRUECOLOR) {
    	
                D(bug("!!! NO WAY GETTING PALETTE FOR src IN BltBitMap\n"));
                colmaps_ok = FALSE;
                success = FALSE;
    	    
            } else if (dstflags == (FLG_TRUECOLOR | FLG_HASCOLMAP)) {
    	
                /* Use the dest colmap for src */
                HIDD_BM_SetColorMap(srcbm_obj, HIDD_BM_COLMAP(bitScaleArgs->bsa_DestBitMap));

            }
        }

        if (   (dstflags == FLG_PALETTE || dstflags == FLG_STATICPALETTE)) {
    	    /* palettized with no pixtab. Nees to get a pixtab from dest*/
            if (srcflags == FLG_TRUECOLOR) {
                D(bug("!!! NO WAY GETTING PALETTE FOR dst IN BltBitMap\n"));
                colmaps_ok = FALSE;
                success = FALSE;
    	    
            } else if (srcflags == (FLG_TRUECOLOR | FLG_HASCOLMAP)) {
    	
                /* Use the src colmap for dst */
                HIDD_BM_SetColorMap(dstbm_obj, HIDD_BM_COLMAP(bitScaleArgs->bsa_SrcBitMap));
    	    
                dst_colmap_set = TRUE;
            }
        }

        if (TRUE == success && TRUE == colmaps_ok)
        {
            ULONG old_drmd;
	    struct TagItem cbtags[] = {
    		{ aHidd_GC_DrawMode, vHidd_GC_DrawMode_Copy },
    		{ TAG_DONE, 0 }
	    };
	    
	    OOP_GetAttr(tmp_gc, aHidd_GC_DrawMode, &old_drmd);
	    
	    OOP_SetAttrs(tmp_gc, cbtags);

            bitScaleArgs->bsa_DestWidth = ScalerDiv(bitScaleArgs->bsa_SrcWidth,
                                           bitScaleArgs->bsa_XDestFactor,
                                           bitScaleArgs->bsa_XSrcFactor);     
    
            bitScaleArgs->bsa_DestHeight = ScalerDiv(bitScaleArgs->bsa_SrcHeight,
                                            bitScaleArgs->bsa_YDestFactor,
                                            bitScaleArgs->bsa_YSrcFactor);

    	    HIDD_BM_BitMapScale(SDD(GfxBase)->pointerbm
	    	, srcbm_obj
    		, dstbm_obj
    		, bitScaleArgs
		, tmp_gc
    	    );
    	    
	    cbtags[0].ti_Data = old_drmd;
	    OOP_SetAttrs(tmp_gc, cbtags);
        } /* if () */
    
        if (src_colmap_set)
            HIDD_BM_SetColorMap(srcbm_obj, NULL);
        if (dst_colmap_set)
            HIDD_BM_SetColorMap(dstbm_obj, NULL);
    }

    if (dstbm_obj)
         RELEASE_HIDD_BM(dstbm_obj, bitScaleArgs->bsa_DestBitMap);

    if (srcbm_obj)
         RELEASE_HIDD_BM(srcbm_obj, bitScaleArgs->bsa_SrcBitMap);

    if (tmp_gc)
        release_cache_object(SDD(GfxBase)->gc_cache, tmp_gc, GfxBase);

    ULOCK_BLIT

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

    if (NULL ==(LinePattern = (UWORD *) AllocMem(sizeof(UWORD)*bitScaleArgs ->bsa_DestHeight, 0)))
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
          for (i = 0; (i < DestBitMap -> Depth) && (i < SrcBitMap -> Depth); i++)
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
    FreeMem(LinePattern, sizeof(UWORD) * bitScaleArgs -> bsa_DestHeight);

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
