/*
    Copyright (C) 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gfx_debug.h"

#include <proto/utility.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "gfx_intern.h"

OOP_Object *GFXHist__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *UtilityBase = CSD(cl)->cs_UtilityBase;
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    OOP_Object *bmObj;
    struct TagItem *tag;

    D(bug("[Gfx:BMHist] %s()\n", __func__);)

    tag = FindTagItem(aHidd_BMHistogram_BitMap, msg->attrList);
    if ((tag) && ((bmObj = (OOP_Object *)tag->ti_Data)))
    {
        D(bug("[Gfx:BMHist] %s: bmobject @ 0x%p\n", __func__, bmObj);)
        o  = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

        if (NULL != o)
        {
            struct HiddBMHistogramData *data = OOP_INST_DATA(cl, o);
            struct BMHistEntry *colentry;
            register HIDDT_Pixel pix;
            HIDDT_Color         pixcol;
            IPTR width, height;
            int x,y;

            InitSemaphore(&data->sema);

            D(bug("[Gfx:BMHist] %s: object @ 0x%p\n", __func__, o);)

            OOP_GetAttr(bmObj, aHidd_BitMap_Width, &width);
            OOP_GetAttr(bmObj, aHidd_BitMap_Height, &height);

            D(bug("[Gfx:BMHist] %s: bm size = %dx%d\n", __func__, width, height);)

            data->hpool = CreatePool(MEMF_ANY, ((sizeof(struct BMHistEntry) * width) * height), sizeof(struct BMHistEntry) * width);
            D(bug("[Gfx:BMHist] %s: entry pool @ 0x%p\n", __func__, data->hpool);)

#if USE_FAST_BMHGETPIXEL
            data->getpixel = OOP_GetMethod(bmObj, HiddBitMapBase + moHidd_BitMap_GetPixel, &data->getpixel_Class);
#endif
#if USE_FAST_BMHUNMAPPIXEL
            data->unmappixel = OOP_GetMethod(bmObj, HiddBitMapBase + moHidd_BitMap_PutPixel, &data->unmappixel_Class);
#endif
#if USE_FAST_BMHFINDCOLOR
            data->findcolorentry = OOP_GetMethod(o, HiddBMHistogramBase + moHidd_BitMap_PutPixel, &data->findcolorentry_Class);
#endif
#if USE_FAST_BMHADDCOLOR
            data->addcolorentry = OOP_GetMethod(o, HiddBMHistogramBase + moHidd_BitMap_PutPixel, &data->addcolorentry_Class);
#endif

            /* create the histogram data */
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x++)
                {
                    pix = BMHGETPIXEL(OOP_OCLASS(bmObj), bmObj, x, y);
                    BMHUNMAPPIXEL(OOP_OCLASS(bmObj), bmObj, pix, &pixcol);

                    if (BMHFINDCOLOR(cl, o, &pixcol, (APTR *)&colentry))
                    {
                        colentry->count++;
                    }
                    else
                    {
                        BMHADDCOLOR(cl, o, &pixcol, 1);
                    }
                }
            }
        }
        return  o;
    }
    return  NULL;
}

VOID GFXHist__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HiddBMHistogramData *data = OOP_INST_DATA(cl, o);
    struct BMHistEntry *colentry, *tmp;

    D(bug("[Gfx:BMHist] %s()\n", __func__);)

    if (data->hpool)
    {
        ObtainSemaphore(&data->sema);
        colentry = data->hist;
        while (colentry)
        {
            tmp = colentry->next;
            FreePooled(data->hpool, colentry, sizeof(struct BMHistEntry));
            colentry = tmp;
        }
        ReleaseSemaphore(&data->sema);
        DeletePool(data->hpool);
        data->hpool = NULL;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
}

VOID GFXHist__Root__Get(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[Gfx:BMHist] %s()\n", __func__);)
    OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
}

APTR GFXHist__Hidd_BMHistogram__AddColorEntry(OOP_Class *cl, OOP_Object *o, struct pHidd_BMHistogram_AddColorEntry *msg)
{
    struct HiddBMHistogramData *data = OOP_INST_DATA(cl, o);
    struct BMHistEntry *colentry;

    D(bug("[Gfx:BMHist] %s()\n", __func__);)

    ObtainSemaphore(&data->sema);
    colentry = AllocPooled(data->hpool, sizeof(struct BMHistEntry));
    if (colentry)
    {
        data->size++;

        D(bug("[Gfx:BMHist] %s: new entry #%d @ 0x%p\n", __func__, data->size, colentry);)

        CopyMem(msg->color, &colentry->color, sizeof(HIDDT_Color));
        colentry->count = msg->count;
        colentry->next = data->hist;
        data->hist = colentry;
    }
    ReleaseSemaphore(&data->sema);

    return colentry;
}

IPTR GFXHist__Hidd_BMHistogram__FindColorEntry(OOP_Class *cl, OOP_Object *o, struct pHidd_BMHistogram_FindColorEntry *msg)
{
    struct HiddBMHistogramData *data = OOP_INST_DATA(cl, o);
    struct BMHistEntry *colentry;

    D(bug("[Gfx:BMHist] %s()\n", __func__);)

    ObtainSemaphoreShared(&data->sema);
    /* handle the special case first ... */
    if (msg->color == (APTR)vBMHistogram_NextEntry)
    {
        if (*(APTR *)msg->colentry)
        {
            colentry = *(struct BMHistEntry **)msg->colentry;
            if ((*(APTR *)msg->colentry = colentry->next) == NULL)
            {
                ReleaseSemaphore(&data->sema);
                return FALSE;
            }
        }
        else
        {
            *(APTR *)msg->colentry = data->hist;
        }
        ReleaseSemaphore(&data->sema);
        return TRUE;
    }

    colentry = data->hist;
    while (colentry)
    {
        if ((colentry->color.red   == msg->color->red) &&
            (colentry->color.green == msg->color->green) &&
            (colentry->color.blue  == msg->color->blue))
        {
            *(APTR *)msg->colentry = colentry;
            ReleaseSemaphore(&data->sema);
            return TRUE;
        }
        colentry = colentry->next;
    }
    ReleaseSemaphore(&data->sema);
    return FALSE;
}

IPTR GFXHist__Hidd_BMHistogram__GetEntryColor(OOP_Class *cl, OOP_Object *o, struct pHidd_BMHistogram_GetEntryColor *msg)
{
    struct BMHistEntry *colentry;

    D(bug("[Gfx:BMHist] %s()\n", __func__);)

    if ((colentry = msg->colentry) != NULL)
    {
        CopyMem(&colentry->color, msg->color, sizeof(HIDDT_Color));
        return TRUE;
    }
    return FALSE;
}

ULONG GFXHist__Hidd_BMHistogram__GetEntryUseCount(OOP_Class *cl, OOP_Object *o, struct pHidd_BMHistogram_GetEntryUseCount *msg)
{
    struct BMHistEntry *colentry;

    D(bug("[Gfx:BMHist] %s()\n", __func__);)

    if ((colentry = msg->colentry) != NULL)
    {
        return colentry->count;
    }
    return 0;
}

VOID GFXHist__Hidd_BMHistogram__Sort(OOP_Class *cl, OOP_Object *o, struct pHidd_BMHistogram_Sort *msg)
{
    D(bug("[Gfx:BMHist] %s()\n", __func__);)

    /*
      TODO: Sort the array based on the options specified in the tag list
    */
}
