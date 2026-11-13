/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Class for VGA and compatible cards.
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/gfx.h>

#include <aros/symbolsets.h>

#include <hardware/custom.h>

#include <devices/inputevent.h>
#include <string.h>

#include LC_LIBDEFS_FILE

static AROS_INTH1(ResetHandler, struct VGAGfx_staticdata *, xsd)
{
    AROS_INTFUNC_INIT

    struct VGAGfxBitMapData *data = NULL;

/* On my machine this fills the screen with colorful vertical stripes
   instead of blanking. So for now we use software method.
        Pavel Fedin.
    vgaBlankScreen(0); */

    if (xsd->visible)
        data = OOP_INST_DATA(xsd->bmclass, xsd->visible);

    if (data)
    {
        struct Box box = {0, 0, data->width - 1, data->height - 1};
        
        vgaEraseArea(data, &box);
    }

    return 0;

    AROS_INTFUNC_EXIT
}

/*********************
**  GfxHidd::New()  **
*********************/

OOP_Object *VGAGfx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem mytags[] = {
        { aHidd_Name            , (IPTR)"vgagfx.hidd"                 },
        { aHidd_HardwareName    , (IPTR)"VGA Compatible Controller"   },
        { aHidd_ProducerName    , (IPTR)"IBM"                         },
        { TAG_MORE, (IPTR)msg->attrList }
    };
    struct pRoot_New mymsg;

    /* Do not allow to create more than one object */
    if (XSD(cl)->vgahidd)
        return NULL;

    /* Init mymsg. We have to use our own message struct because
       one should not alter the one passed to this method.
       message structs passed to a method are always read-only.
       (The user who called us might want to reuse the same msg struct
       for several calls, but that will break if some method changes the
       msg struct contents)
    */
    mymsg.mID   = msg->mID;     /* We got New() method and we are sending
                                   the same method to the superclass    */
    mymsg.attrList = mytags;
    msg = &mymsg;

    EnterFunc(bug("VGAGfx::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o) {
        struct VGAGfxDriverData *data = OOP_INST_DATA(cl, o);
        struct TagItem displaytags[] =
        {
            { aHidd_Display_GfxHidd, (IPTR)o },
            { TAG_DONE,              0       }
        };

        D(bug("[VGAGfx:Driver] %s: object @ 0x%p\n", __func__, o);)

        XSD(cl)->vgadisplay = OOP_NewObject(XSD(cl)->displayclass, NULL, displaytags);
        if (XSD(cl)->vgadisplay) {
            D(bug("[VGAGfx:Driver] %s: display @ 0x%p\n", __func__, XSD(cl)->vgadisplay);)
            XSD(cl)->vgahidd = o;

            data->ResetInterrupt.is_Node.ln_Name = cl->ClassNode.ln_Name;
            data->ResetInterrupt.is_Code = (VOID_FUNC)ResetHandler;
            data->ResetInterrupt.is_Data = XSD(cl);
            AddResetCallback(&data->ResetInterrupt);
        } else {
            OOP_MethodID dispose_mid = XSD(cl)->mid_Dispose;
            OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
            o = NULL;
        }
    }
    ReturnPtr("VGAGfx::New", OOP_Object *, o);
}

VOID VGAGfx__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct VGAGfxDriverData *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->ResetInterrupt);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    XSD(cl)->vgahidd = NULL;
}

VOID VGAGfx__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;
    if (IS_GFX_ATTR(msg->attrID, idx)) {
        switch (idx) {
             case aoHidd_Gfx_SupportsHWCursor:
             case aoHidd_Gfx_NoFrameBuffer:
                *msg->storage = (IPTR)TRUE;
                found = TRUE;
                break;

             case aoHidd_Gfx_DisplayDefault:
                *msg->storage = (IPTR)XSD(cl)->vgadisplay;
                found = TRUE;
                break;
        }
    }
    
    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        
    return;
}

/*********  GfxHidd::CopyBox()  ***************************/

