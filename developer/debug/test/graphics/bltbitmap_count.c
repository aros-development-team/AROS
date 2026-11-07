/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native BltBitMap planar return-count and pixel regression.

    Build with make test-graphics, also included by make test.
    Run bltbitmap_count from a native Shell and separately through Run.
    No screen, window, display pixels or input state is changed. A graphical
    system exercises the driver-backed path; other systems can use the plain
    software path, which already returns the actual count.

    The program copies only its own classic planar bitmaps, exercising the
    native path selected by graphics.library. It checks the returned plane
    count, every destination byte, unchanged source bytes and allocation
    guards. Cases cover depth one, sparse/zero/out-of-depth masks, unequal
    depths, NULL/-1 destination planes, constant source planes and an empty
    rectangle. Allocation prerequisites fail explicitly.

    The expected result is the count of mask-selected writable destination
    planes below min(source depth, destination depth). Source planes represented
    by NULL or -1 remain valid constant inputs. Success prints:
      bltbitmap_count: 56 checks, 0 failed
    and returns zero. A regression returns 10; the COUNT lines retain both the
    observed and expected values.

    This fixture does not qualify truecolour/HIDD-to-HIDD return conventions,
    allocation-failure injection or the m68k native direct-blitter fast path.
*/

#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include <stdio.h>
#include <string.h>

#define WIDTH 64
#define HEIGHT 4
#define ROW 8
#define BYTES (ROW * HEIGHT)
#define GUARD 8

struct GfxBase *GfxBase;
static unsigned checks, failed;

static void check(int ok, const char *kind, const char *name)
{
    checks++;
    if (!ok)
    {
        failed++;
        printf("FAIL %s: %s\n", kind, name);
    }
}

static int getbit(const UBYTE *p, unsigned x, unsigned y)
{
    if (p == NULL)
        return 0;
    if (p == (const UBYTE *)-1)
        return 1;
    return (p[y * ROW + x / 8] >> (7 - (x & 7))) & 1;
}

static void putbit(UBYTE *p, unsigned x, unsigned y, int value)
{
    UBYTE bit = 0x80U >> (x & 7);
    if (value)
        p[y * ROW + x / 8] |= bit;
    else
        p[y * ROW + x / 8] &= (UBYTE)~bit;
}

static void planar(const char *name, UBYTE sd, UBYTE dd, UBYTE mask,
    UBYTE null_dest, UBYTE one_dest, UBYTE constant_src, WORD width)
{
    struct BitMap src, dst;
    UBYTE *sb[8] = {0}, *db[8] = {0};
    UBYTE source[8][BYTES], expected[8][BYTES];
    unsigned p, i, x, y, depth = sd < dd ? sd : dd;
    LONG want = 0, got;
    int allocated = 1, pixels_ok = 1, source_ok = 1, guards_ok = 1;

    InitBitMap(&src, sd, WIDTH, HEIGHT);
    InitBitMap(&dst, dd, WIDTH, HEIGHT);
    for (p = 0; p < 8; p++)
    {
        sb[p] = AllocMem(BYTES + 2 * GUARD, MEMF_CHIP | MEMF_PUBLIC);
        db[p] = AllocMem(BYTES + 2 * GUARD, MEMF_CHIP | MEMF_PUBLIC);
        if (!sb[p] || !db[p])
        {
            allocated = 0;
            break;
        }
        memset(sb[p], 0xa5, BYTES + 2 * GUARD);
        memset(db[p], 0x5a, BYTES + 2 * GUARD);
        for (i = 0; i < BYTES; i++)
        {
            source[p][i] = (UBYTE)(i * 37 + p * 19 + 17);
            expected[p][i] = (UBYTE)(i * 11 + p * 23 + 91);
        }
        memcpy(sb[p] + GUARD, source[p], BYTES);
        memcpy(db[p] + GUARD, expected[p], BYTES);
        if (p < sd)
            src.Planes[p] = (constant_src & (1U << p)) ?
                (p & 1 ? (PLANEPTR)-1 : NULL) : sb[p] + GUARD;
        if (p < dd)
            dst.Planes[p] = (null_dest & (1U << p)) ? NULL :
                (one_dest & (1U << p)) ? (PLANEPTR)-1 : db[p] + GUARD;
    }
    check(allocated, "ALLOC", name);
    if (!allocated)
        goto done;

    for (p = 0; p < depth; p++)
        if (width > 0 && (mask & (1U << p)) &&
            !(null_dest & (1U << p)) && !(one_dest & (1U << p)))
        {
            want++;
            for (y = 1; y < 3; y++)
                for (x = 0; x < (unsigned)width; x++)
                    putbit(expected[p], 7 + x, y,
                        getbit(src.Planes[p], 3 + x, y));
        }

    got = BltBitMap(&src, 3, 1, &dst, 7, 1, width, 2, 0xc0, mask, NULL);
    WaitBlit();
    printf("COUNT %s: got=%ld expected=%ld\n", name, (long)got, (long)want);
    check(got == want, "COUNT", name);
    for (p = 0; p < 8; p++)
    {
        if (memcmp(db[p] + GUARD, expected[p], BYTES))
            pixels_ok = 0;
        if (memcmp(sb[p] + GUARD, source[p], BYTES))
            source_ok = 0;
        for (i = 0; i < GUARD; i++)
            if (sb[p][i] != 0xa5 || sb[p][GUARD + BYTES + i] != 0xa5 ||
                db[p][i] != 0x5a || db[p][GUARD + BYTES + i] != 0x5a)
                guards_ok = 0;
    }
    check(pixels_ok, "PIXELS", name);
    check(source_ok, "SOURCE", name);
    check(guards_ok, "GUARDS", name);

done:
    for (p = 0; p < 8; p++)
    {
        if (sb[p])
            FreeMem(sb[p], BYTES + 2 * GUARD);
        if (db[p])
            FreeMem(db[p], BYTES + 2 * GUARD);
    }
}

int main(void)
{
    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 0);
    check(GfxBase != NULL, "LIBRARY", "graphics.library");
    if (!GfxBase)
        goto done;
    printf("graphics version=%u.%u\n",
        (unsigned)GfxBase->LibNode.lib_Version,
        (unsigned)GfxBase->LibNode.lib_Revision);

    planar("depth-one", 1, 1, 1, 0, 0, 0, 31);
    planar("sparse-mask", 4, 4, 5, 0, 0, 0, 31);
    planar("shallow-source", 3, 5, 255, 0, 0, 0, 31);
    planar("shallow-destination", 5, 2, 255, 0, 0, 0, 31);
    planar("zero-mask", 4, 4, 0, 0, 0, 0, 31);
    planar("mask-outside-depth", 3, 3, 128, 0, 0, 0, 31);
    planar("null-destination", 3, 3, 7, 2, 0, 0, 31);
    planar("ones-destination", 3, 3, 7, 0, 4, 0, 31);
    planar("both-sentinel-destinations", 3, 3, 7, 2, 4, 0, 31);
    planar("empty-rectangle", 3, 3, 7, 0, 0, 0, 0);
    planar("constant-source-planes", 3, 3, 7, 0, 0, 3, 31);

done:
    if (GfxBase)
        CloseLibrary((struct Library *)GfxBase);
    printf("bltbitmap_count: %u checks, %u failed\n", checks, failed);
    return failed ? 10 : 0;
}
