/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/macros.h>
#include <dos/dos.h>
#include <dos/doshunks.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

/* two hunks; one 32-bit relocation of each kind:
   hunk 0 longword 0 -> hunk 1 + 4 (HUNK_RELOC32, absolute)
   hunk 0 longword 1 -> hunk 1 relative to the longword (HUNK_ABSRELOC16)
   hunk 1 longword 1 -> hunk 0 + 4 relative to hunk 1 (HUNK_RELRELOC32) */
static const ULONG image[] =
{
    HUNK_HEADER, 0, 2, 0, 1, 2, 2,
    HUNK_CODE, 2, 0x00000004, 0x00000000,
    HUNK_RELOC32, 1, 1, 0, 0,
    HUNK_ABSRELOC16, 0x00010001, 0x00040000,
    HUNK_END,
    HUNK_DATA, 2, 0x00000000, 0x00000004,
    HUNK_RELRELOC32, 0x00010000, 0x00040000,
    HUNK_END
};

static int failures;

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

/* index of the HUNK_RELOC32 count in image[] */
#define RELOC32_COUNT 12

static BPTR load_image(CONST_STRPTR name, const ULONG *img, ULONG longs)
{
    ULONG file[sizeof(image) / sizeof(ULONG)];
    BPTR fh, seg;
    ULONG i;

    for (i = 0; i < longs; i++)
        file[i] = AROS_LONG2BE(img[i]);
    if (!(fh = Open(name, MODE_NEWFILE)))
    {
        printf("FAIL: cannot create %s\n", name);
        return BNULL;
    }
    Write(fh, file, longs * sizeof(ULONG));
    Close(fh);
    seg = LoadSeg(name);
    DeleteFile(name);
    return seg;
}

int main(void)
{
    CONST_STRPTR name = (CONST_STRPTR)"T:seglistrelocs.hunk";
    ULONG bad[sizeof(image) / sizeof(ULONG)];
    BPTR seg;
    ULONG i;

    seg = load_image(name, image, sizeof(image) / sizeof(ULONG));
    check(seg != BNULL, "LoadSeg() of the hunk file");
    if (seg)
    {
        struct HunkRelocTable *table = NULL;
        struct TagItem tags[] = { { GSLI_68KRELOCS, (IPTR)&table }, { TAG_DONE, 0 } };
        ULONG *h0 = (ULONG *)((UBYTE *)BADDR(seg) + sizeof(BPTR));
        BPTR seg1 = *(BPTR *)BADDR(seg);
        ULONG *h1 = (ULONG *)((UBYTE *)BADDR(seg1) + sizeof(BPTR));
        ULONG got = GetSegListInfo(seg, tags);

        check(seg1 != BNULL, "the seglist has a second segment");
        check(AROS_BE2LONG(h0[0]) == (ULONG)((IPTR)h1 + 4), "absolute relocation applied");
        check(AROS_BE2LONG(h0[1]) == (ULONG)((IPTR)h1 - ((IPTR)h0 + 4)), "relative relocation applied");
        check(AROS_BE2LONG(h1[1]) == (ULONG)(4 + (IPTR)h0 - (IPTR)h1), "hunk-relative relocation applied");
#ifdef __mc68000
        check(got == 0 && table == NULL, "GSLI_68KRELOCS is not answered on 68k");
#else
        check(got == 1 && table != NULL, "GSLI_68KRELOCS answered");
        if (table)
        {
            static const struct { ULONG hunk, target, kind, offset; } want[] =
            {
                { 0, 1, HUNKRELOC_ABS,  0 },
                { 0, 1, HUNKRELOC_REL,  4 },
                { 1, 0, HUNKRELOC_BASE, 4 },
            };
            check(table->hrt_Groups == 3, "three relocation groups");
            for (i = 0; i < 3 && i < table->hrt_Groups; i++)
            {
                const struct HunkRelocGroup *g = &table->hrt_Group[i];
                char what[64];
                sprintf(what, "group %lu: hunk %lu target %lu kind %lu, one offset %lu", (unsigned long)i,
                        (unsigned long)want[i].hunk, (unsigned long)want[i].target,
                        (unsigned long)want[i].kind, (unsigned long)want[i].offset);
                check(g->hrg_Hunk == want[i].hunk && g->hrg_Target == want[i].target &&
                      g->hrg_Kind == want[i].kind && g->hrg_Count == 1 &&
                      g->hrg_Offsets[0] == want[i].offset, what);
            }
        }
#endif
        UnLoadSeg(seg);
    }

    /* a count the table cannot hold must fail the load, not hang or overflow it */
    memcpy(bad, image, sizeof(image));
    bad[RELOC32_COUNT] = 0x80000001;
    check(load_image(name, bad, sizeof(bad) / sizeof(ULONG)) == BNULL, "relocation count above 2^31 fails the load");
    bad[RELOC32_COUNT] = 0x40000001;
    check(load_image(name, bad, sizeof(bad) / sizeof(ULONG)) == BNULL, "relocation count above 2^30 fails the load");

    printf("SEGLISTRELOCS %s\n", failures ? "FAIL" : "PASS");
    return failures ? 10 : 0;
}
