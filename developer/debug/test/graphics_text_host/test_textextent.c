/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Compile the complete, unmodified TextLength() and TextExtent() bodies from
    rom/graphics/textlength.c and rom/graphics/textextent.c against stand-in
    TextFont, RastPort, and TextExtent structures using real AROS field names.
    Every expected pixel measurement below is worked out by hand from the
    literal glyph tables.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "production_constants.h"

typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef int16_t WORD;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef int64_t QUAD;
typedef void *APTR;
typedef const char *CONST_STRPTR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

#define AROS_LIBFUNC_INIT (void)GfxBase;
#define AROS_LIBFUNC_EXIT
#define AROS_LHA(type, name, reg) type name
#define AROS_LH3(ret, name, a1, a2, a3, bt, bn, lvo, lib) \
    static ret production_##name(a1, a2, a3, bt bn)
#define AROS_LH4(ret, name, a1, a2, a3, a4, bt, bn, lvo, lib) \
    static ret production_##name(a1, a2, a3, a4, bt bn)

struct Rectangle {
    WORD MinX, MinY, MaxX, MaxY;
};

struct TextFont {
    UWORD tf_YSize;
    UBYTE tf_Style;
    UBYTE tf_Flags;
    UWORD tf_XSize;
    UWORD tf_Baseline;
    UWORD tf_BoldSmear;
    UWORD tf_Accessors;
    UBYTE tf_LoChar;
    UBYTE tf_HiChar;
    APTR tf_CharData;
    UWORD tf_Modulo;
    APTR tf_CharLoc;
    APTR tf_CharSpace;
    APTR tf_CharKern;
};

struct RastPort {
    WORD cp_x;
    WORD cp_y;
    struct TextFont *Font;
    UBYTE AlgoStyle;
    UBYTE TxFlags;
    UWORD TxHeight;
    UWORD TxWidth;
    UWORD TxBaseline;
    WORD TxSpacing;
};

struct TextExtent {
    UWORD te_Width;
    UWORD te_Height;
    struct Rectangle te_Extent;
};

struct GfxBase {
    int identity;
};

#define TextLength(rp, str, cnt) production_TextLength((rp), (str), (cnt), GfxBase)
#include "production_textextent.inc"
#undef TextLength

#define CHARLOC(bit_off, bit_width) \
    (((ULONG)(bit_off) << 16) | ((ULONG)(bit_width) & 0xFFFFUL))

/*
    Primary 5-slot glyph set covering 'A' (65) .. 'D' (68) plus default slot 4:
      idx 0 ('A'): width =  6, kern =  1, space =  7
      idx 1 ('B'): width = 12, kern =  0, space =  9   (wider than tf_XSize = 8)
      idx 2 ('C'): width =  5, kern = -3, space =  6   (negative kerning)
      idx 3 ('D'): width = 10, kern =  2, space =  8   (wider than tf_XSize = 8)
      idx 4 (def): width = 14, kern = -2, space = 11   (default out-of-range glyph)
    Vertical metrics: tf_YSize = 11, tf_Baseline = 8, tf_BoldSmear = 2, tf_XSize = 8
    => te_Height = 11, MinY = -8, MaxY = 11 - 1 - 8 = 2.
    Italic shear: MaxX += 8 / 2 = 4, MinX -= (11 - 8) / 2 = 1.
*/
static const ULONG k_loc_abcd[5] = {
    CHARLOC(0, 6),
    CHARLOC(8, 12),
    CHARLOC(24, 5),
    CHARLOC(32, 10),
    CHARLOC(48, 14)
};
static const WORD k_kern_abcd[5] = { 1, 0, -3, 2, -2 };
static const WORD k_space_abcd[5] = { 7, 9, 6, 8, 11 };

/* Uniform 8x8 monospace glyph ('A'.. 'A' + default, both width 8, baseline 6). */
static const ULONG k_loc_mono8[2] = { CHARLOC(0, 8), CHARLOC(8, 8) };
static const WORD k_kern_neg20[2] = { -20, -20 };
static const WORD k_space_pos10[2] = { 10, 10 };

/* Wide-step tables (32767 + 32767 + TxSpacing 6 = 65540 px/char) to exceed INT32_MAX. */
static const ULONG k_loc_huge[2] = { CHARLOC(0, 10), CHARLOC(16, 10) };
static const WORD k_kern_huge[2] = { 32767, 32767 };
static const WORD k_space_huge[2] = { 32767, 32767 };

