/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Display class for the AMDGPU hidd: bitmap creation and the
          hardware pointer.
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <oop/oop.h>

#include <string.h>

#include "amdgpu_intern.h"

#include "amdgpu_bitmap.h"
#include LC_LIBDEFS_FILE

/* Overlay planes are only reported when screens can be scanned out from them */
#define OVERLAYS_REPORTED(data) (AMDGPU_PLANE_SCREENS ? (data)->kms.nplanes : 0)

static OOP_AttrBase             HiddBitMapAttrBase;
static OOP_AttrBase             HiddDisplayAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    {IID_Hidd_BitMap,           &HiddBitMapAttrBase             },
    {IID_Hidd_Display,          &HiddDisplayAttrBase            },
    {NULL,                      NULL                            }
};

VOID AmdgpuDisplay__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;

    Hidd_Display_Switch(msg->attrID, idx)
    {
    case aoHidd_Display_SupportsGamma:
        *msg->storage = (IPTR)TRUE;
        found = TRUE;
        break;
    case aoHidd_Display_PlaneCount:
        /* primary + overlays + the hardware cursor */
        *msg->storage = 1 + OVERLAYS_REPORTED(XSD(cl)) + (XSD(cl)->hwCursor ? 1 : 0);
        found = TRUE;
        break;
    case aoHidd_Display_VBlankCount:
        *msg->storage = Amdgpu_KMS_VBlankCount(&XSD(cl)->kms);
        found = TRUE;
        break;
    }

    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *AmdgpuDisplay__Hidd_Display__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    D(bug("[Amdgpu:Display] %s()\n", __func__);)

    if (msg->cl == XSD(cl)->basebm)
    {
        BOOL displayable;
        BOOL framebuffer;
        OOP_Class *classptr = NULL;
        struct TagItem tags[] =
        {
            { TAG_IGNORE, TAG_IGNORE }, /* Placeholder for aHidd_BitMap_ClassPtr */
            { TAG_MORE, (IPTR)msg->attrList }
        };

        struct pHidd_Display_CreateObject comsg;

        displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
        framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
        if (framebuffer)
            classptr = XSD(cl)->amdgpuonbmclass;
        else if (displayable)
            classptr = XSD(cl)->amdgpuoffbmclass;
        else
        {
            HIDDT_ModeID modeid;
            modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
            if (modeid != vHidd_ModeID_Invalid)
                classptr = XSD(cl)->amdgpuoffbmclass;
            else
            {
                HIDDT_StdPixFmt stdpf;
                stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
                if (stdpf == vHidd_StdPixFmt_Unknown)
                {
                    OOP_Object *friend;
                    friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, (IPTR)NULL, msg->attrList);
                    if (friend != NULL)
                    {
                        if (OOP_OCLASS(friend) == XSD(cl)->amdgpuonbmclass)
                        {
                            classptr = XSD(cl)->amdgpuoffbmclass;
                        }
                    }
                }
            }
        }
        if (classptr != NULL)
        {
            tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
            tags[0].ti_Data = (IPTR)classptr;
        }
        comsg.mID = msg->mID;
        comsg.cl = msg->cl;
        comsg.attrList = tags;

        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&comsg);
    }
    else if ((XSD(cl)->basegallium && (msg->cl == XSD(cl)->basegallium)) &&
                XSD(cl)->kms.has3d)
    {
        /* Create the gallium 3d driver object .. */
        object = OOP_NewObject(NULL, CLID_Hidd_Gallium_Amdgpu, msg->attrList);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    D(bug("[Amdgpu:Display] %s: returning 0x%p\n", __func__, object);)
    return object;
}

BOOL AmdgpuDisplay__Hidd_Display__SetGamma(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetGamma *msg)
{
    D(bug("[Amdgpu:Display] %s()\n", __func__);)
    return TRUE;
}

BOOL AmdgpuDisplay__Hidd_Display__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorShape *msg)
{
    struct Amdgpu_staticdata *data = XSD(cl);

    D(bug("[Amdgpu:Display] %s()\n", __func__);)

    /* Without a usable hardware cursor the base class renders one for us */
    if (!data->hwCursor)
        return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (msg->shape == NULL)
    {
        D(bug("[Amdgpu:Display] %s: blanking cursor\n", __func__);)
        Amdgpu_KMS_ShowCursor(&data->kms, FALSE);
        data->mouse.oopshape = NULL;
        FreeVec(data->mouse.shape);
        data->mouse.shape = NULL;

        return TRUE;
    }
    else
    {
        IPTR tmp;

        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &tmp);
        data->mouse.width = tmp;
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &tmp);
        data->mouse.height = tmp;

        /* convert shape to what the device wants: 32bit ARGB, little endian */
        FreeVec(data->mouse.shape);
        tmp = data->mouse.width * data->mouse.height;
        data->mouse.shape = AllocVec(tmp << 2, MEMF_CLEAR|MEMF_PUBLIC);
        if (data->mouse.shape != NULL)
        {
            data->mouse.oopshape = msg->shape;
            HIDD_BM_GetImage(msg->shape, (UBYTE *)data->mouse.shape, data->mouse.width * 4, 0, 0, data->mouse.width, data->mouse.height, vHidd_StdPixFmt_BGRA32);

            if (!Amdgpu_KMS_SetCursorShape(&data->kms, data->mouse.shape, data->mouse.width, data->mouse.height))
            {
                /* the device would not take it; from now on the pointer is drawn in software */
                data->hwCursor = FALSE;
                return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            }
            return TRUE;
        }
    }

    return FALSE;
}

