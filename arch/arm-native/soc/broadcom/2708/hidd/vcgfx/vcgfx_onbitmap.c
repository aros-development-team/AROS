/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 Gfx Hidd Onscreen Bitmap Class.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/oop.h>
#include <proto/mbox.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <stddef.h>

#include "vcgfx_bitmap.h"
#include "vcgfx_hidd.h"
#include "vcgfx_hvs.h"
#include "vcgfx_hvs5.h"
#include "vcgfx_hvs6.h"

#include LC_LIBDEFS_FILE

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      (&((struct VideoCoreGfxBase *)cl->UserData)->vsd)->vcsd_MBoxBase

#define MNAME_ROOT(x) VideoCoreGfxOnBM__Root__ ## x
#define MNAME_BM(x) VideoCoreGfxOnBM__Hidd_BitMap__ ## x

#define OnBitmap 1
#include "vcgfx_bitmapclass.c"

/* Included bitmapclass.c sets its own DEBUG; reset to this file's value. */
#undef DEBUG
#define DEBUG 0

/* bitmapclass.c's MBoxBase resolves through 'cl'; the helpers below carry
 * 'xsd' instead, so switch the macro to pull MBoxBase off it directly.
 */
#undef MBoxBase
#define MBoxBase      xsd->vcsd_MBoxBase

/* The rate a sync object asks for, for telling modes of one size apart;
 * NULL when it cannot say. */
static const struct vcgfx_timing *vc4_sync_timing(OOP_Class *cl, OOP_Object *sync,
                                                   struct vcgfx_timing *t)
{
    IPTR clk = 0, htotal = 0, vtotal = 0;

    if (!sync)
        return NULL;
    OOP_GetAttr(sync, aHidd_Sync_PixelClock, &clk);
    OOP_GetAttr(sync, aHidd_Sync_HTotal, &htotal);
    OOP_GetAttr(sync, aHidd_Sync_VTotal, &vtotal);
    if (!clk || !htotal || !vtotal)
        return NULL;

    t->clock  = clk / 1000;
    t->htotal = htotal;
    t->vtotal = vtotal;
    return t;
}

/* FBFREE, then FBALLOC aligned_width x height, and GETPITCH. Not for
 * BCM2712, which cannot allocate another. */
