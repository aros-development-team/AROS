/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    TextLength() and TextExtent() on fonts made in memory: with a kerning
    and a space table, with one of the two, with none, proportional or
    fixed; with a glyph wider than its cell, a negative TxSpacing, a text
    wider than a WORD, and a count of 0 or less. A font without a kerning
    table measures as with kerning 0, and one without a space table as with
    spaces of tf_XSize, which is how Text() draws them. The extent follows
    the glyphs and stops at the range of a WORD.

    The font has the characters a to d and a default glyph:

        glyph    a   b   c   d   default
        width    4   6   8  10   5
        space    5   7   9  11   6
        kerning  1   0  -2   2   0

    and tf_XSize 8, tf_YSize 8, tf_Baseline 6. Nothing is drawn and no screen
    is opened. The checks that a font without a table and a negative count
    would make read through NULL or past the string on an unfixed
    graphics.library come last.
*/
#include <aros/debug.h>
#include <dos/dos.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#define LONGTEXT 8193

static const ULONG charloc[5] =
{
    (0UL << 16) | 4, (4UL << 16) | 6, (10UL << 16) | 8, (18UL << 16) | 10, (28UL << 16) | 5
};
static const WORD charspace[5] = { 5, 7, 9, 11, 6 };
static const WORD charkern[5] = { 1, 0, -2, 2, 0 };
/* 8 rows of 33 pixels, in words; the measuring functions do not read them */
static UWORD chardata[8 * 3];
static UBYTE longtext[LONGTEXT];
/* the fonts: static, so that every field not set below is 0 */
static struct TextFont both, spaceonly, kernonly, prop, fixed;

static ULONG checks;
static ULONG failures;

static void check(BOOL ok, CONST_STRPTR what)
{
    checks++;
    if (!ok)
    {
        bug("TEXTEXTENTS FAIL: %s\n", what);
        failures++;
    }
    Printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    Flush(Output());
}

static void make_font(struct TextFont *tf, UBYTE flags, const WORD *space, const WORD *kern)
{
    tf->tf_Message.mn_Node.ln_Name = "textextents.font";
    tf->tf_YSize = 8;
    tf->tf_Flags = FPF_DESIGNED | flags;
    tf->tf_XSize = 8;
    tf->tf_Baseline = 6;
    tf->tf_BoldSmear = 1;
    tf->tf_LoChar = 'a';
    tf->tf_HiChar = 'd';
    tf->tf_CharData = chardata;
    tf->tf_Modulo = 6;
    tf->tf_CharLoc = (APTR)charloc;
    tf->tf_CharSpace = (APTR)space;
    tf->tf_CharKern = (APTR)kern;
}

/* One text measured both ways: the length, and the extent's width and ends. */
static void measure(struct RastPort *rp, CONST_STRPTR text, LONG count, LONG len, LONG minx, LONG maxx,
                    CONST_STRPTR what)
{
    struct TextExtent te;
    LONG got;

    te.te_Width = 12345;
    te.te_Extent.MinX = 12345;
    te.te_Extent.MaxX = 12345;
    got = TextLength(rp, text, count);
    TextExtent(rp, text, count, &te);
    check(got == len && (WORD)te.te_Width == (WORD)len && te.te_Extent.MinX == minx && te.te_Extent.MaxX == maxx &&
          te.te_Height == 8 && te.te_Extent.MinY == -6 && te.te_Extent.MaxY == 1, what);
    Printf("     length %ld, extent %ld to %ld, width %ld\n", got, (LONG)te.te_Extent.MinX, (LONG)te.te_Extent.MaxX,
           (LONG)(WORD)te.te_Width);
    Flush(Output());
}

int main(void)
{
    struct RastPort rp;
    ULONG i;

    for (i = 0; i < LONGTEXT; i++)
        longtext[i] = 'a';
    make_font(&both, FPF_PROPORTIONAL, charspace, charkern);
    make_font(&spaceonly, 0, charspace, NULL);
    make_font(&kernonly, 0, NULL, charkern);
    make_font(&prop, FPF_PROPORTIONAL, NULL, NULL);
    make_font(&fixed, 0, NULL, NULL);
    InitRastPort(&rp);

    /* what must not change: both tables, and a fixed font whose glyphs fit */
    SetFont(&rp, &both);
    measure(&rp, "abcd", 4, 33, 0, 32, "both tables: abcd is 33 long and reaches from 0 to 32");
    measure(&rp, "abcd", 0, 0, 0, 0, "both tables: a count of 0 measures nothing");
    SetFont(&rp, &fixed);
    measure(&rp, "abc", 3, 24, 0, 23, "fixed, no tables: abc is 24 long and reaches from 0 to 23");
    measure(&rp, "abc", 0, 0, 0, -1, "fixed, no tables: a count of 0 is 0 long, from 0 to -1");

    /* a fixed font without tables: the extent follows the glyphs */
    measure(&rp, "abcd", 4, 32, 0, 33, "fixed: the d, wider than its cell, reaches to 33");
    rp.TxSpacing = -4;
    measure(&rp, "aaaa", 4, 16, 0, 19, "fixed, TxSpacing -4: aaaa is 16 long and reaches to 19");
    rp.TxSpacing = -12;
    measure(&rp, "aaaa", 4, -16, -16, 7, "fixed, TxSpacing -12: aaaa goes left, from -16 to 7");
    rp.TxSpacing = 0;
    measure(&rp, longtext, LONGTEXT, (WORD)(LONGTEXT * 8), 0, 32767,
            "fixed: 8193 characters reach from 0 to 32767");
    rp.TxSpacing = -16;
    measure(&rp, longtext, LONGTEXT, (WORD)(LONGTEXT * -8), -32768, 7,
            "fixed, TxSpacing -16: 8193 characters reach from -32768 to 7");
    rp.TxSpacing = 0;

    /* a missing table */
    SetFont(&rp, &spaceonly);
    measure(&rp, "abcd", 4, 32, 0, 31, "space table only: abcd is 32 long and reaches from 0 to 31");
    SetFont(&rp, &kernonly);
    measure(&rp, "abcd", 4, 33, 0, 34, "kerning table only: abcd is 33 long and reaches from 0 to 34");
    SetFont(&rp, &prop);
    measure(&rp, "abcd", 4, 32, 0, 33, "proportional, no tables: abcd is 32 long and reaches from 0 to 33");

    /* a negative count */
    SetFont(&rp, &both);
    measure(&rp, "a", -1, 0, 0, 0, "both tables: a count of -1 measures nothing");
    measure(&rp, "a", -32768, 0, 0, 0, "both tables: a count of -32768 measures nothing");

    Printf("textextents: %ld checks, %ld failures\n", (LONG)checks, (LONG)failures);
    Flush(Output());
    return failures ? RETURN_FAIL : RETURN_OK;
}