BOOL AmdgpuDisplay__Hidd_Display__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorPos *msg)
{
    D(bug("[Amdgpu:Display] %s()\n", __func__);)

    if (!XSD(cl)->hwCursor)
        return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    XSD(cl)->mouse.x = msg->x;
    XSD(cl)->mouse.y = msg->y;

    if ((XSD(cl)->visible))
        Amdgpu_KMS_MoveCursor(&XSD(cl)->kms, msg->x, msg->y);

    return TRUE;
}

VOID AmdgpuDisplay__Hidd_Display__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorVisible *msg)
{
    D(bug("[Amdgpu:Display] %s()\n", __func__);)

    if (!XSD(cl)->hwCursor)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }

    XSD(cl)->mouse.visible = msg->visible;
    if ((XSD(cl)->visible))
    {
        Amdgpu_KMS_ShowCursor(&XSD(cl)->kms, msg->visible ? TRUE : FALSE);
        if (msg->visible)
            Amdgpu_KMS_MoveCursor(&XSD(cl)->kms, XSD(cl)->mouse.x, XSD(cl)->mouse.y);
    }
}

/* --- scan-out planes ---------------------------------------------------------- */

/* vHidd_StdPixFmt_* bits for the DRM formats a plane takes (memory byte order) */
static ULONG AmdgpuDisplay_PlanePixFmts(ULONG fmts)
{
    ULONG m = 0;

    if (fmts & KMSFMT_XRGB8888) m |= (1UL << vHidd_StdPixFmt_BGR032) | (1UL << vHidd_StdPixFmt_BGRA32);
    if (fmts & KMSFMT_ARGB8888) m |= (1UL << vHidd_StdPixFmt_BGRA32);
    if (fmts & KMSFMT_XBGR8888) m |= (1UL << vHidd_StdPixFmt_RGB032) | (1UL << vHidd_StdPixFmt_RGBA32);
    if (fmts & KMSFMT_ABGR8888) m |= (1UL << vHidd_StdPixFmt_RGBA32);
    if (fmts & KMSFMT_RGB565)   m |= (1UL << vHidd_StdPixFmt_RGB16_LE);
    if (fmts & KMSFMT_XRGB1555) m |= (1UL << vHidd_StdPixFmt_RGB15_LE);
    return m;
}

/* The KMS framebuffer behind one of our bitmaps, 0 when it has none */
static ULONG AmdgpuDisplay_BitMapFB(OOP_Class *cl, OOP_Object *bm)
{
    if ((OOP_OCLASS(bm) == XSD(cl)->amdgpuonbmclass) || (OOP_OCLASS(bm) == XSD(cl)->amdgpuoffbmclass))
    {
        struct BitmapData *bmd = OOP_INST_DATA(OOP_OCLASS(bm), bm);
        return bmd->fb.fbid;
    }
    return 0;
}

/*
 * Plane layout: 0 = primary (the framebuffer, driven by Show()),
 * 1 .. overlays = overlay planes of the crtc, last = the hardware cursor.
 */
ULONG AmdgpuDisplay__Hidd_Display__GetPlaneInfo(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_GetPlaneInfo *msg)
{
    struct Amdgpu_staticdata *data = XSD(cl);
    ULONG overlays = OVERLAYS_REPORTED(data);
    struct HIDD_PlaneInfo info;
    ULONG len = msg->infoLen;

    if (msg->index == 0)
        return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    memset(&info, 0, sizeof(info));
    info.ZMin = info.ZMax = msg->index;
    info.ScaleMin = info.ScaleMax = HIDD_PLANE_SCALE_UNITY;
    info.PosAlignX = info.PosAlignY = 1;
    if (msg->index <= overlays)
    {
        info.Type       = vHidd_Plane_Overlay;
        info.PixFmtMask = AmdgpuDisplay_PlanePixFmts(data->kms.planes[msg->index - 1].formats);
        info.AlphaModes = (1 << vHidd_CompAlpha_None);
        info.Flags      = PIF_NEEDS_DISPLAYMEM;     /* only our dumb buffers can be scanned out */
    }
    else if ((msg->index == overlays + 1) && data->hwCursor)
    {
        info.Type       = vHidd_Plane_Cursor;
        info.PixFmtMask = 0xFFFFFFFF;               /* converted to ARGB on the way */
        info.AlphaModes = (1 << vHidd_CompAlpha_PerPixel);
        info.MaxWidth   = AMDGPU_CURSOR_SIZE;
        info.MaxHeight  = AMDGPU_CURSOR_SIZE;
    }
    else
        return 0;

    if (len > sizeof(info))
        len = sizeof(info);
    CopyMem(&info, msg->info, len);
    return len;
}

