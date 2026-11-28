/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$        $Log

    Desc: Graphics function Text()
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <string.h>
#include <proto/cybergraphics.h>
#include <aros/macros.h>
#include <aros/debug.h>

#include "gfxfuncsupport.h"

void BltTemplateBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
                          const struct TextExtent *extent, BOOL cells8,
                          struct GfxBase *GfxBase);

void BltTemplateAlphaBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
                               const struct TextExtent *extent,
                               struct GfxBase *GfxBase);

void ColorFontBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
                        const struct TextExtent *extent,
                        struct GfxBase *GfxBase);

/* The functions above keep the extent of what they draw in WORDs, and size
   their raster and count its columns in WORDs too, so Text() hands them a
   text a piece at a time. It measures each piece once, and they take the
   extent from it instead of calling TextExtent(): a text is gone through
   once before it is drawn. */
#define TEXT_PIECE 16384

struct text_piece
{
    ULONG count;            /* characters */
    LONG advance;           /* what TextLength() gives for them */
    LONG minx, maxx;        /* their te_Extent, as TextExtent() works it out */
    BOOL cells8;            /* each is a byte-aligned glyph 8 pixels wide, with
                               no kerning and a space of 8 */
};

#define EXTEND(lo, hi, x) do { if ((x) < (lo)) (lo) = (x); if ((x) > (hi)) (hi) = (x); } while (0)

/* The characters of a text from which one piece is drawn at pos: their
   extent, worked out as TextExtent() does, stays within TEXT_PIECE pixels
   and within the coordinates of a WORD. A first character whose extent
   alone does not fit makes a piece of its own, which Text() does not
   draw. */
static void text_piece(struct RastPort *rp, CONST_STRPTR text, ULONG len, QUAD pos,
                       struct text_piece *p)
{
    struct TextFont *tf = rp->Font;
    LONG x = 0, lo = 0, hi = 0, left = 0, right = 0;
    BOOL cells8 = TRUE;
    ULONG n;

