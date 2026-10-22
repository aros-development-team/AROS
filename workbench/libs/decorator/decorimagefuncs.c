/*
    Copyright (C) 2011-2026, The AROS Development Team.

    Desc: decorator.library - DecorImage container functions
*/

#include <datatypes/pictureclass.h>
#include <libraries/cybergraphics.h>

#include <proto/alib.h>
#include <proto/datatypes.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>

#include <aros/libcall.h>

#include <string.h>

#include <aros/debug.h>

#include <libraries/decorator.h>
#include "decorator_intern.h"

#if AROS_BIG_ENDIAN
#define GET_A(rgb)          (((rgb) >> 24) & 0xFF)
#define MASK_ALPHATRANSP    0X00FFFFFF
#define MASK_ALPHAOPAQUE    0XFF000000
#else
#define GET_A(rgb)          ((rgb) & 0xFF)
#define MASK_ALPHATRANSP    0xFFFFFF00
#define MASK_ALPHAOPAQUE    0X000000FF
#endif

static Object * LoadPicture(CONST_STRPTR filename, struct Screen *scr)
{
    Object *o;

    o = NewDTObject((APTR)filename,
                    DTA_GroupID          , GID_PICTURE,
                    OBP_Precision        , PRECISION_EXACT,
                    PDTA_Screen          , (IPTR)scr,
                    PDTA_FreeSourceBitMap, TRUE,
                    PDTA_DestMode        , PMODE_V43,
                    PDTA_UseFriendBitMap , TRUE,
                    TAG_DONE);

    if (o)
    {
        struct FrameInfo fri = {0};
        
        D(bug("DTM_FRAMEBOX\n", o));
        DoMethod(o,DTM_FRAMEBOX,NULL,(IPTR)&fri,(IPTR)&fri,sizeof(struct FrameInfo),0);
        
        if (fri.fri_Dimensions.Depth > 0)
        {
            D(bug("DTM_PROCLAYOUT\n", o));
            if (DoMethod(o,DTM_PROCLAYOUT,NULL,1))
            {
                return o;
            }
        }
        DisposeDTObject(o);
    }

    return NULL;
}

/* Code taken from BM__Hidd_BitMap__BitMapScale */
/* srcdata point directly to (0,0) point of subbuffer to be read from */
/* Internal implementation of ScaleBuffer */
ULONG *ScaleBuffer(ULONG *srcdata, LONG widthBuffer, LONG widthSrc, LONG heightSrc, LONG widthDest, LONG heightDest)
{
    ULONG * scaleddata = (ULONG *) AllocVec(sizeof(ULONG) * widthDest * heightDest, MEMF_ANY);
    LONG srcline = -1;
    UWORD * linepattern = (UWORD *) AllocVec(sizeof(UWORD) * widthDest, MEMF_ANY);
    ULONG count = 0;
    UWORD ys = 0;
    ULONG xs = 0;
    ULONG dyd = heightDest;
    ULONG dxd = widthDest;
    LONG accuys = dyd;
    LONG accuxs = dxd;
    ULONG dxs = widthSrc;
    ULONG dys = heightSrc;
    LONG accuyd = - (dys >> 1);
    LONG accuxd = - (dxs >> 1);
    ULONG x;
    ULONG * lastscaledlineptr = scaleddata + ((heightDest - 1) * widthDest);

    count = 0;
    while (count < widthDest) {
        accuxd += dxs;
        while (accuxd > accuxs) {
            xs++;
            accuxs += dxd;
        }

        linepattern[count] = xs;

        count++;
    }

    count = 0;
    while (count < heightDest) {
        accuyd += dys;
        while (accuyd > accuys) {
            ys++;
            accuys += dyd;
        }

        if (srcline != ys) {
            ULONG * srcptr = srcdata + (ys * widthBuffer);
            srcline = ys;

            if (widthSrc == 1)
                pixop_splat_row(lastscaledlineptr, widthDest, srcptr[0]);
            else if (widthSrc == widthDest)
                CopyMem(srcptr, lastscaledlineptr, widthDest * sizeof(ULONG));
            else
            {
                for (x = 0; x < widthDest; x++)
                    lastscaledlineptr[x] = srcptr[linepattern[x]];
            }
        }

        CopyMem(lastscaledlineptr, scaleddata + (count * widthDest), widthDest * sizeof(ULONG));

        count++;
    }

    FreeVec(linepattern);

    return scaleddata;
}

