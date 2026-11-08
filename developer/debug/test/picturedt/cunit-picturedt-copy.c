/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    CUnit tests for picture.datatype DTM_COPY: the picture goes to clipboard
    unit 0 as FORM ILBM. Each test writes a picture into a picture object
    with PDTM_WRITEPIXELARRAY, copies it, reads the clip back byte by byte
    (FORM, BMHD, CMAP, BODY, row padding, every pixel) and reads it once
    more through datatypes (DTST_CLIPBOARD), as a receiver would.

    A FORM held by clipboard unit 0 before the run is saved and written back
    at the end; if the unit was empty, the last test picture stays there.
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <devices/clipboard.h>
#include <dos/dos.h>
#include <intuition/classusr.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/datatypes.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

struct Library *DataTypesBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

static UBYTE *saved_clip = NULL;
static ULONG saved_clip_len = 0;

/* --- clipboard.device, raw ------------------------------------------------ */

static struct IOClipReq *clip_open(struct MsgPort **port)
{
    struct IOClipReq *io;

    if (!(*port = CreateMsgPort()))
        return NULL;
    io = (struct IOClipReq *) CreateIORequest(*port, sizeof(struct IOClipReq));
    if (io && OpenDevice("clipboard.device", PRIMARY_CLIP, (struct IORequest *) io, 0) == 0)
        return io;
    if (io)
        DeleteIORequest((struct IORequest *) io);
    DeleteMsgPort(*port);
    return NULL;
}

static void clip_close(struct IOClipReq *io, struct MsgPort *port)
{
    CloseDevice((struct IORequest *) io);
    DeleteIORequest((struct IORequest *) io);
    DeleteMsgPort(port);
}

/* The whole current clip of unit 0 (FreeVec it), or NULL if the unit is empty */
static UBYTE *clip_read(ULONG *len)
{
    struct MsgPort *port;
    struct IOClipReq *io;
    UBYTE head[8], dummy[16], *data = NULL;
    ULONG size;

    *len = 0;
    if (!(io = clip_open(&port)))
        return NULL;
    io->io_Command = CMD_READ;
    io->io_Offset = 0;
    io->io_ClipID = 0;
    io->io_Data = (STRPTR) head;
    io->io_Length = 8;
    DoIO((struct IORequest *) io);
    if (io->io_Actual == 8 && memcmp(head, "FORM", 4) == 0)
    {
        size = ((ULONG) head[4] << 24) | ((ULONG) head[5] << 16) | ((ULONG) head[6] << 8) | head[7];
        size = (size + 1) & ~1;
        if ((data = AllocVec(size + 8, MEMF_ANY)))
        {
            memcpy(data, head, 8);
            io->io_Command = CMD_READ;
            io->io_Data = (STRPTR) data + 8;
            io->io_Length = size;
            DoIO((struct IORequest *) io);
            *len = 8 + io->io_Actual;
        }
    }
    /* read past the end, which ends the read */
    do
    {
        io->io_Command = CMD_READ;
        io->io_Data = (STRPTR) dummy;
        io->io_Length = sizeof(dummy);
        DoIO((struct IORequest *) io);
    } while (io->io_Actual > 0);
    clip_close(io, port);
    return data;
}

static BOOL clip_write(const UBYTE *data, ULONG len)
{
    struct MsgPort *port;
    struct IOClipReq *io;
    BOOL ok;

    if (!(io = clip_open(&port)))
        return FALSE;
    io->io_Command = CMD_WRITE;
    io->io_Offset = 0;
    io->io_ClipID = 0;
    io->io_Data = (STRPTR) data;
    io->io_Length = len;
    ok = DoIO((struct IORequest *) io) == 0 && io->io_Actual == len;
    io->io_Command = CMD_UPDATE;
    ok = DoIO((struct IORequest *) io) == 0 && ok;
    clip_close(io, port);
    return ok;
}

/* --- a picture object ---------------------------------------------------- */