static struct GfxBase g_gfx = { 0x47465800 };
static unsigned g_checks;
static unsigned g_failures;

static void check(BOOL ok, const char *label)
{
    ++g_checks;
    if (!ok)
    {
        ++g_failures;
        printf("FAIL: %s\n", label);
    }
}

static void init_abcd_font(struct TextFont *tf, UBYTE flags,
                           const WORD *kern, const WORD *space)
{
    struct TextFont zero_tf = { 0 };
    *tf = zero_tf;
    tf->tf_YSize = 11;
    tf->tf_Flags = flags;
    tf->tf_XSize = 8;
    tf->tf_Baseline = 8;
    tf->tf_BoldSmear = 2;
    tf->tf_LoChar = (UBYTE)'A';
    tf->tf_HiChar = (UBYTE)'D';
    tf->tf_CharLoc = (APTR)(uintptr_t)k_loc_abcd;
    tf->tf_CharKern = (APTR)(uintptr_t)kern;
    tf->tf_CharSpace = (APTR)(uintptr_t)space;
}

static void init_rp(struct RastPort *rp, struct TextFont *tf,
                    UBYTE style, WORD spacing)
{
    struct RastPort zero_rp = { 0 };
    *rp = zero_rp;
    rp->Font = tf;
    rp->AlgoStyle = style;
    rp->TxHeight = tf->tf_YSize;
    rp->TxWidth = tf->tf_XSize;
    rp->TxBaseline = tf->tf_Baseline;
    rp->TxSpacing = spacing;
}

static void expect_metrics(struct RastPort *rp, CONST_STRPTR s, WORD count,
                           WORD exp_len, UWORD exp_height,
                           WORD exp_minx, WORD exp_miny,
                           WORD exp_maxx, WORD exp_maxy,
                           const char *label)
{
    struct TextExtent te;
    WORD len;

    te.te_Width = 0xBEEFU;
    te.te_Height = 0xBEEFU;
    te.te_Extent.MinX = 12345;
    te.te_Extent.MinY = 12345;
    te.te_Extent.MaxX = -12345;
    te.te_Extent.MaxY = -12345;

    len = production_TextLength(rp, s, count, &g_gfx);
    production_TextExtent(rp, s, count, &te, &g_gfx);

    check(len == exp_len, label);
    check(te.te_Width == (UWORD)len && te.te_Width == (UWORD)exp_len, label);
    check(te.te_Height == exp_height, label);
    check(te.te_Extent.MinX == exp_minx && te.te_Extent.MaxX == exp_maxx, label);
    check(te.te_Extent.MinY == exp_miny && te.te_Extent.MaxY == exp_maxy, label);
}

