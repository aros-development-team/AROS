/*
    Copyright © 2011, The AROS Development Team.
    $Id$
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

#include <string.h>

#include <aros/debug.h>

#include "newimage.h"

#if AROS_BIG_ENDIAN
#define GET_A(rgb) (((rgb) >> 24) & 0xff)
#else
#define GET_A(rgb) ((rgb) & 0xff)
#endif

/* Code taken from BM__Hidd_BitMap__BitMapScale */
/* srcdata point directly to (0,0) point of subbuffer to be read from */
ULONG * ScaleBuffer(ULONG * srcdata, LONG widthBuffer /* stride */, LONG widthSrc, LONG heightSrc, LONG widthDest, LONG heightDest)
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
            //HIDD_BM_GetImage(msg->src, (UBYTE *) srcbuf, bsa->bsa_SrcWidth * sizeof(ULONG), bsa->bsa_SrcX, bsa->bsa_SrcY + ys, bsa->bsa_SrcWidth, 1, vHidd_StdPixFmt_Native32);
            ULONG * srcptr = srcdata + (ys * widthBuffer);
            srcline = ys;

            /* New: use last line as temp buffer */
            for (x = 0; x < widthDest; x++)
                lastscaledlineptr[x] = srcptr[linepattern[x]];

        }

        //HIDD_BM_PutImage(msg->dst, msg->gc, (UBYTE *) dstbuf, bsa->bsa_DestWidth * sizeof(ULONG), bsa->bsa_DestX, bsa->bsa_DestY + count, bsa->bsa_DestWidth, 1, vHidd_StdPixFmt_Native32);
        CopyMem(lastscaledlineptr, scaleddata + (count * widthDest), widthDest * sizeof(ULONG));

        count++;
    }

    FreeVec(linepattern);

    return scaleddata;
}

void DisposeImageContainer(struct NewImage *ni)
{
    if (ni)
    {
        FreeVec(ni->data);

        if (ni->o)
        {
            DisposeDTObject(ni->o);
            ni->o = NULL;
            ni->bitmap = NULL;
            ni->mask = NULL;
        }

        FreeVec(ni->filename);

        FreeVec(ni->subimageinbm);
        FreeBitMap(ni->bitmap2);

        FreeVec(ni);
    }
}

void DisposeLUT8ImageContainer(struct NewLUT8Image *ni)
{
    if (ni)
    {
        FreeVec(ni->data);
        FreeVec(ni);
    }
}

struct NewLUT8Image *NewLUT8ImageContainer(UWORD w, UWORD h)
{
    struct	NewLUT8Image *ni;

    ni = AllocVec(sizeof(struct NewLUT8Image), MEMF_ANY | MEMF_CLEAR);
    if (ni)
    {
        ni->w = w;
        ni->h = h;
        ni->data = AllocVec(w * h, MEMF_ANY | MEMF_CLEAR);
        if (ni->data == NULL)
        {
            FreeVec(ni);
            ni = NULL;
        }
    }
    return ni;
}

struct NewImage *NewImageContainer(UWORD w, UWORD h)
{
    struct	NewImage *ni;

    ni = AllocVec(sizeof(struct NewImage), MEMF_ANY | MEMF_CLEAR);
    if (ni)
    {
        ni->w = w;
        ni->h = h;
        ni->subimagescols = 1;
        ni->subimagesrows = 1;
        ni->data = AllocVec(w * h * sizeof (ULONG), MEMF_ANY | MEMF_CLEAR);
        if (ni->data == NULL)
        {
            FreeVec(ni);
            ni = NULL;
        }
    }
    return ni;
}

struct NewImage *ScaleNewImage(struct NewImage * oni, UWORD neww, UWORD newh)
{
    struct  NewImage *ni;

    ni = AllocVec(sizeof(struct NewImage), MEMF_ANY | MEMF_CLEAR);
    if (ni)
    {
        ni->w = neww;
        ni->h = newh;
        ni->subimagescols = oni->subimagescols;
        ni->subimagesrows = oni->subimagesrows;
        ni->ok = TRUE;
        ni->data = ScaleBuffer(oni->data, oni->w, oni->w, oni->h, ni->w, ni->h);
        if (ni->data == NULL)
        {
            FreeVec(ni);
            ni = NULL;
        }
    }
    return ni;
}

struct NewImage *GetImageFromFile(STRPTR path, STRPTR name,
    ULONG expectedsubimagescols, ULONG expectedsubimagesrows)
{
    struct BitMapHeader        *bmhd = NULL;
    struct NewImage            *ni = NULL;
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
            ni = NewImageContainer(w, h);
            if (ni)
            {
                ni->filename = AllocVec(len, MEMF_CLEAR | MEMF_ANY);
                strncpy(ni->filename, buffer, len);
                ni->subimagescols = expectedsubimagescols;
                ni->subimagesrows = expectedsubimagesrows;
                pa.MethodID = PDTM_READPIXELARRAY;
                pa.pbpa_PixelData = (APTR) ni->data;
                pa.pbpa_PixelFormat = PBPAFMT_ARGB;
                pa.pbpa_PixelArrayMod = w*4;
                pa.pbpa_Left = 0;
                pa.pbpa_Top = 0;
                pa.pbpa_Width = w;
                pa.pbpa_Height = h;
                DoMethodA(pic, (Msg) &pa);
                ni->ok = TRUE;
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
                    dst = ni->data;
                    for (y = 0; y < h; y++)
                    {
                        for (x = 0; x < w; x++)
                        {
#if !AROS_BIG_ENDIAN
                        if (ReadPixel(rp, x, y) == 0) dst[x+y*w] &= 0xffffff00; else dst[x+y*w] |= 0x000000ff;
#else
                        if (ReadPixel(rp, x, y) == 0) dst[x+y*w] &= 0x00ffffff; else dst[x+y*w] |= 0xff000000;
#endif
                        }
                    }
                    FreeRastPort(rp);
                }
            }
        }
        DisposeDTObject(pic);
    }

    FreeVec(buffer);

    return ni;
}

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

/* This function must never return NULL, because logic in drawing code
   checks img->ok instead of img != NULL to make decisions */
struct NewImage * CreateNewImageContainerMatchingScreen(struct NewImage *in, BOOL truecolor, struct Screen* scr)
{
    struct NewImage * out = AllocVec(sizeof(struct NewImage), MEMF_ANY | MEMF_CLEAR);
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
            strcpy(out->filename, in->filename);
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

struct Region *RegionFromLUT8Image(int w, int h, struct NewLUT8Image *s)
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
