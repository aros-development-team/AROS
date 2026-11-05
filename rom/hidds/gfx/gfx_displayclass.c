/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: Graphics display class implementation.
*/

/****************************************************************************************/
#include "gfx_debug.h"

#include <aros/atomic.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/config.h>
#include <cybergraphx/cgxvideo.h>
#include <exec/lists.h>
#include <oop/static_mid.h>
#include <graphics/displayinfo.h>
#include <graphics/view.h>

#include "gfx_intern.h"
#include "gfx_display.h"
#include "gfx_softmodes.h"

#include <string.h>
#include <stddef.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <exec/libraries.h>
#include <exec/memory.h>

#include <utility/tagitem.h>

#include LC_LIBDEFS_FILE

#include <hidd/gfx.h>

/****************************************************************************************/

static VOID copy_bm_and_colmap(OOP_Class *cl, OOP_Object *o,  OOP_Object *src_bm,
                               OOP_Object *dst_bm, UWORD width, UWORD height)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data;
    ULONG                   i;
    IPTR                    numentries;
    OOP_Object              *src_colmap;
    APTR                    psrc_colmap = &src_colmap;

    data = OOP_INST_DATA(cl, o);

    /* We have to copy the colormap into the framebuffer bitmap */
    OOP_GetAttr(src_bm, aHidd_BitMap_ColorMap, (IPTR *)psrc_colmap);
    OOP_GetAttr(src_colmap, aHidd_ColorMap_NumEntries, &numentries);

    for (i = 0; i < numentries; i ++)
    {
        HIDDT_Color col;

        HIDD_CM_GetColor(src_colmap, i, &col);
        HIDD_BM_SetColors(dst_bm, &col, i, 1);
    }

    if (data->fbmode == vHidd_FrameBuffer_Mirrored)
    {
        /*
         * Mirror mode, just turn on visibility.
         * The data will be copied to the framebuffer later,
         * when graphics.library calls UpdateRect() after Show().
         */
        BM__Hidd_BitMap__SetVisible(CSD(cl)->bitmapclass, src_bm, TRUE);
    }
    else
    {
        /*
         * Direct framebuffer mode.
         * graphics.library will call UpdateRect() on the framebuffer object.
         * So we need to copy the data now.
         * We don't support scrolling for this mode, so we simply do the
         * bulk copy and ignore all offsets.
         */
        HIDD_Gfx_CopyBox(data->gfxhidd, src_bm, 0, 0,
                         dst_bm, 0, 0, width, height, data->gc);
    }
}

static UBYTE get_fbmode(OOP_Class *cl, OOP_Object *o)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    return data->fbmode;
}

/*
 * Software cursor support.
 *
 * These helpers render the mouse pointer in software for display drivers that
 * have no hardware sprite (e.g. vesagfx, vgagfx). The pointer is alpha
 * composited onto the bitmap that is currently scanned out (data->cursor_bm).
 * The pixels under the pointer are first copied into a save-under bitmap
 * (data->cursor_backup) so they can be restored when the pointer moves or is
 * hidden. All callers must hold data->fbsem.
 */

/* Restore the area saved under the cursor, removing it from the display. */
static void cursor_Erase(OOP_Class *cl, OOP_Object *o)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (!data->cursor_drawn || !data->cursor_bm || !data->cursor_backup)
        return;

    /* Raw native-format restore: a colour-space CopyBox on a palettized
       target degrades to a nearest-colour search per pixel */
    HIDD_BM_PutImage(data->cursor_bm, data->gc, data->cursor_backup,
                     data->cursor_backupW * data->cursor_backup_bpp,
                     data->cursor_backupX, data->cursor_backupY,
                     data->cursor_backupW, data->cursor_backupH,
                     vHidd_StdPixFmt_Native);
    HIDD_BM_UpdateRect(data->cursor_bm, data->cursor_backupX, data->cursor_backupY,
                       data->cursor_backupW, data->cursor_backupH);

    data->cursor_drawn = FALSE;
}

/* Save the area under the cursor and alpha-composite the pointer image. */
static void cursor_Draw(OOP_Class *cl, OOP_Object *o)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    IPTR fbw = 0, fbh = 0;
    IPTR le = 0, te = 0;
    LONG x, y, w, h, ox = 0, oy = 0;
    ULONG modulo;
    UBYTE *pixels;

    if (data->cursor_drawn || !data->cursor_visible)
        return;
    if (!data->cursor_bm || !data->cursor_argb || !data->cursor_backup)
        return;

    OOP_GetAttr(data->cursor_bm, aHidd_BitMap_Width, &fbw);
    OOP_GetAttr(data->cursor_bm, aHidd_BitMap_Height, &fbh);

    /*
     * The pointer position arrives in display coordinates, but it is composited
     * onto the bitmap. When the bitmap is panned (e.g. a dragged screen) its
     * content is shown at LeftEdge/TopEdge, so translate display -> bitmap by
     * subtracting that offset. This keeps cursor_drawX/Y in bitmap space, which
     * the CursorFB bracket intersection test also relies on.
     */
    OOP_GetAttr(data->cursor_bm, aHidd_BitMap_LeftEdge, &le);
    OOP_GetAttr(data->cursor_bm, aHidd_BitMap_TopEdge, &te);

    data->cursor_drawX = data->cursor_mouseX + data->cursor_xoffset - (LONG)le;
    data->cursor_drawY = data->cursor_mouseY + data->cursor_yoffset - (LONG)te;

    x = data->cursor_drawX;
    y = data->cursor_drawY;
    w = data->cursor_w;
    h = data->cursor_h;

    /* Fully off-screen? Nothing to do. */
    if ((x <= -w) || (y <= -h) || (x >= (LONG)fbw) || (y >= (LONG)fbh))
        return;

    /* Clip against the display edges */
    if (x < 0) { ox = -x; w += x; x = 0; }
    if (y < 0) { oy = -y; h += y; y = 0; }
    if (x + w > (LONG)fbw) w = (LONG)fbw - x;
    if (y + h > (LONG)fbh) h = (LONG)fbh - y;
    if ((w <= 0) || (h <= 0))
        return;

    /* Save the pixels we are about to overwrite */
    HIDD_BM_GetImage(data->cursor_bm, data->cursor_backup,
                     w * data->cursor_backup_bpp,
                     x, y, w, h, vHidd_StdPixFmt_Native);
    data->cursor_backupX = x;
    data->cursor_backupY = y;
    data->cursor_backupW = w;
    data->cursor_backupH = h;

    /* Alpha-composite the pointer image on top */
    modulo = data->cursor_w * 4;
    pixels = data->cursor_argb + oy * modulo + ox * 4;
    HIDD_BM_PutAlphaImage(data->cursor_bm, data->gc, pixels, modulo, x, y, w, h);
    HIDD_BM_UpdateRect(data->cursor_bm, x, y, w, h);

    data->cursor_drawn = TRUE;
}

/*
 * (Re)assert the cursor onto data->cursor_bm: erase from the old location and
 * draw at the current location. Used after the pointer moves, the shape
 * changes, visibility changes or the shown bitmap changes.
 */
static void cursor_Refresh(OOP_Class *cl, OOP_Object *o)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    cursor_Erase(cl, o);
    if (data->cursor_visible)
        cursor_Draw(cl, o);
}

/* (Re)create the save-under bitmap so it matches the current cursor size and
   the format of the bitmap the cursor is drawn onto. */
static BOOL cursor_AllocBackup(OOP_Class *cl, OOP_Object *o)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    OOP_Object *pf = NULL;
    IPTR bpp = 0;

    if (data->cursor_backup)
    {
        FreeVec(data->cursor_backup);
        data->cursor_backup = NULL;
    }

    if (!data->cursor_bm || !data->cursor_w || !data->cursor_h)
        return FALSE;

    OOP_GetAttr(data->cursor_bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);
    if (pf)
        OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bpp);
    if (!bpp)
        return FALSE;

    data->cursor_backup_bpp = bpp;
    data->cursor_backup = AllocVec(data->cursor_w * data->cursor_h * bpp, MEMF_ANY);
    return (data->cursor_backup != NULL);
}

/*
 * Track the bitmap that is currently scanned out and re-assert the software
 * cursor on top of it. Called from Show() (both the framebuffer and the
 * direct/no-framebuffer paths) so the pointer survives screen changes.
 * Caller must hold data->fbsem.
 */
static void cursor_SetTarget(OOP_Class *cl, OOP_Object *o, OOP_Object *bm)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    OOP_Object *real;

    /*
     * The pointer is composited onto the real (unwrapped) framebuffer bitmap so
     * that the compositing operations do not re-enter the CursorFB wrapper.
     */
    real = cursorfb_realbitmap(CSD(cl), bm);
    if (real)
        bm = real;

    if (data->cursor_bm == bm)
    {
        /*
         * Same target. Drawing that reaches the framebuffer goes through the
         * CursorFB wrapper, which lifts and re-renders the pointer around it,
         * so a pointer that is drawn is already on top of the current content.
         * Drawing it again here would save a backup that contains the pointer
         * itself, leaving a copy behind on the next move (issue #999).
         */
        if (!data->cursor_drawn && data->cursor_visible)
            cursor_Draw(cl, o);
        return;
    }

    /* Switching target: remove the pointer from the old one first */
    cursor_Erase(cl, o);

    data->cursor_drawn = FALSE;
    data->cursor_bm = bm;

    /* Backup bitmap is tied to the target's format, recreate it. */
    if (bm)
        cursor_AllocBackup(cl, o);

    if (data->cursor_visible)
        cursor_Draw(cl, o);
}

/*
 * Module-internal cursor primitives exposed to the CursorFB wrapper bitmap class
 * and the gfx CopyBox de-masquerade. They operate on the Display object's cursor
 * state. The caller is responsible for holding the cursor semaphore (returned by
 * GfxDisplay_CursorSem) across a remove/render bracket so that the framebuffer
 * update is atomic.
 */
struct SignalSemaphore *GfxDisplay_CursorSem(struct class_static_data *gcsd, OOP_Object *display)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(gcsd->displayclass, display);
    return &data->fbsem;
}

BOOL GfxDisplay_CursorIntersects(struct class_static_data *gcsd, OOP_Object *display, OOP_Object *bm,
                                 WORD x1, WORD y1, WORD x2, WORD y2)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(gcsd->displayclass, display);

    /* Only the currently shown bitmap carries the pointer */
    if (!data->cursor_drawn || (data->cursor_bm != bm))
        return FALSE;

    /* Reject if the operation rectangle is entirely outside the pointer rectangle */
    if ((x2 < data->cursor_drawX) || (x1 > data->cursor_drawX + data->cursor_w - 1) ||
        (y2 < data->cursor_drawY) || (y1 > data->cursor_drawY + data->cursor_h - 1))
        return FALSE;

    return TRUE;
}

void GfxDisplay_CursorRemove(struct class_static_data *gcsd, OOP_Object *display)
{
    cursor_Erase(gcsd->displayclass, display);
}

void GfxDisplay_CursorRender(struct class_static_data *gcsd, OOP_Object *display)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(gcsd->displayclass, display);

    if (data->cursor_visible)
        cursor_Draw(gcsd->displayclass, display);
}