VOID VGAGfx__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    ULONG mode;
    unsigned char *src = 0, *dest = 0;

    mode = GC_DRMD(msg->gc);

    EnterFunc(bug("VGAGfx.BitMap::CopyBox (%d,%d) to (%d,%d) of dim %d,%d\n",
        msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height));
    D(bug("[VGAGfx] Src: 0x%p, dest: 0x%p\n", msg->src, msg->dest));
    OOP_GetAttr(msg->src,  aHidd_VGABitMap_Drawable, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_VGABitMap_Drawable, (IPTR *)&dest);

    if (!dest || !src ||
        ((mode != vHidd_GC_DrawMode_Copy) &&
         (mode != vHidd_GC_DrawMode_And) &&
         (mode != vHidd_GC_DrawMode_Xor) &&
         (mode != vHidd_GC_DrawMode_Clear) &&
         (mode != vHidd_GC_DrawMode_Invert)))
    {
        /* The source and/or destination object is no VGA bitmap, onscreen nor offscreen.
           Or drawmode is not one of those we accelerate. Let the superclass do the
           copying in a more general way
        */
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
        
    }

    {
        struct VGAGfxBitMapData *data = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct VGAGfxBitMapData *ddata = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
        int i, width, phase, j;
        BOOL descending;

        // start of Source data
        unsigned char *s_start = data->VideoData +
                                 msg->srcX + (msg->srcY * data->bpr);
        // adder for each line
        ULONG s_add = data->bpr - msg->width;
        ULONG cnt = msg->height;

        unsigned char *d_start = ddata->VideoData +
                                 msg->destX + (msg->destY * ddata->bpr);
        ULONG d_add = ddata->bpr - msg->width;

        width = msg->width;

        if ((msg->srcY > msg->destY) || ((msg->srcY == msg->destY) && (msg->srcX >= msg->destX)))
        {
            if ((phase = ((IPTR)s_start & 3L)))
            {
                phase = 4 - phase;
                if (phase > width) phase = width;
                width -= phase;
            }
            descending = FALSE;
        }
        else
        {
            s_start += (cnt - 1) * data->bpr + width;
            d_start += (cnt - 1) * ddata->bpr + width;

            phase = ((IPTR)s_start & 3L);
            if (phase > width) phase = width;
            width -= phase;
            
            descending = TRUE;
        }

        switch(mode)
        {
            case vHidd_GC_DrawMode_Copy:
                HIDD_BM_CopyMemBox8(msg->dest,
                                    data->VideoData,
                                    msg->srcX,
                                    msg->srcY,
                                    ddata->VideoData,
                                    msg->destX,
                                    msg->destY,
                                    msg->width,
                                    msg->height,
                                    data->bpr,
                                    ddata->bpr);
                break;
                
            case vHidd_GC_DrawMode_And:
                if (!descending)
                {
                    while (cnt--)
                    {
                        i = width;
                        j = phase;
                        while (j--)
                        {
                            *d_start++ &= *s_start++;
                        }
                        while (i >= 4)
                        {
                            *((ULONG*)d_start) &= *((ULONG*)s_start);
                            d_start += 4;
                            s_start += 4;
                            i -= 4;
                        }
                        while (i--)
                        {
                            *d_start++ &= *s_start++;
                        }
                        d_start += d_add;
                        s_start += s_add;
                    }
                }
                else
                {
                    while (cnt--)
                    {
                        i = width;
                        j = phase;
                        while (j--)
                        {
                            *--d_start &= *--s_start;
                        }
                        while (i >= 4)
                        {
                            d_start -= 4;
                            s_start -= 4;
                            *((ULONG*)d_start) &= *((ULONG*)s_start);
                            i -= 4;
                        }
                        while (i--)
                        {
                            *--d_start &= *--s_start;
                        }
                        d_start -= d_add;
                        s_start -= s_add;
                    }
                }
                
                break;

            case vHidd_GC_DrawMode_Xor:
                if (!descending)
                {
                    while (cnt--)
                    {
                        i = width;
                        j = phase;
                        while (j--)
                        {
                            *d_start++ ^= *s_start++;
                        }
                        while (i >= 4)
                        {
                            *((ULONG*)d_start) ^= *((ULONG*)s_start);
                            d_start += 4;
                            s_start += 4;
                            i -= 4;
                        }
                        while (i--)
                        {
                            *d_start++ ^= *s_start++;
                        }
                        d_start += d_add;
                        s_start += s_add;
                    }
                }
                else
                {
                    while (cnt--)
                    {
                        i = width;
                        j = phase;
                        while (j--)
                        {
                            *--d_start ^= *--s_start;
                        }
                        while (i >= 4)
                        {
                            d_start -= 4;
                            s_start -= 4;
                            *((ULONG*)d_start) ^= *((ULONG*)s_start);
                            i -= 4;
                        }
                        while (i--)
                        {
                            *--d_start ^= *--s_start;
                        }
                        d_start -= d_add;
                        s_start -= s_add;
                    }
                }
                break;
                
            case vHidd_GC_DrawMode_Clear:
                if (!descending)
                {
                    while (cnt--)
                    {
                        i = width;
                        j = phase;
                        while (j--)
                        {
                            *d_start++ = 0;
                        }
                        while (i >= 4)
                        {
                            *((ULONG*)d_start) = 0;
                            d_start += 4;
                            i -= 4;
                        }
                        while (i--)
                        {
                            *d_start++ = 0;
                        }
                        d_start += d_add;
                    }
                }
                else
                {
                    while (cnt--)
                    {
                        i = width;
                        j = phase;
                        while (j--)
                        {
                            *--d_start = 0;
                        }
                        while (i >= 4)
                        {
                            d_start -= 4;
                            *((ULONG*)d_start) = 0;
                            i -= 4;
                        }
                        while (i--)
                        {
                            *--d_start = 0;
                        }
                        d_start -= d_add;
                    }
                }
                break;
                                
            case vHidd_GC_DrawMode_Invert:
                if (!descending)
                {
                    while (cnt--)
                    {
                        i = width;
                        j = phase;
                        while (j--)
                        {
                            *d_start = ~*d_start;
                            d_start++;
                        }
                        while (i >= 4)
                        {
                            *((ULONG*)d_start) = ~*((ULONG*)d_start);
                            d_start += 4;
                            i -= 4;
                        }
                        while (i--)
                        {
                            *d_start = ~*d_start;
                            d_start++;
                        }
                        d_start += d_add;
                    }
                }
                else
                {
                    while (cnt--)
                    {
                        i = width;
                        j = phase;
                        while (j--)
                        {
                            *d_start = ~*d_start;
                            d_start--;
                        }
                        while (i >= 4)
                        {
                            d_start -= 4;
                            *((ULONG*)d_start) = ~*((ULONG*)d_start);
                            i -= 4;
                        }
                        while (i--)
                        {
                            *d_start = ~*d_start;
                            d_start--;
                        }
                        d_start -= d_add;
                    }
                    break;
                }
                break;
                
        } /* switch(mode) */
    }
    ReturnVoid("VGAGfx.BitMap::CopyBox");
}
