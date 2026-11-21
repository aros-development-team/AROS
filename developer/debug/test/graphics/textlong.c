/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Text() of a text wider than a WORD counts. graphics works out the extent
    of a text, and the raster that it draws the text into, in WORDs, so 8193
    characters of topaz 8, 65544 pixels wide, got a raster of 8 pixels and
    were drawn far past it. Text() now draws a text in pieces of at most
    16384 pixels and leaves out what lies past the coordinates of a WORD.
    The RastPort gets a TmpRas with guard bytes before and after it, which
    Text() takes for a raster that fits in it: a small one shows a raster
    worked out too small, a large one that no piece goes past its raster.
    After a raster written past its guards the test stops, as what follows
    would write past rasters without guards. In JAM2, the X and the Y on
    either side of a piece boundary inside the window come out as XY drawn
    alone, and a last piece, a space, would wipe the first character if it
    were drawn where its WORD coordinate wraps. A count of 0 or less draws
    nothing. The extent of a text wider than a WORD stops at 32767.
    TextExtent() ended the extent of a font without tables at the advance
    of the text, so four y 4 pixels apart got a raster too small for the
    last one, which a TmpRas of that size shows.
    Output: a line per check and a summary.
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <exec/memory.h>
#include <graphics/gfxmacros.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <stdio.h>
#include <string.h>

#define LONGTEXT 8193
#define PIECE 2048          /* characters of topaz 8 in a piece */
#define SMALL 4096
#define LARGE 16448         /* a piece of topaz 8, bold too */
#define NARROW 16           /* a raster of 16 by 8 pixels */
/* A raster drawn as if 8 pixels wide reaches 8193 bytes on, or a WORD of
   pixels back. */
#define GUARD 16384
/* The renderers OR glyph bits into their raster, which any bit set in a
   guard byte of 0 shows. */
#define GUARDBYTE 0x00

static int checks, failures;

static int check(int ok, const char *what)
{
    checks++;
    if (!ok)
        failures++;
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    fflush(stdout);
    return ok;
}

/* A TmpRas of size bytes between guard bytes, in area */
static void guard(struct RastPort *rp, struct TmpRas *tmpras, UBYTE *area, ULONG size)
{
    memset(area, GUARDBYTE, GUARD);
    memset(area + GUARD + size, GUARDBYTE, GUARD);
    InitTmpRas(tmpras, area + GUARD, size);
    rp->TmpRas = tmpras;
}

static int guarded(const UBYTE *area, ULONG size)
{
    ULONG i;

    for (i = 0; i < GUARD; i++)
        if (area[i] != GUARDBYTE || area[GUARD + size + i] != GUARDBYTE)
            return 0;
    return 1;
}

/* The pixels of w columns from x, on the 8 rows of topaz 8 about the
   baseline y */
static void cell(struct RastPort *rp, WORD x, WORD y, WORD w, LONG *pixels)
{
    WORD cx, cy;

    for (cy = 0; cy < 8; cy++)
        for (cx = 0; cx < w; cx++)
            *pixels++ = ReadPixel(rp, x + cx, y - rp->TxBaseline + cy);
}

/* Whether a pixel of those columns is not what it was */
static int drawn(struct RastPort *rp, WORD x, WORD y, WORD w, const LONG *before)
{
    LONG now[128];
    int i;

    cell(rp, x, y, w, now);
    for (i = 0; i < 8 * w; i++)
        if (now[i] != before[i])
            return 1;
    return 0;
}