static BOOL vc4_fballoc(struct VideoCoreGfx_staticdata *xsd,
    ULONG aligned_width, ULONG height, UBYTE bytesperpix,
    APTR *fb_ptr_out, ULONG *fb_pitch_out)
{
    ULONG bitsperpixel = bytesperpix * 8;
    APTR fb_ptr = NULL;
    ULONG fb_pitch = 0, fb_size = 0;

    /* Hold the mailbox lock across the whole multi-transaction sequence
     * (FBFREE -> batched mode/depth/pixfmt/FBALLOC -> GETPITCH). */
    VC4_MBOX_LOCK(xsd);

    /* Free any previous framebuffer the firmware was holding. */
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(6 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_FBFREE);
    xsd->vcsd_MBoxMessage[3] = 0;
    xsd->vcsd_MBoxMessage[4] = 0;
    xsd->vcsd_MBoxMessage[5] = 0;
    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);

    /* SETRES + SETVRES + SETDEPTH + SETPIXFMT + FBALLOC in one batch.
     * SETPIXFMT pins channel order to RGB to match pftags_32bpp (R in
     * low bits); real Pi3 defaults to BGR (brown/blue tint), QEMU to RGB.
     */
    xsd->vcsd_MBoxMessage[1]  = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2]  = AROS_LE2LONG(VCTAG_SETRES);
    xsd->vcsd_MBoxMessage[3]  = AROS_LE2LONG(8);
    xsd->vcsd_MBoxMessage[4]  = AROS_LE2LONG(8);
    xsd->vcsd_MBoxMessage[5]  = AROS_LE2LONG(aligned_width);
    xsd->vcsd_MBoxMessage[6]  = AROS_LE2LONG(height);

    xsd->vcsd_MBoxMessage[7]  = AROS_LE2LONG(VCTAG_SETVRES);
    xsd->vcsd_MBoxMessage[8]  = AROS_LE2LONG(8);
    xsd->vcsd_MBoxMessage[9]  = AROS_LE2LONG(8);
    xsd->vcsd_MBoxMessage[10] = AROS_LE2LONG(aligned_width);
    xsd->vcsd_MBoxMessage[11] = AROS_LE2LONG(height);

    xsd->vcsd_MBoxMessage[12] = AROS_LE2LONG(VCTAG_SETDEPTH);
    xsd->vcsd_MBoxMessage[13] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[14] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[15] = AROS_LE2LONG(bitsperpixel);

    xsd->vcsd_MBoxMessage[16] = AROS_LE2LONG(VCTAG_SETPIXFMT);
    xsd->vcsd_MBoxMessage[17] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[18] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[19] = AROS_LE2LONG(VCPXFMT_BGR);

    xsd->vcsd_MBoxMessage[20] = AROS_LE2LONG(VCTAG_FBALLOC);
    xsd->vcsd_MBoxMessage[21] = AROS_LE2LONG(8);
    xsd->vcsd_MBoxMessage[22] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[23] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[24] = 0;

    xsd->vcsd_MBoxMessage[25] = 0;
    xsd->vcsd_MBoxMessage[0]  = AROS_LE2LONG(26 << 2);

    if ((MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
            == (volatile unsigned int *)-1)
        || (xsd->vcsd_MBoxMessage[1] != AROS_LE2LONG(VCTAG_RESP))
        || (xsd->vcsd_MBoxMessage[22] != AROS_LE2LONG(VCTAG_RESP + 8)))
    {
        VC4_MBOX_UNLOCK(xsd);
        return FALSE;
    }
    fb_ptr = (APTR)(IPTR)(AROS_LE2LONG(xsd->vcsd_MBoxMessage[23]) & 0x3fffffff);
    fb_size = AROS_LE2LONG(xsd->vcsd_MBoxMessage[24]);

    /* Use the firmware-reported pitch verbatim: it may align rows, or
     * (QEMU) keep a default virtual width that ignores our request.
     */
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(7 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_GETPITCH);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[4] = 0;
    xsd->vcsd_MBoxMessage[5] = 0;
    xsd->vcsd_MBoxMessage[6] = 0;

    if ((MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
            != (volatile unsigned int *)-1)
        && (xsd->vcsd_MBoxMessage[4] == AROS_LE2LONG(VCTAG_RESP + 4)))
    {
        fb_pitch = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
    }
    else
    {
        /* Fallback: assume packed rows. */
        fb_pitch = aligned_width * bytesperpix;
    }

    VC4_MBOX_UNLOCK(xsd);

    /* The Pi 4 firmware caps the size at max_framebuffer_width/height
     * and says so only in what it hands back: drawing into that would
     * run past its end. */
    if ((xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        && ((fb_pitch < aligned_width * bytesperpix) || (fb_size < fb_pitch * height)))
    {
        bug("[VideoCoreGfx] %ux%u: the firmware gave %u bytes at pitch %u"
            " - raise max_framebuffer_width/height in config.txt\n",
            (unsigned)aligned_width, (unsigned)height, (unsigned)fb_size,
            (unsigned)fb_pitch);
        return FALSE;
    }

    *fb_ptr_out   = fb_ptr;
    *fb_pitch_out = fb_pitch;
    return TRUE;
}

/* The width of the firmware's framebuffer behind screens that are planes
 * (Pi 1-4). Nothing reads it once the list is ours, but it keeps the
 * mode's shape: where the firmware keeps the timing it fits the plane to
 * the display by aspect, and that rectangle is the one our planes fill. */
#define VC4_TOKEN_FB    256