/*****************************************************************************
    NAME
        ULONG * DScaleBuffer(

    SYNOPSIS
        ULONG *srcdata, LONG widthBuffer, LONG widthSrc, LONG heightSrc, LONG widthDest, LONG heightDest)

    FUNCTION
        Scale a rectangular array of 32-bit pixels into a new packed buffer.

    INPUTS
        srcdata - Non-NULL readable pixel pointer at the source rectangle's
            top-left pixel. Storage must remain valid until return.
        widthBuffer - Source row stride in pixels, at least widthSrc.
        widthSrc, heightSrc - Positive source dimensions, at most 65535 each.
        widthDest, heightDest - Positive destination dimensions. Products,
            row offsets and scaling accumulators must fit target arithmetic.

    RESULT
        New widthDest*heightDest ULONG buffer on normal completion. Free it
        with FreeVec(). Allocation failure is not reliably reported as NULL.

    NOTES
        Scaling selects source samples without interpolation or color/alpha
        conversion. All four bytes of a selected pixel are copied unchanged.
        The output is tightly packed. The source is neither modified nor
        retained; a subrectangle may use a larger source stride.

        No bounds or arithmetic-overflow checks are performed. Source sample
        indices use 16-bit storage. Provide enough source storage for all rows
        and keep scaled sizes within the allocator's byte-size limit.

        Call in task context with decorator.library open. Processing is
        synchronous and has no cancellation or timeout.

    BUGS
        The output and horizontal sampling-table allocations are used without
        NULL checks. Memory exhaustion can cause invalid memory access rather
        than returning an error. Nonpositive dimensions are not supported.

    SEE ALSO
        DScaleDecorImage(), FreeVec()

*****************************************************************************/
AROS_LH6(ULONG *, DScaleBuffer,
    AROS_LHA(ULONG *, srcdata, A0),
    AROS_LHA(LONG, widthBuffer, D0),
    AROS_LHA(LONG, widthSrc, D1),
    AROS_LHA(LONG, heightSrc, D2),
    AROS_LHA(LONG, widthDest, D3),
    AROS_LHA(LONG, heightDest, D4),
    struct Library *, DecoratorBase, 14, Decorator)
{
    AROS_LIBFUNC_INIT
    return ScaleBuffer(srcdata, widthBuffer, widthSrc, heightSrc, widthDest, heightDest);
    AROS_LIBFUNC_EXIT
}

/* Internal implementation of DisposeImageContainer */
void DisposeImageContainer(struct DecorImage *di)
{
    if (di)
    {
        FreeVec(di->data);

        if (di->o)
        {
            DisposeDTObject(di->o);
            di->o = NULL;
            di->bitmap = NULL;
            di->mask = NULL;
        }

        FreeVec(di->filename);

        FreeVec(di->subimageinbm);
        FreeBitMap(di->bitmap2);

        FreeVec(di);
    }
}

/*****************************************************************************
    NAME
        void DDisposeImageContainer(

    SYNOPSIS
        struct DecorImage *di)

    FUNCTION
        Release an owned ARGB image container and its associated resources.

    INPUTS
        di - Owned DecorImage, or NULL for no operation.

    RESULT
        None.

    NOTES
        Releases the pixel buffer, filename, subimage decision table and
        auxiliary bitmap. If a datatype object is present, it is disposed;
        its bitmap and mask are not separately freed. An unrelated bitmap
        assigned only to the bitmap field is not independently released.

        Stop all rendering and other access before disposal. The call does
        not detach element descriptors, release references held elsewhere or
        detect shared ownership. Each owned allocation must have one owner.
        Do not dispose images borrowed from a theme or screen instance.

        Call once in task context with decorator.library open. Do not reuse
        the container or any pointers into its owned resources after return.

    SEE ALSO
        DNewDecorImageContainer(), DGetImageFromFile(), DScaleDecorImage()

*****************************************************************************/
AROS_LH1(void, DDisposeImageContainer,
    AROS_LHA(struct DecorImage *, di, A0),
    struct Library *, DecoratorBase, 8, Decorator)
{
    AROS_LIBFUNC_INIT
    DisposeImageContainer(di);
    AROS_LIBFUNC_EXIT
}

