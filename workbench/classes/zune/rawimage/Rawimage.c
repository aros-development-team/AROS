/*
    Copyright © 2011, Thore Böckelmann. All rights reserved.
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/intuition.h>
#include <proto/utility.h>

#include "Rawimage_mcc.h"


/* ------------------------------------------------------------------------- */

static BOOL setRawimage(struct IClass *cl, Object *obj, struct MUI_RawimageData *rdata)
{
    BOOL success = FALSE;
    LONG format;
    LONG compression;

    ULONG cwidth  = AROS_BE2LONG(rdata->ri_Width);
    ULONG cheight = AROS_BE2LONG(rdata->ri_Height);
    ULONG cformat = AROS_BE2LONG(rdata->ri_Format);
    ULONG csize   = AROS_BE2LONG(rdata->ri_Size);

    switch(cformat)
    {
        case RAWIMAGE_FORMAT_RAW_ARGB_ID:
            format = MUIV_Pixmap_Format_ARGB32;
            compression = MUIV_Pixmap_Compression_None;
            break;

        case RAWIMAGE_FORMAT_RAW_RGB_ID:
            format = MUIV_Pixmap_Format_RGB24;
            compression = MUIV_Pixmap_Compression_None;
            break;

        case RAWIMAGE_FORMAT_BZ2_ARGB_ID:
            format = MUIV_Pixmap_Format_ARGB32;
            compression = MUIV_Pixmap_Compression_BZip2;
            break;

        case RAWIMAGE_FORMAT_BZ2_RGB_ID:
            format = MUIV_Pixmap_Format_RGB24;
            compression = MUIV_Pixmap_Compression_BZip2;
            break;

        case RAWIMAGE_FORMAT_RLE_ARGB_ID:
            format = MUIV_Pixmap_Format_ARGB32;
            compression = MUIV_Pixmap_Compression_RLE;
            break;

        case RAWIMAGE_FORMAT_RLE_RGB_ID:
            format = MUIV_Pixmap_Format_RGB24;
            compression = MUIV_Pixmap_Compression_RLE;
            break;

        default:
            format = -1;
            compression = MUIV_Pixmap_Compression_None;
            break;
    }

    if(format != -1)
    {
        // Pixmap.mui will return failure in case the compression is not supported
        success = SetSuperAttrs(cl, obj, MUIA_FixWidth, cwidth,
                                         MUIA_FixHeight, cheight,
                                         MUIA_Pixmap_Width, cwidth,
                                         MUIA_Pixmap_Height, cheight,
                                         MUIA_Pixmap_Format, format,
                                         MUIA_Pixmap_Data, rdata->ri_Data,
                                         MUIA_Pixmap_Compression, compression,
                                         MUIA_Pixmap_CompressedSize, csize,
                                         TAG_DONE);
    }

    return success;
}

IPTR Rawimage__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    if((obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        TAG_MORE, msg->ops_AttrList)) != NULL)
    {
        struct MUI_RawimageData *rdata;

        if((rdata = (struct MUI_RawimageData *)GetTagData(MUIA_Rawimage_Data, 0, msg->ops_AttrList)) != NULL)
        {
            if(setRawimage(cl, obj, rdata) == FALSE)
            {
                CoerceMethod(cl, obj, OM_DISPOSE);
                obj = NULL;
            }
        }
    }

    return (IPTR)obj;
}


IPTR Rawimage__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_RawimageData *rdata;

    if((rdata = (struct MUI_RawimageData *)GetTagData(MUIA_Rawimage_Data, 0, msg->ops_AttrList)) != NULL)
    {
        setRawimage(cl, obj, rdata);
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}
