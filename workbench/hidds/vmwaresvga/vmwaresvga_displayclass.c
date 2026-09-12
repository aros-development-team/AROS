/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Display class for the VMWare SVGA hidd: bitmap creation and the
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

#include "vmwaresvga_intern.h"

#include LC_LIBDEFS_FILE

static OOP_AttrBase             HiddBitMapAttrBase;
static OOP_AttrBase             HiddDisplayAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    {IID_Hidd_BitMap,           &HiddBitMapAttrBase             },
    {IID_Hidd_Display,          &HiddDisplayAttrBase            },
    {NULL,                      NULL                            }
};

VOID VMWareSVGADisplay__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;

    Hidd_Display_Switch(msg->attrID, idx)
    {
    case aoHidd_Display_SupportsGamma:
        *msg->storage = (IPTR)TRUE;
        found = TRUE;
        break;
    }

    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *VMWareSVGADisplay__Hidd_Display__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CreateObject *msg)
{
    OOP_Object      *object = NULL;

    D(bug("[VMWareSVGA:Display] %s()\n", __func__);)

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
            classptr = XSD(cl)->vmwaresvgaonbmclass;
        else if (displayable)
            classptr = XSD(cl)->vmwaresvgaoffbmclass;
        else
        {
            HIDDT_ModeID modeid;
            modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
            if (modeid != vHidd_ModeID_Invalid)
                classptr = XSD(cl)->vmwaresvgaoffbmclass;
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
                        if (OOP_OCLASS(friend) == XSD(cl)->vmwaresvgaonbmclass)
                        {
                            classptr = XSD(cl)->vmwaresvgaoffbmclass;
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
        object = OOP_NewObject(NULL, CLID_Hidd_Gallium_VMWareSVGA, msg->attrList);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    D(bug("[VMWareSVGA:Display] %s: returning 0x%p\n", __func__, object);)
    return object;
}

BOOL VMWareSVGADisplay__Hidd_Display__SetGamma(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetGamma *msg)
{
    D(bug("[VMWareSVGA:Display] %s()\n", __func__);)
    return TRUE;
}

BOOL VMWareSVGADisplay__Hidd_Display__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorShape *msg)
{
    struct VMWareSVGA_staticdata *data = XSD(cl);

    D(bug("[VMWareSVGA:Display] %s()\n", __func__);)

    /* Without a usable hardware cursor the base class renders one for us */
    if (!data->hwCursor)
        return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (msg->shape == NULL)
    {
        D(bug("[VMWareSVGA:Display] %s: blanking cursor\n", __func__);)
        VMWareSVGA_KMS_ShowCursor(&data->kms, FALSE);
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

            if (!VMWareSVGA_KMS_SetCursorShape(&data->kms, data->mouse.shape, data->mouse.width, data->mouse.height))
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

BOOL VMWareSVGADisplay__Hidd_Display__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorPos *msg)
{
    D(bug("[VMWareSVGA:Display] %s()\n", __func__);)

    if (!XSD(cl)->hwCursor)
        return (BOOL)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    XSD(cl)->mouse.x = msg->x;
    XSD(cl)->mouse.y = msg->y;

    if ((XSD(cl)->visible))
        VMWareSVGA_KMS_MoveCursor(&XSD(cl)->kms, msg->x, msg->y);

    return TRUE;
}

VOID VMWareSVGADisplay__Hidd_Display__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_SetCursorVisible *msg)
{
    D(bug("[VMWareSVGA:Display] %s()\n", __func__);)

    if (!XSD(cl)->hwCursor)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }

    XSD(cl)->mouse.visible = msg->visible;
    if ((XSD(cl)->visible))
    {
        VMWareSVGA_KMS_ShowCursor(&XSD(cl)->kms, msg->visible ? TRUE : FALSE);
        if (msg->visible)
            VMWareSVGA_KMS_MoveCursor(&XSD(cl)->kms, XSD(cl)->mouse.x, XSD(cl)->mouse.y);
    }
}

VOID VMWareSVGADisplay__Hidd_Display__NominalDimensions(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_NominalDimensions *msg)
{
    if (msg->width)
        *(msg->width) = 1024;
    if (msg->height)
        *(msg->height) = 768;
    if (msg->depth)
        *(msg->depth) = 24;
}

static int VMWareSVGADisplay_InitStatic(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[VMWareSVGA:Display] %s()\n", __func__);)

    LIBBASE->vsd.mouse.x = 0;
    LIBBASE->vsd.mouse.y = 0;
    LIBBASE->vsd.mouse.shape = NULL;

    return OOP_ObtainAttrBases(attrbases);
}

static int VMWareSVGADisplay_ExpungeStatic(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[VMWareSVGA:Display] %s()\n", __func__);)

    OOP_ReleaseAttrBases(attrbases);

    return TRUE;
}

ADD2INITLIB(VMWareSVGADisplay_InitStatic, 0)
ADD2EXPUNGELIB(VMWareSVGADisplay_ExpungeStatic, 0)
