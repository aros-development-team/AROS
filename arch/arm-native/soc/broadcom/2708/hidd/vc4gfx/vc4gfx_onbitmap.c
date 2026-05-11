/*
    Copyright (C) 2013-2017, The AROS Development Team. All rights reserved.

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

#include "vc4gfx_bitmap.h"
#include "vc4gfx_hidd.h"

#include LC_LIBDEFS_FILE

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      (&((struct VideoCoreGfxBase *)cl->UserData)->vsd)->vcsd_MBoxBase

#define MNAME_ROOT(x) VideoCoreGfxOnBM__Root__ ## x
#define MNAME_BM(x) VideoCoreGfxOnBM__Hidd_BitMap__ ## x

#define OnBitmap 1
#include "vc4gfx_bitmapclass.c"

/* vc4gfx_bitmapclass.c sets its own DEBUG; reset here so the value
 * chosen at the top of this file applies to the code below.
 */
#undef DEBUG
#define DEBUG 0

/* The file-level MBoxBase above resolves through 'cl' because the
 * included bitmapclass.c expects that. Below we have stand-alone
 * helpers and OOP methods that all carry an 'xsd' pointer; switch the
 * macro to pull MBoxBase off it directly so the helpers can call
 * MBoxWrite/MBoxRead without needing a class pointer.
 */
#undef MBoxBase
#define MBoxBase      xsd->vcsd_MBoxBase

/* Drive the firmware framebuffer to the requested geometry and report
 * back the resulting CPU pointer and scanout pitch. Used by both
 * BitMap::New (initial allocation) and BitMap::Set (mode switch on
 * the framebuffer when gfx.hidd's Show() picks a non-native mode).
 */