    if (rp->AlgoStyle & FSF_BOLD)
        right += tf->tf_BoldSmear;
    if (rp->AlgoStyle & FSF_ITALIC)
    {
        right += tf->tf_Baseline / 2;
        left += (tf->tf_YSize - tf->tf_Baseline) / 2;
    }
    for (n = 0; n < len; n++)
    {
        UBYTE c = text[n];
        ULONG idx = (c < tf->tf_LoChar || c > tf->tf_HiChar) ? NUMCHARS(tf) - 1 : c - tf->tf_LoChar;
        ULONG charloc = ((ULONG *)tf->tf_CharLoc)[idx];
        LONG nx = x, nlo = lo, nhi = hi, minx, maxx;

        if (tf->tf_CharKern)
            nx += ((WORD *)tf->tf_CharKern)[idx];
        EXTEND(nlo, nhi, nx);
        EXTEND(nlo, nhi, nx + (LONG)(charloc & 0xFFFF));
        nx += tf->tf_CharSpace ? ((WORD *)tf->tf_CharSpace)[idx] : tf->tf_XSize;
        EXTEND(nlo, nhi, nx);
        nx += rp->TxSpacing;
        EXTEND(nlo, nhi, nx);
        minx = nlo - left;
        maxx = nhi - 1 + right;
        if (n && (maxx < minx || maxx - minx >= TEXT_PIECE ||
                  pos + minx < -32768 || pos + maxx > 32767))
            break;
        x = nx;
        lo = nlo;
        hi = nhi;
        p->minx = minx;
        p->maxx = maxx;
        if ((charloc & 0xFFFF) != 8 || ((charloc >> 16) & 7) ||
            (tf->tf_CharKern && ((WORD *)tf->tf_CharKern)[idx] != 0) ||
            (tf->tf_CharSpace && ((WORD *)tf->tf_CharSpace)[idx] != 8))
            cells8 = FALSE;
    }
    p->count = n;
    p->advance = x;
    p->cells8 = cells8;
}

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

        AROS_LH3(void, Text,

/*  SYNOPSIS */
        AROS_LHA(struct RastPort *, rp, A1),
        AROS_LHA(CONST_STRPTR     , string, A0),
        AROS_LHA(WORD             , count, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 10, Graphics)

/*  FUNCTION
        Write text to the rastport at the current position.
        The current position is updated to a position after the text.

    INPUTS
        rp     - RastPort
        string - string to print
        count  - number of characters to print

    RESULT

    NOTES
        A count of 0 or less draws nothing. A text wider than 16384 pixels
        is drawn a piece at a time, and where two pieces meet, a JAM2
        background, an underline or the overhang of bold or italic is
        worked out for each piece on its own. A character that reaches past
        the coordinates of a WORD is not drawn, and neither is one wider
        than 16384 pixels alone, or any of a font taller than that or with
        its baseline lower; the pen moves past them all the same.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        29-10-95    digulla automatically created from
                            graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(count > 0) {
        struct ColorTextFont *ctf = (struct ColorTextFont *)rp->Font;
        BOOL                  antialias;
        BOOL                  colorfont;
        /* the renderers keep the height and the baseline, and the italic
           offsets from them, in WORDs too */
        BOOL                  tall = ctf->ctf_TF.tf_YSize > TEXT_PIECE ||
                                     ctf->ctf_TF.tf_Baseline > TEXT_PIECE;
        struct text_piece     piece;
        /* 32767 characters can advance further than a LONG reaches */
        QUAD                  pos = rp->cp_x;

        antialias = (ctf->ctf_TF.tf_Style & FSF_COLORFONT) &&
                    ((ctf->ctf_Flags & CT_COLORMASK) == CT_ANTIALIAS) &&
                    (GetBitMapAttr(rp->BitMap, BMA_DEPTH) >= 15);

        colorfont = (ctf->ctf_TF.tf_Style & FSF_COLORFONT) &&
                    (((ctf->ctf_Flags & CT_COLORMASK) == CT_COLORFONT) || ((ctf->ctf_Flags & CT_COLORMASK) == CT_GREYFONT));

        for (; count > 0; string += piece.count, count -= piece.count) {
            text_piece(rp, string, count, pos, &piece);
            /* A piece too wide or too tall for a raster of WORDs, or that
               reaches past the coordinates of a WORD, is not drawn: no
               bitmap has pixels out there. */
            if (!tall && piece.maxx >= piece.minx && piece.maxx - piece.minx < TEXT_PIECE &&
                pos >= -32768 && pos <= 32767 &&
                pos + piece.minx >= -32768 && pos + piece.maxx <= 32767) {
                struct TextExtent te;

                /* what TextExtent() gives for the piece */
                te.te_Width = piece.advance;
                te.te_Height = ctf->ctf_TF.tf_YSize;
                te.te_Extent.MinX = piece.minx;
                te.te_Extent.MaxX = piece.maxx;
                te.te_Extent.MinY = -ctf->ctf_TF.tf_Baseline;
                te.te_Extent.MaxY = te.te_Height - 1 - ctf->ctf_TF.tf_Baseline;
                if (rp->cp_x != pos)
                    Move(rp, (WORD)pos, rp->cp_y);
                if(antialias) {
                    BltTemplateAlphaBasedText(rp, string, piece.count, &te, GfxBase);
                } else if(colorfont) {
                    ColorFontBasedText(rp, string, piece.count, &te, GfxBase);
                } else {
                    BltTemplateBasedText(rp, string, piece.count, &te, piece.cells8, GfxBase);
                }
            }
            pos += piece.advance;
        }
        Move(rp, (WORD)pos, rp->cp_y);
    }

    AROS_LIBFUNC_EXIT

} /* Text */

/***************************************************************************/

void BltTemplateBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
                          const struct TextExtent *extent, BOOL cells8,
                          struct GfxBase *GfxBase)
{
    struct TextExtent    te = *extent;
    struct TextFont     *tf;
    WORD                 raswidth, raswidth16, raswidth_bpr, rasheight, x, y, gx;
    UBYTE               *raster;
    BOOL                 is_bold, is_italic, fastfont;
    BOOL                 allocated = FALSE;
    ULONG                rassize;

    tf = rp->Font;
    is_bold = (rp->AlgoStyle & FSF_BOLD) != 0;
    is_italic = (rp->AlgoStyle & FSF_ITALIC) != 0;
    /* Text() has measured the piece, and seen whether each of its glyphs
       is a byte-aligned cell of 8 pixels */
    fastfont = cells8 && !is_bold && !is_italic && tf->tf_XSize == 8 &&
               rp->TxSpacing == 0;

    raswidth  = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
    rasheight = te.te_Extent.MaxY - te.te_Extent.MinY + 1;

    raswidth16 = (raswidth + 15) & ~15;
    raswidth_bpr = raswidth16 / 8;
    rassize = RASSIZE(raswidth, rasheight);