static Object *make_picture(UWORD w, UWORD h, UBYTE depth, ULONG fmt, const UBYTE *pixels, ULONG mod,
                            const UBYTE *rgbpal, ULONG ncolors)
{
    struct BitMapHeader *bmhd = NULL;
    Object *o;

    o = NewDTObject(NULL, DTA_SourceType, DTST_RAM, DTA_BaseName, (IPTR) "picture",
                    PDTA_DestMode, PMODE_V43, TAG_DONE);
    if (!o)
        return NULL;
    if (GetDTAttrs(o, PDTA_BitMapHeader, (IPTR) &bmhd, TAG_DONE) != 1 || !bmhd)
    {
        DisposeDTObject(o);
        return NULL;
    }
    bmhd->bmh_Width = bmhd->bmh_PageWidth = w;
    bmhd->bmh_Height = bmhd->bmh_PageHeight = h;
    bmhd->bmh_Depth = depth;
    if (rgbpal)
    {
        struct ColorRegister *cm = NULL;
        ULONG *cregs = NULL, i;

        SetDTAttrs(o, NULL, NULL, PDTA_NumColors, ncolors, TAG_DONE);
        GetDTAttrs(o, PDTA_ColorRegisters, (IPTR) &cm, PDTA_CRegs, (IPTR) &cregs, TAG_DONE);
        if (!cm || !cregs)
        {
            DisposeDTObject(o);
            return NULL;
        }
        for (i = 0; i < ncolors; i++)
        {
            cm[i].red = rgbpal[i * 3];
            cm[i].green = rgbpal[i * 3 + 1];
            cm[i].blue = rgbpal[i * 3 + 2];
            cregs[i * 3] = rgbpal[i * 3] * 0x01010101;
            cregs[i * 3 + 1] = rgbpal[i * 3 + 1] * 0x01010101;
            cregs[i * 3 + 2] = rgbpal[i * 3 + 2] * 0x01010101;
        }
    }
    if (pixels && !DoMethod(o, PDTM_WRITEPIXELARRAY, (IPTR) pixels, fmt, mod, 0, 0, w, h))
    {
        DisposeDTObject(o);
        return NULL;
    }
    return o;
}

/* --- the clip, parsed ---------------------------------------------------- */

struct ilbm
{
    UWORD w, h;
    UBYTE depth, masking, compression;
    const UBYTE *cmap;
    ULONG cmaplen;
    const UBYTE *body;
    ULONG bodylen;
    int nbmhd, ncmap, nbody;
};

static ULONG be32(const UBYTE *p)
{
    return ((ULONG) p[0] << 24) | ((ULONG) p[1] << 16) | ((ULONG) p[2] << 8) | p[3];
}

/* Every length and pad byte checked; FALSE on any structural fault */
static BOOL parse_ilbm(const UBYTE *d, ULONG len, struct ilbm *out)
{
    ULONG p, end;

    memset(out, 0, sizeof(*out));
    if (len < 12 || memcmp(d, "FORM", 4) || memcmp(d + 8, "ILBM", 4))
        return FALSE;
    end = 8 + be32(d + 4);
    if (end > len)
        return FALSE;
    for (p = 12; p < end;)
    {
        ULONG clen;

        if (p + 8 > end)
            return FALSE;
        clen = be32(d + p + 4);
        if (p + 8 + clen > end)
            return FALSE;
        if (!memcmp(d + p, "BMHD", 4))
        {
            if (clen != 20)
                return FALSE;
            out->w = (d[p + 8] << 8) | d[p + 9];
            out->h = (d[p + 10] << 8) | d[p + 11];
            out->depth = d[p + 16];
            out->masking = d[p + 17];
            out->compression = d[p + 18];
            out->nbmhd++;
        }
        else if (!memcmp(d + p, "CMAP", 4))
        {
            out->cmap = d + p + 8;
            out->cmaplen = clen;
            out->ncmap++;
        }
        else if (!memcmp(d + p, "BODY", 4))
        {
            out->body = d + p + 8;
            out->bodylen = clen;
            out->nbody++;
        }
        p += 8 + clen + (clen & 1);
    }
    return p == end;
}

