/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native area fill regression for wide and tall polygons.

    Build with make test-graphics, also included by make test.
    Run areafill_range from a native Shell. No screen, window, display
    pixels or input state is changed.

    Each case fills one polygon with AreaMove(), AreaDraw() and AreaEnd()
    into its own one-plane bitmap, which covers the whole bounds of the
    polygon. The TmpRas has exactly the size AreaEnd() requires and sits
    between two guard blocks. The program checks the result of AreaEnd(),
    the number of bits AreaEnd() left in the TmpRas mask, the number of
    pixels in the bitmap, and both guard blocks.

    Two limits of the fill code are covered:
      - The outline walk doubled its error term in a WORD. An edge whose
        longer span is 10924 or more overflowed it, and the walk then left
        the bounds of the polygon and wrote outside the TmpRas raster.
        flat-10923 is the last span that was in range; flat-10924,
        flat-32766 and tall-32766 are past it.
      - The scanline counter was a WORD that could not pass a MaxY of
        32767, so AreaEnd() never returned for such a polygon. top-row has
        that MaxY.

    top-row runs first, in a child process. If it has not returned after
    five seconds the remaining cases are not run: on a library without the
    wider arithmetic the wide cases would write far outside their raster.
    The waiting loop itself only rewrites one row inside its own raster.

    The expected counts follow from the shapes. A flat triangle
    (0,0),(W,1),(0,1) has its whole lower row, W + 1 pixels, and the first
    W / 2 + 1 pixels of its upper row. The tall one (0,0),(1,H),(0,H) has
    its whole left column, H + 1 pixels, and the last H / 2 of its right
    column. The small shapes show that ordinary output is unchanged.

    flat-full and tall-full span all 32768 values of a coordinate. Their
    mask is checked, but not their pixels: AreaEnd() draws the mask with
    BltTemplate(), whose sizes are WORDs, so no such span reaches a bitmap.

    Success prints:
      areafill_range: 40 checks, 0 failed
    and returns zero. A regression returns 10; the failing lines retain
    both the observed and expected values.

    This fixture does not qualify layered or deeper bitmaps, area patterns,
    AreaEllipse(), negative coordinates or the four-vertex fast path beyond
    one small square.
*/

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include <stdio.h>
#include <string.h>

#define GUARD 4096
#define MAXPOINTS 8

struct Case
{
    const char *name;
    WORD points[MAXPOINTS][2];
    UWORD count;
    ULONG want;         /* bits of the mask, and pixels unless full */
    BOOL full;          /* spans 32768 pixels: BltTemplate() takes no such size */
};

struct Work
{
    const struct Case *c;
    struct BitMap *bitmap;
    struct RastPort rp;
    struct AreaInfo area;
    struct TmpRas tmpras;
    UBYTE vectors[(MAXPOINTS + 2) * 5];
    UBYTE *block;       /* GUARD, raster, GUARD */
    ULONG size;
    WORD minx, miny, maxx, maxy;
    LONG result;
    volatile LONG done;
};

struct GfxBase *GfxBase;
static unsigned checks, failed;

static const struct Case toprow =
    { "top-row", { {0, 32766}, {5, 32767}, {0, 32767} }, 3, 9, FALSE };

static const struct Case cases[] =
{
    { "square", { {2, 2}, {9, 2}, {9, 9}, {2, 9} }, 4, 64, FALSE },
    { "triangle", { {0, 0}, {20, 0}, {0, 20} }, 3, 231, FALSE },
    { "arrow", { {10, 0}, {20, 10}, {14, 10}, {14, 24}, {6, 24}, {6, 10}, {0, 10} }, 7, 247, FALSE },
    { "flat-10923", { {0, 0}, {10923, 1}, {0, 1} }, 3, 16386, FALSE },
    { "flat-10924", { {0, 0}, {10924, 1}, {0, 1} }, 3, 16388, FALSE },
    { "flat-32766", { {0, 0}, {32766, 1}, {0, 1} }, 3, 49151, FALSE },
    { "tall-32766", { {0, 0}, {1, 32766}, {0, 32766} }, 3, 49150, FALSE },
    { "flat-full", { {0, 0}, {32767, 1}, {0, 1} }, 3, 49152, TRUE },
    { "tall-full", { {0, 0}, {1, 32767}, {0, 32767} }, 3, 49152, TRUE },
};

static void check(int ok, const char *kind, const char *name, long observed, long expected)
{
    checks++;
    if (!ok)
    {
        failed++;
        printf("FAIL %s: %s observed=%ld expected=%ld\n", kind, name, observed, expected);
    }
}