/*
 * Make 'bm' (a real, unwrapped framebuffer bitmap) the bitmap that carries the
 * software pointer. Called when a CursorFB-wrapped bitmap becomes visible.
 */
void GfxDisplay_CursorSetTarget(struct class_static_data *gcsd, OOP_Object *display, OOP_Object *bm)
{
    OOP_Class *cl = gcsd->displayclass;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, display);

    ObtainSemaphore(&data->fbsem);
    cursor_SetTarget(cl, display, bm);
    ReleaseSemaphore(&data->fbsem);
}

/*
 * Detach the software pointer from 'bm' when that framebuffer is hidden. Only
 * acts if 'bm' is the bitmap currently carrying the pointer.
 */
void GfxDisplay_CursorUntarget(struct class_static_data *gcsd, OOP_Object *display, OOP_Object *bm)
{
    OOP_Class *cl = gcsd->displayclass;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, display);

    ObtainSemaphore(&data->fbsem);
    if (data->cursor_bm == bm)
    {
        cursor_Erase(cl, display);
        data->cursor_bm = NULL;
    }
    ReleaseSemaphore(&data->fbsem);
}

OOP_Object *Display__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct TagItem nametags[] =
    {
        {aHidd_Name, (IPTR)"Generic Display"},
        {TAG_MORE  , (IPTR)msg->attrList     }
    };
    struct pRoot_New supermsg =
    {
        .mID      = msg->mID,
        /* Provide a sensible default device name unless the driver set one */
        .attrList = FindTagItem(aHidd_Name, msg->attrList) ? msg->attrList : nametags
    };
    struct TagItem gctags[] =
    {
        {aHidd_GC_Foreground, 0},
        {TAG_DONE           , 0}
    };
    struct TagItem dmtags[] =
    {
        {aHidd_DMEnum_Display, 0},
        {TAG_MORE           , 0}
    };
    OOP_Class *dmenumclass = NULL;
    OOP_Class *compositorclass = NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&supermsg);
    if (o)
    {
        struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
        struct TagItem *tstate = msg->attrList;
        struct TagItem *tag;
        IPTR fbtype = -1;

        data->gfxhidd = NULL;
        data->dmenum = NULL;
        data->compositor = NULL;
        data->framebuffer = NULL;
        data->shownbm = NULL;
        data->gc = NULL;
        InitSemaphore(&data->fbsem);

        data->cursor_bm = NULL;
        data->cursor_backup = NULL;
        data->cursor_argb = NULL;
        data->cursor_w = 0;
        data->cursor_h = 0;
        data->cursor_visible = FALSE;
        data->cursor_drawn = FALSE;

        NEWLIST((struct List *)&data->decodes);
        InitSemaphore(&data->decodesem);

        while ((tag = NextTagItem(&tstate)))
        {
            ULONG idx;

            Hidd_Display_Switch(tag->ti_Tag, idx)
            {
            case aoHidd_Display_GfxHidd:
                data->gfxhidd = (OOP_Object *)tag->ti_Data;
                break;
            case aoHidd_Display_ModeTags:
                dmtags[1].ti_Data = tag->ti_Data;
                break;
            case aoHidd_Display_DMEnumClass:
                dmenumclass = (OOP_Class *)tag->ti_Data;
                break;
            case aoHidd_Display_CompositorClass:
                compositorclass = (OOP_Class *)tag->ti_Data;
                break;
            case aoHidd_Display_ConnectorType:
                data->connectortype = tag->ti_Data;
                break;
            case aoHidd_Display_ConnectorID:
                data->connectorid = tag->ti_Data;
                break;
            case aoHidd_Display_DecodeVectors:
                data->decodevectors = (struct HIDD_DecodeInterface *)tag->ti_Data;
                break;
            case aoHidd_Display_DecodeDataSize:
                data->decodedatasize = tag->ti_Data;
                break;
            }
        }

        if (data->gfxhidd)
            OOP_GetAttr(data->gfxhidd, aHidd_Gfx_FrameBufferType, &fbtype);
        data->fbmode = fbtype;

        /*
         * Every display owns a mode enumerator: standard pixel formats are
         * served through it (DMEnum::GetPixFmt), so it must exist even for
         * displays that register no modes of their own (e.g. the default
         * software display, which non-displayable bitmaps are attached to).
         */
        dmtags[0].ti_Data = (IPTR)o;
        data->dmenum = OOP_NewObject(dmenumclass ? dmenumclass : CSD(cl)->dmenumclass, NULL, dmtags);
        if (!data->dmenum)
        {
            OOP_MethodID dispose_mid = msg->mID - moRoot_New + moRoot_Dispose;
            OOP_CoerceMethod(cl, o, &dispose_mid);
            return NULL;
        }

        data->gc = OOP_NewObject(CSD(cl)->gcclass, NULL, gctags);
        if (NULL == data->gc)
        {
            OOP_MethodID dispose_mid = msg->mID - moRoot_New + moRoot_Dispose;
            OOP_CoerceMethod(cl, o, &dispose_mid);
            return NULL;
        }

        /*
         * Every display owns a compositor. It decides what is scanned out when
         * the driver does not compose in hardware (see ComposeViewPorts). The
         * driver may supply its own policy class; the base class composes in
         * software and passes a single full-display bitmap straight through.
         */
        {
            struct TagItem comptags[] =
            {
                {aHidd_Compositor_DisplayHidd, (IPTR)o},
                {TAG_DONE                    , 0      }
            };

            data->compositor = OOP_NewObject(compositorclass ? compositorclass : CSD(cl)->compositorclass, NULL, comptags);
            if (NULL == data->compositor)
            {
                OOP_MethodID dispose_mid = msg->mID - moRoot_New + moRoot_Dispose;
                OOP_CoerceMethod(cl, o, &dispose_mid);
                return NULL;
            }
            /* Damage notification runs after every render operation: resolve it once */
            data->comp_RectChanged = OOP_GetMethod(data->compositor,
                                                   HiddCompositorBase + moHidd_Compositor_BitMapRectChanged,
                                                   &data->comp_RectChanged_cl);
            if (!Compositor__Internal__CanHandleSoftware(CSD(cl)->compositorclass, data->compositor))
                DMEnum__Internal__DisableSoftwareModes(CSD(cl)->dmenumclass, data->dmenum);
        }
    }
    return o;
}

VOID Display__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (NULL != data->compositor)
        OOP_DisposeObject(data->compositor);
    if (NULL != data->gc)
        OOP_DisposeObject(data->gc);
    if (NULL != data->dmenum)
        OOP_DisposeObject(data->dmenum);
    if (NULL != data->cursor_backup)
        FreeVec(data->cursor_backup);
    if (NULL != data->cursor_argb)
        FreeVec(data->cursor_argb);

    /*
     * Decode interfaces their owners did not free. The driver ended their
     * sessions in its own Dispose (before calling us); only memory is left.
     */
    while (!IsListEmpty((struct List *)&data->decodes))
    {
        struct DecodeInstance *inst = (struct DecodeInstance *)RemHead((struct List *)&data->decodes);
        FreeMem(inst, inst->allocsize);
    }

    OOP_DoSuperMethod(cl, o, msg);
}

VOID Display__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Display_Switch (msg->attrID, idx)
    {
    case aoHidd_Display_GfxHidd:
        *msg->storage = (IPTR)data->gfxhidd;
        return;
    case aoHidd_Display_DMEnumerator:
        *msg->storage = (IPTR)data->dmenum;
        return;
    case aoHidd_Display_Compositor:
        *msg->storage = (IPTR)data->compositor;
        return;
    case aoHidd_Display_FrameBuffer:
        *msg->storage = (IPTR)data->framebuffer;
        return;
    case aoHidd_Display_DisplayID:
        *msg->storage = data->displayid;
        return;
    case aoHidd_Display_PlaneCount:
        *msg->storage = 1;      /* the primary plane, driven by Show() */
        return;
    case aoHidd_Display_VBlankCount:
        *msg->storage = 0;      /* not counted by the base class */
        return;
    case aoHidd_Display_ColorKey:
        *msg->storage = HIDD_DISPLAY_DEFAULT_COLORKEY;
        return;
    case aoHidd_Display_CompositionCapabilities:
        *msg->storage = 0;
        if (data->compositor)
            OOP_GetAttr(data->compositor, aHidd_Compositor_Capabilities, msg->storage);
        return;
    case aoHidd_Display_CompositionState:
        *msg->storage = 0;
        if (data->compositor)
            OOP_GetAttr(data->compositor, aHidd_Compositor_State, msg->storage);
        return;
    case aoHidd_Display_DefaultGC:
        *msg->storage = (IPTR)data->gc;
        return;
    case aoHidd_Display_Name:
        *msg->storage = (IPTR)OOP_OCLASS(o)->ClassNode.ln_Name;
        return;
    case aoHidd_Display_SupportsGamma:
        *msg->storage = 0;
        return;
    case aoHidd_Display_ConnectorType:
        *msg->storage = data->connectortype;
        return;
    case aoHidd_Display_ConnectorID:
        *msg->storage = data->connectorid;
        return;
    case aoHidd_Display_SpriteTypes:
        {
            IPTR hwcursor = 0;
            if (data->gfxhidd)
                OOP_GetAttr(data->gfxhidd, aHidd_Gfx_SupportsHWCursor, &hwcursor);
            *msg->storage = hwcursor ? (vHidd_SpriteType_3Plus1 | vHidd_SpriteType_DirectColor) : 0;
        }
        return;
    case aoHidd_Display_SoftCursor:
        {
            /*
             * We need a software cursor when the underlying gfx driver has no
             * hardware sprite. The base Display class renders it.
             */
            IPTR hwcursor = 0;
            if (data->gfxhidd)
                OOP_GetAttr(data->gfxhidd, aHidd_Gfx_SupportsHWCursor, &hwcursor);
            *msg->storage = hwcursor ? FALSE : TRUE;
        }
        return;
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID Display__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct TagItem *tag, *tstate = msg->attrList;
    ULONG idx;

    while ((tag = NextTagItem(&tstate)))
    {
        Hidd_Display_Switch (tag->ti_Tag, idx)
        {
        case aoHidd_Display_FrameBuffer:
            data->framebuffer = (OOP_Object *)tag->ti_Data;
            break;
        case aoHidd_Display_DisplayID:
            data->displayid = tag->ti_Data;
            break;
        case aoHidd_Display_CompositionState:
            if (data->compositor)
                OOP_SetAttrsTags(data->compositor, aHidd_Compositor_State, tag->ti_Data, TAG_DONE);
            break;
        /* aoHidd_Display_Compositor: owned by the display, not settable */
        }
    }
}

/*****************************************************************************************

    NAME
    moHidd_Display_DisplayToBMCoords

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_DisplayToBMCoords *msg);

        OOP_Object *HIDD_Display_DisplayToBMCoords(OOP_Object *Target, UWORD DispX, UWORD DispY, UWORD *TargetX, UWORD *TargetY);

    LOCATION
        hidd.gfx.display

    FUNCTION

    INPUTS
        Target - The BitMap Object to transform the Display co-ordinates to.
        DispX,DispY - The Display co-ordinates to transform.
        TargetX,TargetY - Where to store the transformed co-ordinates.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
VOID Display__Hidd_Display__DisplayToBMCoords(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_DisplayToBMCoords *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    LONG x = msg->DispX, y = msg->DispY;

    /* The compositor knows how (whether) the bitmap is scaled on the display */
    if (data->compositor)
        HIDD_Compositor_MapCoords(data->compositor, msg->Target, FALSE, &x, &y);
    *msg->TargetX = x;
    *msg->TargetY = y;
}