static void test_table_combinations(void)
{
    struct TextFont tf;
    struct RastPort rp;

    /* 1. Both tf_CharKern and tf_CharSpace present. */
    init_abcd_font(&tf, FPF_PROPORTIONAL, k_kern_abcd, k_space_abcd);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    /* "AC": A(k=1,w=6,s=7 -> x=8), C(k=-3->5, w=5->10, s=6->11) => len=11, [0..10] */
    expect_metrics(&rp, "AC", 2, 11, 11, 0, -8, 10, 2,
                   "both tables: AC advance 11, extent [0..10]");
    /* "CA": C(k=-3->minx=-3, w=5->2, s=6->3), A(k=1->4, w=6->10, s=7->11) => len=11, [-3..10] */
    expect_metrics(&rp, "CA", 2, 11, 11, -3, -8, 10, 2,
                   "both tables: leading negative kerning CA gives [-3..10]");
    /* "AB": A(->8), B(k=0, w=12->20, s=9->17) => len=17, [0..19] (wider than advance) */
    expect_metrics(&rp, "AB", 2, 17, 11, 0, -8, 19, 2,
                   "both tables: glyph B wider than space gives MaxX 19 > te_Width-1 (16)");

    /* 2. Kerning table ONLY (tf_CharSpace == NULL; unpatched TextLength read NULL). */
    init_abcd_font(&tf, FPF_PROPORTIONAL, k_kern_abcd, NULL);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    /* "CB": C(k=-3, w=5->2, XSize=8->5), B(k=0, w=12->17, XSize=8->13) => len=13, [-3..16] */
    expect_metrics(&rp, "CB", 2, 13, 11, -3, -8, 16, 2,
                   "kern-only (FPF_PROPORTIONAL): CB uses tf_XSize=8 for space, [-3..16]");
    init_abcd_font(&tf, 0, k_kern_abcd, NULL);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    expect_metrics(&rp, "CB", 2, 13, 11, -3, -8, 16, 2,
                   "kern-only (flags=0): CB uses tf_XSize=8 for space, [-3..16]");

    /* 3. Space table ONLY (tf_CharKern == NULL; unpatched TextLength read NULL). */
    init_abcd_font(&tf, FPF_PROPORTIONAL, NULL, k_space_abcd);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    /* "CB": C(k=0, w=5->5, s=6->6), B(k=0, w=12->18, s=9->15) => len=15, [0..17] */
    expect_metrics(&rp, "CB", 2, 15, 11, 0, -8, 17, 2,
                   "space-only (FPF_PROPORTIONAL): CB treats missing kern as 0, [0..17]");
    init_abcd_font(&tf, 0, NULL, k_space_abcd);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    expect_metrics(&rp, "CB", 2, 15, 11, 0, -8, 17, 2,
                   "space-only (flags=0): CB treats missing kern as 0, [0..17]");

    /* 4. Neither table: both FPF_PROPORTIONAL (unpatched read NULL) and fixed-width. */
    init_abcd_font(&tf, FPF_PROPORTIONAL, NULL, NULL);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    /* "AB": A(x=0..8), B(x=8, w=12->20, x+=8->16) => len=16, [0..19] */
    expect_metrics(&rp, "AB", 2, 16, 11, 0, -8, 19, 2,
                   "neither table (FPF_PROPORTIONAL): AB len=16, MaxX=19 from w=12");

    init_abcd_font(&tf, 0, NULL, NULL);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    /* "AC": A(w=6 <= 8), C(w=5 <= 8) => len=16, [0..15] */
    expect_metrics(&rp, "AC", 2, 16, 11, 0, -8, 15, 2,
                   "neither table (fixed-width): glyphs <= tf_XSize end at te_Width-1 (15)");
    /* "AB": B has w=12 > tf_XSize=8 => len=16, MaxX=19 (unpatched gave 15) */
    expect_metrics(&rp, "AB", 2, 16, 11, 0, -8, 19, 2,
                   "neither table (fixed-width): glyph B wider than tf_XSize gives MaxX=19");
    /* "DA": D(x=0, w=10->10, x=8), A(x=8, w=6->14, x=16) => len=16, [0..15];
       "D" alone: x=0, w=10->10, x=8 => len=8, [0..9] */
    expect_metrics(&rp, "D", 1, 8, 11, 0, -8, 9, 2,
                   "neither table (fixed-width): single wide glyph D (w=10) gives MaxX=9");
}