/*
 * Screens as planes on VC4 and HVS5: a token framebuffer for the firmware - it
 * still sets its mode and builds the list we take over - and the fb
 * object's pixels in scanout memory of ours, so the VideoCore partition
 * holds almost nothing. FALSE when no list of ours came of it.
 */
static BOOL vc4_program_token_fb(struct VideoCoreGfx_staticdata *xsd,
    ULONG aligned_width, ULONG height, UBYTE bytesperpix,
    const struct vcgfx_timing *want, APTR *fb_ptr_out, ULONG *fb_pitch_out)
{
    struct vc4_hvs_state *st = &xsd->vcsd_HVS;
    struct vcgfx_scanbuf old = xsd->vcsd_FBBuf;
    ULONG pitch = aligned_width * bytesperpix, size = pitch * height, tpitch;
    ULONG th = (VC4_TOKEN_FB * height + aligned_width / 2) / aligned_width;
    APTR tptr;

    if (th < 16)
        th = 16;
    if (!vc4_fballoc(xsd, VC4_TOKEN_FB, th, bytesperpix, &tptr, &tpitch))
        return FALSE;

    if (!old.sb_Pixels || (xsd->vcsd_FBBufSize != size))
    {
        if (!vcgfx_scanout_alloc(xsd, &xsd->vcsd_FBBuf, size))
        {
            xsd->vcsd_FBBuf = old;
            return FALSE;
        }
        xsd->vcsd_FBBufSize = size;
    }
    else
        old.sb_Raw = NULL;

    st->hvs_Tiny      = TRUE;
    st->hvs_ShowPtr   = (ULONG)(IPTR)xsd->vcsd_FBBuf.sb_Pixels;
    st->hvs_ShowPitch = pitch;
    st->hvs_ShowW     = aligned_width;
    st->hvs_ShowH     = height;

    vc4_hvs_dump(xsd, (ULONG)(IPTR)tptr, tpitch, VC4_TOKEN_FB, th);
    if (!vc4_hvs_takeover(xsd, (ULONG)(IPTR)tptr, tpitch))
    {
        st->hvs_Tiny = FALSE;
        if (old.sb_Raw)
        {
            vcgfx_scanout_free(xsd, &xsd->vcsd_FBBuf);
            xsd->vcsd_FBBuf = old;
        }
        return FALSE;
    }
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        vc4_hvs5_mode_apply(xsd, aligned_width, height, pitch, want);

    /* The list showing the old buffer has latched away. */
    if (old.sb_Raw)
        vcgfx_scanout_free(xsd, &old);

    *fb_ptr_out   = xsd->vcsd_FBBuf.sb_Pixels;
    *fb_pitch_out = pitch;
    return TRUE;
}

/* Already showing our own buffer behind a token framebuffer (HVS5): a
 * buffer of the new size and our own mode set, no firmware involved. */
static BOOL vc4_program_own_fb(struct VideoCoreGfx_staticdata *xsd,
    ULONG aligned_width, ULONG height, UBYTE bytesperpix,
    const struct vcgfx_timing *want, APTR *fb_ptr_out, ULONG *fb_pitch_out)
{
    struct vcgfx_scanbuf old = xsd->vcsd_FBBuf;
    ULONG oldsize = xsd->vcsd_FBBufSize;
    ULONG pitch = aligned_width * bytesperpix, size = pitch * height;

    if (!xsd->vcsd_HVS.hvs_Active || !xsd->vcsd_HVS.hvs_Tiny)
        return FALSE;

    if (!old.sb_Pixels || (oldsize != size))
    {
        if (!vcgfx_scanout_alloc(xsd, &xsd->vcsd_FBBuf, size))
        {
            xsd->vcsd_FBBuf = old;
            return FALSE;
        }
        xsd->vcsd_FBBufSize = size;
    }
    else
        old.sb_Raw = NULL;

    if (!vc4_hvs5_mode_switch(xsd, aligned_width, height,
                              (ULONG)(IPTR)xsd->vcsd_FBBuf.sb_Pixels, pitch, want))
    {
        /* Not a mode we set (the list is untouched): the firmware decides */
        if (old.sb_Raw)
        {
            vcgfx_scanout_free(xsd, &xsd->vcsd_FBBuf);
            xsd->vcsd_FBBuf     = old;
            xsd->vcsd_FBBufSize = oldsize;
        }
        return FALSE;
    }

    /* The list showing the old buffer has latched away. */
    if (old.sb_Raw)
        vcgfx_scanout_free(xsd, &old);

    *fb_ptr_out   = xsd->vcsd_FBBuf.sb_Pixels;
    *fb_pitch_out = pitch;
    return TRUE;
}