    if (rp->TmpRas && rp->TmpRas->RasPtr && rp->TmpRas->Size >= rassize)
        raster = rp->TmpRas->RasPtr;
    else
    {
        raster = AllocRaster(raswidth, rasheight);
        allocated = raster != NULL;
    }

    if(raster) {
        if (!fastfont)
            SetMem(raster, 0, rassize);

        x = -te.te_Extent.MinX;

        /* The standard Amiga screen font is an unstyled, fixed-width,
         * byte-aligned 8-pixel font. Build its template a byte at a time
         * instead of running the generic bit-copy loop for every pixel. */
        if (fastfont)
        {
            UBYTE *dst = raster;
            ULONG remaining = len;

            if (raswidth_bpr > len)
            {
                for (y = 0; y < rasheight; y++)
                    raster[y * raswidth_bpr + len] = 0;
            }

            while (remaining)
            {
                UBYTE *glyphdata[4];
                ULONG batch = remaining >= 4 ? 4 :
                              remaining >= 2 ? 2 : 1;
                ULONG i;

                for (i = 0; i < batch; i++)
                {
                    UBYTE c = text[i];
                    ULONG idx = (c < tf->tf_LoChar || c > tf->tf_HiChar) ?
                                NUMCHARS(tf) - 1 : c - tf->tf_LoChar;
                    ULONG charloc = ((ULONG *)tf->tf_CharLoc)[idx];

                    glyphdata[i] = (UBYTE *)tf->tf_CharData +
                                   (charloc >> 16) / 8;
                }

                for (y = 0; y < rasheight; y++)
                {
                    UBYTE *row = dst + y * raswidth_bpr;

                    if (batch == 4)
                    {
                        ULONG value =
                            ((ULONG)glyphdata[0][y * tf->tf_Modulo] << 24) |
                            ((ULONG)glyphdata[1][y * tf->tf_Modulo] << 16) |
                            ((ULONG)glyphdata[2][y * tf->tf_Modulo] << 8) |
                            glyphdata[3][y * tf->tf_Modulo];
                        *(ULONG *)row = AROS_LONG2BE(value);
                    }
                    else if (batch == 2)
                    {
                        UWORD value =
                            ((UWORD)glyphdata[0][y * tf->tf_Modulo] << 8) |
                            glyphdata[1][y * tf->tf_Modulo];
                        *(UWORD *)row = AROS_WORD2BE(value);
                    }
                    else
                        *row = glyphdata[0][y * tf->tf_Modulo];
                }

                text += batch;
                dst += batch;
                remaining -= batch;
                x += batch * 8;
            }
        }
        else while(len--) {
            UBYTE c = *text++;
            ULONG idx;
            ULONG charloc;
            UWORD glyphwidth, glyphpos, bold;
            UBYTE *glyphdata;
            UBYTE *dst;
            ULONG srcmask;
            ULONG dstmask;

            if(c < tf->tf_LoChar || c > tf->tf_HiChar) {
                idx = NUMCHARS(tf) - 1;
            } else {
                idx = c - tf->tf_LoChar;
            }

            charloc = ((ULONG *)tf->tf_CharLoc)[idx];

            glyphwidth = charloc & 0xFFFF;
            glyphpos = charloc >> 16;

            if(tf->tf_CharKern) {
                x += ((WORD *)tf->tf_CharKern)[idx];
            }


            if (!is_bold && !is_italic && glyphwidth <= 8) {
                WORD wx = x;
                UWORD srcshift = glyphpos & 7;
                UWORD dstshift = wx & 7;

                glyphdata = (UBYTE *)tf->tf_CharData + glyphpos / 8;
                dst = raster + wx / 8;

                for (y = 0; y < rasheight; y++) {
                    UWORD source = (UWORD)glyphdata[0] << 8;
                    UBYTE bits;
                    UWORD packed;

                    if (srcshift + glyphwidth > 8)
                        source |= glyphdata[1];
                    bits = (source >> (8 - srcshift)) &
                           (0xff << (8 - glyphwidth));
                    packed = ((UWORD)bits << 8) >> dstshift;

                    if (packed & 0xff00)
                        dst[0] |= packed >> 8;
                    if ((dstshift + glyphwidth > 8) && (packed & 0xff))
                        dst[1] |= packed;

                    glyphdata += tf->tf_Modulo;
                    dst += raswidth_bpr;
                }
            } else for(bold = 0; bold <= is_bold; bold++) {
                WORD wx;
                WORD italicshift, italiccheck = 0;

                if(is_italic) {
                    italiccheck = tf->tf_Baseline;
                    italicshift = italiccheck / 2;
                } else {
                    italicshift = 0;
                }

                wx = x + italicshift + (bold ? tf->tf_BoldSmear : 0);

                glyphdata = ((UBYTE *)tf->tf_CharData) + glyphpos / 8;
                dst = raster + wx / 8;

                for(y = 0; y < rasheight; y++) {
                    UBYTE *glyphdatax = glyphdata;
                    UBYTE *dstx = dst;
                    UBYTE srcdata;

                    srcmask = 0x80 >> (glyphpos & 7);
                    dstmask = 0x80 >> (wx & 7);

                    srcdata = *glyphdatax;

                    for(gx = 0; gx < glyphwidth; gx++) {
                        if(srcdata & srcmask) {
                            *dstx |= dstmask;
                        }

                        if(dstmask == 0x1) {
                            dstmask = 0x80;
                            dstx++;
                        } else {
                            dstmask >>= 1;
                        }

                        if(srcmask == 0x1) {
                            srcmask = 0x80;
                            glyphdatax++;
                            srcdata = *glyphdatax;
                        } else {
                            srcmask >>= 1;
                        }

                    } /* for(gx = 0; gx < glyphwidth; gx++) */

                    glyphdata += tf->tf_Modulo;
                    dst += raswidth_bpr;

                    if(is_italic) {
                        italiccheck--;
                        if(italiccheck & 1) {
                            italicshift--;

                            wx--;
                            if((wx & 7) == 7) dst--;

                        }
                    }

                } /* for(y = 0; y < rasheight; y++) */

            } /* for(bold = 0; bold < ((rp->AlgoStyle & FSF_BOLD) ? 2 : 1); bold++) */

            if(tf->tf_CharSpace) {
                x += ((WORD *)tf->tf_CharSpace)[idx];
            } else {
                x += tf->tf_XSize;
            }

            x += rp->TxSpacing;

        } /* while(len--) */

        if(rp->AlgoStyle & FSF_UNDERLINED) {
            UBYTE *dst;
            ULONG prev_word, act_word = 0, next_word, word;
            WORD count;
            LONG underline;

            underline = rp->TxBaseline + 1;
            if(underline < rasheight - 1) underline++;

            if(underline < rasheight) {
                dst = raster + underline * (LONG)raswidth_bpr;
                next_word = *(UWORD *)dst;
#if !AROS_BIG_ENDIAN
                next_word = AROS_WORD2BE(next_word);
#endif
                count  = raswidth16 / 16;

                while(count--) {
                    prev_word = act_word;
                    act_word = next_word;
                    if(count > 1) {
                        next_word = ((UWORD *)dst)[1];

#if !AROS_BIG_ENDIAN
                        next_word = AROS_WORD2BE(next_word);
#endif
                    } else {
                        next_word = 0;
                    }
                    word = ((act_word << 1) & 0xFFFF) + (next_word >> 15);
                    word |= (act_word >> 1) + ((prev_word << 15) & 0xFFFF);
                    word &= ~act_word;

                    word = 0xFFFF & ~ word;
#if !AROS_BIG_ENDIAN
                    word = AROS_BE2WORD(word);
#endif

                    *(UWORD *)dst = word;
                    dst += 2;

                } /* while(count--) */

            } /* if (underline < rasheight) */

        } /* if (rp->AlgoStyle & FSF_UNDERLINED) */

        BltTemplate(raster,
                    0,
                    raswidth_bpr,
                    rp,
                    rp->cp_x + te.te_Extent.MinX,
                    rp->cp_y - rp->TxBaseline,
                    raswidth,
                    rasheight);

        if (allocated)
            FreeRaster(raster, raswidth, rasheight);

    } /* if ((raster = AllocRaster(raswidth, rasheight))) */