/* The value (pen, or 0xBBGGRR for 24 planes) of pixel x in row y of an
   uncompressed BODY */
static ULONG body_pixel(const struct ilbm *il, ULONG x, ULONG y)
{
    ULONG rb = ((il->w + 15) >> 4) << 1, v = 0;
    const UBYTE *row = il->body + y * rb * il->depth;
    UWORD p;

    for (p = 0; p < il->depth; p++)
        if (row[p * rb + (x >> 3)] & (0x80 >> (x & 7)))
            v |= 1UL << p;
    return v;
}

static BOOL padding_clear(const struct ilbm *il)
{
    ULONG rb = ((il->w + 15) >> 4) << 1, x, y;
    UWORD p;

    for (y = 0; y < il->h; y++)
        for (p = 0; p < il->depth; p++)
            for (x = il->w; x < rb * 8; x++)
                if (il->body[(y * il->depth + p) * rb + (x >> 3)] & (0x80 >> (x & 7)))
                    return FALSE;
    return TRUE;
}

/* The clip read back through datatypes as RGB (FreeVec it) */
static UBYTE *clip_rgb_via_datatypes(UWORD *w, UWORD *h)
{
    struct BitMapHeader *bmhd = NULL;
    UBYTE *rgb = NULL;
    Object *o;

    o = NewDTObject((APTR) PRIMARY_CLIP, DTA_SourceType, DTST_CLIPBOARD, DTA_GroupID, GID_PICTURE,
                    PDTA_DestMode, PMODE_V43, PDTA_Remap, FALSE, TAG_DONE);
    if (!o)
        return NULL;
    if (GetDTAttrs(o, PDTA_BitMapHeader, (IPTR) &bmhd, TAG_DONE) == 1 && bmhd)
    {
        *w = bmhd->bmh_Width;
        *h = bmhd->bmh_Height;
        if ((rgb = AllocVec(*w * *h * 3, MEMF_ANY | MEMF_CLEAR)))
        {
            if (!DoMethod(o, PDTM_READPIXELARRAY, (IPTR) rgb, PBPAFMT_RGB, *w * 3, 0, 0, *w, *h))
            {
                FreeVec(rgb);
                rgb = NULL;
            }
        }
    }
    DisposeDTObject(o);
    return rgb;
}

/* --- suite ---------------------------------------------------------------- */

static int init_suite(void)
{
    DataTypesBase = OpenLibrary("datatypes.library", 39);
    IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 39);
    if (!DataTypesBase || !IntuitionBase)
        return -1;
    saved_clip = clip_read(&saved_clip_len);
    return 0;
}

static int clean_suite(void)
{
    if (saved_clip)
    {
        if (!clip_write(saved_clip, saved_clip_len))
            printf("could not restore the earlier clipboard contents\n");
        FreeVec(saved_clip);
        saved_clip = NULL;
    }
    if (IntuitionBase)
        CloseLibrary((struct Library *) IntuitionBase);
    if (DataTypesBase)
        CloseLibrary(DataTypesBase);
    return 0;
}

/* --- tests ---------------------------------------------------------------- */

static void testMETHODS(void)
{
    Object *o = make_picture(4, 4, 24, PBPAFMT_RGB, NULL, 0, NULL, 0);
    ULONG *methods;

    CU_ASSERT_PTR_NOT_NULL_FATAL(o);
    methods = GetDTMethods(o);
    CU_ASSERT_PTR_NOT_NULL(methods);
    /* MultiView enables Edit/Copy this way */
    CU_ASSERT_PTR_NOT_NULL(FindMethod(methods, DTM_COPY));
    DisposeDTObject(o);
}

