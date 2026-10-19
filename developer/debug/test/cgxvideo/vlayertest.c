/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    State machine test for cgxvideo.library. Runs against the null
    backend, which it selects through a local CGXVideoNullBackend
    variable.
*/

#include <dos/var.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <proto/cgxvideo.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>

struct Library *CGXVideoBase;

static int failures;

#define CHECK(cond) check((cond), #cond, __LINE__)

static void check(BOOL ok, const char *what, int line)
{
    if (!ok)
    {
        printf("FAIL line %d: %s\n", line, what);
        failures++;
    }
}

static struct VLayerHandle *create(struct Screen *scr, ULONG type,
    ULONG w, ULONG h, ULONG modulo, ULONG *err)
{
    struct TagItem tags[] =
    {
        { VOA_SrcType,   type   },
        { VOA_SrcWidth,  w      },
        { VOA_SrcHeight, h      },
        { VOA_Error,     (IPTR)err },
        { modulo ? VOA_Modulo : TAG_IGNORE, modulo },
        { TAG_DONE,      0      }
    };

    *err = ~0;
    return CreateVLayerHandleTagList(scr, tags);
}

static void test_no_backend(struct Screen *scr)
{
    ULONG err;

    CHECK(create(scr, SRCFMT_RGB16PC, 64, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSCRMODE);
    CHECK(QueryVLayerAttr(scr, VSQ_SupportedFeatures) == 0);
}

static void test_create(struct Screen *scr)
{
    struct VLayerHandle *vh;
    ULONG err;

    CHECK(create(scr, SRCFMT_YCbCr420, 64, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(scr, 99, 64, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(scr, SRCFMT_RGB16PC, 0, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(scr, SRCFMT_RGB16PC, 64, 0, 0, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(scr, SRCFMT_RGB16PC, 64, 32, 64, &err) == NULL);
    CHECK(err == VOERR_INVSRCFMT);
    CHECK(create(NULL, SRCFMT_RGB16PC, 64, 32, 0, &err) == NULL);
    CHECK(err == VOERR_INVSCRMODE);

    vh = create(scr, SRCFMT_YCbCr16, 64, 32, 256, &err);
    CHECK(vh != NULL);
    CHECK(err == VOERR_OK);
    if (vh)
    {
        CHECK(GetVLayerAttr(vh, VOA_Modulo) == 256);
        DeleteVLayerHandle(vh);
    }

    CHECK(QueryVLayerAttr(scr, VSQ_SupportedFeatures) & VSQ_FEAT_OVERLAY);
    CHECK(!(QueryVLayerAttr(scr, VSQ_SupportedFormats) & VSQ_FMT_YUV420_PLANAR));
}

static void test_states(struct Screen *scr, struct Window *win)
{
    struct VLayerHandle *vh;
    UBYTE *buf;
    ULONG err;

    vh = create(scr, SRCFMT_RGB16PC, 64, 32, 0, &err);
    CHECK(vh != NULL);
    if (!vh)
        return;

    CHECK(GetVLayerAttr(vh, VOA_Width) == 64);
    CHECK(GetVLayerAttr(vh, VOA_Height) == 32);
    CHECK(GetVLayerAttr(vh, VOA_Modulo) == 128);
    CHECK(GetVLayerAttr(vh, VOA_ColorKey) == (IPTR)-1);
    CHECK(GetVLayerAttr(vh, VOA_ColorKeyPen) == (IPTR)-1);

    /* Buffer is only reachable while locked */
    CHECK(GetVLayerAttr(vh, VOA_BaseAddress) == 0);
    CHECK(LockVLayer(vh) == TRUE);
    CHECK(LockVLayer(vh) == FALSE);
    buf = (UBYTE *)GetVLayerAttr(vh, VOA_BaseAddress);
    CHECK(buf != NULL);
    if (buf)
        memset(buf, 0x5a, 128 * 32);

    CHECK(AttachVLayerTagList(vh, win, NULL) != 0);
    CHECK(DetachVLayer(vh) != 0);
    UnlockVLayer(vh);
    CHECK(GetVLayerAttr(vh, VOA_BaseAddress) == 0);

    CHECK(AttachVLayerTagList(vh, NULL, NULL) != 0);
    CHECK(AttachVLayerTagList(vh, win, NULL) == 0);
    CHECK(AttachVLayerTagList(vh, win, NULL) != 0);

    /* Locking is allowed while attached, detaching is not */
    CHECK(LockVLayer(vh) == TRUE);
    CHECK(DetachVLayer(vh) != 0);
    UnlockVLayer(vh);
    CHECK(DetachVLayer(vh) == 0);
    CHECK(DetachVLayer(vh) != 0);

    /* Deletion must be safe while attached and locked */
    CHECK(AttachVLayerTagList(vh, win, NULL) == 0);
    CHECK(LockVLayer(vh) == TRUE);
    DeleteVLayerHandle(vh);

    DeleteVLayerHandle(NULL);
    SwapVLayerBuffer(NULL);
}

int main(void)
{
    struct Screen *scr;
    struct Window *win;

    CGXVideoBase = OpenLibrary("cgxvideo.library", 41);
    if (!CGXVideoBase)
    {
        printf("FAIL: cannot open cgxvideo.library\n");
        return RETURN_FAIL;
    }

    scr = LockPubScreen(NULL);
    win = scr ? OpenWindowTags(NULL, WA_PubScreen, (IPTR)scr,
        WA_Width, 160, WA_Height, 120, WA_Title, (IPTR)"vlayertest",
        TAG_DONE) : NULL;
    if (!win)
    {
        printf("FAIL: cannot open a window\n");
        if (scr)
            UnlockPubScreen(NULL, scr);
        CloseLibrary(CGXVideoBase);
        return RETURN_FAIL;
    }

    DeleteVar("CGXVideoNullBackend", GVF_LOCAL_ONLY);
    test_no_backend(scr);

    SetVar("CGXVideoNullBackend", "1", -1, GVF_LOCAL_ONLY);
    test_create(scr);
    test_states(scr, win);
    DeleteVar("CGXVideoNullBackend", GVF_LOCAL_ONLY);

    CloseWindow(win);
    UnlockPubScreen(NULL, scr);
    CloseLibrary(CGXVideoBase);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