static BOOL vc4_program_fb(struct VideoCoreGfx_staticdata *xsd,
    ULONG aligned_width, ULONG height, UBYTE bytesperpix,
    APTR *fb_ptr_out, ULONG *fb_pitch_out)
{
    ULONG bitsperpixel = bytesperpix * 8;
    APTR fb_ptr = NULL;
    ULONG fb_pitch = 0;

    /* Free any previous framebuffer the firmware was holding. */
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(6 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_FBFREE);
    xsd->vcsd_MBoxMessage[3] = 0;
    xsd->vcsd_MBoxMessage[4] = 0;
    xsd->vcsd_MBoxMessage[5] = 0;
    MBoxWrite((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    MBoxRead((void *)VCMB_BASE, VCMB_PROPCHAN);

    /* SETRES + SETVRES + SETDEPTH + SETPIXFMT + FBALLOC, all in one
     * batch. SETPIXFMT pins the channel order to RGB so the layout
     * matches our pftags_32bpp declaration (R in low bits) on every
     * firmware - on real Pi3 the default would otherwise be BGR,
     * giving a brown/blue-tinted image. QEMU happens to pick RGB by
     * default, hiding the issue there.
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
    xsd->vcsd_MBoxMessage[19] = AROS_LE2LONG(VCPXFMT_RGB);

    xsd->vcsd_MBoxMessage[20] = AROS_LE2LONG(VCTAG_FBALLOC);
    xsd->vcsd_MBoxMessage[21] = AROS_LE2LONG(8);
    xsd->vcsd_MBoxMessage[22] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[23] = AROS_LE2LONG(16);
    xsd->vcsd_MBoxMessage[24] = 0;

    xsd->vcsd_MBoxMessage[25] = 0;
    xsd->vcsd_MBoxMessage[0]  = AROS_LE2LONG(26 << 2);

    MBoxWrite((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    if ((MBoxRead((void *)VCMB_BASE, VCMB_PROPCHAN) != xsd->vcsd_MBoxMessage)
        || (xsd->vcsd_MBoxMessage[1] != AROS_LE2LONG(VCTAG_RESP))
        || (xsd->vcsd_MBoxMessage[22] != AROS_LE2LONG(VCTAG_RESP + 8)))
    {
        return FALSE;
    }
    fb_ptr = (APTR)(AROS_LE2LONG(xsd->vcsd_MBoxMessage[23]) & 0x3fffffff);

    /* Query the firmware-decided pitch. The firmware may align rows or
     * (in QEMU) leave the virtual width at a default that doesn't match
     * what we requested, so we must use the reported pitch verbatim.
     */
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(7 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_GETPITCH);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(4);
    xsd->vcsd_MBoxMessage[4] = 0;
    xsd->vcsd_MBoxMessage[5] = 0;
    xsd->vcsd_MBoxMessage[6] = 0;

    MBoxWrite((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    if ((MBoxRead((void *)VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_MBoxMessage)
        && (xsd->vcsd_MBoxMessage[4] == AROS_LE2LONG(VCTAG_RESP + 4)))
    {
        fb_pitch = AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
    }
    else
    {
        /* Fallback: assume packed rows. */
        fb_pitch = aligned_width * bytesperpix;
    }

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
    MBoxWrite((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    MBoxRead((void *)VCMB_BASE, VCMB_PROPCHAN);

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
    MBoxWrite((void *)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    MBoxRead((void *)VCMB_BASE, VCMB_PROPCHAN);
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

    D(bug("[VideoCoreGfx] VideoCoreGfx.OnBitMap::New()\n"));

    /* Extract attrs we need to drive FBALLOC directly from the incoming
     * attrList. We do this before super-call because ChunkyBM/BitMap cache
     * BytesPerRow at New time - if we wait until after super-call to do
     * FBALLOC + GETPITCH and then SetAttrs, ChunkyBM keeps a stale
     * bytesperrow and rendering uses the wrong stride.
     *
     * The framebuffer bitmap is created with just FrameBuffer=TRUE +
     * ModeID; Width/Height/PixFmt are normally derived inside BitMap::New
     * from the mode database. We replicate that lookup here.
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

    if (!width || !height || !pf)
    {
        OOP_Object *sync = NULL;
        OOP_Object *modepf = NULL;

        if (!HIDD_Gfx_GetMode(xsd->vcsd_VideoCoreGfxInstance, modeid, &sync, &modepf)
            || !sync || !modepf)
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

    /* The Pi firmware's SETDEPTH wants the cell size in bits. AROS
     * pixfmts that pad colour into wider cells (e.g. BGR032: 24-bit
     * colour in a 4-byte cell) have BitsPerPixel == colour depth, not
     * the cell size, so use bytesperpix * 8 which is always the cell
     * width.
     */
    aligned_width = (width + 15) & ~15;
    bytesperpix = (UBYTE)bytesperpix_attr;
    bitsperpixel = bytesperpix * 8;

#if !defined(DEBUGDISPLAY)
    /* Detach the kernel's bootloader-installed framebuffer console.
     * Writing 0x03 to RawPutChar drops ARMI_PutChar (see
     * arch/arm-native/kernel/kernel_debug.c) so subsequent bug()/kprintf
     * output goes to the serial port only - critical, otherwise the
     * console keeps drawing characters into the FB we are about to
     * hand over to Workbench, overwriting the title bar.
     */
    RawPutChar(0x03);

    if (!vc4_program_fb(xsd, aligned_width, height, bytesperpix,
                        &fb_ptr, &fb_pitch))
    {
        D(bug("[VideoCoreGfx] OnBitMap::New: FBALLOC failed\n"));
        return NULL;
    }

    D(bug("[VideoCoreGfx] OnBitMap::New: %dx%d %dbpp (depth %d) FB @ 0x%p, pitch %d\n",
        (int)aligned_width, (int)height, (int)bitsperpixel, (int)depth,
        fb_ptr, (int)fb_pitch));

#endif

    /* Hand the discovered FB and pitch to super::New via an extended
     * attrList. ChunkyBM and BitMap then cache the right values up-front.
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
        data->bpp         = bitsperpixel;
        data->bytesperpix = bytesperpix;
        data->disp        = -1;
        data->data        = &XSD(cl)->data;

        D(bug("[VideoCoreGfx] OnBitMap::New: object @ 0x%p, FB @ 0x%p, pitch %d\n",
            o, data->VideoData, (int)fb_pitch));
    }

    ReturnPtr("VideoCoreGfx.OnBitMap::New: Obj", OOP_Object *, o);
}

/**********  Bitmap::Set()  ***************************************/

/* gfx.hidd's Show() switches the framebuffer to a different ModeID
 * by calling SetAttrs(aHidd_BitMap_ModeID, ...) on us; if we leave
 * that to the BitMap superclass alone, it updates the bitmap's
 * Width/Height/BytesPerRow but the firmware keeps scanning out at
 * the original resolution and pitch - which manifests as sheared
 * "intertwined" rendering at every non-native mode. Reprogram the
 * firmware so its scanout matches the new geometry, then forward
 * the new buffer + pitch to super so the cached attributes line up.
 *
 * Returns IPTR to match BM__Root__Set; gfx.hidd's Show() bails out
 * (and never copies the new bitmap into us) when SetAttrs returns 0.
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

        if (HIDD_Gfx_GetMode(xsd->vcsd_VideoCoreGfxInstance, newmodeid, &sync, &pf)
            && sync && pf)
        {
            IPTR width = 0, height = 0, bytesperpix_attr = 0;
            ULONG aligned_width, fb_pitch = 0;
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
                                   &fb_ptr, &fb_pitch) && fb_ptr)
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
                    data->bytesperpix = bytesperpix;
                    data->bpp         = bytesperpix * 8;

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

/**********  Bitmap::Dispose()  ***********************************/

VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    EnterFunc(bug("VideoCoreGfx.OnBitMap::Dispose()\n"));
    OOP_DoSuperMethod(cl, o, msg);
    ReturnVoid("VideoCoreGfx.OnBitMap::Dispose");
}