/*****************************************************************************************

    NAME
    moHidd_Display_BMToDisplayCoords

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_BMToDisplayCoords *msg);

        OOP_Object *HIDD_Display_BMToDisplayCoords(OOP_Object *Target, UWORD TargetX, UWORD TargetY, UWORD *DispX, UWORD *DispY);

    LOCATION
        hidd.gfx.display

    FUNCTION

    INPUTS
        Target - The BitMap Object to transform the co-ordinates from.
        TargetX,TargetY - The BitMap co-ordinates to transform.
        DispX,DispY - Where to store the transformed co-ordinates.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
VOID Display__Hidd_Display__BMToDisplayCoords(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_BMToDisplayCoords *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    LONG x = msg->TargetX, y = msg->TargetY;

    if (data->compositor)
        HIDD_Compositor_MapCoords(data->compositor, msg->Target, TRUE, &x, &y);
    *msg->DispX = x;
    *msg->DispY = y;
}

/*****************************************************************************************

    NAME
        moHidd_Display_NominalDimensions

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_NominalDimensions *msg);

        OOP_Object *HIDD_Display_NominalDimensions(OOP_Object *gfxHidd, UWORD *width, UWORD *height, UBYTE *depth);

    LOCATION
        hidd.gfx.display

    FUNCTION

    INPUTS
        gfxHidd - The graphics driver used to create the object.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/
VOID Display__Hidd_Display__NominalDimensions(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_NominalDimensions *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("HIDDGfx::NominalDimensions()\n"));

    if (msg->width)
        *(msg->width) = AROS_NOMINAL_WIDTH;
    if (msg->height)
        *(msg->height) = AROS_NOMINAL_HEIGHT;
    if (msg->depth)
    {
        UBYTE nomdepth = AROS_NOMINAL_DEPTH;

        if (data->dmenum)
        {
            HIDDT_ModeID mode = vHidd_ModeID_Invalid;
            OOP_Object *sync = NULL, *pf = NULL;
            ULONG min_ge_nom = ~0UL;

            while ((mode = HIDD_DMEnum_NextModeID(data->dmenum, mode, &sync, &pf)) != vHidd_ModeID_Invalid)
            {
                IPTR d = 0;

                if (DMEnum__Internal__IsSoftwareMode(CSD(cl)->dmenumclass, data->dmenum, mode))
                    continue;

                if (pf)
                {
                    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &d);
                    if (d >= AROS_NOMINAL_DEPTH && (ULONG)d < min_ge_nom)
                        min_ge_nom = (ULONG)d;
                }
            }

            if (min_ge_nom != ~0UL)
                nomdepth = (UBYTE)min_ge_nom;
        }

        *(msg->depth) = nomdepth;
    }
}

/*****************************************************************************************

    NAME
        moHidd_Display_CreateObject

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_CreateObject *msg);

        OOP_Object *HIDD_Display_CreateObject(OOP_Object *gfxHidd, OOP_Class *cl, struct TagItem *tagList);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Create a driver specific Gfx Object of the type "classID"

    INPUTS
        gfxHidd - The graphics driver used to create the object.
        cl - The base OOP_Class of the object to be created.
        tagList - Object specific attributes.

    RESULT
        pointer to the newly created OOP_Object, or NULL on failure.

    NOTES
           Drivers should query the gfx.hidd, or support class for the base class Ptr that
           the driver  objects should use. The gfx hidd itself defines the following -:

        GC
                A GC object is just used for data storage. It is possible to subclass, however
                it is not recommended since it may not be future-proof due to the fact
                GC subclasses can not be interchanged between different drivers.
                Avoid using custom GCs.

        BitMap

                Each graphics driver exposes at least one displayable bitmap class.
                More may be exposed at the drivers discretion to represent nondisplayable bitmaps
                or other driver specific bitmap types.

                Generally bitmap objects are never created directly. Instead they are created
                using the HIDD_Display_CreateObject() call. An implementation of this method in the
                driver should examine bitmap attributes supplied and make a decision if the bitmap
                should be created using the driver's own class or one of the system classes.

                A typical implementation should pay attention to the following bitmap attributes:

                aHIDD_BitMap_ModeID - If this attribute is supplied, the bitmap needs to be
                              either displayable by this driver, or be a friend of a
                              displayable bitmap. A friend bitmap usually repeats the
                              internal layout of its friend so that the driver may
                              perform blitting operations quickly.

                aHIDD_BitMap_Displayable - If this attribute is supplied, the bitmap NEEDS to be
                                   displayable by this driver. Usually this means that
                                   the bitmap object will contain video hardware state
                                   information. This attribute will always be accompanied
                                   by aHIDD_BitMap_ModeID.

                aHIDD_BitMap_FrameBuffer - The bitmap needs to be a framebuffer bitmap. A
                                   framebuffer bitmap is necessary for some kinds of
                                   hardware which have a small fixed amount of video
                                   RAM which can hold only one screen at a time. Setting
                                   this attribute requires that a valid ModeID be also set.

                aHIDD_BitMap_Friend - If there's no ModeID supplied, you may wish to check class
                              of friend bitmap. This can be useful if your driver uses
                              different classes for displayable and non-displayable bitmaps.
                              By default base class will pick up friend's class and use it
                              for new bitmap if nothing is specified, here you may override
                              this behavior.

                If a driver wants to specify a custom class for the bitmap being created,
                it should pass the aoHidd_BitMap_ClassPtr attribute to the base class.
                Bitmap objects should not be directly created, otherwise necessary information
                provided by the base class will be missing.

                This method must be implemented by the subclass. aHIDD_BitMap_ClassPtr or
                aHIDD_BitMap_ClassID must be provided to the base class for a displayable bitmap.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        The base class implementation currently does the folliwing in order to determine
        a class for a nondisplayable bitmap (in the listed order):

        1. Check aHIDD_BitMap_ClassPtr and aHIDD_BitMap_ClassID. If one of them is supplied,
           the class is already set by a subclass.
        2. Check aHIDD_BitMap_StdPixFmt. If this attribute is supplied, figure out type of
           the pixelformat (chunky or planar), and use one of two system's default classes.
        3. Check aHIDD_BitMap_Friend. If friend bitmap is supplied, obtain its class from
           aHIDD_BitMap_ClassPtr value of friend bitmap.
        4. If everything fails, bitmap creation fails too.

        This behavior is subject to change, but will maintain backwards compatibility.

*****************************************************************************************/

/* TRUE if this display needs a software pointer (the gfx driver has no HW sprite). */
static BOOL display_NeedSoftCursor(OOP_Class *cl, OOP_Object *o)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    IPTR hwcursor = 0;

    if (data->gfxhidd)
        OOP_GetAttr(data->gfxhidd, aHidd_Gfx_SupportsHWCursor, &hwcursor);

    return hwcursor ? FALSE : TRUE;
}