    Move(rp, rp->cp_x + te.te_Width, rp->cp_y);

}

/***************************************************************************/

/*
 * Lay the glyphs of a chunky (one byte per pixel) font out into raster:
 * additive for antialiased fonts, replacing for colour fonts.
 */
static void chunky_text(struct RastPort *rp, CONST_STRPTR text, ULONG len, UBYTE *raster,
                        WORD raswidth_bpr, WORD rasheight, WORD x, UBYTE *chardata, BOOL additive)
{
    struct TextFont *tf = rp->Font;
    BOOL is_bold   = (rp->AlgoStyle & FSF_BOLD) != 0;
    BOOL is_italic = (rp->AlgoStyle & FSF_ITALIC) != 0;
    WORD y, gx;

    while(len--) {
        UBYTE c = *text++;
        ULONG idx;
        ULONG charloc;
        UWORD glyphwidth, glyphpos, bold;
        UBYTE *glyphdata;
        UBYTE *dst;

        if(c < tf->tf_LoChar || c > tf->tf_HiChar) {
            idx = NUMCHARS(tf) - 1;
        } else {
            idx = c - tf->tf_LoChar;
        }

        charloc = ((ULONG *)tf->tf_CharLoc)[idx];

        glyphwidth = charloc & 0xFFFF;
        glyphpos = charloc >> 16;

        if(tf->tf_CharKern) {
            x += ((WORD *)tf->tf_CharKern)[idx];
        }

        for(bold = 0; bold <= is_bold; bold++) {
            WORD wx;
            WORD italicshift, italiccheck = 0;

            if(is_italic) {
                italiccheck = tf->tf_Baseline;
                italicshift = italiccheck / 2;
            } else {
                italicshift = 0;
            }

            wx = x + italicshift + (bold ? tf->tf_BoldSmear : 0);

            glyphdata = chardata + glyphpos;
            dst = raster + wx;

            for(y = 0; y < rasheight; y++) {
                UBYTE *glyphdatax = glyphdata;
                UBYTE *dstx = dst;

                if(additive) {
                    for(gx = 0; gx < glyphwidth; gx++) {
                        UWORD old = *dstx;

                        old += *glyphdatax++;
                        if(old > 255) old = 255;
                        *dstx++ = old;
                    }
                } else {
                    for(gx = 0; gx < glyphwidth; gx++) {
                        UBYTE p = *glyphdatax++;

                        if(p || !bold) *dstx = p;

                        dstx++;
                    }
                }

                glyphdata += tf->tf_Modulo * 8;
                dst += raswidth_bpr;

                if(is_italic) {
                    italiccheck--;
                    if(italiccheck & 1) {
                        italicshift--;
                        dst--;
                    }
                }
            }
        }

        if(tf->tf_CharSpace) {
            x += ((WORD *)tf->tf_CharSpace)[idx];
        } else {
            x += tf->tf_XSize;
        }

        x += rp->TxSpacing;
    }
}


void BltTemplateAlphaBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
                               const struct TextExtent *extent,
                               struct GfxBase *GfxBase)
{
    struct TextExtent    te = *extent;
    WORD                 raswidth, raswidth_bpr, rasheight;
    UBYTE               *raster;

    /* CyberGfxBase is placed inside GfxBase, so it's static */
    if(!CyberGfxBase)
        CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
    if(!CyberGfxBase)
        return;

    raswidth  = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
    rasheight = te.te_Extent.MaxY - te.te_Extent.MinY + 1;

    raswidth_bpr = raswidth;

    if((raster = AllocVec(raswidth * rasheight, MEMF_CLEAR))) {
        chunky_text(rp, text, len, raster, raswidth_bpr, rasheight, -te.te_Extent.MinX,
                    (UBYTE *)((struct ColorTextFont *)rp->Font)->ctf_CharData[0], TRUE);

        if(rp->AlgoStyle & FSF_UNDERLINED) {
            UBYTE *dst;
            UBYTE prev_byte, act_byte = 0, next_byte;
            WORD count;
            LONG underline;

            underline = rp->TxBaseline + 1;
            if(underline < rasheight - 1) underline++;

            if(underline < rasheight) {
                dst = raster + underline * (LONG)raswidth_bpr;
                count  = raswidth;

                next_byte = *dst;

                while(count--) {
                    prev_byte = act_byte;
                    act_byte = next_byte;
                    if(count > 1) {
                        next_byte = dst[1];
                    } else {
                        next_byte = 0;
                    }

                    *dst++ = (act_byte || (!prev_byte && !next_byte)) ? 255 : 0;

                } /* while(count--) */

            } /* if (underline < rasheight) */

        } /* if (rp->AlgoStyle & FSF_UNDERLINED) */

        BltTemplateAlpha(raster,
                         0,
                         raswidth_bpr,
                         rp,
                         rp->cp_x + te.te_Extent.MinX,
                         rp->cp_y - rp->TxBaseline,
                         raswidth,
                         rasheight);

        FreeVec(raster);

    } /* if ((raster = AllocVec(raswidth * rasheight, MEMF_CLEAR))) */

    Move(rp, rp->cp_x + te.te_Width, rp->cp_y);

}