static void test_default_glyph_spacing_and_styles(void)
{
    struct TextFont tf;
    struct RastPort rp;
    const char out_of_range[3] = { '!', 'A', (char)0xFF };

    /* Out-of-range characters (< tf_LoChar, > tf_HiChar, and 0xFF) use idx 4 (w=14,k=-2,s=11). */
    init_abcd_font(&tf, FPF_PROPORTIONAL, k_kern_abcd, k_space_abcd);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    /* '!'(def: x=-2,x2=12,x=9), 'A'(x=10,x2=16,x=17), 0xFF(def: x=15,x2=29,x=26) => len=26, [-2..28] */
    expect_metrics(&rp, out_of_range, 3, 26, 11, -2, -8, 28, 2,
                   "default glyph used for < LoChar, > HiChar, and 0xFF");

    init_abcd_font(&tf, 0, NULL, NULL);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    /* 'Z' > 'D' -> default glyph (w=14 > tf_XSize=8) => len=8, [0..13] */
    expect_metrics(&rp, "Z", 1, 8, 11, 0, -8, 13, 2,
                   "fixed-width out-of-range Z uses default glyph width 14 -> MaxX=13");

    /* Negative TxSpacing on fixed-width 8x8 font ("AAAA" with TxSpacing = -4, as in textlong.c):
       positions 0..8, 4..12, 8..16, 12..20 => len = 4 * 4 = 16, [0..19]. */
    struct TextFont mono = { 0 };
    mono.tf_YSize = 8;
    mono.tf_XSize = 8;
    mono.tf_Baseline = 6;
    mono.tf_BoldSmear = 1;
    mono.tf_LoChar = (UBYTE)'A';
    mono.tf_HiChar = (UBYTE)'A';
    mono.tf_CharLoc = (APTR)(uintptr_t)k_loc_mono8;
    init_rp(&rp, &mono, FS_NORMAL, -4);
    expect_metrics(&rp, "AAAA", 4, 16, 8, 0, -6, 19, 1,
                   "negative TxSpacing (-4) on 8px font: len=16, MaxX=19, Y=[-6..1]");

    /* Strongly negative TxSpacing (-12 on tf_XSize=8, w=6): net step -4 per char.
       "AAA": char0 x=0,x2=6,x=8->-4; char1 x=-4,x2=2,x=4->-8; char2 x=-8,x2=-2,x=0->-12
       => len = -12, minx = -12, maxx = 8 - 1 = 7. */
    init_abcd_font(&tf, 0, NULL, NULL);
    init_rp(&rp, &tf, FS_NORMAL, -12);
    expect_metrics(&rp, "AAA", 3, -12, 11, -12, -8, 7, 2,
                   "negative TxSpacing (-12) moving left: len=-12, MinX=-12, MaxX=7");

    /* Negative TxSpacing (-5) with proportional tables on "AC":
       A: x=1,x2=7,x=8,x=3; C: x=0,x2=5,x=6,x=1 => len=1, [0..7]. */
    init_abcd_font(&tf, FPF_PROPORTIONAL, k_kern_abcd, k_space_abcd);
    init_rp(&rp, &tf, FS_NORMAL, -5);
    expect_metrics(&rp, "AC", 2, 1, 11, 0, -8, 7, 2,
                   "negative TxSpacing (-5) with both tables: len=1, [0..7]");

    /* Bold (tf_BoldSmear = 2) and Italic (Baseline/2 = 4, (YSize-Baseline)/2 = 1) on "CA":
       unstyled "CA" is len=11, MinX=-3, MaxX=10. */
    init_rp(&rp, &tf, FSF_BOLD, 0);
    expect_metrics(&rp, "CA", 2, 11, 11, -3, -8, 12, 2,
                   "FSF_BOLD adds tf_BoldSmear (2) to MaxX without changing te_Width");
    init_rp(&rp, &tf, FSF_ITALIC, 0);
    expect_metrics(&rp, "CA", 2, 11, 11, -4, -8, 14, 2,
                   "FSF_ITALIC shifts MinX by -1 and MaxX by +4 without changing te_Width");
    init_rp(&rp, &tf, FSF_BOLD | FSF_ITALIC, 0);
    expect_metrics(&rp, "CA", 2, 11, 11, -4, -8, 16, 2,
                   "FSF_BOLD | FSF_ITALIC combines MinX=-4 and MaxX=16");
}

