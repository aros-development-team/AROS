#include <intuition/diattr.h>
#include <intuition/iobsolete.h>
#include <proto/intuition.h>

#include <stdio.h>

struct RefResults
{
    IPTR val;
    IPTR error;
};

static UWORD test_pens[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static struct DrawInfo test_di =
{
    2,
    6,
    test_pens,
    0xC0DEBAD1,
    4,
    {22, 22},
    DRIF_NEWLOOK,
    0xC0DEBAD2,
    0xC0DEBAD3
};

static void TestAttr(ULONG id, struct DrawInfo *dinfo)
{
    IPTR err = 0;
    IPTR val = GetDrawInfoAttr(dinfo, id, &err);

    printf("AttrID 0x%08X Value %08lX\tError: %ld\n", id, val, err);
}

static void TestAllAttrs(struct DrawInfo *dinfo)
{
    ULONG attr;

    /* All colors, including invalid one */
    for (attr = DRIPEN_DETAIL; attr <= DRIPEN_NUMDRIPENS; attr++)
    {
        TestAttr(GDIA_Color | attr, dinfo);
    }

    /* Now all pens */
    for (attr = DRIPEN_DETAIL; attr <= DRIPEN_NUMDRIPENS; attr++)
    {
        TestAttr(GDIA_Pen | attr, dinfo);
    }

    /* The rest of attributes, including invalid attr */
    for (attr = GDIA_Version; attr <= 0x00C00000; attr += 0x00100000)
    {
        TestAttr(attr, dinfo);
    }
}

int main(void)
{
    struct Screen *scr = LockPubScreen(NULL);
    struct DrawInfo *di;

    if (!scr)
    {
        printf("Failed to lock default public screen!\n");
        return 20;
    }

    di = GetScreenDrawInfo(scr);
    printf("Checking attributes for DrawInfo %p public screen %p\n", di, scr);
    TestAllAttrs(di);

#ifdef __MORPHOS__
    /* On MorphOS the function simply fails if meets non-own dri_Version */
    test_di.dri_Version = di->dri_Version;
#endif
    printf("Checking test DrawInfo\n");
    TestAllAttrs(&test_di);

    UnlockPubScreen(NULL, scr);
    printf("Checking default values...\n");
    TestAllAttrs(NULL);

    return 0;
}