/***************************************************************************/

void ColorFontBasedText(struct RastPort *rp, CONST_STRPTR text, ULONG len,
                        const struct TextExtent *extent,
                        struct GfxBase *GfxBase)
{
    struct TextExtent    te = *extent;
    struct TextFont     *tf;
    WORD                 raswidth, raswidth_bpr, rasheight;
    UBYTE               *raster, *chunky;

    tf = rp->Font;
    if(!ExtendFont(tf, NULL)) return;

    chunky = ((struct TextFontExtension_intern *)(tf->tf_Extension))->hash->chunky_colorfont;

    if((rp->DrawMode & ~INVERSVID) == JAM2) {
        ULONG             old_drmd = GetDrMd(rp);

        SetDrMd(rp, old_drmd ^ INVERSVID);
        RectFill(rp, rp->cp_x + te.te_Extent.MinX,
                 rp->cp_y + te.te_Extent.MinY,
                 rp->cp_x + te.te_Extent.MaxX,
                 rp->cp_y + te.te_Extent.MaxY);
        SetDrMd(rp, old_drmd);
    }


    raswidth  = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
    rasheight = te.te_Extent.MaxY - te.te_Extent.MinY + 1;

    raswidth_bpr = raswidth;

    if((raster = AllocVec(raswidth * rasheight, MEMF_CLEAR))) {
        chunky_text(rp, text, len, raster, raswidth_bpr, rasheight, -te.te_Extent.MinX, chunky, FALSE);

#if 0
        if(rp->AlgoStyle & FSF_UNDERLINED) {
            UBYTE *dst;
            UBYTE prev_byte, act_byte = 0, next_byte;
            WORD count;
            LONG underline;

            underline = rp->TxBaseline + 1;
            if(underline < rasheight - 1) underline++;

            if(underline < rasheight) {
                dst = raster + underline * (LONG)raswidth_bpr;
                count  = raswidth;

                next_byte = *dst;

                while(count--) {
                    prev_byte = act_byte;
                    act_byte = next_byte;
                    if(count > 1) {
                        next_byte = dst[1];
                    } else {
                        next_byte = 0;
                    }

                    *dst++ = (act_byte || (!prev_byte && !next_byte)) ? 255 : 0;

                } /* while(count--) */

            } /* if (underline < rasheight) */

        } /* if (rp->AlgoStyle & FSF_UNDERLINED) */

#endif

        {
            HIDDT_PixelLUT  pixlut;
            HIDDT_Pixel     pixtab[256];

            pixlut.entries = AROS_PALETTE_SIZE;
            pixlut.pixels  = IS_HIDD_BM(rp->BitMap) ? HIDD_BM_PIXTAB(rp->BitMap) : NULL;

            if((rp->Flags & RPF_REMAP_COLORFONTS) &&
                    (CTF(tf)->ctf_ColorFontColors) &&
                    ((CTF(tf)->ctf_Flags & CT_COLORMASK) != CT_GREYFONT) && /* <-- FIX/CHECK/SUPPORT CT_GREYFONT) */
                    IS_HIDD_BM(rp->BitMap) &&
                    (GetBitMapAttr(rp->BitMap, BMA_DEPTH) > 8)) {
                UWORD *colortable = CTF(tf)->ctf_ColorFontColors->cfc_ColorTable;
                WORD   i;

                for(i = 0; i < CTF(tf)->ctf_ColorFontColors->cfc_Count; i++) {
                    UWORD rgb12 = *colortable++;
                    HIDDT_Color col;

                    col.red   = ((rgb12 >> 8) & 0x0F) * 0x1111;
                    col.green = ((rgb12 >> 4) & 0x0F) * 0x1111;
                    col.blue  = ((rgb12 >> 0) & 0x0F) * 0x1111;

                    pixtab[i] = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);
                }

                pixlut.pixels = pixtab;
            }

            write_transp_pixels_8(rp, raster, raswidth_bpr,
                                  rp->cp_x + te.te_Extent.MinX,
                                  rp->cp_y - rp->TxBaseline,
                                  rp->cp_x + te.te_Extent.MinX + raswidth - 1,
                                  rp->cp_y - rp->TxBaseline + rasheight - 1,
                                  &pixlut, 0, TRUE, GfxBase);

        }

        FreeVec(raster);

    } /* if ((raster = AllocVec(raswidth * rasheight, MEMF_CLEAR))) */

    Move(rp, rp->cp_x + te.te_Width, rp->cp_y);

}