/* Without picture data DTM_COPY fails, says why and leaves the clip alone */
static void testNODATA(void)
{
    static const UBYTE marker[] = "FORM\0\0\0\x0c" "FTXTCHRS\0\0\0\0";
    Object *o = make_picture(4, 4, 24, PBPAFMT_RGB, NULL, 0, NULL, 0);
    UBYTE *after;
    ULONG len;

    CU_ASSERT_PTR_NOT_NULL_FATAL(o);
    CU_ASSERT_FATAL(clip_write(marker, 20));
    CU_ASSERT(DoMethod(o, DTM_COPY, (IPTR) NULL) == FALSE);
    CU_ASSERT(IoErr() != 0);
    after = clip_read(&len);
    CU_ASSERT(after && len == 20 && memcmp(after, marker, 20) == 0);
    FreeVec(after);
    DisposeDTObject(o);
}

/* Copy o, check the clip against expect (w*h RGB) and its own layout */
static void copy_and_check(Object *o, UWORD w, UWORD h, UBYTE depth, const UBYTE *expect,
                           const UBYTE *index, const UBYTE *pal)
{
    struct ilbm il;
    UBYTE *clip, *rgb;
    ULONG len, x, y, bad = 0;
    UWORD rw = 0, rh = 0;

    CU_ASSERT_FATAL(DoMethod(o, DTM_COPY, (IPTR) NULL) == TRUE);
    clip = clip_read(&len);
    CU_ASSERT_PTR_NOT_NULL_FATAL(clip);
    CU_ASSERT_FATAL(parse_ilbm(clip, len, &il));
    CU_ASSERT(il.nbmhd == 1 && il.nbody == 1);
    CU_ASSERT(il.w == w && il.h == h && il.depth == depth);
    CU_ASSERT(il.compression == cmpNone && il.masking == mskNone);
    CU_ASSERT_FATAL(il.bodylen == (ULONG) (((w + 15) >> 4) << 1) * depth * h);
    if (index)
    {
        CU_ASSERT(il.ncmap == 1 && il.cmaplen == (3UL << depth));
        CU_ASSERT(il.cmap && memcmp(il.cmap, pal, il.cmaplen) == 0);
    }
    else
        CU_ASSERT(il.ncmap == 0);
    CU_ASSERT(padding_clear(&il));
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
        {
            ULONG v = body_pixel(&il, x, y), i = y * w + x;
            if (index ? v != index[i]
                      : v != (expect[i * 3] | (expect[i * 3 + 1] << 8) | ((ULONG) expect[i * 3 + 2] << 16)))
                bad++;
        }
    CU_ASSERT_EQUAL(bad, 0);
    FreeVec(clip);

    /* the receiver's view */
    rgb = clip_rgb_via_datatypes(&rw, &rh);
    CU_ASSERT_PTR_NOT_NULL_FATAL(rgb);
    CU_ASSERT(rw == w && rh == h);
    if (rw == w && rh == h)
        CU_ASSERT(memcmp(rgb, expect, w * h * 3) == 0);
    FreeVec(rgb);
}

/* 24-bit RGB, 13 pixels wide (odd, rows padded to 16) */
static void testRGB(void)
{
    enum { W = 13, H = 4 };
    UBYTE px[W * H * 3];
    ULONG x, y;
    Object *o;

    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++)
        {
            px[(y * W + x) * 3] = x * 19;
            px[(y * W + x) * 3 + 1] = 255 - y * 61;
            px[(y * W + x) * 3 + 2] = (x * y * 37 + 1) & 0xff;
        }
    o = make_picture(W, H, 24, PBPAFMT_RGB, px, W * 3, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(o);
    copy_and_check(o, W, H, 24, px, NULL, NULL);
    DisposeDTObject(o);
}

/* 1x1 RGB: the smallest picture, 15 padding bits per plane row */
static void testONEPIXEL(void)
{
    UBYTE px[3] = { 0xff, 0x00, 0x81 };
    Object *o = make_picture(1, 1, 24, PBPAFMT_RGB, px, 3, NULL, 0);

    CU_ASSERT_PTR_NOT_NULL_FATAL(o);
    copy_and_check(o, 1, 1, 24, px, NULL, NULL);
    DisposeDTObject(o);
}