/* Internal implementation of DisposeLUT8ImageContainer */
void DisposeLUT8ImageContainer(struct DecorImageLUT8 *di)
{
    if (di)
    {
        FreeVec(di->data);
        FreeVec(di);
    }
}

/*****************************************************************************
    NAME
        void DDisposeLUT8ImageContainer(

    SYNOPSIS
        struct DecorImageLUT8 *di)

    FUNCTION
        Release an owned eight-bit image container and its pixel buffer.

    INPUTS
        di - Owned DecorImageLUT8, or NULL for no operation.

    RESULT
        None.

    NOTES
        Stop all access before calling. The buffer and container are freed;
        no reference counting, synchronization or shared-buffer detection is
        performed. Call once in task context with decorator.library open.

    SEE ALSO
        DNewLUT8ImageContainer()

*****************************************************************************/
AROS_LH1(void, DDisposeLUT8ImageContainer,
    AROS_LHA(struct DecorImageLUT8 *, di, A0),
    struct Library *, DecoratorBase, 12, Decorator)
{
    AROS_LIBFUNC_INIT
    DisposeLUT8ImageContainer(di);
    AROS_LIBFUNC_EXIT
}

/* Internal implementation of NewLUT8ImageContainer */
struct DecorImageLUT8 *NewLUT8ImageContainer(UWORD w, UWORD h)
{
    struct      DecorImageLUT8 *di;

    di = AllocVec(sizeof(struct DecorImageLUT8), MEMF_ANY | MEMF_CLEAR);
    if (di)
    {
        di->w = w;
        di->h = h;
        di->data = AllocVec(w * h, MEMF_ANY | MEMF_CLEAR);
        if (di->data == NULL)
        {
            FreeVec(di);
            di = NULL;
        }
    }
    return di;
}

/*****************************************************************************
    NAME
        struct DecorImageLUT8 * DNewLUT8ImageContainer(

    SYNOPSIS
        UWORD w, UWORD h)

    FUNCTION
        Allocate an empty eight-bit image container and pixel buffer.

    INPUTS
        w, h - Positive pixel dimensions. The product must fit the target's
            signed multiplication and allocation limits; it is not checked.

    RESULT
        Owned DecorImageLUT8, or NULL if either allocation fails.

    NOTES
        The tightly packed w*h byte buffer is cleared. No palette is allocated
        or attached. Pixel interpretation belongs to the consuming operation;
        DRegionFromLUT8Image() treats zero as transparent and nonzero as opaque.

        Release the container with DDisposeLUT8ImageContainer(). Call in task
        context with decorator.library open; do not use zero dimensions.

    SEE ALSO
        DDisposeLUT8ImageContainer(), DRegionFromLUT8Image()

*****************************************************************************/
AROS_LH2(struct DecorImageLUT8 *, DNewLUT8ImageContainer,
    AROS_LHA(UWORD, w, D0),
    AROS_LHA(UWORD, h, D1),
    struct Library *, DecoratorBase, 11, Decorator)
{
    AROS_LIBFUNC_INIT
    return NewLUT8ImageContainer(w, h);
    AROS_LIBFUNC_EXIT
}

/* Internal implementation of NewDecorImageContainer */
struct DecorImage *NewDecorImageContainer(UWORD w, UWORD h)
{
    struct      DecorImage *di;

    di = AllocVec(sizeof(struct DecorImage), MEMF_ANY | MEMF_CLEAR);
    if (di)
    {
        di->w = w;
        di->h = h;
        di->subimagescols = 1;
        di->subimagesrows = 1;
        di->data = AllocVec(w * h * sizeof (ULONG), MEMF_ANY | MEMF_CLEAR);
        if (di->data == NULL)
        {
            FreeVec(di);
            di = NULL;
        }
    }
    return di;
}