BOOL AmdgpuDisplay__Hidd_Display__CommitPlanes(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CommitPlanes *msg)
{
    struct Amdgpu_staticdata *data = XSD(cl);
    ULONG overlays = OVERLAYS_REPORTED(data);
    ULONG cursorplane = data->hwCursor ? overlays + 1 : 0;
    struct HIDD_PlaneState *primary = NULL;
    ULONG i;

    /* Validate first: a commit is all or nothing */
    for (i = 0; i < msg->count; i++)
    {
        struct HIDD_PlaneState *st = &msg->states[i];

        if (st->Plane == 0)
        {
            primary = st;
            continue;
        }
        if ((st->Plane > overlays) && (st->Plane != cursorplane))
            return FALSE;
        if (st->BitMap && (st->Plane <= overlays))
        {
            if (!AmdgpuDisplay_BitMapFB(cl, st->BitMap))
                return FALSE;                       /* not a scan-out buffer */
            if (((st->Dest.MaxX - st->Dest.MinX) != (st->Src.MaxX - st->Src.MinX)) ||
                ((st->Dest.MaxY - st->Dest.MinY) != (st->Src.MaxY - st->Src.MinY)))
                return FALSE;                       /* no scaling */
            if (st->AlphaMode != vHidd_CompAlpha_None)
                return FALSE;
        }
    }

    if (primary)
    {
        struct pHidd_Display_CommitPlanes p = { msg->mID, primary, 1, msg->flags };

        if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)&p))
            return FALSE;
    }
    if (msg->flags & fHidd_Commit_Test)
        return TRUE;

    for (i = 0; i < msg->count; i++)
    {
        struct HIDD_PlaneState *st = &msg->states[i];

        if (st->Plane == 0)
            continue;
        if (st->Plane == cursorplane)
        {
            if (st->BitMap)
            {
                struct pHidd_Display_SetCursorShape shape = { OOP_GetMethodID(IID_Hidd_Display, moHidd_Display_SetCursorShape), st->BitMap, 0, 0 };
                struct pHidd_Display_SetCursorPos   pos   = { OOP_GetMethodID(IID_Hidd_Display, moHidd_Display_SetCursorPos), st->Dest.MinX, st->Dest.MinY };
                struct pHidd_Display_SetCursorVisible vis = { OOP_GetMethodID(IID_Hidd_Display, moHidd_Display_SetCursorVisible), TRUE };

                OOP_DoMethod(o, (OOP_Msg)&shape);
                OOP_DoMethod(o, (OOP_Msg)&pos);
                OOP_DoMethod(o, (OOP_Msg)&vis);
            }
            else
            {
                struct pHidd_Display_SetCursorVisible vis = { OOP_GetMethodID(IID_Hidd_Display, moHidd_Display_SetCursorVisible), FALSE };

                OOP_DoMethod(o, (OOP_Msg)&vis);
            }
            continue;
        }
        /* Overlay */
        if (st->BitMap)
        {
            if (!Amdgpu_KMS_SetPlane(&data->kms, st->Plane - 1, AmdgpuDisplay_BitMapFB(cl, st->BitMap),
                                       st->Dest.MinX, st->Dest.MinY,
                                       st->Dest.MaxX - st->Dest.MinX + 1, st->Dest.MaxY - st->Dest.MinY + 1,
                                       st->Src.MinX, st->Src.MinY,
                                       st->Src.MaxX - st->Src.MinX + 1, st->Src.MaxY - st->Src.MinY + 1))
                return FALSE;
        }
        else
            Amdgpu_KMS_SetPlane(&data->kms, st->Plane - 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    if (!(msg->flags & fHidd_Commit_NoWait))
        Amdgpu_KMS_WaitVBlank(&data->kms);
    return TRUE;
}

VOID AmdgpuDisplay__Hidd_Display__WaitLatch(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_WaitLatch *msg)
{
    Amdgpu_KMS_WaitVBlank(&XSD(cl)->kms);
}

VOID AmdgpuDisplay__Hidd_Display__NominalDimensions(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_NominalDimensions *msg)
{
    if (msg->width)
        *(msg->width) = 1024;
    if (msg->height)
        *(msg->height) = 768;
    if (msg->depth)
        *(msg->depth) = 24;
}

static int AmdgpuDisplay_InitStatic(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Amdgpu:Display] %s()\n", __func__);)

    LIBBASE->vsd.mouse.x = 0;
    LIBBASE->vsd.mouse.y = 0;
    LIBBASE->vsd.mouse.shape = NULL;

    return OOP_ObtainAttrBases(attrbases);
}

static int AmdgpuDisplay_ExpungeStatic(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[Amdgpu:Display] %s()\n", __func__);)

    OOP_ReleaseAttrBases(attrbases);

    return TRUE;
}

ADD2INITLIB(AmdgpuDisplay_InitStatic, 0)
ADD2EXPUNGELIB(AmdgpuDisplay_ExpungeStatic, 0)