OOP_Object *Display__Hidd_Display__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CreateObject *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    OOP_Object *object = NULL;

    EnterFunc(bug("HIDDGfx::CreateObject()\n"));

    if (msg->cl == CSD(cl)->gcclass)
    {
        object = OOP_NewObject(NULL, CLID_Hidd_GC, msg->attrList);
    }
    else if (msg->cl == CSD(cl)->bitmapclass)
    {
        struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;

        struct TagItem bmtags[] =
        {
            {aHidd_BitMap_Display, 0},  /* 0 */
            {aHidd_BitMap_PixFmt , 0},  /* 1 */
            {TAG_IGNORE      , 0},      /* 2 */
            {TAG_IGNORE      , 0},      /* 3 */
            {TAG_IGNORE      , 0},      /* 4 */
            {TAG_IGNORE      , 0},      /* 5 */
            {TAG_MORE        , 0},      /* 6 */
        };
        struct TagItem *tag, *tstate = msg->attrList;
        struct TagItem *cloned_attrlist = NULL;
        ULONG idx;

        STRPTR        classid     = NULL;
        OOP_Class    *classptr    = NULL;
        BOOL          displayable = FALSE;
        BOOL          framebuffer = FALSE;
        BOOL          scanout     = FALSE;
        HIDDT_ModeID  modeid      = vHidd_ModeID_Invalid;
        BOOL          got_modeid  = FALSE;
        BOOL          softmode    = FALSE;
        BOOL          softfriend  = FALSE;
        HIDDT_StdPixFmt pixfmt    = vHidd_StdPixFmt_Unknown;
        OOP_Object   *friend_bm   = NULL;
        OOP_Object   *sync            = NULL;
        OOP_Object   *pf              = NULL;

        BOOL              gotclass   = FALSE;
        BOOL      got_width  = FALSE;
        BOOL      got_height = FALSE;
        BOOL      got_depth  = FALSE;

#define BMAO(x) aoHidd_BitMap_ ## x
#define BMAF(x) (1L << aoHidd_BitMap_ ## x)

#define BM_DIMS_AF  (BMAF(Width) | BMAF(Height))

#define SET_TAG(tags, idx, tag, val)    \
    tags[idx].ti_Tag = tag ; tags[idx].ti_Data = (IPTR)val;

#define SET_BM_TAG(tags, idx, tag, val) \
    SET_TAG(tags, idx, aHidd_BitMap_ ## tag, val)

#define COPY_BM_TAG(tags, idx, tag, obj)        \
    tags[idx].ti_Tag = aHidd_BitMap_ ## tag;    \
    OOP_GetAttr(obj, aHidd_BitMap_ ## tag , &tags[idx].ti_Data)

        while ((tag = NextTagItem(&tstate)))
        {
            if (IS_BITMAP_ATTR(tag->ti_Tag, idx))
            {
                switch (idx)
                {
                    case aoHidd_BitMap_Displayable:
                        displayable = tag->ti_Data;
                        break;

                    case aoHidd_BitMap_FrameBuffer:
                        framebuffer = tag->ti_Data;
                        break;

                    case aoHidd_BitMap_Scanout:
                        scanout = tag->ti_Data;
                        break;

                    case aoHidd_BitMap_Width:
                        got_width = TRUE;
                        break;

                    case aoHidd_BitMap_Height:
                        got_height = TRUE;
                        break;

                    case aoHidd_BitMap_Depth:
                        got_depth = TRUE;
                        break;

                    case aoHidd_BitMap_ModeID:
                        /* Make sure it is a valid mode, and retrieve sync/pixelformat data */
                        if (!HIDD_DMEnum_GetMode(data->dmenum, tag->ti_Data, &sync, &pf))
                        {
                            D(bug("!!! Gfx::CreateObject: USER PASSED INVALID MODEID !!!\n"));
                            return NULL;
                        }
                        modeid = (HIDDT_ModeID)tag->ti_Data;
                        got_modeid = TRUE;
                        break;

                    case aoHidd_BitMap_Friend:
                        friend_bm = (OOP_Object *)tag->ti_Data;
                        break;

                    case aoHidd_BitMap_PixFmt:
                        D(bug("!!! Gfx::CreateObject: USER IS NOT ALLOWED TO PASS aHidd_BitMap_PixFmt !!!\n"));
                        return NULL;

                    case aoHidd_BitMap_StdPixFmt:
                        pixfmt = tag->ti_Data;
                        break;

                    case aoHidd_BitMap_ClassPtr:
                        classptr = (OOP_Class *)tag->ti_Data;
                        gotclass = TRUE;
                        break;

                    case aoHidd_BitMap_ClassID:
                        classid  = (STRPTR)tag->ti_Data;
                        gotclass = TRUE;
                        break;
                }
            }
        }

        if (got_modeid && data->dmenum)
        {
            softmode = DMEnum__Internal__IsSoftwareMode(CSD(cl)->dmenumclass, data->dmenum, modeid);
        }
        else if (friend_bm && pixfmt == vHidd_StdPixFmt_Unknown &&
                 !GetTagData(aHidd_BitMap_VideoFormat, vHidd_VideoFmt_None, msg->attrList))
        {
            IPTR friend_modeid = vHidd_ModeID_Invalid;

            OOP_GetAttr(friend_bm, aHidd_BitMap_ModeID, &friend_modeid);
            if (friend_modeid != vHidd_ModeID_Invalid && data->dmenum)
            {
                softfriend = DMEnum__Internal__IsSoftwareMode(CSD(cl)->dmenumclass, data->dmenum,
                                                              (HIDDT_ModeID)friend_modeid);
            }
        }

        if ((softmode || softfriend) && scanout)
        {
            /* Software modes are never hardware scanout targets */
            return NULL;
        }

        if (softmode && framebuffer)
        {
            /*
             * Hardware framebuffers must use the sync's native TrueColor ModeID.
             * Clone msg->attrList and rewrite every aHidd_BitMap_ModeID tag so
             * caller tag arrays are never mutated in place and BM__Root__New
             * sees only the native backing ModeID.
             */
            HIDDT_ModeID backing_mode = DMEnum__Internal__GetBackingMode(CSD(cl)->dmenumclass,
                                                                         data->dmenum, modeid);
            if (backing_mode == vHidd_ModeID_Invalid ||
                !HIDD_DMEnum_GetMode(data->dmenum, backing_mode, &sync, &pf))
            {
                return NULL;
            }

            cloned_attrlist = CloneTagItems(msg->attrList);
            if (!cloned_attrlist)
                return NULL;

            tstate = cloned_attrlist;
            while ((tag = NextTagItem(&tstate)))
            {
                if (tag->ti_Tag == aHidd_BitMap_ModeID)
                    tag->ti_Data = (IPTR)backing_mode;
            }

            modeid = backing_mode;
            softmode = FALSE;
        }
        else if (softmode)
        {
            /*
             * Ensure the compositor's palette worker exists before creating a
             * software-mode bitmap. If worker creation fails, reject only the
             * optional software bitmap without failing native display operation.
             */
            if (!data->compositor ||
                !Compositor__Internal__CanHandleSoftware(CSD(cl)->compositorclass, data->compositor) ||
                !Compositor__Internal__EnsurePaletteTask(CSD(cl)->compositorclass, data->compositor))
            {
                return NULL;
            }

            /*
             * Software-mode screen and offscreen bitmaps always live in RAM
             * (ChunkyBM), even if a driver subclass prepended its own ClassPtr.
             */
            classptr = CSD(cl)->chunkybmclass;
            classid  = NULL;
            gotclass = TRUE;
        }
        else if (softfriend)
        {
            classptr = CSD(cl)->chunkybmclass;
            classid  = NULL;
            gotclass = TRUE;
        }

        /* If we have a friend bitmap, we can inherit some attributes from it */
         if (friend_bm)
        {
            if (!got_width)
            {
                COPY_BM_TAG(bmtags, 2, Width, friend_bm);
                got_width = TRUE;
            }

            if (!got_height)
            {
                COPY_BM_TAG(bmtags, 3, Height, friend_bm);
                got_height = TRUE;
            }

            if (!got_depth)
            {
                COPY_BM_TAG(bmtags, 4, Depth, friend_bm);
            }
        }

        if (framebuffer)
        {
            /* FrameBuffer implies Displayable */
            SET_BM_TAG(bmtags, 5, Displayable, TRUE);
            displayable = TRUE;
        }
        else if (displayable)
        {
            /*
             * Displayable, but not framebuffer (user's screen).
             * If we are working in framebuffer mode, we treat all such
             * bitmaps as framebuffer's friends and can inherit its class.
             */
            if ((!gotclass) && data->framebuffer && (get_fbmode(cl, o) != vHidd_FrameBuffer_None))
            {
                classptr = OOP_OCLASS(data->framebuffer);
                gotclass = TRUE;

                D(bug("[GFX] Using class 0x%p (%s) for displayable bitmap\n", classptr, classptr->ClassNode.ln_Name));
            }
        }

        if (displayable)
        {
            /* Displayable bitmap. Here we must have ModeID and class. */
            if (!sync)
            {
                D(bug("!!! Gfx::CreateObject: USER HAS NOT PASSED MODEID FOR DISPLAYABLE BITMAP !!!\n"));
                return NULL;
            }

            if (!gotclass)
            {
                D(bug("!!! Gfx::CreateObject: SUBCLASS DID NOT PASS CLASS FOR DISPLAYABLE BITMAP !!!\n"));
                if (cloned_attrlist)
                    FreeTagItems(cloned_attrlist);
                return NULL;
            }
        }
        else /* if (!displayable) */
        {
            /*
             * This is an offscreen bitmap and we need to guess its pixelformat.
             * In order to do this we need one of (in the order of preference):
             * - ModeID
             * - StdPixFmt
             * - Friend
             */

            if (sync)
            {
                /*
                 * We have alredy got sync for the modeid case.
                 * Obtain missing size information from it.
                 */
                if (!got_width)
                {
                    bmtags[2].ti_Tag = aHidd_BitMap_Width;
                    OOP_GetAttr(sync, aHidd_Sync_HDisp, &bmtags[2].ti_Data);
                }

                if (!got_height)
                {
                    bmtags[3].ti_Tag = aHidd_BitMap_Height;
                    OOP_GetAttr(sync, aHidd_Sync_VDisp, &bmtags[3].ti_Data);
                }
            }
            else if (pixfmt != vHidd_StdPixFmt_Unknown)
            {
                /* Next to look for is StdPixFmt */
                pf = DMEnum__Internal__GetPixFmt(CSD(cl)->dmenumclass, pixfmt);
                if (NULL == pf)
                {
                    D(bug("!!! Gfx::CreateObject(): USER PASSED BOGUS StdPixFmt !!!\n"));
                    return NULL;
                }
            }
            else if (friend_bm)
            {
                /* Last alternative is that the user passed a friend bitmap */

                /*
                 * A video bitmap (aHidd_BitMap_VideoFormat) has no RGB layout:
                 * the bitmap class registers its own pixel format, the friend
                 * only supplies size, class and display.
                 */
                if (!GetTagData(aHidd_BitMap_VideoFormat, vHidd_VideoFmt_None, msg->attrList))
                    OOP_GetAttr(friend_bm, aHidd_BitMap_PixFmt, (IPTR *)&pf);

                /*
                 * Inherit the class from friend bitmap (if not already specified).
                 * We do it because friend bitmap may be a display HIDD bitmap.
                 * A shown bitmap may be wrapped in the software-cursor CursorFB
                 * bitmap - inherit from the real bitmap it wraps, never from the
                 * wrapper: instantiating the wrapper class directly would produce
                 * a CursorFB with no real bitmap behind it.
                 */
                if (!gotclass)
                {
                    OOP_Object *realfriend = cursorfb_realbitmap(CSD(cl), friend_bm);

                    classptr = OOP_OCLASS(realfriend ? realfriend : friend_bm);
                    gotclass  = TRUE;

                    D(bug("[GFX] Friend bitmap is 0x%p has class 0x%p (%s)\n", friend_bm, classptr, classptr->ClassNode.ln_Name));
                }
            }
            else
            {
                D(bug("!!! Gfx::CreateObject: INSUFFICIENT ATTRS TO CREATE OFFSCREEN BITMAP !!!\n"));
                return NULL;
            }

            /* Did the subclass provide an offbitmap class for us? */
            if (!gotclass)
            {
                /* Have to find a suitable class ourselves from the pixelformat */
                HIDDT_BitMapType bmtype;

                OOP_GetAttr(pf, aHidd_PixFmt_BitMapType, &bmtype);
                switch (bmtype)
                {
                    case vHidd_BitMapType_Chunky:
                        classptr = CSD(cl)->chunkybmclass;
                        break;

                    case vHidd_BitMapType_Planar:
                        classptr = CSD(cl)->planarbmclass;
                        break;

                    default:
                        D(bug("!!! Gfx::CreateObject: UNKNOWN BITMAPTYPE %d !!!\n", bmtype));
                        return NULL;
                }
                D(bug("[GFX] Bitmap type is %u, using class 0x%p\n", bmtype, classptr));

            } /* if (!gotclass) */

        } /* if (!displayable) */

        /* Set the tags we want to pass to the selected bitmap class */
        bmtags[0].ti_Data = (IPTR)o;
        bmtags[1].ti_Data = (IPTR)pf;
        bmtags[6].ti_Data = (IPTR)(cloned_attrlist ? cloned_attrlist : msg->attrList);

        object = OOP_NewObject(classptr, classid, bmtags);

        if (cloned_attrlist)
            FreeTagItems(cloned_attrlist);

        /*
         * Drivers without a hardware sprite get a software pointer rendered by
         * this Display class. To keep it from being corrupted when the desktop
         * draws underneath it, the framebuffer / displayable bitmap is wrapped
         * in a CursorFB bitmap that brackets every drawing operation with a
         * hide/show of the pointer. The drivers themselves are untouched.
         */
        if (object && !softmode && (displayable || framebuffer) && display_NeedSoftCursor(cl, o))
        {
            OOP_Object *wrapped = create_cursorfb(CSD(cl), o, object);
            if (wrapped)
                object = wrapped;
        }

        /* Remember the framebuffer. It can be needed for default Show() implementation. */
        if (framebuffer)
            data->framebuffer = object;
    }

    ReturnPtr("HIDDGfx::CreateObject", OOP_Object *, object);
}

/*****************************************************************************************

    NAME
        moHidd_Display_Show

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_Show *msg);

        OOP_Object *HIDD_Display_Show(OOP_Object *gfxHidd, OOP_Object *bitMap, ULONG flags);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Change currently displayed bitmap on the screen.

        The bitmap object supplied must have been created with aHidd_BitMap_Displayable
        attribute set to TRUE.

        The function's behavior differs a lot depending on whether the driver uses a
        framebuffer or video hardware is able to switch screens itself.

        If the driver uses a framebuffer bitmap, it is supposed to copy the supplied bitmap
        into the framebuffer and return a framebuffer pointer. It also can be asked to
        copy back old framebuffer contents into previous bitmap object. It is driver's
        job to keep track of which bitmap object was displayed last time. This is what
        default implementation does. Note that it is very basic, and even does not support
        changing display resolution. It's not recommended to rely on it in production
        drivers (unless your video hardware supports only one mode).

        If the driver does not use a framebuffer, it is supposed to reprogram the hardware
        here to display an appropriate region of video RAM. Do not call the base class
        in this case, its implementation relies on framebuffer existance and will always
        return NULL which indicates an error.

        It is valid to get NULL value in bitMap parameter. This means that there is
        nothing to display and the screen needs to be blanked out. It is valid for
        non-framebuffer-based driver to return NULL as a reply then. In all other cases
        NULL return value means an error.

        Please avoid returning errors at all. graphics.library/LoadView() has no error
        indication. An error during showing a bitmap would leave the display in
        unpredictable state.

        If the driver does not use a framebuffer, consider using HIDD_Display_ShowViewPorts().
        It's more straightforward, flexible and offers support for screen composition.

    INPUTS
        gfxHidd - a display driver object, whose display you wish to change.
        bitMap  - a pointer to a bitmap object which needs to be shown or NULL.
        flags   - currently only one flag is defined:

        fHidd_Display_Show_CopyBack - Copy back the image data from framebuffer bitmap
                                  to old displayed bitmap. Used only if the driver
                                  needs a framebuffer.

    RESULT
        A pointer to a currently displayed bitmap object or NULL (read FUNCTION paragraph for
        detailed description)

    NOTES
        Drivers which use mirrored video data buffer do not have to update the display
        immediately in this method. moHidd_BitMap_UpdateRect will be sent to the returned
        bitmap if it's not NULL. Of course display blanking (if NULL bitmap was received)
        needs to be performed immediately.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Display_ShowViewPorts, graphics.library/LoadView()

    INTERNALS

*****************************************************************************************/

OOP_Object *Display__Hidd_Display__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_Show *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    OOP_Object              *bm = msg->bitMap;
    IPTR oldwidth  = 0;
    IPTR oldheight = 0;
    IPTR newwidth  = 0;
    IPTR newheight = 0;

    if (NULL == data->framebuffer)
    {
        if (bm && data->dmenum)
        {
            IPTR modeid = vHidd_ModeID_Invalid;

            OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
            if (DMEnum__Internal__IsSoftwareMode(CSD(cl)->dmenumclass, data->dmenum, (HIDDT_ModeID)modeid))
                return NULL;
        }

        /*
         * No framebuffer: the shown bitmap is scanned out directly (e.g. vesagfx).
         * The driver's own Show() has already made the bitmap visible. Here we
         * just track it as the software-cursor target and (re)assert the pointer
         * on top of it. This path is reached when a NoFrameBuffer driver's Show()
         * forwards to the base class via OOP_DoSuperMethod().
         */
        ObtainSemaphore(&data->fbsem);
        cursor_SetTarget(cl, o, bm);
        data->shownbm = bm;
        ReleaseSemaphore(&data->fbsem);
        return bm;
    }

    ObtainSemaphore(&data->fbsem);

    /* The software cursor will be re-asserted onto the new content below. */
    cursor_Erase(cl, o);

    if (data->shownbm)
    {
        /* Get size of old bitmap */
        OOP_GetAttr(data->shownbm, aHidd_BitMap_Width, &oldwidth);
        OOP_GetAttr(data->shownbm, aHidd_BitMap_Height, &oldheight);

        if (data->fbmode == vHidd_FrameBuffer_Mirrored)
        {
            BM__Hidd_BitMap__SetVisible(CSD(cl)->bitmapclass, data->shownbm, FALSE);
        }
        else if (msg->flags & fHidd_Display_Show_CopyBack)
        {
            /* Copy the framebuffer data back into the old shown bitmap */
            copy_bm_and_colmap(cl, o, data->framebuffer, data->shownbm, oldwidth, oldheight);
        }
    }

    if (bm == data->framebuffer)
    {
        /* If showing the framebuffer itself, just detach from old bitmap and that's all. */
        data->shownbm = NULL;
        cursor_SetTarget(cl, o, data->framebuffer);
        ReleaseSemaphore(&data->fbsem);

        return data->framebuffer;
    } else if (bm) {
        IPTR modeid;

        /*
         * Switch framebuffer display mode.
         * This operation can fail if the bitmap has inappropriate mode.
         */
        OOP_GetAttr(bm, aHidd_BitMap_ModeID, &modeid);
        if (data->dmenum &&
            DMEnum__Internal__IsSoftwareMode(CSD(cl)->dmenumclass, data->dmenum, (HIDDT_ModeID)modeid))
        {
            modeid = DMEnum__Internal__GetBackingMode(CSD(cl)->dmenumclass, data->dmenum,
                                                      (HIDDT_ModeID)modeid);
        }
        if (!OOP_SetAttrsTags(data->framebuffer, aHidd_BitMap_ModeID, modeid, TAG_DONE))
        {
            ReleaseSemaphore(&data->fbsem);
            return NULL;
        }

        /* Get size of new bitmap */
        OOP_GetAttr(bm, aHidd_BitMap_Width, &newwidth);
        OOP_GetAttr(bm, aHidd_BitMap_Height, &newheight);

        /* Copy it into the framebuffer */
        copy_bm_and_colmap(cl, o, bm, data->framebuffer, newwidth, newheight);
    }

    /*
     * Clear remaining parts of the framebuffer (if previous bitmap was larger than new one)
     * Note that if the new bitmap is NULL, newwidth and newheight will both be zero.
     * This will cause clearing the whole display.
     */
    if (oldheight) /* width and height can be zero only together, check one of them */
    {
        IPTR fbwidth = 0, fbheight = 0;

        /*
         * The mode switch above may have resized the framebuffer itself.
         * Clamp to what the framebuffer is now; for a driver whose
         * framebuffer keeps a fixed size this changes nothing.
         */
        OOP_GetAttr(data->framebuffer, aHidd_BitMap_Width, &fbwidth);
        OOP_GetAttr(data->framebuffer, aHidd_BitMap_Height, &fbheight);
        if (oldwidth > fbwidth)
            oldwidth = fbwidth;
        if (oldheight > fbheight)
            oldheight = fbheight;

        if (newwidth < oldwidth)
            HIDD_BM_FillRect(data->framebuffer, data->gc, newwidth, 0, oldwidth - 1, oldheight - 1);
        if ((newheight < oldheight) && newwidth)
            HIDD_BM_FillRect(data->framebuffer, data->gc, 0, newheight, newwidth - 1, oldheight - 1);
    }

    /* Remember new displayed bitmap */
    data->shownbm = bm;

    /*
     * (Re)assert the software cursor on top of the freshly shown content.
     * The scanned-out bitmap is the mirrored bitmap itself, or the framebuffer
     * for direct-framebuffer drivers.
     */
    cursor_SetTarget(cl, o, (data->fbmode == vHidd_FrameBuffer_Mirrored) ? bm : data->framebuffer);

    ReleaseSemaphore(&data->fbsem);

    /* Return the actual bitmap to perform further operations on */
    return (data->fbmode == vHidd_FrameBuffer_Mirrored) ? bm : data->framebuffer;
}