/*****************************************************************************
    NAME
        struct DecorImage * DNewDecorImageContainer(

    SYNOPSIS
        UWORD w, UWORD h)

    FUNCTION
        Allocate an empty ARGB image container and pixel buffer.

    INPUTS
        w, h - Positive pixel dimensions. Their product and the four-byte
            pixel allocation must fit the target's arithmetic and allocation
            limits; no size validation is performed.

    RESULT
        Owned DecorImage, or NULL if the container or pixel allocation fails.

    NOTES
        The buffer is cleared and tightly packed, with four bytes per pixel
        in alpha, red, green, blue order in memory. The subimage grid is 1 by 1.
        The ok flag remains FALSE; initialize the pixels and set it TRUE before
        passing the image to rendering operations that test that flag.

        No filename, datatype object, screen bitmap or subimage decision table
        is created. This container alone is not a valid source for
        DCreateDecorImageContainerMatchingScreen(), which expects a filename.

        Release the result with DDisposeImageContainer(). Call in task context
        with decorator.library open. Zero dimensions have no defined useful
        image result and must not be used.

    SEE ALSO
        DDisposeImageContainer(), DScaleDecorImage(), DGetImageFromFile()

*****************************************************************************/
AROS_LH2(struct DecorImage *, DNewDecorImageContainer,
    AROS_LHA(UWORD, w, D0),
    AROS_LHA(UWORD, h, D1),
    struct Library *, DecoratorBase, 6, Decorator)
{
    AROS_LIBFUNC_INIT
    return NewDecorImageContainer(w, h);
    AROS_LIBFUNC_EXIT
}

/* Internal implementation of ScaleDecorImage */
struct DecorImage *ScaleDecorImage(struct DecorImage *oni, UWORD neww, UWORD newh)
{
    struct  DecorImage *di;

    di = AllocVec(sizeof(struct DecorImage), MEMF_ANY | MEMF_CLEAR);
    if (di)
    {
        di->w = neww;
        di->h = newh;
        di->subimagescols = oni->subimagescols;
        di->subimagesrows = oni->subimagesrows;
        di->ok = TRUE;
        di->data = ScaleBuffer(oni->data, oni->w, oni->w, oni->h, di->w, di->h);
        if (di->data == NULL)
        {
            FreeVec(di);
            di = NULL;
        }
    }
    return di;
}

/*****************************************************************************
    NAME
        struct DecorImage * DScaleDecorImage(

    SYNOPSIS
        struct DecorImage *di, UWORD neww, UWORD newh)

    FUNCTION
        Create a scaled copy of an image pixel buffer.

    INPUTS
        di - Live DecorImage with a non-NULL readable data buffer and positive
            dimensions. Its ok flag is not checked.
        neww, newh - Positive destination dimensions whose allocation and
            scaling arithmetic fit the target's limits.

    RESULT
        Owned DecorImage on normal completion, or NULL if allocation of the
        outer container fails. Deeper allocation failures are not reliably
        reported. Dispose the result with DDisposeImageContainer().

    NOTES
        The complete source pixel buffer is scaled using DScaleBuffer().
        The subimage row and column counts are copied without validation;
        choose destination dimensions that preserve any required grid layout.
        The returned ok flag is TRUE after successful scaling.

        Filename, datatype object, bitmap, mask and subimage decision table
        are not copied. The result is a pixel-buffer image, not a screen-matched
        copy or a filename-backed source for screen matching.

        The source remains owned by the caller. Keep it stable until return.
        Call synchronously in task context with decorator.library open.

    SEE ALSO
        DScaleBuffer(), DDisposeImageContainer()

*****************************************************************************/
AROS_LH3(struct DecorImage *, DScaleDecorImage,
    AROS_LHA(struct DecorImage *, oni, A0),
    AROS_LHA(UWORD, neww, D0),
    AROS_LHA(UWORD, newh, D1),
    struct Library *, DecoratorBase, 7, Decorator)
{
    AROS_LIBFUNC_INIT
    return ScaleDecorImage(oni, neww, newh);
    AROS_LIBFUNC_EXIT
}

