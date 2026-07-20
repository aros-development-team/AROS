/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    $Id$
*/

/**************************************************************************************************/

//#define DEBUG 1
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

#include <jxl/decode.h>

#include "../dtio64.h"

DTIO_DOS64_SUPPORT()

ADD2LIBS("SYS:Classes/datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**************************************************************************************************/

static LONG JXL_Decode (Class *cl, Object *o, BPTR file, struct BitMapHeader *bmh)
{
    LONG error = (0);
    UBYTE *buffer = NULL, *pixels = NULL;
    QUAD size;

    D(bug("[jpegxl.datatype] %s()\n", __func__));

    size = DTIO_GetFileSize(file);
    if (DTIO_SIZE_OK(size + 1))
        buffer = AllocVec(size + 1, MEMF_PRIVATE);

    if (buffer != NULL)
    {
        Seek(file, 0, OFFSET_BEGINNING);

        if (DTIO_Read64(file, buffer, size) != -1)
        {
            JxlSignature sig = JxlSignatureCheck(buffer, (size < 12) ? (size_t)size : 12);
            if ((sig == JXL_SIG_CODESTREAM) || (sig == JXL_SIG_CONTAINER))
            {
                JxlDecoder *dec = JxlDecoderCreate(NULL);

                D(bug("[jpegxl.datatype] %s: JxlDecoder allocated @ 0x%p\n", __func__, dec));

                if (dec != NULL)
                {
                    JxlBasicInfo info;
                    JxlPixelFormat format;
                    ULONG mode = PBPAFMT_RGB;
                    BOOL decoded = FALSE, done = FALSE;

                    format.num_channels = 3;
                    format.data_type = JXL_TYPE_UINT8;
                    format.endianness = JXL_NATIVE_ENDIAN;
                    format.align = 0;

                    if ((JxlDecoderSubscribeEvents(dec, JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS) ||
                        (JxlDecoderSetInput(dec, buffer, size) != JXL_DEC_SUCCESS))
                    {
                        D(bug("[jpegxl.datatype] %s: failed to initialise the decoder\n", __func__));
                        error = DTERROR_INVALID_DATA;
                        done = TRUE;
                    }
                    else
                        JxlDecoderCloseInput(dec);

                    while (!done)
                    {
                        JxlDecoderStatus status = JxlDecoderProcessInput(dec);

                        switch (status)
                        {
                        case JXL_DEC_BASIC_INFO:
                            if (JxlDecoderGetBasicInfo(dec, &info) != JXL_DEC_SUCCESS)
                            {
                                D(bug("[jpegxl.datatype] %s: failed to get basic info\n", __func__));
                                error = DTERROR_INVALID_DATA;
                                done = TRUE;
                                break;
                            }

                            D(bug("[jpegxl.datatype] %s: image dimensions %ux%u, alpha bits %u\n", __func__, info.xsize, info.ysize, info.alpha_bits));

                            bmh->bmh_Width = info.xsize;
                            bmh->bmh_Height = info.ysize;

                            if (info.alpha_bits > 0)
                            {
                                format.num_channels = 4;
                                mode = PBPAFMT_RGBA;
                                bmh->bmh_Masking = mskHasAlpha;
                                bmh->bmh_Depth = 32;
                            }
                            else
                                bmh->bmh_Depth = 24;
                            break;

                        case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
                            {
                                size_t outsize;
                                if (JxlDecoderImageOutBufferSize(dec, &format, &outsize) != JXL_DEC_SUCCESS)
                                {
                                    D(bug("[jpegxl.datatype] %s: failed to get output buffer size\n", __func__));
                                    error = DTERROR_INVALID_DATA;
                                    done = TRUE;
                                    break;
                                }

                                D(bug("[jpegxl.datatype] %s: allocating %lu byte output buffer\n", __func__, (unsigned long)outsize));

                                if ((pixels = AllocVec(outsize, MEMF_PRIVATE)) == NULL)
                                {
                                    error = ERROR_NO_FREE_STORE;
                                    done = TRUE;
                                    break;
                                }

                                if (JxlDecoderSetImageOutBuffer(dec, &format, pixels, outsize) != JXL_DEC_SUCCESS)
                                {
                                    D(bug("[jpegxl.datatype] %s: failed to set output buffer\n", __func__));
                                    error = DTERROR_INVALID_DATA;
                                    done = TRUE;
                                }
                            }
                            break;

                        case JXL_DEC_FULL_IMAGE:
                            /* we only want the first frame .. */
                            decoded = TRUE;
                            done = TRUE;
                            break;

                        case JXL_DEC_SUCCESS:
                            done = TRUE;
                            break;

                        default:
                            D(bug("[jpegxl.datatype] %s: decode error (status = %u)\n", __func__, status));
                            error = DTERROR_INVALID_DATA;
                            done = TRUE;
                            break;
                        }
                    }

                    JxlDecoderDestroy(dec);

                    if (decoded && (error == (0)))
                    {
                        D(bug("[jpegxl.datatype] %s: image decoded @ 0x%p\n", __func__, pixels));

                        SetDTAttrs(o, NULL, NULL,
                                DTA_NominalHoriz,       bmh->bmh_Width,
                                DTA_NominalVert,        bmh->bmh_Height,
                                PDTA_SourceMode,        PMODE_V43,
                                TAG_END);

                        DoSuperMethod(cl, o,
                                PDTM_WRITEPIXELARRAY, pixels, mode,
                                bmh->bmh_Width * format.num_channels, 0, 0,
                                bmh->bmh_Width, bmh->bmh_Height);
                    }
                    else if (error == (0))
                        error = DTERROR_INVALID_DATA;

                    if (pixels)
                        FreeVec(pixels);
                }
                else
                    error = ERROR_NO_FREE_STORE;
            }
            else
            {
                D(bug("[jpegxl.datatype] %s: not a JPEG XL file (signature = %u)\n", __func__, sig));
                error = DTERROR_INVALID_DATA;
            }
        }
        else
        {
            D(bug("[jpegxl.datatype] %s: failed to read file data\n", __func__));
            error = DTERROR_COULDNT_OPEN;
        }
        FreeVec(buffer);
    }
    else
        error = ERROR_NO_FREE_STORE;

    return error;
}

LONG JXL_Import(struct IClass *cl, Object *o, struct TagItem *tags)
{
    struct BitMapHeader *bmh = NULL;
    char *filename = (char *)GetTagData(DTA_Name, 0, tags);
    SIPTR srctype;
    LONG error = ERROR_OBJECT_NOT_FOUND;
    BPTR file = BNULL;

    D(bug("[jpegxl.datatype] %s('%s')\n", __func__, filename));

    GetDTAttrs(o,
            PDTA_BitMapHeader,          &bmh,
            (filename) ? DTA_Handle : TAG_IGNORE,
                                        &file,
            DTA_SourceType,             &srctype,
            TAG_END);

    D(
        bug("[jpegxl.datatype] %s: PDTA_BitMapHeader @ 0x%p\n", __func__, bmh);
        bug("[jpegxl.datatype] %s: DTA_SourceType = %08x\n", __func__, srctype);
        bug("[jpegxl.datatype] %s: DTA_Handle = %p\n", __func__, file);
     )

    if (bmh && file && srctype == DTST_FILE)
    {
        D(bug("[jpegxl.datatype] %s: attempt to decode file @ 0x%p ...\n", __func__, file));

        error = JXL_Decode(cl, o, file, bmh);
        if (error == (0))
        {
            SetDTAttrs(o, NULL, NULL,
                    DTA_ObjName, FilePart(filename),
                    TAG_END);
        }
    }

    if(srctype == DTST_RAM)
        error = (0);

    return error;
}

#if defined(JPEGXLCLASS_USESUPPORTPROC)
/*****************************************************************************************************/
/* Use a support process to load the image, because libjxl decoding may use quite a bit of stack     */
/*****************************************************************************************************/

struct ImportProcMsg
{
    struct Message  ipm_ExecMessage;
    Object          *ipm_Object;
    struct IClass   *ipm_Class;
    struct TagItem  *ipm_Tags;
    IPTR            *ipm_retval;
};

VOID ImportProc_entry(void)
{
    struct Process *proc = (struct Process *) FindTask(NULL);
    WaitPort(&proc->pr_MsgPort);
    struct ImportProcMsg *imsg = (struct ImportProcMsg *) GetMsg(&proc->pr_MsgPort);
    volatile IPTR exitflag = 0;
    IPTR retval;
    imsg->ipm_retval = (IPTR *)&exitflag;
    retval = JXL_Import(imsg->ipm_Class, imsg->ipm_Object, imsg->ipm_Tags);
    exitflag = retval;
    ReplyMsg(&imsg->ipm_ExecMessage);
    // Wait until its safe for us to exit...
    while (exitflag == retval)
        ;
}

LONG CreateImportProcess(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MsgPort *mport;
    struct Process *proc = NULL;
    LONG retval = -1;

    if ((mport = CreateMsgPort()))
    {
        struct ImportProcMsg *imsg = (struct ImportProcMsg*)AllocVec(sizeof(struct ImportProcMsg),MEMF_PUBLIC);
        if (imsg)
        {
            imsg->ipm_Object                        = obj;
            imsg->ipm_Class                         = cl;
            imsg->ipm_ExecMessage.mn_Node.ln_Type   = NT_MESSAGE;
            imsg->ipm_ExecMessage.mn_ReplyPort      = mport;
            imsg->ipm_Tags                          = msg->ops_AttrList;

            /* libjxl's modular/vardct decode paths can use a considerable
             * amount of stack, so provide plenty
             */
            if ((proc = CreateNewProcTags(NP_Entry, ImportProc_entry,
#if __WORDSIZE > 32
                              NP_StackSize, 131072,
#else
                              NP_StackSize, 65536,
#endif
                              NP_Name,"jpegxl.datatype import process",
                          TAG_DONE)))
            {
                IPTR *exitflag = NULL;
                struct ImportProcMsg *reply;
                PutMsg(&proc->pr_MsgPort,&imsg->ipm_ExecMessage);
                WaitPort(mport);
                while ((reply = (struct ImportProcMsg *)GetMsg(mport)) != NULL)
                {
                    if (retval == -1)
                    {
                        exitflag = reply->ipm_retval;
                        retval = *exitflag;
                    }
                }
                if (exitflag)
                    *exitflag = !retval;
            }
            FreeVec(imsg);
            imsg = NULL;
        }
        DeleteMsgPort(mport);
        mport = NULL;
    }
    return retval;
}
#endif

/**************************************************************************************************/

IPTR JXL__OM_NEW(struct IClass *cl, Object *o, struct opSet *msg)
{
    D(bug("[jpegxl.datatype] %s()\n", __func__));

    IPTR retval = DoSuperMethodA(cl, o, msg);
    LONG error;

    if (retval) {

        D(bug("[jpegxl.datatype] %s: object @ 0x%p\n", __func__, retval));

#if defined(JPEGXLCLASS_USESUPPORTPROC)
        error = CreateImportProcess(cl, (Object *)retval, msg);
#else
        error = JXL_Import(cl, (Object *)retval, msg->ops_AttrList);
#endif
        if (error != (0)) {
            D(bug("[jpegxl.datatype] %s: disposing...\n", __func__));

            CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
            SetIoErr(error);

            retval = (IPTR)NULL;
        }
    }
    return retval;
}

/**************************************************************************************************/

IPTR JXL__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    IPTR retval = FALSE;

    D(bug("[jpegxl.datatype] %s()\n", __func__));

    if (dtw->dtw_Mode == DTWM_RAW) {
        /* no JPEG XL encoding support (yet) */
        D(bug("[jpegxl.datatype] %s: raw write not supported\n", __func__));
    }
    else
        retval = DoSuperMethodA( cl, o, (Msg)dtw );

    return retval;
}