static BOOL prepare(struct Work *w, const struct Case *c)
{
    ULONG width, row, i;

    memset(w, 0, sizeof(*w));
    w->c = c;
    w->minx = w->maxx = c->points[0][0];
    w->miny = w->maxy = c->points[0][1];
    for (i = 1; i < c->count; i++)
    {
        if (c->points[i][0] < w->minx) w->minx = c->points[i][0];
        if (c->points[i][0] > w->maxx) w->maxx = c->points[i][0];
        if (c->points[i][1] < w->miny) w->miny = c->points[i][1];
        if (c->points[i][1] > w->maxy) w->maxy = c->points[i][1];
    }
    /* The row length that AreaEnd() derives from the bounds. */
    width = (ULONG)(w->maxx - w->minx + 1);
    row = (width & 0x0f) ? ((width >> 3) & 0xfffe) + 2 : (width >> 3) & 0xfffe;
    w->size = row * (ULONG)(w->maxy - w->miny + 1);
    w->block = AllocVec(GUARD + w->size + GUARD, MEMF_ANY);
    w->bitmap = AllocBitMap((ULONG)w->maxx + 1, (ULONG)w->maxy + 1, 1, BMF_CLEAR, NULL);
    if (!w->block || !w->bitmap)
        return FALSE;
    memset(w->block, 0xA5, GUARD + w->size + GUARD);
    memset(w->block + GUARD, 0, w->size);
    InitRastPort(&w->rp);
    w->rp.BitMap = w->bitmap;
    SetAPen(&w->rp, 1);
    SetDrMd(&w->rp, JAM1);
    InitArea(&w->area, w->vectors, MAXPOINTS + 2);
    w->rp.AreaInfo = &w->area;
    InitTmpRas(&w->tmpras, w->block + GUARD, w->size);
    w->rp.TmpRas = &w->tmpras;
    return TRUE;
}

static void fill(struct Work *w)
{
    const struct Case *c = w->c;
    UWORD i;

    AreaMove(&w->rp, c->points[0][0], c->points[0][1]);
    for (i = 1; i < c->count; i++)
        AreaDraw(&w->rp, c->points[i][0], c->points[i][1]);
    w->result = AreaEnd(&w->rp);
}

static void judge(struct Work *w)
{
    const struct Case *c = w->c;
    ULONG pixels = 0, mask = 0, broken = 0, i;
    LONG x, y;

    for (i = 0; i < GUARD; i++)
    {
        if (w->block[i] != 0xA5)
            broken++;
        if (w->block[GUARD + w->size + i] != 0xA5)
            broken++;
    }
    for (i = 0; i < w->size; i++)
    {
        UBYTE b = w->block[GUARD + i];

        for (; b; b >>= 1)
            mask += b & 1u;
    }
    check(w->result == 0, "RESULT", c->name, w->result, 0);
    check(mask == c->want, "MASK", c->name, (long)mask, (long)c->want);
    if (!c->full)
    {
        for (y = w->miny; y <= w->maxy; y++)
            for (x = w->minx; x <= w->maxx; x++)
                if (ReadPixel(&w->rp, x, y))
                    pixels++;
        check(pixels == c->want, "PIXELS", c->name, (long)pixels, (long)c->want);
    }
    check(broken == 0, "GUARDS", c->name, (long)broken, 0);
}

static void release(struct Work *w)
{
    if (w->bitmap)
        FreeBitMap(w->bitmap);
    FreeVec(w->block);
}

static struct Work *child_work;

static void child(void)
{
    struct Work *w = child_work;

    fill(w);
    w->done = 1;
}

int main(void)
{
    static struct Work first, work;
    struct TagItem tags[] =
    {
        { NP_Entry, (IPTR)child },
        { NP_Name, (IPTR)"areafill_range top-row" },
        { NP_StackSize, 65536 },
        { NP_Input, 0 },
        { NP_CloseInput, FALSE },
        { NP_Output, 0 },
        { NP_CloseOutput, FALSE },
        { TAG_DONE, 0 }
    };
    ULONG i;

    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    check(GfxBase != NULL, "LIBRARY", "graphics.library", GfxBase != NULL, 1);
    if (!GfxBase)
        goto done;
    printf("graphics version=%u.%u\n",
        (unsigned)GfxBase->LibNode.lib_Version,
        (unsigned)GfxBase->LibNode.lib_Revision);

    if (!prepare(&first, &toprow))
    {
        check(0, "MEMORY", toprow.name, 0, 1);
        release(&first);
        goto done;
    }
    child_work = &first;
    if (!CreateNewProc(tags))
    {
        check(0, "PROCESS", toprow.name, 0, 1);
        release(&first);
        goto done;
    }
    for (i = 0; i < 50 && !first.done; i++)
        Delay(5);
    check(first.done != 0, "RETURN", toprow.name, first.done, 1);
    if (!first.done)
    {
        /* The child still uses the raster and the bitmap: nothing is freed,
           and the wide cases, which this library cannot hold, are not run. */
        printf("areafill_range: AreaEnd() did not return; the remaining cases were not run\n");
        goto done;
    }
    judge(&first);
    release(&first);

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        if (prepare(&work, &cases[i]))
        {
            fill(&work);
            judge(&work);
        }
        else
        {
            check(0, "MEMORY", cases[i].name, 0, 1);
        }
        release(&work);
    }

done:
    if (GfxBase && (!child_work || first.done))
        CloseLibrary((struct Library *)GfxBase);
    printf("areafill_range: %u checks, %u failed\n", checks, failed);
    return failed ? 10 : 0;
}