/* Internal implementation of GetImageFromFile */
struct DecorImage *GetImageFromFile(STRPTR path, STRPTR name, ULONG expectedsubimagescols, ULONG expectedsubimagesrows)
{
    struct BitMapHeader        *bmhd = NULL;
    struct DecorImage            *di = NULL;
    struct BitMap              *map = NULL;
    struct RastPort            *rp = NULL;
    Object                     *pic;
    struct pdtBlitPixelArray    pa;
    STRPTR                      buffer;
    UWORD                       w, h, x, y;
    UBYTE                       mask;
    ULONG                      *dst;
    LONG                        len;
    
    len = strlen(path) + strlen(name) + 2;
    buffer = AllocVec(len, MEMF_CLEAR | MEMF_ANY);
    strncpy(buffer, path, len);
    AddPart(buffer, name, len);

    pic = NewDTObject(buffer,  DTA_SourceType,  DTST_FILE,
                               DTA_GroupID,     GID_PICTURE,
                               PDTA_Remap,      FALSE,
                               PDTA_DestMode,   PMODE_V43,
                               TAG_DONE);
    if (pic)
    {
        GetAttr(PDTA_BitMapHeader, pic, (APTR)&bmhd);
        if(bmhd)
        {

            w = bmhd->bmh_Width;
            h = bmhd->bmh_Height;
            mask = bmhd->bmh_Masking;
            di = NewDecorImageContainer(w, h);
            if (di)
            {
                di->filename = AllocVec(len, MEMF_CLEAR | MEMF_ANY);
                strncpy(di->filename, buffer, len);
                di->subimagescols = expectedsubimagescols;
                di->subimagesrows = expectedsubimagesrows;
                pa.MethodID = PDTM_READPIXELARRAY;
                pa.pbpa_PixelData = (APTR) di->data;
                pa.pbpa_PixelFormat = PBPAFMT_ARGB;
                pa.pbpa_PixelArrayMod = w*4;
                pa.pbpa_Left = 0;
                pa.pbpa_Top = 0;
                pa.pbpa_Width = w;
                pa.pbpa_Height = h;
                DoMethodA(pic, (Msg) &pa);
                di->ok = TRUE;
                if (bmhd->bmh_Depth <= 8)
                {
                    GetAttr(PDTA_BitMap, pic, (APTR)&map);
                    if (map && (mask == mskHasTransparentColor))
                    {
                        rp = CreateRastPort();
                        if (rp) rp->BitMap = map;
                    }
                }

                if (rp)
                {
                    dst = di->data;
                    for (y = 0; y < h; y++)
                    {
                        for (x = 0; x < w; x++)
                        {
                            if (ReadPixel(rp, x, y) == bmhd->bmh_Transparent)
                                dst[x+y*w] &= MASK_ALPHATRANSP;
                            else
                                dst[x+y*w] |= MASK_ALPHAOPAQUE;
                        }
                    }
                    FreeRastPort(rp);
                }
            }
            }
        DisposeDTObject(pic);
    }

    FreeVec(buffer);

    return di;
}