static void texts(struct Window *win, UBYTE *area, UBYTE *text)
{
    struct RastPort *rp = win->RPort;
    struct TmpRas tmpras;
    struct TextExtent te;
    LONG before[128];
    WORD top = win->BorderTop, x, y;
    int wiped;

    /* a background of pen 0 for the cells to change from */
    SetAPen(rp, 0);
    RectFill(rp, win->BorderLeft, win->BorderTop, win->Width - win->BorderRight - 1,
             win->Height - win->BorderBottom - 1);
    SetAPen(rp, 1);

    /* 8193 characters, plain topaz 8 drawn a byte at a time, then bold
       drawn a pixel at a time, into a raster too small for the old count */
    memset(text, 'W', LONGTEXT);
    guard(rp, &tmpras, area, SMALL);
    y = top + 10;
    cell(rp, 10, y, 8, before);
    Move(rp, 10, y);
    Text(rp, text, LONGTEXT);
    if (!check(guarded(area, SMALL), "Text() of 8193 characters of topaz 8 stays inside its raster"))
        return;
    check(rp->cp_x == (WORD)(10 + LONGTEXT * 8) && rp->cp_y == y,
          "and moves the pen by their width, modulo a WORD");
    check(drawn(rp, 10, y, 8, before), "and draws the first of them");
    guard(rp, &tmpras, area, SMALL);
    SetSoftStyle(rp, FSF_BOLD, AskSoftStyle(rp));
    y = top + 20;
    cell(rp, 10, y, 8, before);
    Move(rp, 10, y);
    Text(rp, text, LONGTEXT);
    SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
    if (!check(guarded(area, SMALL), "Text() of 8193 bold characters stays inside its raster"))
        return;
    check(rp->cp_x == (WORD)(10 + LONGTEXT * 8), "and moves the pen as plain ones");
    check(drawn(rp, 10, y, 8, before), "and draws the first of them");

    /* two whole pieces and one more character, the boundary of the first
       two at x = 110, all of them drawn into a TmpRas that fits a piece:
       in JAM2 the X and the Y on either side of it come out as XY drawn
       alone, where two spaces wiped XY first */
    memset(text, 'W', 2 * PIECE + 1);
    text[PIECE - 1] = 'X';
    text[PIECE] = 'Y';
    x = 110 - PIECE * 8;
    y = top + 35;
    SetDrMd(rp, JAM2);
    Move(rp, 102, y);
    Text(rp, (CONST_STRPTR)"XY", 2);
    cell(rp, 102, y, 16, before);
    Move(rp, 102, y);
    Text(rp, (CONST_STRPTR)"  ", 2);
    wiped = drawn(rp, 102, y, 16, before);
    guard(rp, &tmpras, area, LARGE);
    Move(rp, x, y);
    Text(rp, text, 2 * PIECE + 1);
    SetDrMd(rp, JAM1);
    if (!check(guarded(area, LARGE), "pieces of 2048 characters stay inside their raster"))
        return;
    check(wiped && !drawn(rp, 102, y, 16, before),
          "the last character of a piece and the first of the next are drawn");
    guard(rp, &tmpras, area, LARGE);
    SetSoftStyle(rp, FSF_BOLD, AskSoftStyle(rp));
    Move(rp, x, top + 45);
    Text(rp, text, 2 * PIECE + 1);
    SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
    if (!check(guarded(area, LARGE), "bold pieces stay inside their raster"))
        return;

    /* a space in JAM2 at the end, past 32767 pixels: it is not drawn
       where its WORD coordinate wraps, on the first character, which keeps
       the pixels of a W drawn alone */
    memset(text, 'W', LONGTEXT);
    text[LONGTEXT - 1] = ' ';
    rp->TmpRas = NULL;
    SetDrMd(rp, JAM2);
    y = top + 55;
    Move(rp, 10, y);
    Text(rp, text, 1);
    cell(rp, 10, y, 8, before);
    Move(rp, 10, y);
    Text(rp, text, LONGTEXT);
    SetDrMd(rp, JAM1);
    check(!drawn(rp, 10, y, 8, before), "a piece past 32767 pixels does not wrap onto the first one");

    /* a count of 0 or less draws nothing */
    y = top + 70;
    cell(rp, 10, y, 8, before);
    Move(rp, 10, y);
    Text(rp, text, 0);
    check(rp->cp_x == 10 && rp->cp_y == y && !drawn(rp, 10, y, 8, before), "Text() of 0 characters draws nothing");
    Text(rp, text, -1);
    check(rp->cp_x == 10 && rp->cp_y == y && !drawn(rp, 10, y, 8, before), "Text() of -1 characters draws nothing");
    Text(rp, text, -32768);
    check(rp->cp_x == 10 && rp->cp_y == y && !drawn(rp, 10, y, 8, before), "Text() of -32768 characters draws nothing");

    /* the extent of a text wider than a WORD stops at 32767 */
    memset(text, 'W', LONGTEXT);
    TextExtent(rp, text, 4096, &te);
    check(te.te_Extent.MinX == 0 && te.te_Extent.MaxX == 32767,
          "TextExtent() of 4096 characters of topaz 8 ends at 32767");
    TextExtent(rp, text, LONGTEXT, &te);
    check(te.te_Extent.MinX == 0 && te.te_Extent.MaxX == 32767,
          "TextExtent() of 8193 characters stops at 32767");

    /* four y with a TxSpacing of -4: the last one, at 12, reaches past
       the 16 pixels of their advance, and its columns past them come out
       as those of a y drawn alone at 22 a row below */
    y = top + 85;
    Move(rp, 22, y + 10);
    Text(rp, (CONST_STRPTR)"y", 1);
    cell(rp, 26, y + 10, 4, before);
    rp->TxSpacing = -4;
    guard(rp, &tmpras, area, NARROW);
    Move(rp, 10, y);
    Text(rp, (CONST_STRPTR)"yyyy", 4);
    rp->TxSpacing = 0;
    rp->TmpRas = NULL;
    if (!check(guarded(area, NARROW), "four y 4 pixels apart stay inside the raster of their extent"))
        return;
    check(!drawn(rp, 26, y, 4, before), "and the last of them is drawn whole");
}

int main(void)
{
    static struct TextAttr topaz8 = { (STRPTR)"topaz.font", 8, FS_NORMAL, 0 };
    struct TextFont *font = NULL;
    struct Window *win;
    UBYTE *area, *text;

    area = AllocVec(2 * GUARD + LARGE, MEMF_ANY);
    text = AllocVec(LONGTEXT, MEMF_ANY);
    win = OpenWindowTags(NULL, WA_Left, 20, WA_Top, 20, WA_Width, 320, WA_InnerHeight, 100,
                         WA_Title, (IPTR)"textlong", WA_Flags, WFLG_DRAGBAR | WFLG_SMART_REFRESH,
                         TAG_DONE);
    if (check(area && text && win && (font = OpenFont(&topaz8)), "a window, topaz 8 and the buffers"))
    {
        struct RastPort *rp = win->RPort;
        struct TextFont *old = rp->Font;

        SetFont(rp, font);
        SetBPen(rp, 0);
        SetDrMd(rp, JAM1);
        texts(win, area, text);
        rp->TmpRas = NULL;
        rp->TxSpacing = 0;
        SetFont(rp, old);
    }
    if (font)
        CloseFont(font);
    if (win)
        CloseWindow(win);
    FreeVec(text);
    FreeVec(area);
    printf("textlong: %d checks, %d failed\n", checks, failures);
    return failures ? 20 : 0;
}