/* ARGB, 17 wide: colour copied, alpha not written (24 planes, no mask) */
static void testARGB(void)
{
    enum { W = 17, H = 3 };
    UBYTE px[W * H * 4], rgb[W * H * 3];
    ULONG i;
    Object *o;

    for (i = 0; i < W * H; i++)
    {
        px[i * 4] = i * 15;                 /* alpha */
        rgb[i * 3] = px[i * 4 + 1] = i * 5;
        rgb[i * 3 + 1] = px[i * 4 + 2] = 200 - i;
        rgb[i * 3 + 2] = px[i * 4 + 3] = i ^ 0x5a;
    }
    o = make_picture(W, H, 32, PBPAFMT_ARGB, px, W * 4, NULL, 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(o);
    copy_and_check(o, W, H, 24, rgb, NULL, NULL);
    DisposeDTObject(o);
}

/* Colour-mapped, 3 planes, 17 wide: CMAP of 8 entries and the same indices */
static void testLUT(void)
{
    enum { W = 17, H = 5, N = 8 };
    UBYTE idx[W * H], rgb[W * H * 3], pal[N * 3];
    ULONG i;
    Object *o;

    for (i = 0; i < N; i++)
    {
        pal[i * 3] = i * 36;
        pal[i * 3 + 1] = 255 - i * 30;
        pal[i * 3 + 2] = (i & 1) ? 0xf0 : 0x0f;
    }
    for (i = 0; i < W * H; i++)
    {
        idx[i] = ((i % W) * 3 + i / W) % N;
        memcpy(rgb + i * 3, pal + idx[i] * 3, 3);
    }
    o = make_picture(W, H, 3, PBPAFMT_LUT8, idx, W, pal, N);
    CU_ASSERT_PTR_NOT_NULL_FATAL(o);
    copy_and_check(o, W, H, 3, rgb, idx, pal);
    DisposeDTObject(o);
}

/* Colour-mapped, header says 1 plane but the pens go up to 5: three planes
   are written, so no pen loses its high bits */
static void testLUTDEPTH(void)
{
    enum { W = 9, H = 2, N = 8 };
    UBYTE idx[W * H], rgb[W * H * 3], pal[N * 3];
    ULONG i;
    Object *o;

    for (i = 0; i < N * 3; i++)
        pal[i] = i * 10;
    for (i = 0; i < W * H; i++)
    {
        idx[i] = i % 6;
        memcpy(rgb + i * 3, pal + idx[i] * 3, 3);
    }
    o = make_picture(W, H, 1, PBPAFMT_LUT8, idx, W, pal, N);
    CU_ASSERT_PTR_NOT_NULL_FATAL(o);
    copy_and_check(o, W, H, 3, rgb, idx, pal);
    DisposeDTObject(o);
}

int main(void)
{
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("PictureDT_Copy_Suite", init_suite, clean_suite);
    if (NULL == pSuite)
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if ((NULL == CU_add_test(pSuite, "DTA_Methods lists DTM_COPY", testMETHODS)) ||
        (NULL == CU_add_test(pSuite, "DTM_COPY without picture data", testNODATA)) ||
        (NULL == CU_add_test(pSuite, "DTM_COPY RGB, odd width", testRGB)) ||
        (NULL == CU_add_test(pSuite, "DTM_COPY 1x1", testONEPIXEL)) ||
        (NULL == CU_add_test(pSuite, "DTM_COPY ARGB", testARGB)) ||
        (NULL == CU_add_test(pSuite, "DTM_COPY colour-mapped", testLUT)) ||
        (NULL == CU_add_test(pSuite, "DTM_COPY pens above the header depth", testLUTDEPTH)))
    {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_set_mode(CU_BRM_SILENT);
    CU_automated_package_name_set("PictureDTCopyUnitTests");
    CU_set_output_filename("PictureDTCopy");
    CU_automated_enable_junit_xml(CU_TRUE);
    CU_automated_run_tests();
    CU_cleanup_registry();

    return CU_get_error();
}