/*****************************************************************************
    NAME
        struct DecorImage * DGetImageFromFile(

    SYNOPSIS
        STRPTR path, STRPTR name, ULONG expectedsubimagescols, ULONG expectedsubimagesrows)

    FUNCTION
        Load a picture file into an owned ARGB image container.

    INPUTS
        path - Non-NULL, NUL-terminated directory path.
        name - Non-NULL, NUL-terminated name appended with DOS AddPart().
        expectedsubimagescols, expectedsubimagesrows - Positive grid counts
            stored in the result. Supply counts consistent with the image
            dimensions; this function does not validate the grid.

    RESULT
        Owned DecorImage, or NULL when the file cannot be opened as a picture,
        no bitmap header is available, or checked image allocation fails.
        A non-NULL result does not prove that pixel decoding succeeded.

    NOTES
        The image contains tightly packed ARGB bytes. For indexed pictures
        using a transparent-color index, matching source pixels receive zero
        alpha and other pixels receive full alpha. The temporary datatype
        object is disposed before return.

        The combined filename is copied into the result without resolving it
        to an absolute path. Input strings need remain valid only until return.
        Use a stable absolute path if the image will later be screen-matched.
        No extension is added and no alternate filename is tried here.

        Release the result with DDisposeImageContainer(). Call from a DOS
        process with decorator.library open. File access and picture decoding
        are synchronous, with no caller timeout.

    BUGS
        Path and filename allocations are unchecked. Allocation failure can
        cause invalid memory access. The pixel-read method's result is ignored
        and ok is set TRUE even if that method fails.

    SEE ALSO
        DDisposeImageContainer(), DCreateDecorImageContainerMatchingScreen()

*****************************************************************************/
AROS_LH4(struct DecorImage *, DGetImageFromFile,
    AROS_LHA(STRPTR, path, A0),
    AROS_LHA(STRPTR, name, A1),
    AROS_LHA(ULONG, expectedsubimagescols, D0),
    AROS_LHA(ULONG, expectedsubimagesrows, D1),
    struct Library *, DecoratorBase, 9, Decorator)
{
    AROS_LIBFUNC_INIT
    return GetImageFromFile(path, name, expectedsubimagescols, expectedsubimagesrows);
    AROS_LIBFUNC_EXIT
}

/* This function must never return NULL, because logic in drawing code
   checks img->ok instead of img != NULL to make decisions */
/* Internal implementation of CreateDecorImageContainerMatchingScreen */
struct DecorImage *CreateDecorImageContainerMatchingScreen(struct DecorImage *in, BOOL truecolor, struct Screen *scr)
{
    struct DecorImage * out = AllocVec(sizeof(struct DecorImage), MEMF_ANY | MEMF_CLEAR);
    out->ok = FALSE;

