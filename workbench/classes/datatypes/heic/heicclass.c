/*
    Copyright © 2020-2023, The AROS Development Team. All rights reserved.
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

#include "libheif/heif.h"
#include "libheif/heif_version.h"
#include "libheif/heif_plugin.h"

ADD2LIBS("SYS:Classes/datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**************************************************************************************************/

static LONG HEIC_Decode (Class *cl, Object *o, BPTR file, struct BitMapHeader *bmh)
{
    LONG status, error = (0);
    UBYTE *buffer, *output;
    struct FileInfoBlock *fib;
    ULONG size = 0;
    LONG width, height;
    LONG level = 0;
    enum heif_filetype_result heifres;

    D(bug("[heic.datatype] %s()\n", __func__));

    fib = AllocDosObject(DOS_FIB, NULL);
    ExamineFH(file, fib);
    size = fib->fib_Size;
    buffer = AllocVec(size+1, MEMF_PRIVATE);
    FreeDosObject(DOS_FIB, fib);

    if (buffer != NULL)
    {
        ULONG headersize = 12;
        if (size < headersize)
                headersize = 12;

        Seek(file, 0, OFFSET_BEGINNING);

        if (Read(file, buffer, headersize) != -1)
        {
            heifres = heif_check_filetype(buffer, headersize);
            if ((size > 20) && ((heifres == heif_filetype_yes_supported) || (heifres == heif_filetype_maybe)))
            {
                headersize = 20;

                Seek(file, 0, OFFSET_BEGINNING);
                if (Read(file, buffer, headersize) != -1)
                {
                    const char *mime_type = heif_get_file_mime_type(buffer, headersize);
                    if (mime_type != 0)
                    {
                        struct heif_context *heifc;

                        D(bug("[heic.datatype] %s: HEIC MIME type = %s\n", __func__, mime_type));

                        heifc = heif_context_alloc();
                        D(bug("[heic.datatype] %s: heif_context allocated @ 0x%p\n", __func__, heifc));

                        Seek(file, 0, OFFSET_BEGINNING);
                        if (Read(file, buffer, size) != -1)
                        {
                            ULONG mode = PBPAFMT_RGB;
                            int hmode = heif_chroma_interleaved_RGB;
                            struct heif_error err;

                            err = heif_context_read_from_memory_without_copy(heifc, buffer, size, NULL);
                            if (err.code)
                            {
                                    D(bug("[heic.datatype] %s: failed to get context (%s)\n", __func__, err.message));
                            }
                            
                            int numImages = heif_context_get_number_of_top_level_images(heifc);
                            D(bug("[heic.datatype] %s: file contains %u images\n", __func__, numImages));
                            if (numImages >= 1)
                            {
                                struct heif_image_handle *handle;
                                struct heif_error err;

                                /* we only want the first image for now .. */
                                heif_item_id IDs[1];
                                heif_context_get_list_of_top_level_image_IDs(heifc, IDs, 1); //numImages);


                                err = heif_context_get_image_handle(heifc, IDs[0], &handle);
                                if (err.code)
                                {
                                        D(bug("[heic.datatype] %s: failed to get image handle (%s)\n", __func__, err.message));
                                }

                                D(bug("[heic.datatype] %s: handle = 0x%p\n", __func__, handle));
                                                                heif_image_handle_is_primary_image(handle);

                                bmh->bmh_Width = heif_image_handle_get_width(handle);
                                bmh->bmh_Height = heif_image_handle_get_height(handle);

                                D(bug("[heic.datatype] %s: base dimensions %ux%u\n", __func__, bmh->bmh_Width, bmh->bmh_Height));

                                if (heif_image_handle_has_alpha_channel(handle))
                                {
                                                mode = PBPAFMT_RGBA;
                                                hmode = heif_chroma_interleaved_RGBA;
                                }

                                struct heif_image* image = NULL;
                                struct heif_decoding_options* options = heif_decoding_options_alloc();

                                D(bug("[heic.datatype] %s: decoding options @ 0x%p\n", __func__, options));
                                err = heif_decode_image(handle, &image, heif_colorspace_RGB , hmode, options);

                                heif_decoding_options_free(options);

                                if (err.code)
                                {
                                        D(bug("[heic.datatype] %s: failed to decode image data (%s)\n", __func__, err.message));
                                }
                                else
                                {
                                    int stride = 0;
                                    const UBYTE *p;

                                    D(bug("[heic.datatype] %s: image decoded @ 0x%p\n", __func__, image));

                                    bmh->bmh_Width = (size_t) heif_image_get_width(image, heif_channel_interleaved);
                                    bmh->bmh_Height = (size_t) heif_image_get_height(image, heif_channel_interleaved);
                                    bmh->bmh_Depth = (size_t) heif_image_get_bits_per_pixel(image, heif_channel_interleaved);

                                    D(bug("[heic.datatype] %s: image dimensions %ux%ux%u\n", __func__, bmh->bmh_Width, bmh->bmh_Height, bmh->bmh_Depth));

                                    p = heif_image_get_plane_readonly(image, heif_channel_interleaved, &stride);

				    D(bug("[heic.datatype] %s: data @ 0x%p, stride %u\n", __func__, p, stride);)

                                    SetDTAttrs(o, NULL, NULL,
                                            DTA_NominalHoriz,	bmh->bmh_Width,
                                            DTA_NominalVert,	bmh->bmh_Height,
                                            PDTA_SourceMode,	PMODE_V43,
                                            TAG_END);

                                    DoSuperMethod(cl, o,
                                            PDTM_WRITEPIXELARRAY, p, mode,
                                            stride, 0, 0, bmh->bmh_Width, bmh->bmh_Height);

                                    heif_image_release(image);
                                }
                                heif_image_handle_release(handle);
                            }
                            else
                            {
                                D(bug("[heic.datatype] %s: no images???\n", __func__));
                                error = DTERROR_INVALID_DATA;
                            }
                        }
                        else
                        {
                            D(bug("[heic.datatype] %s: failed to read file data\n", __func__));
                            error = DTERROR_INVALID_DATA;
                        }
                    }
                    else
                    {
                        D(bug("[heic.datatype] %s: Failed to determine mime type\n", __func__));
                        error = DTERROR_INVALID_DATA;
                    }
                }
                else
                {
                    D(bug("[heic.datatype] %s: failed to read mime data\n", __func__));
                    error = DTERROR_INVALID_DATA;
                }
            }
            else
            {
                D(bug("[heic.datatype] %s: Unsupported/Not a HEIC file (result = %u)\n", __func__, heifres));
                error = DTERROR_INVALID_DATA;
            }
        }
        else
        {
            D(bug("[heic.datatype] %s: failed to read the file header...\n", __func__));
            error = DTERROR_COULDNT_OPEN;
        }
    }
    else
        error = ERROR_NO_FREE_STORE;

    return error;
}

