/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics function BitMapScale()
*/

#include <exec/types.h>
#include <aros/debug.h>
#include <graphics/scale.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"
#include "objcache.h"

/* Helper: produce an 8-bit mask covering 'nbits' bits starting at the MSB. */
static inline UBYTE mask_msb_nbits(ULONG nbits)
{
  /* nbits in 1..8 */
  ULONG m = (0xFFUL << (8UL - nbits)) & 0xFFUL;
  return (UBYTE)m;
}

/* Helper: clamp v into [lo, hi] (inclusive). */
static inline ULONG clamp_ul(ULONG v, ULONG lo, ULONG hi)
{
  if (v < lo) return lo;
  if (v > hi) return v;
  return v;
}

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
        Pass a BitScaleArgs structure filled with the following arguments
        to this function:
          bsa_SrcX, bsa_SrcY - upper left coordinate in source bitmap
          bsa_SrcWidth, bsa_SrcHeight - Width and Height of source bitmap
          bsa_DestX, bsa_DestY - upper left coordinate in destination
                                 bitmap
          bsa_DestWidth, bsa_DestHeight - this function will set these
                values. Use the bsa_???Factor for scaling
          bsa_XSrcFactor:bsa_XDestFactor - Set these to get approximately
                the same ratio as bsa_SrcWidth:bsa_DestWidth, but
                usually not exactly the same number.
          bsa_YSrcFactor:bsa_YDestFactor - Set these to get approximately
                the same ratio as bsa_SrcHeight:DestHeight, but
                usually not exactly the same number.
          bsa_SrcBitMap - pointer to source bitmap to be scaled
          bsa_DestBitMap - pointer to destination bitmap which will
                           hold the scaled bitmap. Make sure it's
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
        ScalerDiv(), graphics/scale.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  if (!bitScaleArgs || !bitScaleArgs->bsa_SrcBitMap || !bitScaleArgs->bsa_DestBitMap)
    goto out;

  if (bitScaleArgs->bsa_SrcBitMap == bitScaleArgs->bsa_DestBitMap) {
    D(bug("BitMapScale: overlapping src/dst BitMap not supported\n"));
    goto out;
  }

  if (IS_HIDD_BM(bitScaleArgs->bsa_SrcBitMap) ||
      IS_HIDD_BM(bitScaleArgs->bsa_DestBitMap))
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
    tmp_gc    = obtain_cache_object(CDD(GfxBase)->gc_cache, GfxBase);

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

        if ((srcflags == FLG_PALETTE || srcflags == FLG_STATICPALETTE)) {
            /* palettized with no colmap. Need to get a colmap from dest */
            if (dstflags == FLG_TRUECOLOR) {
                D(bug("BitMapScale: cannot derive palette for src from truecolor dest\n"));
                colmaps_ok = FALSE;
                success = FALSE;
            } else if (dstflags == (FLG_TRUECOLOR | FLG_HASCOLMAP)) {
                /* Use the dest colmap for src */
                HIDD_BM_SetColorMap(srcbm_obj, HIDD_BM_COLMAP(bitScaleArgs->bsa_DestBitMap));
                src_colmap_set = TRUE;
            }
        }

        if ((dstflags == FLG_PALETTE || dstflags == FLG_STATICPALETTE)) {
            /* palettized with no colmap. Need to get a colmap from src */
            if (srcflags == FLG_TRUECOLOR) {
                D(bug("BitMapScale: cannot derive palette for dest from truecolor src\n"));
                colmaps_ok = FALSE;
                success = FALSE;
            } else if (srcflags == (FLG_TRUECOLOR | FLG_HASCOLMAP)) {
                /* Use the src colmap for dst */
                HIDD_BM_SetColorMap(dstbm_obj, HIDD_BM_COLMAP(bitScaleArgs->bsa_SrcBitMap));
                dst_colmap_set = TRUE;
            }
        }

        if (success && colmaps_ok)
        {
            struct monitor_driverdata *driver, *dst_driver;
            OOP_Object *bm_obj;
            HIDDT_DrawMode old_drmd;
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

            /*
             * Select a driver to call. The same as in BltBitMap(), but select
             * bitmap object instead of driver object.
             */
            driver     = GET_BM_DRIVERDATA(bitScaleArgs->bsa_SrcBitMap);
            dst_driver = GET_BM_DRIVERDATA(bitScaleArgs->bsa_DestBitMap);

            if (driver == (struct monitor_driverdata *)CDD(GfxBase))
                bm_obj = dstbm_obj;
            else if (dst_driver->flags & DF_UseFakeGfx)
                bm_obj = dstbm_obj;
            else
                bm_obj = srcbm_obj;

            HIDD_BM_BitMapScale(bm_obj,
                                srcbm_obj,
                                dstbm_obj,
                                bitScaleArgs,
                                tmp_gc);

            update_bitmap(bitScaleArgs->bsa_DestBitMap, dstbm_obj,
                          bitScaleArgs->bsa_DestX, bitScaleArgs->bsa_DestY,
                          bitScaleArgs->bsa_DestWidth, bitScaleArgs->bsa_DestHeight,
                          GfxBase);

            cbtags[0].ti_Data = old_drmd;
            OOP_SetAttrs(tmp_gc, cbtags);
        }

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
        release_cache_object(CDD(GfxBase)->gc_cache, tmp_gc, GfxBase);

    ULOCK_BLIT
  }
  else
  {
    /*
     * Algorithm for plain Amiga bitmaps.
     *
     * Note: This path intentionally uses BYTE accesses; 16/32-bit accesses are
     * problematic if the bitmap has widths that lead to addressing issues in
     * this algorithm (historically observed for odd bit counts per line).
     */
    ULONG *LinePattern = NULL;

    /* Compute destination size */
    bitScaleArgs->bsa_DestWidth = ScalerDiv(bitScaleArgs->bsa_SrcWidth,
                                           bitScaleArgs->bsa_XDestFactor,
                                           bitScaleArgs->bsa_XSrcFactor);

    bitScaleArgs->bsa_DestHeight = ScalerDiv(bitScaleArgs->bsa_SrcHeight,
                                            bitScaleArgs->bsa_YDestFactor,
                                            bitScaleArgs->bsa_YSrcFactor);

    if (bitScaleArgs->bsa_DestWidth == 0 || bitScaleArgs->bsa_DestHeight == 0)
      goto out;

    {
      /* Allocate mapping: for each destination row, which source row to sample */
      ULONG DestHeight = (ULONG)bitScaleArgs->bsa_DestHeight;

      LinePattern = (ULONG *)AllocMem(sizeof(ULONG) * DestHeight, 0);
      if (LinePattern == NULL)
        goto out;

      {
        ULONG ys0 = (ULONG)bitScaleArgs->bsa_SrcY;
        ULONG ys  = ys0;
        ULONG count = 0;
        ULONG dyd = (ULONG)bitScaleArgs->bsa_DestHeight;
        ULONG dys = (ULONG)bitScaleArgs->bsa_SrcHeight;
        LONG  accuys = (LONG)dyd;
        LONG  accuyd = -((LONG)dys >> 1);

        /* Highest valid sampled Y (inclusive). */
        ULONG y_max = ys0 + (ULONG)bitScaleArgs->bsa_SrcHeight - 1;

        while (count < DestHeight) {
          accuyd += (LONG)dys;
          while (accuyd > accuys)
          {
            ys++;
            accuys += (LONG)dyd;
          }

          LinePattern[count] = clamp_ul(ys, ys0, y_max);
          count++;
        }
      }

      /* Scale */
      {
        struct BitMap *SrcBitMap  = bitScaleArgs->bsa_SrcBitMap;
        struct BitMap *DestBitMap = bitScaleArgs->bsa_DestBitMap;

        ULONG destX0 = (ULONG)bitScaleArgs->bsa_DestX;
        ULONG destY0 = (ULONG)bitScaleArgs->bsa_DestY;
        ULONG srcX0  = (ULONG)bitScaleArgs->bsa_SrcX;

        ULONG DestWidthEnd = (ULONG)bitScaleArgs->bsa_DestWidth + destX0;
        ULONG count = destX0;

        ULONG dxd = (ULONG)bitScaleArgs->bsa_DestWidth;
        ULONG dxs = (ULONG)bitScaleArgs->bsa_SrcWidth;

        LONG accuxs = (LONG)dxd;
        LONG accuxd = -((LONG)dxs >> 1);

        /* Highest valid sampled X (inclusive). */
        ULONG x_max = srcX0 + (ULONG)bitScaleArgs->bsa_SrcWidth - 1;

        while (count < DestWidthEnd) {
          ULONG xs;
          ULONG possible_columns, columncounter, this_x;

          /* DDA: map dest x -> source x */
          accuxd += (LONG)dxs;
          while (accuxd > accuxs) {
            accuxs += (LONG)dxd;
          }
          xs = srcX0 + ((accuxs - accuxd + (LONG)dxs - 1) / (LONG)dxd);
          xs = clamp_ul(xs, srcX0, x_max);

          /*
           * Try to group adjacent columns which map to adjacent source columns
           * and do not cross byte boundaries in either src or dest.
           */
          {
            ULONG dstBit = count & 7UL;
            ULONG srcBit = xs & 7UL;

            possible_columns = (7UL - dstBit);
            if ((7UL - srcBit) < possible_columns)
              possible_columns = (7UL - srcBit);
          }

          columncounter = 1;
          this_x = xs;

          {
            LONG  accuxd_tmp = accuxd;
            LONG  accuxs_tmp = accuxs;
            ULONG next_x = xs;
            ULONG count2 = count + 1;

            while (possible_columns > 0 && count2 < DestWidthEnd) {
              /* where's the next x-source-coordinate going to be? */
              accuxd_tmp += (LONG)dxs;
              while (accuxd_tmp > accuxs_tmp) {
                next_x++;
                accuxs_tmp += (LONG)dxd;
              }
              if (next_x > x_max)
                next_x = x_max;

              if (this_x + 1 == next_x) {
                columncounter++;
                this_x++;
                count2++;
                /* Always advance accumulators when we consume a column */
                accuxd = accuxd_tmp;
                accuxs = accuxs_tmp;
              } else {
                break;
              }

              possible_columns--;
            }
          }

          /* Compute read mask in source byte: 'columncounter' bits, starting at bit xs&7 from MSB side */
          {
            ULONG srcBit = xs & 7UL;
            ULONG dstBit = count & 7UL;
            LONG  preshift = (LONG)srcBit - (LONG)dstBit;

            UBYTE baseMask = mask_msb_nbits(columncounter);      /* e.g. n=2 => 0xC0 */
            UBYTE readMask = (UBYTE)((ULONG)baseMask >> srcBit); /* position for srcBit */

            ULONG i, y;
            for (i = 0; (i < (ULONG)DestBitMap->Depth) && (i < (ULONG)SrcBitMap->Depth); i++) {
              UBYTE *srcPlane = (UBYTE *)SrcBitMap->Planes[i];
              UBYTE *dstPlane = (UBYTE *)DestBitMap->Planes[i];

              for (y = 0; y < (ULONG)bitScaleArgs->bsa_DestHeight; y++) {
                ULONG srcY = LinePattern[y];
                ULONG srcByteIndex = (srcY * (ULONG)SrcBitMap->BytesPerRow) + (xs >> 3);
                UBYTE srcByte = srcPlane[srcByteIndex];

                ULONG dstY = destY0 + y;
                ULONG dstByteIndex = (dstY * (ULONG)DestBitMap->BytesPerRow) + (count >> 3);
                UBYTE dstByte = dstPlane[dstByteIndex];

                UBYTE copyData;
                UBYTE writeMask;
                UBYTE andMask;

                /* Extract desired bits from source byte */
                srcByte = (UBYTE)((ULONG)srcByte & (ULONG)readMask);

                if (preshift > 0) {
                  ULONG sh = (ULONG)preshift;
                  copyData  = (UBYTE)(((ULONG)srcByte << sh) & 0xFFUL);
                  writeMask = (UBYTE)(((ULONG)readMask << sh) & 0xFFUL);
                } else {
                  ULONG sh = (ULONG)(-preshift);
                  copyData  = (UBYTE)(((ULONG)srcByte >> sh) & 0xFFUL);
                  writeMask = (UBYTE)(((ULONG)readMask >> sh) & 0xFFUL);
                }

                andMask = (UBYTE)(~writeMask);

                /* Preserve existing bits outside writeMask */
                dstPlane[dstByteIndex] = (UBYTE)((dstByte & andMask) | copyData);
              }
            }
          }

          count += columncounter;
        }
      }

      FreeMem(LinePattern, sizeof(ULONG) * (ULONG)bitScaleArgs->bsa_DestHeight);
    }
  }

out:

  AROS_LIBFUNC_EXIT
} /* BitMapScale */