    if (in != NULL)
    {
        out->w = in->w;
        out->h = in->h;
        out->subimagescols = in->subimagescols;
        out->subimagesrows = in->subimagesrows;
        out->filename = NULL;
        out->bitmap = NULL;
        out->bitmap2 = NULL;
        out->mask = NULL;
        out->o = NULL;
        if (in->data != NULL)
        {
            out->data = AllocVec(out->w * out->h * sizeof(ULONG), MEMF_ANY | MEMF_CLEAR);
            if (out->data != NULL) CopyMem(in->data, out->data, out->w * out->h * sizeof(ULONG));
        }
        out->ok = (out->data != NULL) ? TRUE : FALSE;
        /* Allocate decision table, all values are set to FALSE (MEMF_CLEAR) */
        out->subimageinbm = AllocVec(in->subimagescols * in->subimagesrows * sizeof(BOOL), MEMF_ANY | MEMF_CLEAR);

        out->filename = AllocVec(strlen(in->filename) + 5, MEMF_ANY | MEMF_CLEAR); /* size + 5 -> covers all */
        strcpy(out->filename, in->filename);

        if (!truecolor)
        {
            /* If this is LUT screen, try to load LUT version of image */
            out->ok = FALSE;
            strcat(out->filename, "_LUT");
        
            out->o = LoadPicture(out->filename, scr);
            if (out->o == NULL)
            {
                /* Load the original image with conversion */
                out->o = LoadPicture(in->filename, scr);
                if (out->o != NULL)
                    strcpy(out->filename, in->filename);
            }
            
            if (out->o)
            {
                GetDTAttrs(out->o, PDTA_DestBitMap, (IPTR)&out->bitmap, TAG_DONE);
                if (out->bitmap == NULL)
                    GetDTAttrs(out->o, PDTA_BitMap, (IPTR)&out->bitmap, TAG_DONE);

                if (out->bitmap != NULL)
                {
                    GetDTAttrs(out->o, PDTA_MaskPlane, (IPTR)&out->mask, TAG_DONE);
                    out->ok = TRUE;
                }
                else
                {
                    DisposeDTObject(out->o);
                    out->o = NULL;
                }
            }
        }

        if (out->data != NULL)
        {
            ULONG subimagewidth = out->w / out->subimagescols;
            ULONG subimageheight = out->h / out->subimagesrows;
            ULONG col = 0, row = 0, x = 0, y = 0;
            out->mask = NULL;
            out->o = NULL;
            BOOL atleastone = FALSE;

            /* It is possible that some subimage do not have alpha channel.
               If that is true, we could create a bitmap that can be later used
               for blitting instead of alpha drawing */

            /* Scan subimages and detect which don't have alpha channel */
            for (row = 0; row < out->subimagesrows; row++)
            {
                for (col = 0; col < out->subimagescols; col++)
                {
                    /* Assume image can be put to bitmap */
                    out->subimageinbm[col + (row * out->subimagescols)] = TRUE;
                    /* Place pointer at beginning of subimage */
                    ULONG * ptr = out->data + (row * out->w * subimageheight) + (col * subimagewidth);
                    for (y = 0; y < subimageheight; y++)
                    {
                        for (x = 0; x < subimagewidth; x++)
                        {
                            if (GET_A(*ptr) != 0xFF)
                               out->subimageinbm[col + (row * out->subimagescols)] = FALSE;
                            ptr++;
                        }
                        ptr += (subimagewidth * (out->subimagescols - 1)); /* Advance to next subimage line */
                    }
                }
            }

            /* Check if there was at least one subimage without alpha channel */
            D(bug("File: %s : ", out->filename));
            for (row = 0; row < out->subimagesrows; row++)
            {
                for (col = 0; col < out->subimagescols; col++)
                {
                    D(bug("sb(%d,%d):", col, row));
                    if (out->subimageinbm[col + (row * out->subimagescols)])
                    {
                        D(bug("YES, "));
                        atleastone = TRUE;
                    }
                    else
                    {
                        D(bug("NO, "));
                    }
                }
            }
            D(bug("\n"));

            /* If yes, generate a bitmap */
            if (atleastone)
            {
                struct RastPort * rp = CreateRastPort();
                out->bitmap2 = AllocBitMap(out->w, out->h, 1, 0, scr->RastPort.BitMap);
                rp->BitMap = out->bitmap2;
                WritePixelArray(out->data, 0, 0, out->w * 4, rp, 0, 0, out->w, out->h, RECTFMT_ARGB);
                FreeRastPort(rp);
            }
        }
    }
    
    return out;
}

/*****************************************************************************
    NAME
        struct DecorImage * DCreateDecorImageContainerMatchingScreen(

    SYNOPSIS
        struct DecorImage *in, BOOL truecolor, struct Screen *scr)

    FUNCTION
        Create an image copy prepared for a target screen.

    INPUTS
        in - Source DecorImage with a valid filename, dimensions and nonzero
            subimage grid consistent with those dimensions; normally returned
            by DGetImageFromFile(). NULL requests an empty unusable container.
        truecolor - TRUE to use copied ARGB data; FALSE to try palette-matched
            picture loading for the target screen.
        scr - Live, non-NULL Screen with a valid RastPort.BitMap. Keep the
            screen open until the returned image has been disposed.

    RESULT
        Owned DecorImage on normal completion. Check its ok flag before use;
        a returned container can be unusable. Allocation failures are not
        reliably converted to a NULL result.

    NOTES
        Dimensions and grid counts are copied. Source pixels, when present,
        are copied into a separate allocation. The source remains caller-owned
        and must stay stable until return; it is not retained afterward.

        The palette path first loads filename with _LUT appended, then tries
        the original file remapped to scr. The true-color path does not reopen
        the file, but still requires a valid source filename. Relative names
        use the process's current directory at conversion time.

        For copied pixels, fully opaque subimages are recorded in a decision
        table and may receive an auxiliary screen-compatible bitmap. No grid,
        buffer-size or arithmetic validation is performed. A zero grid count
        can cause division by zero.

        Dispose the result with DDisposeImageContainer() before closing scr.
        Call from a DOS process with decorator.library open. Conversion and
        any picture loading are synchronous and have no caller timeout.

    BUGS
        Several allocations and rendering resources are used without checks.
        Memory exhaustion can cause invalid memory access. When palette loading
        succeeds and copied source pixels are also present, the datatype-object
        pointer is cleared without disposal, leaking its owned resources.

    SEE ALSO
        DGetImageFromFile(), DDisposeImageContainer()

*****************************************************************************/
AROS_LH3(struct DecorImage *, DCreateDecorImageContainerMatchingScreen,
    AROS_LHA(struct DecorImage *, in, A0),
    AROS_LHA(BOOL, truecolor, D0),
    AROS_LHA(struct Screen *, scr, A1),
    struct Library *, DecoratorBase, 10, Decorator)
{
    AROS_LIBFUNC_INIT
    return CreateDecorImageContainerMatchingScreen(in, truecolor, scr);
    AROS_LIBFUNC_EXIT
}

