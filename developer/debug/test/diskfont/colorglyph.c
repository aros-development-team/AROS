/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <diskfont/diskfonttag.h>
#include <diskfont/glyph.h>
#include <diskfont/oterrors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bullet.h>

#include <stdio.h>

struct Library *BulletBase;

static int failures;

static void check(BOOL ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static struct GlyphEngine *openfont(CONST_STRPTR path, ULONG points, ULONG dpi)
{
    struct GlyphEngine *ge;

    if (!(ge = OpenEngine()))
        return NULL;
    if (SetInfo(ge, OT_Spec1 | OT_Indirect, (IPTR)path,
                    OT_DeviceDPI, (dpi << 16) | dpi,
                    OT_PointHeight, points << 16,
                    TAG_DONE) != OTERR_Success)
    {
        CloseEngine(ge);
        return NULL;
    }
    return ge;
}

/* the glyph at one size and resolution; its height is returned */
static ULONG testcolor(CONST_STRPTR path, ULONG points, ULONG dpi)
{
    struct GlyphEngine *ge;
    struct ColorGlyphMap *cgm = NULL;
    ULONG x, y, opaque = 0, coloured = 0, rc, height = 0;
    char what[80];

    if (!(ge = openfont(path, points, dpi)))
    {
        check(FALSE, "open the colour font");
        return 0;
    }

    SetInfo(ge, OT_GlyphCode, 0x1F600, TAG_DONE);
    rc = ObtainInfo(ge, OT_ColorGlyphMap32, (IPTR)&cgm, TAG_DONE);
    snprintf(what, sizeof(what), "U+1F600 as a colour glyph at %u points, %u dpi", (unsigned)points, (unsigned)dpi);
    check(rc == OTERR_Success && cgm != NULL, what);

    if (rc == OTERR_Success && cgm)
    {
        height = cgm->cgm_BlackHeight;
        printf("glyph %ux%u, modulo %u, X0 %d Y0 %d X1 %d Y1 %d, XOrigin %d, em fraction %u.%03u\n",
            (unsigned)cgm->cgm_BlackWidth, (unsigned)cgm->cgm_BlackHeight, (unsigned)cgm->cgm_BMModulo,
            (int)cgm->cgm_X0, (int)cgm->cgm_Y0, (int)cgm->cgm_X1, (int)cgm->cgm_Y1,
            (int)(cgm->cgm_XOrigin >> 16), (unsigned)(cgm->cgm_Width >> 16), (unsigned)((cgm->cgm_Width & 0xffff) * 1000 / 65536));
        /* the strike's line height is the point size, as an outline font's
           height is, so a glyph is no taller than the point size */
        snprintf(what, sizeof(what), "no taller than the %u pixels asked for", (unsigned)points);
        check(height > points / 2 && height <= points, what);
        check(cgm->cgm_X0 == -(cgm->cgm_XOrigin >> 16) && cgm->cgm_Y0 == (cgm->cgm_YOrigin >> 16) && cgm->cgm_Y1 == cgm->cgm_Y0,
            "origin fields agree as in a GlyphMap");
        check(cgm->cgm_X1 > cgm->cgm_X0 && cgm->cgm_Width > 0 && cgm->cgm_Width <= (2 << 16),
            "advance is positive and at most two ems");

        for (y = 0; y < cgm->cgm_BlackHeight; y++)
        {
            UBYTE *row = (UBYTE *)cgm->cgm_ARGBMap + y * cgm->cgm_BMModulo;

            for (x = 0; x < cgm->cgm_BlackWidth; x++)
            {
                UBYTE *px = row + x * 4;

                if (px[0] > 128)
                {
                    opaque++;
                    if (px[1] != px[2] || px[2] != px[3])
                        coloured++;
                }
            }
        }
        printf("opaque pixels %u, coloured %u\n", (unsigned)opaque, (unsigned)coloured);
        check(opaque > 0 && coloured > 0, "the glyph has opaque coloured pixels");

        ReleaseInfo(ge, OT_ColorGlyphMap32, (IPTR)cgm, TAG_DONE);
    }

    CloseEngine(ge);
    return height;
}

/* the other requests have no instance on a bitmap-only colour face */
static void testcolormono(CONST_STRPTR path)
{
    struct GlyphEngine *ge;
    struct GlyphMap *gm = NULL;
    ULONG rc;

    if (!(ge = openfont(path, 32, 72)))
    {
        check(FALSE, "open the colour font again");
        return;
    }
    SetInfo(ge, OT_GlyphCode, 0x1F600, TAG_DONE);
    rc = ObtainInfo(ge, OT_GlyphMap8Bits, (IPTR)&gm, TAG_DONE);
    check(rc != OTERR_Success && gm == NULL, "no antialiased glyph from a bitmap-only colour font");
    if (rc == OTERR_Success && gm)
        ReleaseInfo(ge, OT_GlyphMap8Bits, (IPTR)gm, TAG_DONE);
    CloseEngine(ge);
}

static void testoutline(CONST_STRPTR path)
{
    struct GlyphEngine *ge;
    struct GlyphMap *gm = NULL;
    ULONG rc;

    if (!(ge = openfont(path, 13, 72)))
    {
        check(FALSE, "open the outline font");
        return;
    }

    SetInfo(ge, OT_GlyphCode, 'A', TAG_DONE);
    rc = ObtainInfo(ge, OT_GlyphMap8Bits, (IPTR)&gm, TAG_DONE);
    check(rc == OTERR_Success && gm && gm->glm_BlackHeight > 0, "'A' from an outline font at 13 points");
    if (rc == OTERR_Success && gm)
        ReleaseInfo(ge, OT_GlyphMap8Bits, (IPTR)gm, TAG_DONE);

    CloseEngine(ge);
}

int main(int argc, char **argv)
{
    CONST_STRPTR color = argc > 1 ? (CONST_STRPTR)argv[1] : (CONST_STRPTR)"FONTS:TrueType/NotoColorEmoji.ttf";
    CONST_STRPTR outline = argc > 2 ? (CONST_STRPTR)argv[2] : (CONST_STRPTR)"FONTS:TrueType/Barlow-Bold.ttf";
    BPTR lock;

    if (!(BulletBase = OpenLibrary("freetype2.library", 0)))
    {
        printf("FAIL: no freetype2.library\n");
        return RETURN_FAIL;
    }

    if ((lock = Lock(color, SHARED_LOCK)))
    {
        ULONG h72, h144;

        UnLock(lock);
        h72 = testcolor(color, 32, 72);
        h144 = testcolor(color, 32, 144);
        check(h72 && h144 == h72, "the resolution does not change the glyph, as for an outline font");
        testcolormono(color);
    }
    else
        printf("SKIP: %s not found\n", color);

    if ((lock = Lock(outline, SHARED_LOCK)))
    {
        UnLock(lock);
        testoutline(outline);
    }
    else
        printf("SKIP: %s not found\n", outline);

    CloseLibrary(BulletBase);

    printf("%s\n", failures ? "COLORGLYPH FAIL" : "COLORGLYPH PASS");
    return failures ? RETURN_FAIL : RETURN_OK;
}