LONG HEIC_Import(struct IClass *cl, Object *o, struct TagItem *tags)
{
    struct BitMapHeader *bmh = NULL;
    char *filename = (char *)GetTagData(DTA_Name, 0, tags);
    SIPTR srctype;
    LONG error = ERROR_OBJECT_NOT_FOUND;
    BPTR file = BNULL;

    D(bug("[heic.datatype] %s('%s')\n", __func__, filename));

    GetDTAttrs(o,
            PDTA_BitMapHeader,	&bmh,
            (filename) ? DTA_Handle : TAG_IGNORE,
                &file,
            DTA_SourceType,		&srctype,
            TAG_END);

    D(
        bug("[heic.datatype] %s: PDTA_BitMapHeader @ 0x%p\n", __func__, bmh);
        bug("[heic.datatype] %s: DTA_SourceType = %08x\n", __func__, srctype);
        bug("[heic.datatype] %s: DTA_Handle = %p\n", __func__, file);
     )

    if (bmh && file && srctype == DTST_FILE)
    {
        D(bug("[heic.datatype] %s: attempt to decode file @ 0x%p ...\n", __func__, file));

        error =  HEIC_Decode(cl, o, file, bmh);
        if (error == (0))
        {
            SetDTAttrs(o, NULL, NULL,
                    DTA_ObjName,FilePart(filename),
                    TAG_END);
        }
    }

    if(srctype == DTST_RAM)
        error = (0);

    return error;
}

/**************************************************************************************************/

IPTR HEIF__OM_NEW(struct IClass *cl, Object *o, struct opSet *msg)
{
    D(bug("[heic.datatype] %s()\n", __func__));

    IPTR retval = DoSuperMethodA(cl, o, msg);
    LONG error;

    if (retval) {

        D(bug("[heic.datatype] %s: object @ 0x%p\n", __func__, retval));

        error =  HEIC_Import(cl, (Object *)retval, msg->ops_AttrList);
        if (error != (0)) {
            D(bug("[heic.datatype] %s: disposing...\n", __func__));

            CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
            SetIoErr(error);

            retval = (IPTR)NULL;
        }
    }
    return retval;
}

/**************************************************************************************************/

IPTR HEIF__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    IPTR retval = FALSE;
    LONG error;

    D(bug("[heic.datatype] %s()\n", __func__));

    if (dtw->dtw_Mode == DTWM_RAW) {
        D(bug("[heic.datatype] %s: exporting in heic format...\n", __func__));

    }
    else
        retval = DoSuperMethodA( cl, o, (Msg)dtw );

    return retval;
}