static void test_counts_and_word_clamping(void)
{
    struct TextFont tf;
    struct TextFont mono = { 0 };
    struct TextFont huge = { 0 };
    struct RastPort rp;
    char one_byte = 'A';
    static char buf_8193[8193];
    static char buf_32767[32767];

    for (unsigned i = 0; i < sizeof(buf_8193); ++i)
        buf_8193[i] = 'A';
    for (unsigned i = 0; i < sizeof(buf_32767); ++i)
        buf_32767[i] = 'A';

    /* count == 0 and negative counts on proportional/table fonts never read past 0 bytes. */
    init_abcd_font(&tf, FPF_PROPORTIONAL, k_kern_abcd, k_space_abcd);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    expect_metrics(&rp, &one_byte, 0, 0, 11, 0, -8, 0, 2,
                   "proportional count == 0 gives len=0, [0..0]");
    expect_metrics(&rp, &one_byte, -1, 0, 11, 0, -8, 0, 2,
                   "proportional count == -1 gives len=0, [0..0] without overrunning buffer");
    expect_metrics(&rp, &one_byte, -32768, 0, 11, 0, -8, 0, 2,
                   "proportional count == -32768 gives len=0, [0..0]");

    /* count == 0 and negative count on fixed-width no-table font. */
    init_abcd_font(&tf, 0, NULL, NULL);
    init_rp(&rp, &tf, FS_NORMAL, 0);
    expect_metrics(&rp, &one_byte, 0, 0, 11, 0, -8, -1, 2,
                   "fixed-width count == 0 gives len=0, [0..-1]");
    /* count = -5: len = -5 * 8 = -40 (te_Width = 65496), maxx = 65495 -> clamped to 32767. */
    expect_metrics(&rp, &one_byte, -5, -40, 11, 0, -8, 32767, 2,
                   "fixed-width count == -5 does not dereference string and clamps MaxX to 32767");

    /* Extents reaching and exceeding +32767 on 8x8 monospace font (YSize=16, Baseline=12 -> Y=[-12..3]). */
    mono.tf_YSize = 16;
    mono.tf_XSize = 8;
    mono.tf_Baseline = 12;
    mono.tf_BoldSmear = 2;
    mono.tf_LoChar = (UBYTE)'A';
    mono.tf_HiChar = (UBYTE)'A';
    mono.tf_CharLoc = (APTR)(uintptr_t)k_loc_mono8;
    init_rp(&rp, &mono, FS_NORMAL, 0);

    /* 4096 * 8 = 32768 px: WORD TextLength = -32768 (te_Width = 32768), MaxX = 32767. */
    expect_metrics(&rp, buf_8193, 4096, -32768, 16, 0, -12, 32767, 3,
                   "4096 * 8px reaches exact MaxX = 32767 with Y=[-12..3]");
    /* 4097 * 8 = 32776 px: WORD TextLength = -32760 (te_Width = 32776), MaxX stops at 32767. */
    expect_metrics(&rp, buf_8193, 4097, -32760, 16, 0, -12, 32767, 3,
                   "4097 * 8px (32775) clamps MaxX at 32767");
    /* 8193 * 8 = 65544 px = 65536 + 8: WORD TextLength = 8 (te_Width = 8), MaxX stops at 32767. */
    expect_metrics(&rp, buf_8193, 8193, 8, 16, 0, -12, 32767, 3,
                   "8193 * 8px (65544) clamps MaxX at 32767 instead of wrapping to 7");
    /* 4096 * 8px + FSF_BOLD (BoldSmear=2 -> 32769) clamps MaxX at 32767. */
    init_rp(&rp, &mono, FSF_BOLD, 0);
    expect_metrics(&rp, buf_8193, 4096, -32768, 16, 0, -12, 32767, 3,
                   "FSF_BOLD past 32767 clamps MaxX at 32767");

    /* Extents passing -32768 via negative TxSpacing (-18 on 8px font -> net -10/char):
       3277 * (-10) = -32770 -> WORD len = +32766 (te_Width = 32766), MinX stops at -32768, MaxX = 7. */
    init_rp(&rp, &mono, FS_NORMAL, -18);
    expect_metrics(&rp, buf_8193, 3277, 32766, 16, -32768, -12, 7, 3,
                   "negative TxSpacing past -32768 clamps MinX at -32768 and keeps MaxX=7");

    /* Extents passing -32768 via negative tf_CharKern (-20) + space (+10), w=8, TxSpacing=0:
       char0: x=-20, x2=-12, x=-10 (maxx stays 0); after 3278 chars x=-32780 (kern hit -32790);
       maxx-- gives MaxX = -1, MinX stops at -32768; WORD len = -32780 + 65536 = 32756. */
    mono.tf_Flags = FPF_PROPORTIONAL;
    mono.tf_CharKern = (APTR)(uintptr_t)k_kern_neg20;
    mono.tf_CharSpace = (APTR)(uintptr_t)k_space_pos10;
    init_rp(&rp, &mono, FS_NORMAL, 0);
    expect_metrics(&rp, buf_8193, 3278, 32756, 16, -32768, -12, -1, 3,
                   "negative kerning past -32768 clamps MinX at -32768 and MaxX=-1");

    /* QUAD accumulator past 32-bit LONG (> INT32_MAX):
       per char = 32767 + 32767 + 6 = 65540 (65536 + 4);
       32767 * 65540 = 2,147,549,180 > INT32_MAX; mod 65536 = 32767 * 4 = -4 (te_Width = 65532). */
    huge.tf_YSize = 10;
    huge.tf_Flags = FPF_PROPORTIONAL;
    huge.tf_XSize = 10;
    huge.tf_Baseline = 7;
    huge.tf_LoChar = (UBYTE)'A';
    huge.tf_HiChar = (UBYTE)'A';
    huge.tf_CharLoc = (APTR)(uintptr_t)k_loc_huge;
    huge.tf_CharKern = (APTR)(uintptr_t)k_kern_huge;
    huge.tf_CharSpace = (APTR)(uintptr_t)k_space_huge;
    init_rp(&rp, &huge, FS_NORMAL, 6);
    expect_metrics(&rp, buf_32767, 32767, -4, 10, 0, -7, 32767, 2,
                   "32767 chars exceeding INT32_MAX do not overflow QUAD and clamp MaxX at 32767");
}

int main(void)
{
    test_table_combinations();
    test_default_glyph_spacing_and_styles();
    test_counts_and_word_clamping();

    printf("test_textextent: %u checks, %u failed\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