static BOOL vc4_program_fb(struct VideoCoreGfx_staticdata *xsd,
    ULONG aligned_width, ULONG height, UBYTE bytesperpix,
    const struct vcgfx_timing *want, APTR *fb_ptr_out, ULONG *fb_pitch_out)
{
    APTR fb_ptr = NULL;
    ULONG fb_pitch = 0;

    if ((xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        && vc4_program_own_fb(xsd, aligned_width, height, bytesperpix, want,
                              fb_ptr_out, fb_pitch_out))
        return TRUE;

    /* The firmware builds its lists for the mode it set itself, so a mode
     * of ours goes back to that one first. */
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        vc4_hvs5_mode_restore(xsd);

    /* Mode is changing: the FBALLOC sequence below makes the firmware
     * rebuild its display list and repoint the channel, so give up list
     * ownership first; it is re-taken once the new framebuffer is up. */
    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_HVS.hvs_Active = FALSE;
    VC4_MBOX_UNLOCK(xsd);

    if ((xsd->vcsd_HVSGen != VCGFX_HVS_HVS6) && vcgfx_planes_supported(xsd))
    {
        if (vc4_program_token_fb(xsd, aligned_width, height, bytesperpix, want,
                                 fb_ptr_out, fb_pitch_out))
            return TRUE;
        bug("[VideoCoreGfx] no display list behind a token framebuffer,"
            " asking the firmware for a full one\n");
    }

    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
    {
        /* BCM2712 FBFREE+FBALLOC returns a null base and loses the
         * scanned surface, so adopt the boot framebuffer instead. */
        if ((aligned_width <= xsd->vcsd_BootFBWidth)
            && (height <= xsd->vcsd_BootFBHeight))
        {
            fb_ptr   = (APTR)(IPTR)xsd->vcsd_BootFB;
            fb_pitch = xsd->vcsd_BootFBPitch;
        }
        else
        {
            /* Larger than the boot surface: a framebuffer of our own. */
            fb_pitch = aligned_width * bytesperpix;
            if (!(fb_ptr = vc4_hvs6_alloc_fb(xsd, fb_pitch, height)))
                return FALSE;
        }
    }
    else if (!vc4_fballoc(xsd, aligned_width, height, bytesperpix, &fb_ptr, &fb_pitch))
        return FALSE;

    D(bug("[VideoCoreGfx] %s: fb @ 0x%p, pitch %d\n",
        __PRETTY_FUNCTION__, fb_ptr, (int)fb_pitch));

    /* Phase-1 HVS bring-up: dump the firmware's live display list and PV
     * timing for this mode (read-only, self-skips on QEMU). */
    vc4_hvs_dump(xsd, (ULONG)(IPTR)fb_ptr, fb_pitch, aligned_width, height);

    /* Unconditional report: on a black screen serial is all there is. */
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
    {
        vc4_hvs6_report(xsd, (ULONG)(IPTR)fb_ptr, fb_pitch, aligned_width, height);

        /* A mode set holds this ~100 ms; cursor updates must wait. */
        VC4_MBOX_LOCK(xsd);
        vc4_hvs6_takeover(xsd, (ULONG)(IPTR)fb_ptr, fb_pitch, aligned_width, height, want);
        VC4_MBOX_UNLOCK(xsd);
    }

    /* Phase 2: own the display list from here on. Cursor updates
     * become dlist repoints; falls back to the firmware paths
     * (and returns FALSE) if the live list can't be inherited. */
    vc4_hvs_takeover(xsd, (ULONG)(IPTR)fb_ptr, fb_pitch);
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS5)
        vc4_hvs5_mode_apply(xsd, aligned_width, height, fb_pitch, want);

    *fb_ptr_out   = fb_ptr;
    *fb_pitch_out = fb_pitch;
    return TRUE;
}

