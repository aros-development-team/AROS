/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

/**************************************************************************************************/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/datatypes.h>
#include <proto/graphics.h>

#include <exec/exec.h>
#include <dos/dos.h>
#include <utility/utility.h>
#include <datatypes/pictureclass.h>

#include <stdlib.h>

#include "src/webp/encode.h"
#include "src/webp/decode.h"

#define RGB8to32(RGB) (((ULONG)(RGB) << 24)|((ULONG)(RGB) << 16)|((ULONG)(RGB) << 8)|((ULONG)(RGB)))

ADD2LIBS("SYS:Classes/datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**************************************************************************************************/

static LONG WebP_Export (Class *cl, Object *o, struct dtWrite *msg)
{
    struct BitMapHeader *bmh = NULL;
    BPTR fh = msg->dtw_FileHandle;
    ULONG width, height, depth;
    UBYTE *buffer, *output;
    size_t size = 0;

    D(bug("[webp.datatype] %s()\n", __func__));

    GetDTAttrs(o,
            PDTA_BitMapHeader,	&bmh,
            TAG_END);	

    if (bmh)
    {
        width = bmh->bmh_Width;
        height = bmh->bmh_Height;
        depth = bmh->bmh_Depth;

        if ((buffer = AllocVec(width * height * 4, MEMF_PRIVATE)) != NULL)
        {
            DoSuperMethod(cl, o,
                PDTM_READPIXELARRAY,
                buffer, PBPAFMT_RGBA,
                width * 4, 0, 0, width, height);

            size = WebPEncodeRGBA(buffer, width, height, width * 4, 75.f, &output);

            if(size != 0)
            {
                Write(fh, output, size);

                FreeVec(buffer);
                free(output);
                return (0);
            }
        }
    }
    return DTERROR_COULDNT_SAVE;
}

/**************************************************************************************************/

static LONG WebP_Decode (Class *cl, Object *o, BPTR file, struct BitMapHeader *bmh)
{
    LONG status, error = (0);
    UBYTE *buffer, *output;
    struct FileInfoBlock *fib;
    ULONG size = 0;
    LONG width, height;
    LONG level = 0;

    D(bug("[webp.datatype] %s()\n", __func__));

    fib = AllocDosObject(DOS_FIB, NULL);
    ExamineFH(file, fib);
    size = fib->fib_Size;
    buffer = AllocVec(size+1, MEMF_PRIVATE);
    FreeDosObject(DOS_FIB, fib);

    if (buffer != NULL)
    {
        Seek(file, 0, OFFSET_BEGINNING);

        if (Read(file, buffer, size) != -1)
        {
            output = WebPDecodeRGBA(buffer, size, &width, &height);

            FreeVec(buffer);

            if (output != NULL)
            {
                bmh->bmh_Width = width;
                bmh->bmh_Height = height;
                bmh->bmh_Depth = 32;

                bmh->bmh_Masking = mskHasAlpha;

                SetDTAttrs(o, NULL, NULL,
                DTA_NominalHoriz,	bmh->bmh_Width,
                DTA_NominalVert,	bmh->bmh_Height,
                PDTA_SourceMode,	PMODE_V43,
                DTA_ErrorLevel,	&level,
                DTA_ErrorNumber,	&error,
                TAG_END);

                DoSuperMethod(cl, o,
                PDTM_WRITEPIXELARRAY, output, PBPAFMT_RGBA,
                bmh->bmh_Width * 4, 0, 0, bmh->bmh_Width, bmh->bmh_Height);

                free(output);
            }
            else
                error = DTERROR_INVALID_DATA;
        }
        else
            error = DTERROR_COULDNT_OPEN;
    }
    else
        error = ERROR_NO_FREE_STORE;
        
    return error;
}

static LONG  WebP_Import (Class *cl, Object *o, struct TagItem *tags) {
    struct BitMapHeader *bmh = NULL;
    char *filename = (char *)GetTagData(DTA_Name, 0, tags);
    SIPTR srctype;
    LONG error = ERROR_OBJECT_NOT_FOUND;
    BPTR file = BNULL;

    D(bug("[webp.datatype] %s()\n", __func__));

    GetDTAttrs(o,
            PDTA_BitMapHeader,	&bmh,
            (filename) ? DTA_Handle : TAG_IGNORE,
                &file,
            DTA_SourceType,		&srctype,
            TAG_END);

    if (bmh && file && srctype == DTST_FILE) {
        D(bug("[webp.datatype] %s: attempt to decode file @ 0x%p\n", __func__, file));
        error =  WebP_Decode(cl, o, file, bmh);
        if (error == (0)) {
            SetDTAttrs(o, NULL, NULL,
                    DTA_ObjName,FilePart(filename),
                    TAG_END);
        }
    }

    if(srctype == DTST_RAM)
        error = (0);

    return error;
}

IPTR WebP__OM_NEW(struct IClass *cl, Object *o, struct opSet *msg)
{
    D(bug("[webp.datatype] %s()\n", __func__));

    IPTR retval = DoSuperMethodA(cl, o, msg);
    LONG error;

    if (retval) {

        D(bug("[webp.datatype] %s: object @ 0x%p\n", __func__, retval));

        error =  WebP_Import(cl, (Object *)retval, msg->ops_AttrList);
        if (error != (0)) {
            D(bug("[webp.datatype] %s: disposing...\n", __func__));

            CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
            SetIoErr(error);

            retval = (IPTR)NULL;
        }
    }
    return retval;
}

/**************************************************************************************************/

IPTR WebP__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    IPTR retval = FALSE;
    LONG error;

    D(bug("[webp.datatype] %s()\n", __func__));

    if (dtw->dtw_Mode == DTWM_RAW) {
        D(bug("[webp.datatype] %s: exporting in webp format...\n", __func__));

        error = WebP_Export(cl, o, dtw);
        if (error == (0)) {
            retval = TRUE;
        } else {
            SetIoErr(error);
        }
    }
    else
        retval = DoSuperMethodA( cl, o, (Msg)dtw );
    return retval;
}