/*****************************************************************************************

    NAME
        moHidd_Display_ShowViewPorts

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_ShowViewPorts *msg);

        ULONG HIDD_Display_ShowViewPorts(OOP_Object *gfxHidd, struct HIDD_ViewPortData *data, struct View *view);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Show one or more bitmaps on the screen.

        It is completely up to the driver how to implement this function. The driver may
        or may not support hardware-assisted screens composition. Bitmaps are sorted
        in the list in descending z-order. The driver is expected to put at least frontmost
        bitmap on display.

        It is valid to get NULL pointer as data parameter. This means that there's
        nothing to show and the screen should go blank.

        Bitmaps display offsets are stored in their aHidd_BitMap_LeftEdge and
        aHidd_BitMap_TopEdge attributes. This function is not expected to modify their
        values somehow. They are assumed to be preserved between calls unless changed
        explicitly by the system.

        If you implement this method, you don't have to implement HIDD_Display_Show() because
        it will never be called.

        Note that there is no more error indication - the driver is expected to be
        error-free here.

    INPUTS
        gfxHidd - a display driver object, whose display you wish to change.
        data    - a singly linked list of bitmap objects to show

    RESULT
        TRUE if this method is supported by the driver, FALSE otherwise

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Display_Show

    INTERNALS
        Default base class implementation simply returns FALSE. This causes
        the system to use HIDD_Display_Show() instead.

*****************************************************************************************/

ULONG Display__Hidd_Display__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_ShowViewPorts *msg)
{
    /*
     * The base class does not compose in hardware. Returning FALSE makes
     * graphics.library call ComposeViewPorts(), which hands the chain to the
     * display's compositor.
     */
    return FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_Display_ComposeViewPorts

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_ComposeViewPorts *msg);

        OOP_Object *HIDD_Display_ComposeViewPorts(OOP_Object *display, struct HIDD_ViewPortData *Data, BOOL *composing);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Show a chain of bitmaps through the display's compositor. This is what
        graphics.library calls when the driver's ShowViewPorts() returned FALSE:
        the compositor shows the frontmost bitmap directly when it covers the
        whole display, and composes the chain into one bitmap otherwise.

    INPUTS
        display   - the display object
        Data      - a singly linked list of bitmaps to show, frontmost first;
                    NULL blanks the display
        composing - set to TRUE when the display now shows a composed bitmap
                    rather than the frontmost bitmap itself

    RESULT
        The bitmap object actually scanned out (what Show() returned), or NULL
        for a blank display.

    NOTES
        A driver with a hardware compositor overrides ShowViewPorts() and
        never sees this method. A driver that wants the system compositor for
        some chains only returns FALSE from its ShowViewPorts() for them.

    SEE ALSO
        moHidd_Display_ShowViewPorts, moHidd_Display_Show

*****************************************************************************************/

OOP_Object *Display__Hidd_Display__ComposeViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_ComposeViewPorts *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (data->compositor)
        return HIDD_Compositor_BitMapStackChanged(data->compositor, msg->Data, msg->composing);

    *msg->composing = FALSE;
    return HIDD_Display_Show(o, msg->Data ? msg->Data->Bitmap : NULL, fHidd_Display_Show_CopyBack);
}

/*****************************************************************************************

    NAME
        moHidd_Display_ViewPortPositionChange

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_ViewPortPositionChange *msg);

        BOOL HIDD_Display_ViewPortPositionChange(OOP_Object *display, OOP_Object *bitMap, SIPTR *x, SIPTR *y);

    LOCATION
        hidd.gfx.display

    FUNCTION
        A shown bitmap is being moved on the display (screen drag or scroll).
        The caller has already stored the new offsets in the bitmap's LeftEdge
        and TopEdge attributes; the display's compositor validates them, clamps
        them to what composition allows and writes the result back.

    INPUTS
        display - the display object
        bitMap  - the bitmap being moved
        x, y    - the requested offsets, updated to the validated ones

    RESULT
        TRUE when composition is active after the move, FALSE when the
        bitmap is scanned out directly.

*****************************************************************************************/