/* The firmware drops cursor info on FBALLOC. After a mode switch, push
 * the saved shape and visibility state back so the cursor survives.
 */
static VOID vc4_restore_cursor(struct VideoCoreGfx_staticdata *xsd)
{
    if (!xsd->vcsd_CurBuf || !xsd->vcsd_CurWidth || !xsd->vcsd_CurHeight)
        return;

    /* HVS6 has no firmware cursor; the takeover emptied the plane. */
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
    {
        vc4_hvs_update_cursor(xsd);
        return;
    }

    /* Owning the display list: the takeover already baked the current
     * cursor state into our list, and the firmware cursor tags would
     * make the firmware rebuild its list and steal the channel back. */
    if (xsd->vcsd_HVS.hvs_Active)
        return;

    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(12 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORINFO);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(24);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(24);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(xsd->vcsd_CurWidth);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG(xsd->vcsd_CurHeight);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG(0);
    xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(xsd->vcsd_CurBufBus);
    xsd->vcsd_MBoxMessage[9] = AROS_LE2LONG(xsd->vcsd_CurHotX);
    xsd->vcsd_MBoxMessage[10] = AROS_LE2LONG(xsd->vcsd_CurHotY);
    xsd->vcsd_MBoxMessage[11] = 0;
    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);

    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_SETCURSORSTATE);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[4] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[5] = AROS_LE2LONG(xsd->vcsd_CurVisible ? 1 : 0);
    xsd->vcsd_MBoxMessage[6] = AROS_LE2LONG((ULONG)xsd->vcsd_CurX);
    xsd->vcsd_MBoxMessage[7] = AROS_LE2LONG((ULONG)xsd->vcsd_CurY);
    xsd->vcsd_MBoxMessage[8] = AROS_LE2LONG(0);
    xsd->vcsd_MBoxMessage[9] = 0;
    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    VC4_MBOX_UNLOCK(xsd);
}

/*********** BitMap::New() *************************************/

OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    OOP_Object *pf;
    IPTR width, height, depth;
    IPTR bitsperpixel = 0, bytesperpix_attr = 0;
    ULONG aligned_width, fb_pitch = 0;
    APTR fb_ptr = NULL;
    UBYTE bytesperpix;
    HIDDT_ModeID modeid;
    OOP_Object *sync = NULL, *modepf = NULL;
    struct vcgfx_timing want;

    D(bug("[VideoCoreGfx] VideoCoreGfx.OnBitMap::New()\n"));

    /* Extract the attrs we need for FBALLOC before super-call: ChunkyBM/
     * BitMap cache BytesPerRow at New time, so doing FBALLOC + GETPITCH
     * later and SetAttrs'ing would leave a stale stride.
     *
     * The FB bitmap arrives with just FrameBuffer=TRUE + ModeID;
     * Width/Height/PixFmt are normally derived from the mode database
     * inside BitMap::New, so we replicate that lookup here.
     */
    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    width  = GetTagData(aHidd_BitMap_Width,  0, msg->attrList);
    height = GetTagData(aHidd_BitMap_Height, 0, msg->attrList);
    pf     = (OOP_Object *)GetTagData(aHidd_BitMap_PixFmt, 0, msg->attrList);

    if (modeid == vHidd_ModeID_Invalid)
    {
        D(bug("[VideoCoreGfx] OnBitMap::New: no ModeID\n"));
        return NULL;
    }

    /* The mode's sync tells refresh rates of one size apart. */
    if (!HIDD_DMEnum_GetMode(xsd->vcsd_DMEnum, modeid, &sync, &modepf))
        sync = NULL;

    if (!width || !height || !pf)
    {
        if (!sync || !modepf)
        {
            D(bug("[VideoCoreGfx] OnBitMap::New: GetMode(0x%08x) failed\n", (ULONG)modeid));
            return NULL;
        }
        if (!width)
            OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
        if (!height)
            OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
        if (!pf)
            pf = modepf;
    }

    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
    OOP_GetAttr(pf, aHidd_PixFmt_BitsPerPixel, &bitsperpixel);
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bytesperpix_attr);
    if (!width || !height || !bytesperpix_attr)
    {
        D(bug("[VideoCoreGfx] OnBitMap::New: bad geometry %dx%d bytespp %d\n",
            (int)width, (int)height, (int)bytesperpix_attr));
        return NULL;
    }

    /* SETDEPTH wants the cell size in bits. AROS pixfmts that pad colour
     * into wider cells (e.g. BGR032: 24-bit in a 4-byte cell) report
     * BitsPerPixel as colour depth, so use bytesperpix * 8 (the cell width).
     */
    aligned_width = (width + 15) & ~15;
    bytesperpix = (UBYTE)bytesperpix_attr;
    bitsperpixel = (IPTR)bytesperpix * 8;