/* Internal implementation of RegionFromLUT8Image */
struct Region *RegionFromLUT8Image(int w, int h, struct DecorImageLUT8 *s)
{
    struct Region  *shape;
    int             x, y;
    BOOL            transp, transpstate;
    BOOL            failed = FALSE;

    if (s == NULL) return NULL;

    UBYTE   *src = s->data;

    shape = NewRegion();

    if (shape) {
        struct Rectangle rect = {0, s->h, w-1, h-1};
        if (!OrRectRegion(shape, &rect)) failed = TRUE;
        for(y = 0; y < s->h; y++)
        {
            struct Rectangle rect = {0, y, 0, y};
            transpstate = TRUE;
            for(x = 0; x < s->w; x++)
            {
                transp = src[y * s->w + x] == 0;
                if (transpstate)
                {
                    if (!transp)
                    {
                        rect.MaxX = rect.MinX = x;
                        transpstate = FALSE;
                    }
                } else {
                    if (transp)
                    {
                        OrRectRegion(shape, &rect);
                        transpstate = TRUE;
                    }
                    else
                    {
                        rect.MaxX++;
                    }
                }
            }
            if (!transpstate) if (!OrRectRegion(shape, &rect)) failed = TRUE;
        }
        if (failed)
        {
            DisposeRegion(shape);
            shape = NULL;
        }
    }
    return shape;
}

/*****************************************************************************
    NAME
        struct Region * DRegionFromLUT8Image(

    SYNOPSIS
        int w, int h, struct DecorImageLUT8 *s)

    FUNCTION
        Build a region from nonzero pixels in an eight-bit image.

    INPUTS
        w, h - Intended overall region width and height. Use positive values
            representable by graphics Rectangle coordinates.
        s - Live DecorImageLUT8 with readable, tightly packed data, or NULL.
            Its width and height must fit the region coordinate range.

    RESULT
        Owned Region, or NULL for a NULL source, failure to allocate a region,
        or a detected rectangle-insertion failure. Release it with DisposeRegion().

    NOTES
        Each nonzero source pixel contributes to opaque horizontal runs;
        zero pixels are transparent. Source rows use s->w and s->h directly.
        The function also adds the rectangle from (0, s->h) through (w-1, h-1)
        below the source image. It does not scale or clip source runs to w or h;
        use w >= s->w and h >= s->h for a containing extent.

        The source buffer is borrowed only until return. Region construction
        is synchronous and does not acquire a reference or lock the image.
        Call in task context with decorator.library open.

    BUGS
        Failure to add a run ending at a transparent pixel is ignored; only
        the lower rectangle and runs reaching the row end are checked. A
        non-NULL result can therefore be incomplete after allocation failure.

    SEE ALSO
        DNewLUT8ImageContainer(), DisposeRegion()

*****************************************************************************/
AROS_LH3(struct Region *, DRegionFromLUT8Image,
    AROS_LHA(int, w, D0),
    AROS_LHA(int, h, D1),
    AROS_LHA(struct DecorImageLUT8 *, s, A0),
    struct Library *, DecoratorBase, 13, Decorator)
{
    AROS_LIBFUNC_INIT
    return RegionFromLUT8Image(w, h, s);
    AROS_LIBFUNC_EXIT
}