BOOL Display__Hidd_Display__ViewPortPositionChange(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_ViewPortPositionChange *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (data->compositor)
        return HIDD_Compositor_BitMapPositionChange(data->compositor, msg->bitMap, msg->x, msg->y);

    return FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_Display_UpdateRect

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_UpdateRect *msg);

        VOID HIDD_Display_UpdateRect(OOP_Object *display, OOP_Object *bitMap, WORD x, WORD y, UWORD width, UWORD height);

    LOCATION
        hidd.gfx.display

    FUNCTION
        A rectangle of a bitmap belonging to this display has been rendered to.
        While composing, the compositor re-composes the visible part of it;
        otherwise the change is passed to the bitmap's own UpdateRect().

    NOTES
        This is called after every rendering operation, so graphics.library
        caches the method with OOP_GetMethod() and the display calls its
        compositor through a cached method pointer as well.

*****************************************************************************************/

VOID Display__Hidd_Display__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_UpdateRect *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (data->comp_RectChanged)
    {
        struct pHidd_Compositor_BitMapRectChanged cmsg =
        {
            .mID    = HiddCompositorBase + moHidd_Compositor_BitMapRectChanged,
            .bm     = msg->bitMap,
            .x      = msg->x,
            .y      = msg->y,
            .width  = msg->width,
            .height = msg->height
        };

        data->comp_RectChanged(data->comp_RectChanged_cl, data->compositor, (OOP_Msg)&cmsg);
    }
    else
        HIDD_BM_UpdateRect(msg->bitMap, msg->x, msg->y, msg->width, msg->height);
}

/*****************************************************************************************

    NAME
        moHidd_Display_SetCursorShape

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_SetCursorShape *msg);

        BOOL HIDD_Display_SetCursorShape(OOP_Object *gfxHidd, OOP_Object *shape,
                                     WORD xoffset, WORD yoffset);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Set mouse pointer shape.

        A pointer image is contained in the specified bitmap object. The bitmap object
        may contain a colormap if the system wants to specify own colors for the pointer.
        The supplied colormap will also contain alpha channel values.

        It is up to driver what to do if, for example, alpha channel is not supported by
        the hardware. Or if given bitmap type is not supported (for example truecolor
        bitmap on LUT-only hardware). It is expected that the driver converts bitmap
        data to a more appropriate form in such a case.

        A hotspot is given as an offset from the actual hotspot to the top-left corner
        of the pointer image. It is generally needed only for hosted display drivers
        which utilize host's support for mouse pointer.

        The default implementation in the base class just does nothing. A software mouse
        pointer is implemented in a special layer called fakegfx.hidd inside
        graphics.library. If a software pointer emulation is used, this method will
        never be called.

    INPUTS
        gfxHidd - a display driver object, for whose display you wish to change the pointer
        shape   - a pointer to a bitmap object, containing pointer bitmap
        xoffset - a horizontal hotspot offset
        yoffset - a vertical hotspot offset

    RESULT
        TRUE on success, FALSE on failure

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Display_SetCursorPos, moHidd_Display_SetCursorVisible

    INTERNALS

*****************************************************************************************/

BOOL Display__Hidd_Display__SetCursorShape(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_Display_SetCursorShape *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    IPTR width = 0, height = 0;
    UBYTE *argb;

    ObtainSemaphore(&data->fbsem);

    /* Remove the current pointer from the display before changing it */
    cursor_Erase(cl, o);

    data->cursor_xoffset = msg->xoffset;
    data->cursor_yoffset = msg->yoffset;

    if (!msg->shape)
    {
        /* No shape - drop the pointer entirely */
        if (data->cursor_argb)
        {
            FreeVec(data->cursor_argb);
            data->cursor_argb = NULL;
        }
        data->cursor_w = 0;
        data->cursor_h = 0;
        ReleaseSemaphore(&data->fbsem);
        return TRUE;
    }

    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);

    if ((width == 0) || (height == 0))
    {
        ReleaseSemaphore(&data->fbsem);
        return FALSE;
    }

    /* Extract the pointer image as ARGB32. The pointer bitmap carries an alpha
       colormap (transparent index has alpha 0) so this yields correct
       transparency for alpha compositing onto truecolor displays. */
    argb = AllocVec(width * height * 4, MEMF_ANY);
    if (!argb)
    {
        ReleaseSemaphore(&data->fbsem);
        return FALSE;
    }

    HIDD_BM_GetImage(msg->shape, argb, width * 4, 0, 0, width, height, vHidd_StdPixFmt_ARGB32);

    if (data->cursor_argb)
        FreeVec(data->cursor_argb);
    data->cursor_argb = argb;
    data->cursor_w = width;
    data->cursor_h = height;

    /* The save-under must match the new pointer size */
    cursor_AllocBackup(cl, o);

    if (data->cursor_visible)
        cursor_Draw(cl, o);

    ReleaseSemaphore(&data->fbsem);

    return TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_Display_SetCursorVisible

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_SetCursorVisible *msg);

        VOID HIDD_Display_SetCursorVisible(OOP_Object *gfxHidd, BOOL visible);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Control mouse pointer visiblity.

        The default implementation in the base class does nothing. If a software pointer
        emulation is used, this method will never be called.

    INPUTS
        gfxHidd - a display driver object, on whose display you wish to turn
            pointer on or off
        visible - TRUE to enable pointer display, FALSE to disable it

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Display_SetCursorPos, moHidd_Display_SetCursorVisible

    INTERNALS

*****************************************************************************************/

VOID Display__Hidd_Display__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorVisible *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    ObtainSemaphore(&data->fbsem);

    data->cursor_visible = msg->visible;
    if (msg->visible)
        cursor_Draw(cl, o);
    else
        cursor_Erase(cl, o);

    ReleaseSemaphore(&data->fbsem);
}

/*****************************************************************************************

    NAME
        moHidd_Display_SetCursorPos

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_SetCursorPos *msg);

        BOOL HIDD_Display_SetCursorPos(OOP_Object *gfxHidd, WORD x, WORD y);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Set current mouse pointer position.

        This is a real position on top-left image corner relative to top-left corner of
        the physical display. Neither logical screen origin nor hotspot are taken into
        account here.

        The default implementation in the base class does nothing and just returns TRUE.
        If a software pointer emulation is used, this method will never be called.

    INPUTS
        gfxHidd - a display driver object, on whose display you wish to position the pointer
        x       - An x coordinate of the pointer (relative to the physical screen origin)
        y       - A y coordinate of the pointer (relative to the physical screen origin)

    RESULT
        Always TRUE. Reserved for future, do not use it.

    NOTES
        This method is called by graphics.library/MoveSprite() which has no return value.
        However, for historical reasons, this method has a return value. Drivers should
        always return TRUE in order to ensure future compatibility.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Display_SetCursorShape, moHidd_Display_SetCursorVisible, graphics.library/MoveSprite()

    INTERNALS

*****************************************************************************************/

BOOL Display__Hidd_Display__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorPos *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    ObtainSemaphore(&data->fbsem);

    data->cursor_mouseX = msg->x;
    data->cursor_mouseY = msg->y;

    /* Erase from the old position and redraw at the new one */
    cursor_Refresh(cl, o);

    ReleaseSemaphore(&data->fbsem);

    return TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_Display_ModeProperties

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_ModeProperties *msg);

        ULONG HIDD_Display_ModeProperties(OOP_Object *gfxHidd, HIDDT_ModeID modeID,
                                      struct HIDD_ModeProperties *props, ULONG propsLen);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Obtain an information about the video mode.

        Video mode description structure may grow in future, so be careful and always check
        propsLen parameter value. A system may ask you for less data than you can provide.
        Always return an actual value. Do not just zero out fields you don't know about,
        this is not expected to be backwards compatible.

    INPUTS
        gfxHidd  - a pointer to a display driver object whose display mode you want to query
        modeID   - a mode ID to query
        props    - a pointer to a storage area where HIDD_ModeProperties structure will be put
        propsLen - length of the supplied buffer in bytes.

    RESULT
        Actual length of obtained structure

    NOTES
        Returned data must reflect only real hardware capabilities. For example, do not
        count emulated sprites. The system takes care about emulated features itself.

    EXAMPLE

    BUGS

    SEE ALSO
        aoHidd_Gfx_HWSpriteTypes, aoHidd_Gfx_SupportsHWCursor

    INTERNALS
        Default implementation in the base class relies on aHidd_Display_SpriteTypes attribute,
        not vice versa. If you override this method, do not forget to override this attribute too.

*****************************************************************************************/

ULONG Display__Hidd_Display__ModeProperties(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_ModeProperties *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct HIDD_ModeProperties props = {0, 0, 0};
    IPTR has_hw_cursor = 0;
    ULONG len = msg->propsLen;

    D(bug("[GFXHIDD] Hidd::Gfx::ModeProperties(0x%08lX, 0x%p, %u)\n", msg->modeID, msg->props, msg->propsLen));
    OOP_GetAttr(o, aHidd_Display_SpriteTypes, &has_hw_cursor);
    if (has_hw_cursor) {
        D(bug("[GFXHIDD] Driver has hardware mouse cursor implementation\n"));
        props.DisplayInfoFlags = DIPF_IS_SPRITES;
        props.NumHWSprites = 1;
    }

    if (len > sizeof(props))
        len = sizeof(props);
    D(bug("[GFXHIDD] Copying %u bytes\n", len));
    CopyMem(&props, msg->props, len);

    return len;
}

/*****************************************************************************************

    NAME
        moHidd_Display_GetGamma

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_GetGamma *msg);

        BOOL HIDD_Display_GetGamma(OOP_Object *gfxHidd, UBYTE *Red, UBYTE *Green, UBYTE *Blue);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Get current gamma table for the display.

        This method was neither ever implemented nor used. Currently obsolete and
        considered reserved.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Display_SetGamma

    INTERNALS

*****************************************************************************************/

BOOL Display__Hidd_Display__GetGamma(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_GetGamma *msg)
{
    return FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_Display_SetGamma

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_SetGamma *msg);

        BOOL HIDD_Display_SetGamma(OOP_Object *gfxHidd, UBYTE *Red, UBYTE *Green, UBYTE *Blue);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Set current gamma table for the display.

        A gamma table consists of three 256-byte tables: one for red component, one for
        green and one for blue.

    INPUTS
        gfxHidd - A display driver object
        Red     - A pointer to a 256-byte array for red component
        Green   - A pointer to a 256-byte array for green component
        Blue    - A pointer to a 256-byte array for blue component

    RESULT
        FALSE if the driver doesn't support gamma correction, otherwise TRUE

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

BOOL Display__Hidd_Display__SetGamma(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetGamma *msg)
{
    return FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_Display_GetMaxSpriteSize

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_GetMaxSpriteSize *msg);

        BOOL HIDD_Display_GetMaxSpriteSize(OOP_Object *gfxHidd, ULONG Type, UWORD *Width, UWORD *Height);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Query maximum allowed size for the given sprite type.

    INPUTS
        gfxHidd - A display driver object
        Type    - Type of the sprite image (one of vHidd_SpriteType_... values)
        Width   - A pointer to UWORD where width will be placed.
        Height  - A pointer to UWORD where height will be placed.

    RESULT
        FALSE is the given sprite type is not supported, otherwise TRUE.

    NOTES
        Default implementation in the base class just return some small values
        which it hopes can be supported by every driver if the driver supports given
        sprite type. It is strongly suggested to reimplement this method in the display
        driver.

        Width and Height are considered undefined if the method returns FALSE.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