#if !defined(DEBUGDISPLAY)
    /* Detach the bootloader's FB console: 0x03 to RawPutChar drops
     * ARMI_PutChar (arch/arm-native/kernel/kernel_debug.c) so debug output
     * goes to serial only. Otherwise it keeps drawing into the FB we hand
     * to Workbench, overwriting the title bar.
     */
    RawPutChar(0x03);

    if (!vc4_program_fb(xsd, aligned_width, height, bytesperpix,
                        vc4_sync_timing(cl, sync, &want), &fb_ptr, &fb_pitch))
    {
        D(bug("[VideoCoreGfx] OnBitMap::New: FBALLOC failed\n"));
        return NULL;
    }

    D(bug("[VideoCoreGfx] OnBitMap::New: %dx%d %dbpp (depth %d) FB @ 0x%p, pitch %d\n",
        (int)aligned_width, (int)height, (int)bitsperpixel, (int)depth,
        fb_ptr, (int)fb_pitch));

#endif

    /* Hand the FB and pitch to super::New so ChunkyBM/BitMap cache the
     * right values up-front.
     */
    {
        struct TagItem extra_tags[] =
        {
            { aHidd_ChunkyBM_Buffer,    (IPTR)fb_ptr           },
            { aHidd_BitMap_BytesPerRow, fb_pitch               },
            { TAG_MORE,                 (IPTR)msg->attrList    }
        };
        struct pRoot_New new_msg;

        new_msg.mID = msg->mID;
        new_msg.attrList = extra_tags;

        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&new_msg);
    }

    if (o)
    {
        struct BitmapData *data = OOP_INST_DATA(cl, o);

        memset(data, 0, sizeof(struct BitmapData));
        data->VideoData   = (UBYTE *)fb_ptr;
        data->width       = aligned_width;
        data->height      = height;
        data->bytesperrow = fb_pitch;
        data->bpp         = bitsperpixel;
        data->bytesperpix = bytesperpix;
        data->disp        = -1;
        data->data        = &XSD(cl)->data;
        data->dispwidth   = width;

        xsd->vcsd_FBObj   = o;

        D(bug("[VideoCoreGfx] OnBitMap::New: object @ 0x%p, FB @ 0x%p, pitch %d\n",
            o, data->VideoData, (int)fb_pitch));
    }

    ReturnPtr("VideoCoreGfx.OnBitMap::New: Obj", OOP_Object *, o);
}

/**********  Bitmap::Set()  ***************************************/