BOOL Display__Hidd_Display__GetMaxSpriteSize(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_GetMaxSpriteSize *msg)
{
    IPTR types;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;

    OOP_GetAttr(o, aHidd_Display_SpriteTypes, &types);

    if (types & msg->Type) {
        *msg->Width  = 16;
        *msg->Height = 32;
        return TRUE;
    } else
        return FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_Display_MakeViewPort

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_MakeViewPort *msg);

        ULONG HIDD_Display_MakeViewPort(OOP_Object *gfxHidd, struct HIDD_ViewPortData *data, struct View *view)

    LOCATION
        hidd.gfx.display

    FUNCTION
        Performs driver-specific setup on a given ViewPort.

    INPUTS
        gfxHidd - A display driver object.
        data    - a pointer to a HIDD_ViewPortData structure.
        view    - the View which contains the ViewPort.

    RESULT
        The same code as used as return value for graphics.library/MakeVPort().

    NOTES
        When graphics.library calls this method, a complete view is not built yet.
        This means that data->Next pointer contains invalid data and needs to be
        ignored.

        It is valid to keep private data pointer in data->UserData accross calls.
        Newly created HIDD_ViewPortData is guraranteed to have NULL there.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Display_DeinitViewPort

    INTERNALS
        Base class implementation just does nothing. This function is mainly intended
        to provide support for copperlist maintenance by Amiga(tm) chipset driver.

*****************************************************************************************/

ULONG Display__Hidd_Display__MakeViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_MakeViewPort *msg)
{
    D(bug("Gfx::MakeViewPort: object 0x%p, data 0x%p\n", o, msg->Data));

    return MVP_OK;
}

/*****************************************************************************************

    NAME
        moHidd_Display_DeinitViewPort

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_DeinitViewPort *msg);

        ULONG HIDD_Display_DeinitViewPort(OOP_Object *gfxHidd, struct HIDD_ViewPortData *data)

    LOCATION
        hidd.gfx.display

    FUNCTION
        Performs driver-specific cleanup on a given ViewPort.

    INPUTS
        gfxHidd - A display driver object.
        data    - a pointer to a HIDD_ViewPortData structure.

    RESULT
        The same code as used as return value for graphics.library/MakeVPort().

    NOTES
        When graphics.library calls this method, the ViewPort is already unlinked
        from its view, and the bitmap can already be deallocated.
        This means that both data->Next and data->Bitmap pointers can contain invalid
        values.

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_Display_MakeViewPort

    INTERNALS
        Base class implementation just does nothing. This function is mainly intended
        to provide support for copperlist disposal by Amiga(tm) chipset driver.

*****************************************************************************************/

void Display__Hidd_Display__DeinitViewPort(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_DeinitViewPort *msg)
{
    D(bug("Gfx::CleanViewPort: object 0x%p, data 0x%p\n", o, msg->Data));
}

/*****************************************************************************************

    NAME
        moHidd_Display_InitViewPorts

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_InitViewPorts *msg);

        ULONG HIDD_Display_InitViewPorts(OOP_Object *gfxHidd, struct HIDD_ViewPortData *data, struct View *view)

    LOCATION
        hidd.gfx.display

    FUNCTION
        Performs driver-specific setup on a given view.

    INPUTS
        gfxHidd - A display driver object.
        data    - a pointer to a chain of HIDD_ViewPortData structures.
        view    - A pointer to graphics.library View structure being prepared.

    RESULT
        MCOP_OK if there was no error or MCOP_NO_MEM otherwise.
        MCOP_NOP is not allowed as a return value of this method.

    NOTES
        graphics.library calls this method in MrgCop() after the complete view
        is built. data->Next pointer contains valid data.

        This function can be repeatedly called several times, and there is no
        cleanup counterpart for it. This should be taken into account in method
        implementation.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Base class implementation just does nothing. This function is mainly intended
        to provide support for copperlist maintenance by Amiga(tm) chipset driver.

*****************************************************************************************/

ULONG Display__Hidd_Display__InitViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_ShowViewPorts *msg)
{
    D(bug("[HiddDisplay] %s()\n", __func__));

    return MCOP_OK;
}

/*****************************************************************************************

    NAME
        moHidd_Display_SurfaceStackChanged

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_SurfaceStackChanged *msg);

        VOID HIDD_Display_SurfaceStackChanged(OOP_Object *display, struct HIDD_ViewPortData *Data);

    LOCATION
        hidd.gfx.display

    FUNCTION
        The composited layer surfaces (Data->Surfaces) of a shown ViewPort
        changed. The display's compositor re-composes the affected area.

    NOTES
        graphics.library calls this from its private LayerSurfacesChanged()
        entry on behalf of layers.library.

*****************************************************************************************/

VOID Display__Hidd_Display__SurfaceStackChanged(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SurfaceStackChanged *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (data->compositor)
        HIDD_Compositor_SurfaceStackChanged(data->compositor, msg->Data);
}

/*****************************************************************************************

    NAME
        moHidd_Display_GetPlaneInfo

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_GetPlaneInfo *msg);

        ULONG HIDD_Display_GetPlaneInfo(OOP_Object *display, ULONG index, struct HIDD_PlaneInfo *info, ULONG infoLen);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Describe one of the display's scan-out planes: its type, the z-order
        positions it can take, the pixel formats it can scan out, how much it
        can scale, which alpha blending it does, and its alignment and size
        limits. The display's compositor uses this to decide which composition
        requests can be scanned out directly and which must be blitted.

    INPUTS
        display - the display object
        index   - 0 .. aHidd_Display_PlaneCount - 1
        info    - where to store the description
        infoLen - size of the buffer, so that the structure may grow

    RESULT
        The number of bytes stored, 0 if there is no such plane.

    NOTES
        The base class has one plane, the primary: any displayable pixel
        format, no scaling, no blending. It is what Show() drives.

*****************************************************************************************/

ULONG Display__Hidd_Display__GetPlaneInfo(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_GetPlaneInfo *msg)
{
    struct HIDD_PlaneInfo info =
    {
        .Type       = vHidd_Plane_Primary,
        .ZMin       = 0,
        .ZMax       = 0,
        .PixFmtMask = 0xFFFFFFFF,
        .ScaleMin   = HIDD_PLANE_SCALE_UNITY,
        .ScaleMax   = HIDD_PLANE_SCALE_UNITY,
        .AlphaModes = (1 << vHidd_CompAlpha_None),
        .PosAlignX  = 1,
        .PosAlignY  = 1,
        .MaxWidth   = 0,
        .MaxHeight  = 0,
        .Flags      = 0
    };
    ULONG len = msg->infoLen;

    if (msg->index != 0)
        return 0;

    if (len > sizeof(info))
        len = sizeof(info);
    CopyMem(&info, msg->info, len);

    return len;
}

/*****************************************************************************************

    NAME
        moHidd_Display_CommitPlanes

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_CommitPlanes *msg);

        BOOL HIDD_Display_CommitPlanes(OOP_Object *display, struct HIDD_PlaneState *states, ULONG count, ULONG flags);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Apply a complete set of plane states atomically: every plane named in
        the array takes its new bitmap, rectangles and blending at the same
        frame boundary; planes not named keep their state. A NULL BitMap
        disables a plane.

    INPUTS
        display - the display object
        states  - the plane states to apply
        count   - how many
        flags   - fHidd_Commit_NoWait: return as soon as the state is queued;
                  fHidd_Commit_Test: validate against GetPlaneInfo() only

    RESULT
        TRUE when the states are valid (and, unless testing, applied).

    NOTES
        The base class implements its single primary plane with Show(): the
        bitmap of the plane 0 state is shown, unscaled, at the origin.

    SEE ALSO
        moHidd_Display_GetPlaneInfo, moHidd_Display_WaitLatch, moHidd_Display_Show

*****************************************************************************************/

BOOL Display__Hidd_Display__CommitPlanes(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CommitPlanes *msg)
{
    struct HIDD_PlaneState *primary = NULL;
    ULONG i;

    for (i = 0; i < msg->count; i++)
    {
        struct HIDD_PlaneState *st = &msg->states[i];

        if (st->Plane != 0)
            return FALSE;
        if (st->BitMap)
        {
            /* The primary scans the bitmap out as it is: no offset, no scaling, no blending */
            if ((st->Dest.MinX != 0) || (st->Dest.MinY != 0) || (st->Src.MinX != 0) || (st->Src.MinY != 0))
                return FALSE;
            if (((st->Dest.MaxX - st->Dest.MinX) != (st->Src.MaxX - st->Src.MinX)) ||
                ((st->Dest.MaxY - st->Dest.MinY) != (st->Src.MaxY - st->Src.MinY)))
                return FALSE;
            if (st->AlphaMode != vHidd_CompAlpha_None)
                return FALSE;
        }
        primary = st;
    }

    if (!primary || (msg->flags & fHidd_Commit_Test))
        return TRUE;

    HIDD_Display_Show(o, primary->BitMap, fHidd_Display_Show_CopyBack);
    return TRUE;
}

/*****************************************************************************************

    NAME
        moHidd_Display_WaitLatch

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_WaitLatch *msg);

        VOID HIDD_Display_WaitLatch(OOP_Object *display);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Wait until the last CommitPlanes() has been scanned out, i.e. until
        the bitmaps it replaced are no longer read by the hardware and may be
        reused or freed.

    NOTES
        The base class shows bitmaps synchronously and returns at once.

*****************************************************************************************/

VOID Display__Hidd_Display__WaitLatch(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_WaitLatch *msg)
{
}

/*****************************************************************************************

    NAME
        moHidd_Display_QueryScanout

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_QueryScanout *msg);

        ULONG HIDD_Display_QueryScanout(OOP_Object *display, HIDDT_StdPixFmt pixFmt, ULONG videoFmt,
                                        ULONG width, ULONG height, struct HIDD_ScanoutCaps *caps);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Asks whether a plane of this display can show a client surface of the
        given format and source size: a video format (videoFmt != 0, pixFmt is
        then ignored) or a standard RGB pixel format. The answer is SCF_* flags,
        0 when no plane can show it at all; caps (may be NULL) receives the
        details. cgxvideo.library and gallium.library ask this before creating
        a scan-out bitmap (aHidd_BitMap_Scanout) through moHidd_Display_CreateObject.

        The base class answers 0: its one plane shows screens, not surfaces.
        A driver answers from its plane set; a driver that cannot scan the
        format out in hardware must answer 0, there is no software fallback in
        the system (the client provides its own).

    INPUTS
        display  - the display
        pixFmt   - vHidd_StdPixFmt_* of an RGB surface
        videoFmt - vHidd_VideoFmt_* of a video surface, or vHidd_VideoFmt_None
        width    - source width in pixels
        height   - source height in pixels
        caps     - filled in when non-NULL

    RESULT
        SCF_* flags, 0 = not possible.

    SEE ALSO
        moHidd_Display_AttachSurface, moHidd_Display_GetPlaneInfo

*****************************************************************************************/