/* gfx.hidd's Show() switches the FB ModeID via SetAttrs on us. Left to the
 * BitMap superclass alone it updates Width/Height/BytesPerRow but the
 * firmware keeps scanning out at the old geometry — sheared "intertwined"
 * rendering at every non-native mode. So reprogram the firmware first, then
 * forward the new buffer + pitch to super so cached attributes line up.
 *
 * Returns IPTR to match BM__Root__Set; Show() bails (never copying the new
 * bitmap into us) when SetAttrs returns 0.
 */
IPTR MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct VideoCoreGfx_staticdata *xsd = XSD(cl);
    struct BitmapData *data = OOP_INST_DATA(cl, o);
    HIDDT_ModeID newmodeid = vHidd_ModeID_Invalid;
    BOOL has_modeid = FALSE;
    struct TagItem *tag, *tstate;
    ULONG idx;
    IPTR result;

    D(bug("[VideoCoreGfx] OnBitMap::Set: entered, attrList=0x%p\n", msg->attrList));

    tstate = msg->attrList;
    while ((tag = NextTagItem(&tstate)))
    {
        if (IS_BM_ATTR(tag->ti_Tag, idx) && idx == aoHidd_BitMap_ModeID)
        {
            newmodeid = (HIDDT_ModeID)tag->ti_Data;
            has_modeid = TRUE;
        }
    }

    D(bug("[VideoCoreGfx] OnBitMap::Set: has_modeid=%d, newmodeid=0x%08lx\n",
        (int)has_modeid, (unsigned long)newmodeid));

    if (has_modeid && newmodeid != vHidd_ModeID_Invalid)
    {
        OOP_Object *sync = NULL, *pf = NULL;

        if (HIDD_DMEnum_GetMode(xsd->vcsd_DMEnum, newmodeid, &sync, &pf)
            && sync && pf)
        {
            IPTR width = 0, height = 0, bytesperpix_attr = 0;
            ULONG aligned_width, fb_pitch = 0;
            struct vcgfx_timing want;
            APTR fb_ptr = NULL;
            UBYTE bytesperpix;

            OOP_GetAttr(sync, aHidd_Sync_HDisp,         &width);
            OOP_GetAttr(sync, aHidd_Sync_VDisp,         &height);
            OOP_GetAttr(pf,   aHidd_PixFmt_BytesPerPixel, &bytesperpix_attr);

            if (width && height && bytesperpix_attr)
            {
                aligned_width = (width + 15) & ~15;
                bytesperpix   = (UBYTE)bytesperpix_attr;

                if (vc4_program_fb(xsd, aligned_width, height, bytesperpix,
                                   vc4_sync_timing(cl, sync, &want), &fb_ptr, &fb_pitch)
                    && fb_ptr)
                {
                    struct TagItem extra_tags[] =
                    {
                        { aHidd_ChunkyBM_Buffer,    (IPTR)fb_ptr        },
                        { aHidd_BitMap_BytesPerRow, fb_pitch            },
                        { TAG_MORE,                 (IPTR)msg->attrList }
                    };
                    struct pRoot_Set new_msg;

                    new_msg.mID = msg->mID;
                    new_msg.attrList = extra_tags;

                    result = OOP_DoSuperMethod(cl, o, (OOP_Msg)&new_msg);

                    data->VideoData   = (UBYTE *)fb_ptr;
                    data->width       = aligned_width;
                    data->height      = height;
                    data->bytesperrow = fb_pitch;
                    data->bytesperpix = bytesperpix;
                    data->bpp         = bytesperpix * 8;
                    data->dispwidth   = width;

                    D(bug("[VideoCoreGfx] OnBitMap::Set: switched to %dx%d %dbpp FB @ 0x%p, pitch %d (super=%d)\n",
                        (int)aligned_width, (int)height, (int)data->bpp,
                        fb_ptr, (int)fb_pitch, (int)result));

                    vc4_restore_cursor(xsd);
                    return result;
                }

                D(bug("[VideoCoreGfx] OnBitMap::Set: vc4_program_fb failed for %dx%d\n",
                    (int)aligned_width, (int)height));
            }
        }
    }

    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