ULONG Display__Hidd_Display__QueryScanout(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_QueryScanout *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;

    if (msg->caps)
        SetMem(msg->caps, 0, sizeof(struct HIDD_ScanoutCaps));
    return 0;
}

/*****************************************************************************************

    NAME
        moHidd_Display_QueryDecode

    SYNOPSIS
        ULONG OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_QueryDecode *msg);

        ULONG HIDD_Display_QueryDecode(OOP_Object *display, LONG profile, ULONG width,
                                       ULONG height, struct HIDD_DecodeCaps *caps);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Asks whether this display's hardware decoder can decode a stream of
        the given VA-API profile (a VAProfile value, <va/va.h>) and picture
        size. The answer is DCF_* flags, 0 when it cannot; caps (may be NULL)
        receives the details: size limits, the vHidd_VideoFmt_* and
        vHidd_VideoLayout_* it writes, and how many reference frames it holds.
        A size of 0 x 0 asks about the profile alone.

        Decoding then goes through a decode interface
        (moHidd_Display_GetDecodeInterface) into video bitmaps created with
        aHidd_BitMap_DecodeTarget through moHidd_Display_CreateObject.

        The base class answers 0: there is no software decoder in the system.

    INPUTS
        display - the display
        profile - VAProfile value
        width   - picture width in pixels, or 0
        height  - picture height in pixels, or 0
        caps    - filled in when non-NULL

    RESULT
        DCF_* flags, 0 = not possible.

    SEE ALSO
        moHidd_Display_GetDecodeInterface, moHidd_Display_QueryScanout

*****************************************************************************************/

ULONG Display__Hidd_Display__QueryDecode(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_QueryDecode *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;

    if (msg->caps)
        SetMem(msg->caps, 0, sizeof(struct HIDD_DecodeCaps));
    return 0;
}

/* Entries a driver's (older, shorter) decode table does not have */
static BOOL DecodeStub_BeginFrame(APTR obj, OOP_Object *bitmap, ULONG buffer)            { return FALSE; }
static BOOL DecodeStub_SubmitParams(APTR obj, ULONG type, APTR data, ULONG size, ULONG count) { return FALSE; }
static BOOL DecodeStub_SubmitBitstream(APTR obj, APTR data, ULONG size)                 { return FALSE; }
static BOOL DecodeStub_EndFrame(APTR obj)                                                { return FALSE; }
static ULONG DecodeStub_Sync(APTR obj, OOP_Object *bitmap, ULONG buffer, ULONG timeoutms) { return vHidd_DecodeStatus_Error; }
static ULONG DecodeStub_Status(APTR obj, OOP_Object *bitmap, ULONG buffer)              { return vHidd_DecodeStatus_Error; }

static const struct HIDD_DecodeInterface DecodeStubs =
{
    sizeof(struct HIDD_DecodeInterface),
    DecodeStub_BeginFrame,
    DecodeStub_SubmitParams,
    DecodeStub_SubmitBitstream,
    DecodeStub_EndFrame,
    DecodeStub_Sync,
    DecodeStub_Status
};

/*****************************************************************************************

    NAME
        moHidd_Display_GetDecodeInterface

    SYNOPSIS
        APTR OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_GetDecodeInterface *msg);

        APTR HIDD_Display_GetDecodeInterface(OOP_Object *display, LONG profile, ULONG width,
                                             ULONG height, struct Hook *resolve, ULONG flags);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Starts a hardware decode session for a stream of the given VA-API
        profile and coded size, and returns its decode interface: an opaque
        object used with the HIDD_Decode_*() stubs of <hidd/gfx.h>. Each call
        starts a new session; a caller decoding two streams gets two
        interfaces. End a session with moHidd_Display_FreeDecodeInterface.

        Frames are decoded into buffers of video bitmaps created through this
        display with aHidd_BitMap_DecodeTarget; HIDD_Decode_BeginFrame() names
        the target of each. Surface IDs in the VA structures the caller
        submits (reference frames) are resolved with the resolve hook:
        CallHookPkt(resolve, iface, struct HIDD_DecodeSurfaceMsg *), which
        fills in the bitmap and buffer holding that surface and returns TRUE.

    INPUTS
        display - the display
        profile - VAProfile value (<va/va.h>)
        width   - coded picture width
        height  - coded picture height
        resolve - surface ID hook, must stay valid for the session
        flags   - reserved, 0

    RESULT
        The interface, or NULL when the display cannot decode the stream.

    NOTES
        The base class only builds the instance: it allocates the driver's
        vector table (aHidd_Display_DecodeVectors) followed by
        aHidd_Display_DecodeDataSize bytes of cleared driver data and returns
        the data part, or NULL when the driver gave no vectors. A driver
        overrides the method, calls the superclass and sets up its session in
        the data part, as ata.device bus drivers do for their PIO and DMA
        interfaces; it must not allocate the interface itself.

        A driver's Dispose ends the sessions still open before it calls the
        superclass, which then frees their memory.

    SEE ALSO
        moHidd_Display_FreeDecodeInterface, moHidd_Display_QueryDecode,
        aHidd_BitMap_DecodeTarget

*****************************************************************************************/

APTR Display__Hidd_Display__GetDecodeInterface(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_GetDecodeInterface *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    struct DecodeInstance *inst;
    IPTR size;
    ULONG vecsize;

    if (!data->decodevectors || !msg->resolve)
        return NULL;

    size = sizeof(struct DecodeInstance) + data->decodedatasize;
    inst = AllocMem(size, MEMF_PUBLIC | MEMF_CLEAR);
    if (!inst)
        return NULL;

    inst->allocsize = size;

    /* Driver entries over the stubs; the table always has the current size */
    CopyMem((APTR)&DecodeStubs, &inst->vec, sizeof(struct HIDD_DecodeInterface));
    vecsize = data->decodevectors->di_Size;
    if (vecsize > sizeof(struct HIDD_DecodeInterface))
        vecsize = sizeof(struct HIDD_DecodeInterface);
    if (vecsize > offsetof(struct HIDD_DecodeInterface, di_BeginFrame))
        CopyMem((UBYTE *)data->decodevectors + offsetof(struct HIDD_DecodeInterface, di_BeginFrame),
                &inst->vec.di_BeginFrame,
                vecsize - offsetof(struct HIDD_DecodeInterface, di_BeginFrame));
    inst->vec.di_Size = sizeof(struct HIDD_DecodeInterface);

    ObtainSemaphore(&data->decodesem);
    AddTail((struct List *)&data->decodes, (struct Node *)&inst->node);
    ReleaseSemaphore(&data->decodesem);

    return DECODEINST_DATA(inst);
}

/*****************************************************************************************

    NAME
        moHidd_Display_FreeDecodeInterface

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_FreeDecodeInterface *msg);

        VOID HIDD_Display_FreeDecodeInterface(OOP_Object *display, APTR iface);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Ends a decode session started with moHidd_Display_GetDecodeInterface.
        Decoding in flight completes first; the target buffers keep what was
        decoded into them.

    INPUTS
        display - the display the interface was obtained from
        iface   - the interface, may be NULL

    NOTES
        A driver overrides it to end its session (waiting for its engine) and
        then calls the superclass, which frees the instance.

*****************************************************************************************/

VOID Display__Hidd_Display__FreeDecodeInterface(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_FreeDecodeInterface *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);
    struct DecodeInstance *inst;

    if (!msg->iface)
        return;

    inst = DECODEINST_FROM(msg->iface);

    ObtainSemaphore(&data->decodesem);
    Remove((struct Node *)&inst->node);
    ReleaseSemaphore(&data->decodesem);

    FreeMem(inst, inst->allocsize);
}

/*****************************************************************************************

    NAME
        moHidd_Display_AttachSurface

    SYNOPSIS
        BOOL OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_AttachSurface *msg);

        BOOL HIDD_Display_AttachSurface(OOP_Object *display, OOP_Object *screenBitMap,
                                        struct HIDD_CompositeRequest *request, struct Hook *hook);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Attaches a client surface (a video overlay or a 3D colour buffer) to a
        screen shown on this display. The request describes the surface: BitMap
        is a scan-out bitmap, Src the part of it shown, Dest its rectangle in
        screen bitmap coordinates, Shape the visible region in the same
        coordinates (NULL = all of Dest), Flags carry CRF_PLANE_REQUIRED (video,
        never blitted) or CRF_PLANE_OK (may be blitted when no plane is free)
        and CRF_SCALED. The request belongs to the caller and must stay valid and
        unchanged while attached, except through moHidd_Display_UpdateSurface.

        The display's compositor shows the surface above the screen's own
        content, on a hardware plane where it can (cropped to its visible
        rectangle or colour keyed), and calls hook (CallHookPkt(hook, display,
        struct HIDD_SurfaceHookMsg *)) when the client should look at the screen
        again (vHidd_SurfaceHook_Damage) or the plane state changed. The hook
        runs in the task that caused the event, with no compositor lock held,
        and must not block.

    INPUTS
        display      - the display
        screenBitMap - the screen's bitmap object (HIDD_BM_OBJ of the screen bitmap)
        request      - the surface description, owned by the caller
        hook         - notification hook, may be NULL

    RESULT
        TRUE when attached. FALSE when the display cannot compose surfaces.

    SEE ALSO
        moHidd_Display_UpdateSurface, moHidd_Display_DetachSurface, moHidd_Display_QueryScanout

*****************************************************************************************/

BOOL Display__Hidd_Display__AttachSurface(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_AttachSurface *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (!data->compositor || !msg->request || !msg->screenBitMap)
        return FALSE;
    return HIDD_Compositor_AttachSurface(data->compositor, msg->screenBitMap, msg->request, msg->hook);
}

/*****************************************************************************************

    NAME
        moHidd_Display_UpdateSurface

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_UpdateSurface *msg);

        VOID HIDD_Display_UpdateSurface(OOP_Object *display, struct HIDD_CompositeRequest *request);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Tells the display that an attached request changed (Dest, Shape, Src,
        Flags, Opacity). The caller edits the request in place and then calls
        this; the compositor re-evaluates the surface's plane and redraws.

    INPUTS
        display - the display
        request - an attached request

*****************************************************************************************/

VOID Display__Hidd_Display__UpdateSurface(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_UpdateSurface *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (data->compositor && msg->request)
        HIDD_Compositor_UpdateSurface(data->compositor, msg->request);
}

/*****************************************************************************************

    NAME
        moHidd_Display_DetachSurface

    SYNOPSIS
        VOID OOP_DoMethod(OOP_Object *obj, struct pHidd_Display_DetachSurface *msg);

        VOID HIDD_Display_DetachSurface(OOP_Object *display, struct HIDD_CompositeRequest *request);

    LOCATION
        hidd.gfx.display

    FUNCTION
        Removes an attached surface. When it returns the plane (if any) is off,
        the area is redrawn and the hook is not called again; the request and
        the bitmap may then be freed.

    INPUTS
        display - the display
        request - an attached request

*****************************************************************************************/

VOID Display__Hidd_Display__DetachSurface(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_DetachSurface *msg)
{
    struct HIDDDisplayData *data = OOP_INST_DATA(cl, o);

    if (data->compositor && msg->request)
        HIDD_Compositor_DetachSurface(data->compositor, msg->request);
}
