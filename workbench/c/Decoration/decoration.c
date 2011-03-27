/*
    Copyright  1995-2011, The AROS Development Team.
    $Id$
*/

/******************************************************************************

    NAME

        Decoration

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

        Allows user definable skins for the intuition windows, menus and gadgets.
    	
    NOTES

        Must be launched before Wanderer - usually in the S:startup-sequence
	
    BUGS

    SEE ALSO

	IPREFS
	
    INTERNALS

******************************************************************************/

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>
#include <proto/datatypes.h>

#include <dos/dos.h>
#include <intuition/classusr.h>
#include <intuition/classes.h>
#include <intuition/windecorclass.h>
#include <intuition/scrdecorclass.h>
#include <intuition/menudecorclass.h>
#include <intuition/extensions.h>
#include <intuition/intuitionbase.h>
#include <graphics/rpattr.h>
#include <graphics/gfxmacros.h>
#include <libraries/mui.h>
#include <libraries/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <utility/tagitem.h>

#include <string.h>

#include <aros/debug.h>
#include <aros/detach.h>

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

#include "windowdecorclass.h"
#include "screendecorclass.h"
#include "newimage.h"
#include "config.h" /* TODO: remove */

/**************************************************************************************************/


#define PUTIMAGE_MEN(id) data->img_##id=data->sd->img_##id

#define SETIMAGE_SCR(id) SetImage(data->img_##id, &sd->img_##id, truecolor, screen)

#define SETIMAGE_MEN(id) md->img_##id=&sd->img_##id

#define DELIMAGE_SCR(id) RemoveLUTImage(&sd->img_##id)

#if AROS_BIG_ENDIAN
#define GET_A(rgb) ((rgb >> 24) & 0xff)
#define GET_R(rgb) ((rgb >> 16) & 0xff)
#define GET_G(rgb) ((rgb >> 8) & 0xff)
#define GET_B(rgb) (rgb & 0xff)
#define SET_ARGB(a, r, g, b) a << 24 | r << 16 | g << 8 | b
#else
#define GET_A(rgb) (rgb & 0xff)
#define GET_R(rgb) ((rgb >> 8) & 0xff)
#define GET_G(rgb) ((rgb >> 16) & 0xff)
#define GET_B(rgb) ((rgb >> 24) & 0xff)
#define SET_ARGB(a, r, g, b) b << 24 | g << 16 | r << 8 | a
#endif

#define MDA_Configuration 0x10002
#define SDA_Configuration 0x20002


#define MDA_ScreenData 0x10003
#define SDA_ScreenData 0x20003


    struct IClass 	*cl, *scrcl, *menucl;





/**************************************************************************************************/






    struct menudecor_data
    {
        struct scrdecor_data *sd;

        struct DrawInfo *dri;
        struct Screen *scr;
        struct NewImage *img_menu;
        struct NewImage *img_amigakey;
        struct NewImage *img_menucheck;
        struct NewImage *img_submenu;
    };



    struct MenuData
    {
        struct  NewImage    *ni;
        struct  BitMap      *map;

        struct NewImage *img_menu;
        struct NewImage *img_amigakey;
        struct NewImage *img_menucheck;
        struct NewImage *img_submenu;
        LONG   ActivePen;
        LONG   DeactivePen;
        BOOL   truecolor;

    };




    struct myrgb
    {
        int red,green,blue;
    };

    BOOL InitWindowSkinning(STRPTR path, struct windecor_data *data);
    BOOL InitMenuSkinning(STRPTR path, struct menudecor_data *data);
    BOOL InitScreenSkinning(STRPTR path, struct scrdecor_data *data);
    void DisposeWindowSkinning(struct windecor_data *data);
    void DisposeMenuSkinning(struct menudecor_data *data);
    void DisposeScreenSkinning(struct scrdecor_data *data);

    void DrawTileToRP(struct RastPort *rp, struct NewImage *ni, ULONG col, UWORD offx, UWORD offy, UWORD x, UWORD y, WORD w, WORD h);


static Object *LoadPicture(CONST_STRPTR filename, struct Screen *scr)
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
    struct BitMapHeader *bmhd;
    
    GetDTAttrs(o,PDTA_BitMapHeader, (IPTR)&bmhd, TAG_DONE);
    
    struct FrameInfo fri = {0};
    
    D(bug("DTM_FRAMEBOX\n", o));
    DoMethod(o,DTM_FRAMEBOX,NULL,(IPTR)&fri,(IPTR)&fri,sizeof(struct FrameInfo),0);
    
    if (fri.fri_Dimensions.Depth>0)
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


    void SetImage(struct NewImage *in, struct NewImage *out, BOOL truecolor, struct Screen* scr)
    {
        out->ok = FALSE;
        if (in != NULL)
        {
            out->w = in->w;
            out->h = in->h;
            out->istiled = in->istiled;
            out->data = in->data;
            out->tile_left = in->tile_left;
            out->tile_right = in->tile_right;
            out->tile_top = in->tile_top;
            out->tile_bottom = in->tile_bottom;
            out->inner_left = in->inner_left;
            out->inner_right = in->inner_right;
            out->inner_top = in->inner_top;
            out->inner_bottom = in->inner_bottom;
            out->bitmap = NULL;
            out->mask = NULL;
            out->o = NULL;
            out->ok = (in->data != NULL) ? TRUE : FALSE;
            if (!truecolor)
            {
                out->ok = FALSE;
                STRPTR file = AllocVec(strlen(in->filename) + 5, MEMF_ANY | MEMF_CLEAR);
                if (file != NULL)
                {
                    strcpy(file, in->filename);
                    strcat(file, "_LUT");
                    out->o = LoadPicture(file, scr);
                    FreeVec(file);
                }
                out->filename = in->filename; 
                if (out->o == NULL) out->o = LoadPicture(in->filename, scr);
                if (out->o)
                {
                        GetDTAttrs(out->o, PDTA_DestBitMap, (IPTR)&out->bitmap, TAG_DONE);
                        if (out->bitmap == NULL) GetDTAttrs(out->o, PDTA_BitMap, (IPTR)&out->bitmap, TAG_DONE);

                        if (out->bitmap) GetDTAttrs(out->o, PDTA_MaskPlane, (IPTR)&out->mask, TAG_DONE);
                        if (out->bitmap == NULL)
                        {
                            DisposeDTObject(out->o);
                            out->o = NULL;
                        } else out->ok = TRUE;
                }
            }
        }
    }

void RemoveLUTImage(struct NewImage *ni)
{
    if (ni)
    {
        if (ni->ok)
        {
            if (ni->o) DisposeDTObject(ni->o);
            ni->o = NULL;
            ni->bitmap = NULL;
            ni->mask = NULL;
        }
    }
}

void DrawAlphaStateImageToRP(struct windecor_data *data, struct RastPort *rp, struct NewImage *ni, ULONG state, UWORD xp, UWORD yp, BOOL multiple)
{

    UWORD   ix, iy, dx;
    UBYTE  *d;

    int images = (data == NULL) ? 2 : data->threestate ? 3 : 4;

    if (ni->ok)
    {
        dx = 0;
        d = (UBYTE *) ni->data;
        ix=ni->w;
        iy=ni->h;
        if (multiple)
        {
            switch(state)
            {
                case IDS_NORMAL:
                    break;
                case IDS_SELECTED:
                    dx += ix / images;
                    d += ix / images * 4;
                    break;
                case IDS_INACTIVENORMAL:
                    dx += ix / images * 2;
                    d += ix / images * 8;
                    break;
            }
        }
        else
        images = 1;

        if (ni->bitmap == NULL)
        {
            WritePixelArrayAlpha(d, 0 , 0, ix*4, rp, xp, yp, ix / images, iy, 0xffffffff);
        }
        else
        {
            if (ni->mask)
            {
                BltMaskBitMapRastPort(ni->bitmap, dx, 0, rp, xp, yp, ix / images, iy, 0xe0, (PLANEPTR) ni->mask);  
            }
            else BltBitMapRastPort(ni->bitmap, dx, 0, rp, xp, yp, ix / images, iy, 0xc0);
        }
    }
}

void DrawPartImageToRP(struct RastPort *rp, struct NewImage *ni, UWORD x, UWORD y, UWORD sx, UWORD sy, UWORD sw, UWORD sh)
{
    if (ni->ok)
    {
        if (ni->bitmap == NULL)
        {
            WritePixelArray(ni->data, sx, sy, ni->w*4, rp, x, y, sw, sh, RECTFMT_ARGB);
        }
        else
        {
            BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, y, sw, sh, 0xc0);
        }
    }
}

void DisposeImageContainer(struct NewImage *ni)
{
    if (ni)
    {
        if (ni->data)
        {
            FreeVec(ni->data);
        }
        if (ni->o) DisposeDTObject(ni->o);
        if (ni->filename) FreeVec(ni->filename);
        FreeVec(ni);
    }
}

void DisposeLUT8ImageContainer(struct NewLUT8Image *ni)
{
    if (ni)
    {
        if (ni->data)
        {
            FreeVec(ni->data);
        }
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
        ni->data = AllocVec(w * h * sizeof (ULONG), MEMF_ANY | MEMF_CLEAR);
        if (ni->data == NULL)
        {
            FreeVec(ni);
            ni = NULL;
        }
    }
    return ni;
}

struct NewImage *GetImageFromFile(STRPTR path, STRPTR name, BOOL fixmode)
{
    struct	BitMapHeader       *bmhd = NULL;
    struct	NewImage           *ni = NULL;
    struct  BitMap             *map = NULL;
    struct  RastPort           *rp = NULL;
    Object                     *pic;
    struct	pdtBlitPixelArray   pa;
    char                        Buffer[256];
    UWORD                       w, h, tc, x, y;
    UBYTE                       mask;
    ULONG                       a;
    ULONG   *dst;

    if (fixmode)
    {
        strcpy(Buffer, name);
        strcat(Buffer, "Default");
    }
    else
    strcpy(Buffer, name);

    pic = NewDTObject(Buffer,  DTA_SourceType,  DTST_FILE,
                               DTA_GroupID,     GID_PICTURE,
                               PDTA_Remap,      FALSE,
                               PDTA_DestMode,   PMODE_V43,
                               TAG_DONE);
    if (pic)
    {

        get(pic, PDTA_BitMapHeader, &bmhd);
        if(bmhd)
        {

            w = bmhd->bmh_Width;
            h = bmhd->bmh_Height;
            mask = bmhd->bmh_Masking;
            ni = NewImageContainer(w, h);
            if (ni)
            {
                int len = strlen(path) + strlen(Buffer) +2;
                ni->filename = AllocVec(len, MEMF_CLEAR | MEMF_ANY);
                if (ni->filename != NULL)
                {
                    strncpy(ni->filename, path, len);
                    AddPart(ni->filename, Buffer, len);

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
                        get(pic, PDTA_BitMap, &map);
                        if (map && (mask == mskHasTransparentColor))
                        {
                            rp = CreateRastPort();
                            if (rp) rp->BitMap = map;
                            tc = bmhd->bmh_Transparent;
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
                    else
                    {
    
                        if (mask != mskHasAlpha)
                        {
    #if !AROS_BIG_ENDIAN
                            for (a= 0; a < (w*h); a++) ni->data[a] |= 0x000000ff;
    #else
                            for (a= 0; a < (w*h); a++) ni->data[a] |= 0xff000000;
    #endif
                        }
                    }
                }
                else 
                {
                    DisposeImageContainer(ni);
                    ni = NULL;
                }
            }
        }

        DisposeDTObject(pic);
    }

    return ni;
}

struct NewImage *GetImageFromRP(struct RastPort *rp, UWORD x, UWORD y, UWORD w, UWORD h)
{
    struct NewImage *ni;

    ni = NewImageContainer(w, h);
    if (ni)
    {
        ReadPixelArray(ni->data, 0, 0, w*4, rp, x, y, w, h, RECTFMT_ARGB);
    }
    return ni;
}

void PutImageToRP(struct RastPort *rp, struct NewImage *ni, UWORD x, UWORD y) {

    if (ni)
    {
        if (ni->data) WritePixelArray(ni->data, 0, 0, ni->w*4, rp, x, y, ni->w, ni->h, RECTFMT_ARGB);
        DisposeImageContainer(ni);
    }
}

/* the following code is taken from zune sources and modified */

void FillPixelArrayGradientDelta(LONG pen, BOOL tc, struct RastPort *rp, int xt, int yt, int xb, int yb, int xp, int yp, int w, int h, ULONG start_rgb, ULONG end_rgb, int angle, int dx, int dy)
{
    
    /* The basic idea of this algorithm is to calc the intersection between the
    * diagonal of the rectangle (xs,ys) with dimension (xw,yw) a with the line starting
    * at (x,y) (every pixel inside the rectangle) and angle angle with direction vector (vx,vy).
    *
    * Having the intersection point we then know the color of the pixel.
    *
    * TODO: Turn the algorithm into a incremental one
    *       Remove the use of floating point variables
    */
    double rad = angle*M_PI/180;
    double cosarc = cos(rad);
    double sinarc = sin(rad);

    struct myrgb startRGB,endRGB;

    int diffR, diffG, diffB;

    int r,t; /* some helper variables to short the code */
    int l,y,c,x;
    int y1; /* The intersection point */
    int incr_y1; /* increment of y1 */
    int xs,ys,xw,yw;
    int xadd,ystart,yadd;
//    double vx = -cosarc;
//    double vy = sinarc;
    int vx = (int)(-cosarc*0xff);
    int vy = (int)(sinarc*0xff);
    
    int width = xb - xt + 1;
    int height = yb - yt + 1;
    
    if ((w <= 0) || (h <= 0)) return;
    if (!tc)
    {
        if (pen != -1) SetAPen(rp, pen); else SetAPen(rp, 2);
        RectFill(rp, xp, yp, xp + w - 1, yp + h - 1);
        return;
    }

    UBYTE *buf = AllocVec(w*h*3, 0);
    if (buf == NULL) return;
    startRGB.red = (start_rgb >> 16) & 0xff;
    startRGB.green = (start_rgb >> 8) & 0xff;
    startRGB.blue = start_rgb & 0xff;

    endRGB.red = (end_rgb >> 16) & 0xff;
    endRGB.green = (end_rgb >> 8) & 0xff;
    endRGB.blue = end_rgb & 0xff;

    diffR = endRGB.red - startRGB.red;
    diffG = endRGB.green - startRGB.green;
    diffB = endRGB.blue - startRGB.blue;

    /* Normalize the angle */
    if (angle < 0) angle = 360 - ((-angle)%360);
    if (angle >= 0) angle = angle % 360;

    if (angle <= 90 || (angle > 180 && angle <= 270))
    {
        /* The to be intersected diagonal goes from the top left edge to the bottom right edge */
        xs = 0;
        ys = 0;
        xw = width;
        yw = height;
    } else
    {
        /* The to be intersected diagonal goes from the bottom left edge to the top right edge */
        xs = 0;
        ys = height;
        xw = width;
        yw = -height;
    }
		
    if (angle > 90 && angle <= 270)
    {
	/* for these angle we have y1 = height - y1. Instead of
        *
        *  y1 = height - (-vy*(yw*  xs -xw*  ys)         + yw*(vy*  x -vx*  y))        /(-yw*vx + xw*vy);
        *
        * we can write
        *
        *  y1 =          (-vy*(yw*(-xs)-xw*(-ys+height)) + yw*(vy*(-x)-vx*(-y+height)))/(-yw*vx + xw*vy);
        *
        * so height - y1 can be expressed with the normal formular adapting some parameters.
        *
        * Note that if one would exchanging startRGB/endRGB the values would only work
        * for linear color gradients
 */
        xadd = -1;
        yadd = -1;
        ystart = height;

        xs = -xs;
        ys = -ys + height;
    }
    else
    {
        xadd = 1;
        yadd = 1;
        ystart = 0;
    }

    r = -vy*(yw*xs-xw*ys);
    t = -yw*vx + xw*vy;

    /* The formular as shown above is
    *
    * 	 y1 = ((-vy*(yw*xs-xw*ys) + yw*(vy*x-vx*y)) /(-yw*vx + xw*vy));
    *
    * We see that only yw*(vy*x-vx*y) changes during the loop.
    *
    * We write
    *
    *   Current Pixel: y1(x,y) = (r + yw*(vy*x-vx*y))/t = r/t + yw*(vy*x-vx*y)/t
    *   Next Pixel:    y1(x+xadd,y) = (r + vw*(vy*(x+xadd)-vx*y))/t
    *
    *   t*(y1(x+xadd,y) - y1(x,y)) = yw*(vy*(x+xadd)-vx*y) - yw*(vy*x-vx*y) = yw*vy*xadd;
    *
    */

    incr_y1 = yw*vy*xadd;
    UBYTE *bufptr = buf;
    for (l = 0, y = ystart + ((yp - yt)* yadd); l < h; l++, y+=yadd)
    {

	/* Calculate initial y1 accu, can be brought out of the loop as well (x=0). It's probably a
        * a good idea to add here also a value of (t-1)/2 to ensure the correct rounding
        * This (and for r) is also a place were actually a overflow can happen |yw|=16 |y|=16. So for
 * vx nothing is left, currently 9 bits are used for vx or vy */
        int y1_mul_t_accu = r - yw*vx*y;


       
        for (c = 0, x = ((xp - xt) * xadd); c < w; c++, x+=xadd)
        {
            int red,green,blue;

	    /* Calculate the intersection of two lines, this is not the fastet way to do but
            * it is intuitive. Note: very slow! Will be optimzed later (remove FFP usage
            * and making it incremental)...update: it's now incremental and no FFP is used
            * but it probably can be optimized more by removing some more of the divisions and
     * further specialize the stuff here (use of three accus). */
            /*	    y1 = (int)((-vy*(yw*xs-xw*ys) + yw*(vy*x-vx*y)) /(-yw*vx + xw*vy));*/
            y1 = y1_mul_t_accu / t;
					
            red = startRGB.red + (int)(diffR*y1/height);
            green = startRGB.green + (int)(diffG*y1/height);
            blue = startRGB.blue + (int)(diffB*y1/height);
            /* By using full 32 bits this can be made faster as well */
            *bufptr++ = red;
            *bufptr++ = green;
            *bufptr++ = blue;

            y1_mul_t_accu += incr_y1;
        }
	/* By bringing building the gradient array in the same format as the RastPort BitMap a call
 * to WritePixelArray() can be made also faster */
    }
    WritePixelArray(buf,0,0,w*3, rp,dx,dy,w,h,RECTFMT_RGB);
    FreeVec(buf);
}

void FillPixelArrayGradient(LONG pen, BOOL tc, struct RastPort *rp, int xt, int yt, int xb, int yb, int xp, int yp, int w, int h, ULONG start_rgb, ULONG end_rgb, int angle)
{
    FillPixelArrayGradientDelta(pen, tc, rp, xt, yt, xb, yb, xp, yp, w, h, start_rgb, end_rgb, angle, xp, yp);
}


void DrawPartToImage(struct NewImage *src, struct NewImage *dest, UWORD sx, UWORD sy, UWORD sw, UWORD sh, UWORD dx, UWORD dy)
{
    UWORD   x, y;

    for (y = 0; y < sh; y++)
    {
        for (x = 0; x < sw; x++)
        {
            dest->data[dx  + x + (dy + y) * dest->w] = src->data[sx + x + (sy + y) * src->w];
        }
    }
}

void DrawTileToImage(struct NewImage *src, struct NewImage *dest, UWORD _sx, UWORD _sy, UWORD _sw, UWORD _sh, UWORD _dx, UWORD _dy, UWORD _dw, UWORD _dh)
{

    ULONG   dy, dx;
    LONG    dh, height, dw, width;

    if (src == NULL) return;
    if (dest == NULL) return;

    dh = _sh;
    dy = _dy;
    height = _dh;

    while (height > 0)
    {
        if ((height-dh)<0) dh = height;
        height -= dh;
        dw = _sw;
        width = _dw;
        dx = _dx;
        while (width > 0)
        {
            if ((width-dw)<0) dw = width;
            width -= dw;
            DrawPartToImage(src, dest, _sx, _sy, dw, dh, dx, dy);
            dx += dw;
        }
        dy += dh;
    }
}

void DrawMapTileToRP(struct NewImage *src, struct RastPort *rp, UWORD _sx, UWORD _sy, UWORD _sw, UWORD _sh, UWORD _dx, UWORD _dy, UWORD _dw, UWORD _dh)
{

    ULONG   dy, dx;
    LONG    dh, height, dw, width;

    if (src == NULL) return;
    if (rp == NULL) return;

    dh = _sh;
    dy = _dy;
    height = _dh;

    if (!src->ok) return;

    while (height > 0)
    {
        if ((height-dh)<0) dh = height;
        height -= dh;
        dw = _sw;
        width = _dw;
        dx = _dx;
        while (width > 0)
        {
            if ((width-dw)<0) dw = width;
            width -= dw;

            if (src->mask)
            {
                BltMaskBitMapRastPort(src->bitmap, _sx, _sy, rp, dx, dy, dw, dh, 0xe0, (PLANEPTR) src->mask);  
            }
            else BltBitMapRastPort(src->bitmap, _sx, _sy, rp, dx, dy, dw, dh, 0xc0);

            dx += dw;
        }
        dy += dh;
    }
}
/**************************************************************************************************/







/**************************************************************************************************/

/**************************************************************************************************/



/**************************************************************************************************/



/**************************************************************************************************/

void ShadeLine(LONG pen, BOOL tc, struct windecor_data *d, struct RastPort *rp, struct NewImage *ni, ULONG basecolor, UWORD fact, UWORD _offy, UWORD x0, UWORD y0, UWORD x1, UWORD y1)
{
    int px, py, x, y;
    ULONG   c;
    int     c0, c1, c2, c3;
    UWORD   offy = 0;

    if ((x1 < x0) || (y1 < y0)) return;
    if (!tc)
    {
        SetAPen(rp, pen);
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
        return;
    }
    if (d->usegradients)
    {
        c = basecolor;
        c3 = (c >> 24) & 0xff;
        c2 = (c >> 16) & 0xff;
        c1 = (c >> 8) & 0xff;
        c0 = c & 0xff;
        c0 *= fact;
        c1 *= fact;
        c2 *= fact;
        c3 *= fact;
        c0 = c0 >> 8;
        c1 = c1 >> 8;
        c2 = c2 >> 8;
        c3 = c3 >> 8;
        if (c0 > 255) c0 = 255;
        if (c1 > 255) c1 = 255;
        if (c2 > 255) c2 = 255;
        if (c3 > 255) c3 = 255;
        c = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
        SetRPAttrs(rp, RPTAG_FgColor, c, TAG_DONE);
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
    }
    else if (ni->ok)
    {
        if (x0 == x1)
        {
            x = x0 % ni->w; 
            for (py = y0; py < y1; py++)
            {
                y = (py - offy) % ni->h;
                c = ni->data[x + y * ni->w];
                c0 = (c >> 24) & 0xff;
                c1 = (c >> 16) & 0xff;
                c2 = (c >> 8) & 0xff;
                c3 = c & 0xff;
                c0 *= fact;
                c1 *= fact;
                c2 *= fact;
                c3 *= fact;
                c0 = c0 >> 8;
                c1 = c1 >> 8;
                c2 = c2 >> 8;
                c3 = c3 >> 8;
                if (c0 > 255) c0 = 255;
                if (c1 > 255) c1 = 255;
                if (c2 > 255) c2 = 255;
                if (c3 > 255) c3 = 255;
                c = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
                WriteRGBPixel(rp, x0, py, c);
            }
        } else {
            y = (y0 - offy) % ni->h;
            for (px = x0; px < x1; px++) {
                x = px % ni->h;
                c = ni->data[x + y * ni->w];
                c0 = (c >> 24) & 0xff;
                c1 = (c >> 16) & 0xff;
                c2 = (c >> 8) & 0xff;
                c3 = c & 0xff;
                c0 *= fact;
                c1 *= fact;
                c2 *= fact;
                c3 *= fact;
                c0 = c0 >> 8;
                c1 = c1 >> 8;
                c2 = c2 >> 8;
                c3 = c3 >> 8;
                if (c0 > 255) c0 = 255;
                if (c1 > 255) c1 = 255;
                if (c2 > 255) c2 = 255;
                if (c3 > 255) c3 = 255;
                c = (c3 << 24) | (c2 << 16) | (c1 << 8) | c0;
                WriteRGBPixel(rp, px, y0, c);
            }
        }
    }
    else
    {
        Move(rp, x0, y0);
        Draw(rp, x1, y1);
    }
}

int WriteTiledImage(struct Window *win, struct RastPort *rp, struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp, int dw, int dh)
{
    int     w = dw;
    int     x = xp;
    int     ddw;

    if (!ni->ok) return xp;

    if ((sw == 0) || (dw == 0)) return xp;
    if (win)
    {
        if (x > win->Width) return xp;
        if ((x + w) > win->Width) w = win->Width - x;
    }

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;
        if (ni->bitmap == NULL)
        {
            WritePixelArrayAlpha(ni->data, sx , sy, ni->w*4, rp, x, yp, ddw, dh, 0xffffffff);
        }
        else
        {
            if (ni->mask)
            {
                BltMaskBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xe0, (PLANEPTR) ni->mask);  
            }
            else BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xc0);
        }
        w -= ddw;
        x += ddw;
    }
    return x;
}

int WriteTiledImageNoAlpha(struct Window *win, struct RastPort *rp, struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp, int dw, int dh)
{
    int     w = dw;
    int     x = xp;
    int     ddw;

    if (!ni->ok) return x;

    if ((sw == 0) || (dw == 0)) return xp;

    if (win)
    {
        if (x > win->Width) return xp;
        if ((x + w) > win->Width) w = win->Width - x;
    }

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;
        if (ni->bitmap == NULL)
        {
            WritePixelArray(ni->data, sx , sy, ni->w*4, rp, x, yp, ddw, dh, RECTFMT_ARGB);
        }
        else
        {
            BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xc0);
        }
        w -= ddw;
        x += ddw;
    }
    return x;
}


void WriteAlphaPixelArray(struct NewImage *src, struct NewLUT8Image *dst, int sx, int sy, int dx, int dy, int w, int h)
{
    ULONG  *s = src->data;
    ULONG   argb;
    UBYTE  *d = dst->data;
    int     x, y;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            argb = s[sx + x + (sy + y) * src->w];
            d[dx + x + (dy + y) * dst->w] = GET_A(argb);
        }
    }
}

int WriteTiledImageTitle(BOOL fill, struct Window *win, struct RastPort *rp, struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp, int dw, int dh)
{
    int     w = dw;
    int     x = xp;
    int     ddw;

    if (!ni->ok) return x;

    if (!fill) return WriteTiledImageNoAlpha(win, rp, ni, sx, sy, sw, sh, xp, yp, dw, dh);

    if ((sw == 0) || (dw == 0)) return xp;

    if (win)
    {
        if (x > win->Width) return xp;
        if ((x + w) > win->Width) w = win->Width - x;
    }

    while (w > 0)
    {
        ddw = sw;
        if (w < ddw) ddw = w;
        if (ni->bitmap == NULL)
        {
            if (fill) WritePixelArrayAlpha(ni->data, sx, sy, ni->w*4, rp, x, yp, ddw, dh, 0xffffffff); //RECTFMT_ARGB);
            else WritePixelArray(ni->data, sx, sy, ni->w*4, rp, x, yp, ddw, dh, RECTFMT_ARGB);

        }
        else
        {
            if (fill)
            {
                if (ni->mask)
                {
                    BltMaskBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xe0, (PLANEPTR) ni->mask);  
                }
                else BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xc0);
            }
            else
            {
                BltBitMapRastPort(ni->bitmap, sx, sy, rp, x, yp, ddw, dh, 0xc0);
            }
        }
        w -= ddw;
        x += ddw;
    }
    return x;
}



int WriteTiledImageHorizontal(struct RastPort *rp, struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp, int dw, int dh)
{
    return WriteTiledImage(NULL, rp, ni, sx, sy, sw, sh, xp, yp, dw, dh);
}

int WriteTiledImageVertical(struct RastPort *rp, struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp, int dw, int dh)
{
    int     h = dh;
    int     y = yp;
    int     ddh;

    if (!ni->ok) return y;

    if ((sh == 0) || (dh == 0)) return yp;

    while (h > 0)
    {
        ddh = sh;
        if (h < ddh) ddh = h;
        if (ni->bitmap == NULL)
        {
            WritePixelArrayAlpha(ni->data, sx , sy, ni->w*4, rp, xp, y, dw, ddh, 0xffffffff);
        }
        else
        {
            if (ni->mask)
            {
                BltMaskBitMapRastPort(ni->bitmap, sx, sy, rp, xp, y, dw, ddh, 0xe0, (PLANEPTR) ni->mask);  
            }
            else BltBitMapRastPort(ni->bitmap, sx, sy, rp, xp, y, dw, ddh, 0xc0);
        }
        h -= ddh;
        y += ddh;
    }
    return y;
}



void DrawTileToRP(struct RastPort *rp, struct NewImage *ni, ULONG color, UWORD offx, UWORD offy, UWORD x, UWORD y, WORD w, WORD h)
{

    ULONG   ow, oh, sy, sx, dy, dx;
    LONG    dh, height, dw, width;

    if ((w <= 0) || (h <= 0)) return;

    if (ni == NULL)
    {
        FillPixelArray(rp, x, y, w, h, color);
        return;
    }

    ow = ni->w;
    oh = ni->h;

    sy = (y - offy )% oh;
    dh = oh - sy;
    height = h;
    dy = y;
    while (height > 0)
    {
        if ((height-dh)<0) dh = height;
        height -= dh;

        sx = (x - offx) % ow;
        dw = ow - sx;
        width = w;
        dx = x;
        while (width > 0)
        {
            if ((width-dw)<0) dw = width;
            width -= dw;
            if (ni->bitmap == NULL)
            {
                WritePixelArray(ni->data, sx, sy, ni->w*4, rp, dx, dy, dw, dh, RECTFMT_ARGB);
            }
            else
            {
                BltBitMapRastPort(ni->bitmap, sx, sy, rp, dx, dy, dw, dh, 0xc0);
            }
            dx += dw;
            sx = 0;
            dw = ow;
        }
        dy += dh;
        sy = 0;
        dh = oh;
    }
}

void DrawTileToRPRoot(struct RastPort *rp, struct NewImage *ni, ULONG color, UWORD offx, UWORD offy, UWORD x, UWORD y, WORD w, WORD h)
{

    ULONG   ow, oh, sy, sx, dy, dx, _dy, _dx;
    LONG    dh, height, dw, width;

    if (!ni->ok) return;

    if ((w <= 0) || (h <= 0)) return;

    if (!ni->ok)
    {
        FillPixelArray(rp, x, y, w, h, color);
        return;
    }

    ow = ni->w;
    oh = ni->h;

    _dy = 0;

    sy = (y - offy )% oh;
    dh = oh - sy;
    height = h;
    dy = y;
    while (height > 0)
    {
        if ((height-dh)<0) dh = height;
        height -= dh;

        sx = (x - offx) % ow;
        dw = ow - sx;
        width = w;
        dx = x;
        _dx = 0;
        while (width > 0)
        {
            if ((width-dw)<0) dw = width;
            width -= dw;
            if (ni->bitmap == NULL)
            {
                WritePixelArray(ni->data, sx, sy, ni->w*4, rp, _dx, _dy, dw, dh, RECTFMT_ARGB);
            }
            else
            {
                BltBitMapRastPort(ni->bitmap, sx, sy, rp, _dx, _dy, dw, dh, 0xc0);
            }
            dx += dw;
            _dx += dw;
            sx = 0;
            dw = ow;
        }
        dy += dh;
        _dy += dh;
        sy = 0;
        dh = oh;
    }
}

void  SetImageTint(struct NewImage *dst, UWORD ratio, ULONG argb)
{

    ULONG  *d;
    ULONG   rgb;
    UWORD   r, g, b, w, h;
    UBYTE   rs, gs, bs, rd, gd, bd;
    int     x, y;

    if (dst == NULL) return;
   
    d = dst->data;

    w = dst->w;
    h = dst->h;
   
    rs = (argb >> 16) & 0xff;
    gs = (argb >> 8) & 0xff;
    bs = argb & 0xff;
   
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            rgb = d[x + y * w];
            rd = GET_R(rgb);
            gd = GET_G(rgb);
            bd = GET_B(rgb);
            r = ((rs * ratio) >> 8) + ((rd * (255 - ratio)) >> 8);
            g = ((gs * ratio) >> 8) + ((gd * (255 - ratio)) >> 8);
            b = ((bs * ratio) >> 8) + ((bd * (255 - ratio)) >> 8);

            r = r & 0xff;
            g = g & 0xff;
            b = b & 0xff;

            d[x + y * w] = SET_ARGB(255, r, g, b);
        }
    }
}

void  MixImage(struct NewImage *dst, struct NewImage *src, UWORD ratio, UWORD w, UWORD h, UWORD dx, UWORD dy)
{
    ULONG  *s, *d;
    ULONG   rgba, rgb;
    UWORD   r, g, b;
    UBYTE   as, rs, gs, bs, rd, gd, bd;
    BOOL    tiled = FALSE;
    int     x, y;

    if (src == NULL) return;
    if (dst == NULL) return;

    s = src->data;
    d = dst->data;

    if (src) if (src->istiled) tiled = TRUE;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            rgba = s[x+y*src->w];
            as = GET_A(rgba);
            rs = GET_R(rgba);
            gs = GET_G(rgba);
            bs = GET_B(rgba);
            rgb = d[x+dx + (y+dy) * dst->w];

            rd = GET_R(rgb);
            gd = GET_G(rgb);
            bd = GET_B(rgb);

            r = ((rs * ratio) >> 8) + ((rd * (255 - ratio)) >> 8);
            g = ((gs * ratio) >> 8) + ((gd * (255 - ratio)) >> 8);
            b = ((bs * ratio) >> 8) + ((bd * (255 - ratio)) >> 8);

            if (tiled) {
                r = ((r * as) >> 8) + ((rd * (255 - as)) >> 8);
                g = ((g * as) >> 8) + ((gd * (255 - as)) >> 8);
                b = ((b * as) >> 8) + ((bd * (255 - as)) >> 8);
            }

            r = r & 0xff;
            g = g & 0xff;
            b = b & 0xff;

            d[x+dx + (y+dy) * dst->w] = SET_ARGB(as, r, g, b);
        }
    }
}

void BlurSourceAndMixTexture(struct NewImage *pic, struct NewImage *texture, UWORD ratio)
{
    int     x, y, ytw, t1, t2, b1, b2, l1, l2, r1, r2;
    UWORD   red, green, blue, alpha= 0, rs, gs, bs, as;
    ULONG   rgb, argb;
    int     width, w, height, ah, aw, xpos, ypos;
    BOOL    tiled = FALSE;
    ULONG  *raw, tw, th;

    if (pic == NULL) return;
    if (pic->data == NULL) return;

    tw = pic->w;
    if (texture) if (texture->istiled) tiled = TRUE;
    th = pic->h;
    raw = pic->data;
    height = th;
    width = tw;

    if (raw)
    {
        for (y = 0; y < th; y++)
        {
            t1 = tw;
            t2 = tw+tw;
            b1 = tw;
            b2 = tw+tw;
            if (y == 0) t1 = t2 = 0;
            else if (y == 1) t2 = t1;

            if (y == (th-1)) b1 = b2 = 0;
            else if (y == (th-2)) b2 = b1;

            ytw = y*tw;

            for (x = 0; x < tw; x++)
            {
                r1 = 1;
                r2 = 2;
                l1 = 1;
                l2 = 2;

                if (x == 0) l1 = r1 = 0;
                else if (x == 1) l2 = l1;

                if (x == (tw-1)) r1 = r2 = 0;
                else if (x == (tw-2)) r2 = r1;

                rgb = raw[x+ytw];
                red = GET_R(rgb);
                green = GET_G(rgb);
                blue = GET_B(rgb);

                rgb = raw[x+ytw-t2];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-l1-t1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-t1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-t1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-t1+r1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-l2];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw-l1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+r1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+r2];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+b1-l1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+b1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+b1+r1];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                rgb = raw[x+ytw+b2];
                red += GET_R(rgb);
                green += GET_G(rgb);
                blue += GET_B(rgb);

                red = red/14;
                green = green/14;
                blue = blue/14;
                alpha = 255;

                if (tiled)
                {
                    argb = raw[x+ytw];
                    as = 255 - GET_A(texture->data[x + y * texture->w]);
                    rs = GET_R(argb);
                    gs = GET_G(argb);
                    bs = GET_B(argb);

                    red = ((rs * as) >> 8) + ((red * (255 - as)) >> 8);
                    green = ((gs * as) >> 8) + ((green * (255 - as)) >> 8);
                    blue = ((bs * as) >> 8) + ((blue * (255 - as)) >> 8);

                    raw[x+ytw] = SET_ARGB(as, red, green, blue);

                }
                else
                {
                    raw[x+ytw] = SET_ARGB(alpha, red, green, blue);
                }
            }
        }
    }
    if (ratio < 100)
    {
        if (texture)
        {
            ypos = 0;
            while (height>0)
            {
                ah = texture->h;
                if (ah > height) ah = height;
                xpos = 0;
                w = width;
                while (w>0)
                {
                    aw = texture->w;
                    if (aw > w) aw = w;
                    MixImage(pic, texture, 255 - (2.55 * ratio), aw, ah, xpos, ypos);
                    w -= aw;
                    xpos += aw;
                }
                height -= ah;
                ypos += ah;
            }
        }
    }
}

void TileImageToImage(struct NewImage *src, struct NewImage *dest)
{
    ULONG  *s, *d;
    UWORD   y, h;

    if (dest == NULL) return;
    if (src == NULL) return;
    y = 0;

    h = src->h;

    s = src->data;
    d = dest->data;
    if (src->istiled == FALSE) return;
    dest->istiled = TRUE;

    if ((src->tile_top + src->tile_bottom) > dest->h) return;
    if ((src->tile_left + src->tile_right) > dest->w) return;

    DrawTileToImage(src, dest, 0, y, src->tile_left, src->tile_top, 0 , 0, src->tile_left, src->tile_top);
    DrawTileToImage(src, dest, 0, y + h - src->tile_bottom, src->tile_left, src->tile_bottom, 0 , dest->h - src->tile_bottom, src->tile_left, src->tile_bottom);
    DrawTileToImage(src, dest, src->w - src->tile_right, y, src->tile_right, src->tile_top, dest->w - src->tile_right, 0, src->tile_right, src->tile_top);
    DrawTileToImage(src, dest, src->w - src->tile_right, y + h - src->tile_bottom, src->tile_right, src->tile_bottom, dest->w - src->tile_right , dest->h - src->tile_bottom, src->tile_right, src->tile_bottom);

    DrawTileToImage(src, dest, src->tile_left, y, src->w - src->tile_left - src->tile_right, src->tile_top, src->tile_left, 0, dest->w - src->tile_left - src->tile_right, src->tile_top);
    DrawTileToImage(src, dest, src->tile_left, y + h - src->tile_bottom, src->w - src->tile_left - src->tile_right, src->tile_bottom, src->tile_left, dest->h - src->tile_bottom, dest->w - src->tile_left - src->tile_right, src->tile_bottom);
    DrawTileToImage(src, dest, 0, y + src->tile_top, src->tile_left, h - src->tile_bottom - src->tile_top, 0 , src->tile_top + 0, src->tile_left, dest->h - src->tile_top - src->tile_bottom - 0);
    DrawTileToImage(src, dest, src->w - src->tile_right, y + src->tile_top, src->tile_right,  h - src->tile_bottom - src->tile_top, dest->w - src->tile_right, src->tile_top + 0, src->tile_right, dest->h - src->tile_top - src->tile_bottom - 0);
    DrawTileToImage(src, dest, src->tile_left, y + src->tile_top, src->w - src->tile_left - src->tile_right, h - src->tile_bottom - src->tile_top, src->tile_left, src->tile_top + 0, dest->w - src->tile_left - src->tile_right, dest->h - src->tile_top - src->tile_bottom - 0);
}

void TileMapToBitmap(struct NewImage *src, struct BitMap *map, UWORD dw, UWORD dh)
{
    UWORD   y, h;

    if (map == NULL) return;
    if (src == NULL) return;
    y = 0;

    h = src->h;

    if (src->istiled == FALSE) return;

    if ((src->tile_top + src->tile_bottom) > dh) return;
    if ((src->tile_left + src->tile_right) > dw) return;

    struct RastPort *dest = CreateRastPort();

    if (dest != NULL)
    {
        dest->BitMap = map;

        DrawMapTileToRP(src, dest, 0, y, src->tile_left, src->tile_top, 0 , 0, src->tile_left, src->tile_top);
        DrawMapTileToRP(src, dest, 0, y + h - src->tile_bottom, src->tile_left, src->tile_bottom, 0 , dh - src->tile_bottom, src->tile_left, src->tile_bottom);
        DrawMapTileToRP(src, dest, src->w - src->tile_right, y, src->tile_right, src->tile_top, dw - src->tile_right, 0, src->tile_right, src->tile_top);
        DrawMapTileToRP(src, dest, src->w - src->tile_right, y + h - src->tile_bottom, src->tile_right, src->tile_bottom, dw - src->tile_right , dh - src->tile_bottom, src->tile_right, src->tile_bottom);

        DrawMapTileToRP(src, dest, src->tile_left, y, src->w - src->tile_left - src->tile_right, src->tile_top, src->tile_left, 0, dw - src->tile_left - src->tile_right, src->tile_top);
        DrawMapTileToRP(src, dest, src->tile_left, y + h - src->tile_bottom, src->w - src->tile_left - src->tile_right, src->tile_bottom, src->tile_left, dh - src->tile_bottom, dw - src->tile_left - src->tile_right, src->tile_bottom);
        DrawMapTileToRP(src, dest, 0, y + src->tile_top, src->tile_left, h - src->tile_bottom - src->tile_top, 0 , src->tile_top + 0, src->tile_left, dh - src->tile_top - src->tile_bottom - 0);
        DrawMapTileToRP(src, dest, src->w - src->tile_right, y + src->tile_top, src->tile_right,  h - src->tile_bottom - src->tile_top, dw - src->tile_right, src->tile_top + 0, src->tile_right, dh - src->tile_top - src->tile_bottom - 0);
        DrawMapTileToRP(src, dest, src->tile_left, y + src->tile_top, src->w - src->tile_left - src->tile_right, h - src->tile_bottom - src->tile_top, src->tile_left, src->tile_top + 0, dw - src->tile_left - src->tile_right, dh - src->tile_top - src->tile_bottom - 0);
        FreeRastPort(dest);
    }
}

void RenderBackgroundTiled(struct NewImage *pic, struct NewImage *texture, UWORD ratio)
{
    struct NewImage *ni;

    if (texture)
    {
        ni = NewImageContainer(pic->w, pic->h);
        if (ni)
        {
            if (texture->istiled)
            {
                TileImageToImage(texture, ni);
                BlurSourceAndMixTexture(pic, ni, ratio);
            }
            else BlurSourceAndMixTexture(pic, texture, ratio);

            DisposeImageContainer(ni);
        }
        else BlurSourceAndMixTexture(pic, texture, ratio);
    }
    else BlurSourceAndMixTexture(pic, NULL, ratio);
}

void RenderBackground(struct NewImage *pic, struct NewImage *texture, UWORD ratio)
{
    if (texture)
    {
        if (texture->istiled) RenderBackgroundTiled(pic, texture, ratio); else BlurSourceAndMixTexture(pic, texture, ratio);
    }
    else BlurSourceAndMixTexture(pic, NULL, ratio);
}

/**************************************************************************************************/



/**************************************************************************************************/




/**************************************************************************************************/





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








static IPTR scrdecor_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct scrdecor_data   *data;
    UWORD                   barh;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);

    if (obj)
    {
        data = INST_DATA(cl, obj);
        STRPTR path = (STRPTR) GetTagData(SDA_Configuration, (IPTR) "Theme:", msg->ops_AttrList);

        if (!InitScreenSkinning(path, data))
        {
            CoerceMethod(cl,obj,OM_DISPOSE);
            obj = NULL;
        }
        else
        {
            barh = data->sbarheight;

            if (data->img_sbarlogo) if (data->img_sbarlogo->h > barh) barh = data->img_sbarlogo->h;
            if (data->img_stitlebar) if (data->img_stitlebar->h > barh) barh = data->img_stitlebar->h;
        }
    }
    return (IPTR)obj;
}

static IPTR scrdecor_dispose(Class *cl, Object *obj, struct opSet *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    DisposeScreenSkinning(data);

    return 1;
}

/**************************************************************************************************/

static IPTR scrdecor_get(Class *cl, Object *obj, struct opGet *msg)
{

    switch(msg->opg_AttrID)
    {
        case SDA_TrueColorOnly:
            *msg->opg_Storage = TRUE;
            break;

        case SDA_ScreenData:
            *msg->opg_Storage = (IPTR)INST_DATA(cl, obj);
            break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    return 1;
}

/**************************************************************************************************/

static void scr_findtitlearea(struct Screen *scr, LONG *left, LONG *right)
{
    struct Gadget *g;

    *left = 0;
    *right = scr->Width - 1;

    for (g = scr->FirstGadget; g; g = g->NextGadget)
    {
        if (!(g->Flags & GFLG_RELRIGHT))
        {
            if (g->LeftEdge + g->Width > *left) *left = g->LeftEdge + g->Width;
        }
        else
        {
            if (g->LeftEdge + scr->Width - 1 - 1 < *right) *right = g->LeftEdge + scr->Width - 1 - 1;
        }
    }
}

IPTR scrdecor_draw_screenbar(Class *cl, Object *obj, struct sdpDrawScreenBar *msg)
{
    struct scrdecor_data   *data = INST_DATA(cl, obj);
    struct ScreenData      *sd = (struct ScreenData *) msg->sdp_UserBuffer;
    struct TextExtent       te;
    struct RastPort        *rp = msg->sdp_RPort;
    struct Screen          *scr = msg->sdp_Screen;
    UWORD                  *pens = msg->sdp_Dri->dri_Pens;
    LONG                    left, right, titlelen = 0;
    BOOL                    hastitle = TRUE;
    BOOL		    beeping = scr->Flags & BEEPING;

    if (beeping) {
        SetAPen(rp, pens[BARDETAILPEN]);
        RectFill(rp, 0, 0, scr->Width, data->img_stitlebar->h);
    } else {
        if (sd->img_stitlebar.ok)
	    WriteTiledImage(NULL, rp, &sd->img_stitlebar, 0, 0, data->img_stitlebar->w, data->img_stitlebar->h, 0, 0, scr->Width, data->img_stitlebar->h);
    }
    if (sd->img_sbarlogo.ok) WriteTiledImage(NULL, rp, &sd->img_sbarlogo, 0, 0, data->img_sbarlogo->w, data->img_sbarlogo->h, data->slogo_off, (scr->BarHeight + 1 - data->img_sbarlogo->h) / 2, data->img_sbarlogo->w, data->img_sbarlogo->h);
    if (scr->Title == NULL) hastitle = FALSE;

    if (hastitle)
    {
        scr_findtitlearea(scr, &left, &right);
        titlelen = strlen(scr->Title);
        titlelen = TextFit(rp, scr->Title, titlelen, &te, NULL, 1, right - data->stitle_off, data->sbarheight);
        if (titlelen == 0) hastitle = 0;
    }

    if (hastitle)
    {
        UWORD tx = data->stitle_off;
        UWORD ty = (scr->BarHeight + 1 - msg->sdp_Dri->dri_Font->tf_YSize) / 2 + rp->TxBaseline;

        SetFont(rp, msg->sdp_Dri->dri_Font);
        SetDrMd(rp, JAM1);
//         Move(rp, data->stitle_off, (scr->BarHeight + 1 - msg->sdp_Dri->dri_Font->tf_YSize) / 2 + rp->TxBaseline);
//         Text(rp, scr->Title, titlelen);

        if (!sd->truecolor || ((data->outline == FALSE) && (data->shadow == FALSE)))
        {
	    SetAPen(rp, pens[beeping ? BARBLOCKPEN : BARDETAILPEN]);
            Move(rp, tx, ty);
            Text(rp, scr->Title, titlelen);
        }
        else if (data->outline)
        {

                SetSoftStyle(rp, FSF_BOLD, AskSoftStyle(rp));
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->shadow_col, TAG_DONE);

                Move(rp, tx + 1, ty ); Text(rp, scr->Title, titlelen);
                Move(rp, tx + 2, ty ); Text(rp, scr->Title, titlelen);
                Move(rp, tx , ty ); Text(rp, scr->Title, titlelen);
                Move(rp, tx, ty + 1);  Text(rp, scr->Title, titlelen);
                Move(rp, tx, ty + 2);  Text(rp, scr->Title, titlelen);
                Move(rp, tx + 1, ty + 2);  Text(rp, scr->Title, titlelen);
                Move(rp, tx + 2, ty + 1);  Text(rp, scr->Title, titlelen);
                Move(rp, tx + 2, ty + 2);  Text(rp, scr->Title, titlelen);

                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->text_col, TAG_DONE);
                Move(rp, tx + 1, ty + 1);
                Text(rp, scr->Title, titlelen);
                SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
        }
        else
        {
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->shadow_col, TAG_DONE);
                Move(rp, tx + 1, ty + 1 );
                Text(rp, scr->Title, titlelen);

                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->text_col, TAG_DONE);
                Move(rp, tx, ty);
                Text(rp, scr->Title, titlelen);

        }
    }
    return TRUE;
}

IPTR scrdecor_getdefsize_sysimage(Class *cl, Object *obj, struct sdpGetDefSizeSysImage *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);

    if (msg->sdp_Which == SDEPTHIMAGE)
    {
        if (data->img_sdepth)
        {
            *msg->sdp_Height = data->img_sdepth->h;
            *msg->sdp_Width = data->img_sdepth->w >> 1;
        }
        else return DoSuperMethodA(cl, obj, (Msg) msg);
    }
    else return DoSuperMethodA(cl, obj, (Msg) msg);

    return TRUE;
}

IPTR scrdecor_draw_sysimage(Class *cl, Object *obj, struct sdpDrawSysImage *msg)
{
    struct scrdecor_data   *data = INST_DATA(cl, obj);
    struct ScreenData      *sd = (struct ScreenData *) msg->sdp_UserBuffer;

    struct RastPort        *rp = msg->sdp_RPort;
    LONG                    left = msg->sdp_X;
    LONG                    top = msg->sdp_Y;
    LONG                    state = msg->sdp_State;

    if (msg->sdp_Which == SDEPTHIMAGE)
    {
        if (data->img_sdepth)
        {
            DrawAlphaStateImageToRP(NULL, rp, &sd->img_sdepth, state, left, top, TRUE);
        }
        else return DoSuperMethodA(cl, obj, (Msg) msg);
    }
    else return DoSuperMethodA(cl, obj, (Msg) msg);

    return TRUE;
}

IPTR scrdecor_layoutscrgadgets(Class *cl, Object *obj, struct sdpLayoutScreenGadgets *msg)
{
    struct Gadget          *gadget = msg->sdp_Gadgets;

    struct scrdecor_data   *data = INST_DATA(cl, obj);

    while(gadget)
    {
        switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
        {
            case GTYP_SDEPTH:
                gadget->LeftEdge = -gadget->Width;
                gadget->TopEdge = (data->sbarheight - data->img_sdepth->h) >> 1;
                gadget->Flags &= ~GFLG_RELWIDTH;
                gadget->Flags |= GFLG_RELRIGHT;
                break;
        }

        if (msg->sdp_Flags & SDF_LSG_MULTIPLE)
        {
            gadget = gadget->NextGadget;
        }
        else
        {
            gadget = NULL;
        }
    }

    return TRUE;
}


IPTR scrdecor_initscreen(Class *cl, Object *obj, struct sdpInitScreen *msg)
{
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct ScreenData *sd = (struct ScreenData *)msg->sdp_UserBuffer;
    struct Screen *screen = msg->sdp_Screen;

    sd->truecolor = msg->sdp_TrueColor;

    BOOL    truecolor = sd->truecolor;

    msg->sdp_WBorTop = data->winbarheight - 1 - msg->sdp_FontHeight;
    msg->sdp_BarHBorder = 1;
    msg->sdp_BarHeight = data->sbarheight - 1; //compatiblity issue
    msg->sdp_WBorLeft = data->leftborder;
    msg->sdp_WBorRight = data->rightborder;
    msg->sdp_WBorBottom = data->bottomborder;

    sd->ActivePen = -1;
    sd->DeactivePen = -1;
    if (!truecolor) {
        sd->ActivePen = ObtainPen(screen->ViewPort.ColorMap, -1, (data->lut_col_a << 8) & 0xff000000, (data->lut_col_a << 16) & 0xff000000, (data->lut_col_a << 24) & 0xff000000, PEN_EXCLUSIVE);
        sd->DeactivePen = ObtainPen(screen->ViewPort.ColorMap, -1, (data->lut_col_d << 8) & 0xff000000, (data->lut_col_d << 16) & 0xff000000, (data->lut_col_d << 24) & 0xff000000, PEN_EXCLUSIVE);
    }

    SETIMAGE_SCR(sdepth);
    SETIMAGE_SCR(sbarlogo);
    SETIMAGE_SCR(stitlebar);

    SETIMAGE_SCR(size);
    SETIMAGE_SCR(close);
    SETIMAGE_SCR(depth);
    SETIMAGE_SCR(zoom);
    SETIMAGE_SCR(up);
    SETIMAGE_SCR(down);
    SETIMAGE_SCR(left);
    SETIMAGE_SCR(right);
    SETIMAGE_SCR(mui);
    SETIMAGE_SCR(popup);
    SETIMAGE_SCR(snapshot);
    SETIMAGE_SCR(iconify);
    SETIMAGE_SCR(lock);
    SETIMAGE_SCR(winbar_normal);
    SETIMAGE_SCR(border_normal);
    SETIMAGE_SCR(border_deactivated);
    SETIMAGE_SCR(verticalcontainer);
    SETIMAGE_SCR(verticalknob);
    SETIMAGE_SCR(horizontalcontainer);
    SETIMAGE_SCR(horizontalknob);

    SETIMAGE_SCR(menu);
    SETIMAGE_SCR(amigakey);
    SETIMAGE_SCR(menucheck);
    SETIMAGE_SCR(submenu);

    return TRUE;
}

IPTR scrdecor_exitscreen(Class *cl, Object *obj, struct sdpExitScreen *msg)
{
    struct ScreenData *sd = (struct ScreenData *)msg->sdp_UserBuffer;

    DELIMAGE_SCR(sdepth);
    DELIMAGE_SCR(sbarlogo);
    DELIMAGE_SCR(stitlebar);

    DELIMAGE_SCR(size);
    DELIMAGE_SCR(close);
    DELIMAGE_SCR(depth);
    DELIMAGE_SCR(zoom);
    DELIMAGE_SCR(up);
    DELIMAGE_SCR(down);
    DELIMAGE_SCR(left);
    DELIMAGE_SCR(right);
    DELIMAGE_SCR(mui);
    DELIMAGE_SCR(popup);
    DELIMAGE_SCR(snapshot);
    DELIMAGE_SCR(iconify);
    DELIMAGE_SCR(lock);
    DELIMAGE_SCR(winbar_normal);
    DELIMAGE_SCR(border_normal);
    DELIMAGE_SCR(border_deactivated);
    DELIMAGE_SCR(verticalcontainer);
    DELIMAGE_SCR(verticalknob);
    DELIMAGE_SCR(horizontalcontainer);
    DELIMAGE_SCR(horizontalknob);

    DELIMAGE_SCR(menu);
    DELIMAGE_SCR(amigakey);
    DELIMAGE_SCR(menucheck);
    DELIMAGE_SCR(submenu);

    return TRUE;
}

/**************************************************************************************************/

IPTR scrdecor_dispatcher(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR retval;
  
    switch(msg->MethodID)
    {
        case OM_NEW:
            retval = scrdecor_new(cl, obj, (struct opSet *)msg);
            break;

        case OM_DISPOSE:
            retval = scrdecor_dispose(cl, obj, (struct opSet *)msg);
            break;

        case OM_GET:
            retval = scrdecor_get(cl, obj, (struct opGet *)msg);
            break;

        case SDM_GETDEFSIZE_SYSIMAGE:
            retval = scrdecor_getdefsize_sysimage(cl, obj, (struct sdpGetDefSizeSysImage *)msg);
            break;

        case SDM_DRAW_SCREENBAR:
            retval = scrdecor_draw_screenbar(cl, obj, (struct sdpDrawScreenBar *)msg);
            break;

        case SDM_DRAW_SYSIMAGE:
            retval = scrdecor_draw_sysimage(cl, obj, (struct sdpDrawSysImage *)msg);
            break;

        case SDM_LAYOUT_SCREENGADGETS:
            retval = scrdecor_layoutscrgadgets(cl, obj, (struct sdpLayoutScreenGadgets *)msg);
            break;

        case SDM_INITSCREEN:
            retval = scrdecor_initscreen(cl, obj, (struct sdpInitScreen *)msg);
            break;

       case SDM_EXITSCREEN:
            retval = scrdecor_exitscreen(cl, obj, (struct sdpExitScreen *)msg);
            break;

        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}

IPTR menudecor_getdefsizes(Class *cl, Object *obj, struct mdpGetDefSizeSysImage *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);

    struct NewImage *n = NULL;
    BOOL  isset = FALSE;

    switch(msg->mdp_Which)
    {
        case AMIGAKEY:
            n = data->img_amigakey;
            isset = TRUE;
            break;

        case MENUCHECK:
            n = data->img_menucheck;
            isset = TRUE;
            break;

        case SUBMENUIMAGE:
            n = data->img_submenu;
            isset = TRUE;
            break;

        default:
            return FALSE;
    }

    if (!isset || (n==NULL)) return DoSuperMethodA(cl, obj, (Msg) msg);

    *msg->mdp_Width = n->w;
    *msg->mdp_Height = n->h;
    return TRUE;
}

IPTR menudecor_draw_sysimage(Class *cl, Object *obj, struct mdpDrawSysImage *msg)
{
    struct ScreenData      *md = (struct ScreenData *) msg->mdp_UserBuffer;
    struct RastPort        *rp = msg->mdp_RPort;
    struct NewImage        *ni = NULL;
    LONG                    state = msg->mdp_State;
    LONG                    left = msg->mdp_X;
    LONG                    top = msg->mdp_Y;
    WORD                    addx = 0;
    WORD                    addy = 0;
    BOOL                    isset = FALSE;
    
    switch(msg->mdp_Which)
    {
        case AMIGAKEY:
            if (md && md->img_amigakey.ok)
            {
                ni = &md->img_amigakey;
                isset = TRUE;
            }
            break;

        case MENUCHECK:
            if (md && md->img_amigakey.ok)
            {
                ni = &md->img_menucheck;
                isset = TRUE;
            }
            break;

        case SUBMENUIMAGE:
            if (md && md->img_submenu.ok)
            {
                ni = &md->img_submenu;
                isset = TRUE;
            }
            break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    if (!isset || (ni == NULL)) return DoSuperMethodA(cl, obj, (Msg)msg);

    DrawAlphaStateImageToRP(NULL, rp, ni, state, left+addx, top+addy, FALSE);

    return TRUE;
}

/**************************************************************************************************/

IPTR menudecor_renderbackground(Class *cl, Object *obj, struct mdpDrawBackground *msg)
{
    struct RastPort    *rp = msg->mdp_RPort;
    struct NewImage    *ni;
    struct MenuData    *md = (struct MenuData *) msg->mdp_UserBuffer;
    UWORD               flags = msg->mdp_Flags;

    if (!msg->mdp_TrueColor)
    {   

        if ((flags & HIGHITEM) && md->map)
        {
        }
        else
        {
            if (md->map)
            {
                BltBitMapRastPort(md->map, msg->mdp_ItemLeft, msg->mdp_ItemTop, rp, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemWidth, msg->mdp_ItemHeight, 0xc0);
                return TRUE;
            }
        }
        return FALSE;

    }

    if ((flags & HIGHITEM) && md->ni)
    {
        ni = NewImageContainer(msg->mdp_ItemWidth, msg->mdp_ItemHeight);
        if (ni)
        {
            DrawPartToImage(md->ni, ni, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemWidth, msg->mdp_ItemHeight, 0, 0);
            SetImageTint(ni, 60, 0x00888800);
            PutImageToRP(rp, ni, msg->mdp_ItemLeft, msg->mdp_ItemTop);
        }
    }
    else
    {
        if (md->ni) DrawPartImageToRP(rp, md->ni, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemLeft, msg->mdp_ItemTop, msg->mdp_ItemWidth, msg->mdp_ItemHeight);
    }

    return TRUE;
}

IPTR menudecor_initmenu(Class *cl, Object *obj, struct mdpInitMenu *msg)
{
    struct RastPort        *rp = msg->mdp_RPort;
    struct MenuData        *md = (struct MenuData *) msg->mdp_UserBuffer;
    struct ScreenData      *sd = (struct ScreenData *) msg->mdp_ScreenUserBuffer;

    SETIMAGE_MEN(menu);
    SETIMAGE_MEN(amigakey);
    SETIMAGE_MEN(menucheck);
    SETIMAGE_MEN(submenu);

    md->truecolor = msg->mdp_TrueColor;

//    if (!msg->mdp_TrueColor) return DoSuperMethodA(cl, obj, (Msg) msg);

    if (md->truecolor)
    {
        md->ni = GetImageFromRP(rp, msg->mdp_Left, msg->mdp_Top, msg->mdp_Width, msg->mdp_Height);
        if (md->ni) {
            md->ni->ok = TRUE;
            RenderBackground(md->ni, md->img_menu, 20);
        }
    }
    else
    {
        md->map = AllocBitMap(msg->mdp_Width, msg->mdp_Height, 1, BMF_CLEAR, rp->BitMap);
        if (md->map) {
            BltBitMap(rp->BitMap, msg->mdp_Left, msg->mdp_Top, md->map, 0, 0, msg->mdp_Width, msg->mdp_Height, 0xc0, 0xff, NULL);

            TileMapToBitmap(md->img_menu, md->map, msg->mdp_Width, msg->mdp_Height);
        }
    }

    return TRUE;
}

IPTR menudecor_exitmenu(Class *cl, Object *obj, struct mdpExitMenu *msg)
{
    struct MenuData      *md = (struct MenuData *) msg->mdp_UserBuffer;

    if (md->ni) DisposeImageContainer(md->ni);
    if (md->map) FreeBitMap(md->map);

    return TRUE;
}

IPTR menudecor_getmenuspaces(Class *cl, Object *obj, struct mdpGetMenuSpaces *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);

    if (data->img_menu)
    {
        msg->mdp_InnerLeft =  data->img_menu->inner_left;
        msg->mdp_InnerTop =  data->img_menu->inner_top;
        msg->mdp_InnerRight =  data->img_menu->inner_right;
        msg->mdp_InnerBottom =  data->img_menu->inner_bottom;
        msg->mdp_ItemInnerLeft = 1;
        msg->mdp_ItemInnerTop = 2;
        msg->mdp_ItemInnerRight = 2;
        msg->mdp_ItemInnerBottom = 1;
        if ((data->img_menu->tile_left + data->img_menu->tile_right) > (data->img_menu->inner_left + data->img_menu->inner_right)) msg->mdp_MinWidth = data->img_menu->tile_left + data->img_menu->tile_right; else msg->mdp_MinWidth = data->img_menu->inner_left + data->img_menu->inner_right;
        if ((data->img_menu->tile_top + data->img_menu->tile_bottom) > (data->img_menu->inner_top + data->img_menu->inner_bottom)) msg->mdp_MinHeight = data->img_menu->tile_top + data->img_menu->tile_bottom; else msg->mdp_MinHeight = data->img_menu->inner_top + data->img_menu->inner_bottom;
    }
    return TRUE;
}

void DisposeMenuSkinning(struct menudecor_data *data)
{
}

BOOL InitMenuSkinning(STRPTR path, struct menudecor_data *data) {

    char    buffer[256];
    char    *line, *v;
    BPTR    file;
    BOOL    tiled = FALSE;
    int     tile_left = 0;
    int     tile_top = 0;
    int     tile_right = 0;
    int     tile_bottom = 0;
    int     inner_left = 0;
    int     inner_top = 0;
    int     inner_right = 0;
    int     inner_bottom = 0;
    BPTR    lock;
    BPTR    olddir = 0;
    
    lock = Lock(path, ACCESS_READ);
    if (lock)
    {
        olddir = CurrentDir(lock);
    }
    else return FALSE;

    file = Open("Menu/Config", MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "TileLeft ")) == line)
                {
                    tile_left = GetInt(v);
                    tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileTop ")) == line)
                {
                    tile_top = GetInt(v);
                    tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileRight ")) == line)
                {
                    tile_right = GetInt(v);
                    tiled = TRUE;
                }
                else  if ((v = strstr(line, "TileBottom ")) == line)
                {
                    tile_bottom = GetInt(v);
                    tiled = TRUE;
                }
                else if ((v = strstr(line, "InnerLeft ")) == line)
                {
                    inner_left = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerTop ")) == line)
                {
                    inner_top = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerRight ")) == line)
                {
                    inner_right = GetInt(v);
                }
                else  if ((v = strstr(line, "InnerBottom ")) == line)
                {
                    inner_bottom = GetInt(v);
                }
            }
        }
        while(line);
        Close(file);
    }

    PUTIMAGE_MEN(menu);
    PUTIMAGE_MEN(amigakey);
    PUTIMAGE_MEN(menucheck);
    PUTIMAGE_MEN(submenu);

    if (data->img_menu)
    {
        data->img_menu->tile_left = tile_left;
        data->img_menu->tile_right = tile_right;
        data->img_menu->tile_top = tile_top;
        data->img_menu->tile_bottom = tile_bottom;
        data->img_menu->inner_left = inner_left;
        data->img_menu->inner_right = inner_right;
        data->img_menu->inner_top = inner_top;
        data->img_menu->inner_bottom = inner_bottom;
        data->img_menu->istiled = tiled;
    }
    if (olddir) CurrentDir(olddir);
    UnLock(lock);
    if (data->img_menu) return TRUE;
    DisposeMenuSkinning(data);
    return FALSE;
}

IPTR menudecor__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct menudecor_data *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
        data = INST_DATA(cl, obj);

        STRPTR path = (STRPTR) GetTagData(MDA_Configuration, (IPTR) "Theme:", msg->ops_AttrList);
        data->sd = (struct scrdecor_data *) GetTagData(MDA_ScreenData, 0, msg->ops_AttrList);

        if (!InitMenuSkinning(path, data))
        {
            CoerceMethod(cl,obj,OM_DISPOSE);
            obj = NULL;
        }
    }
    return (IPTR)obj;
}


IPTR menudecor__OM_DISPOSE(Class *cl, Object *obj, struct opSet *msg)
{
    struct menudecor_data *data = INST_DATA(cl, obj);

    DisposeMenuSkinning(data);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR menudecor_dispatcher(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR retval;

    switch(msg->MethodID)
    {
        case OM_NEW:
            retval = menudecor__OM_NEW(cl, obj, (struct opSet *) msg);
            break;

        case OM_DISPOSE:
            retval = menudecor__OM_DISPOSE(cl, obj, (struct opSet *) msg);
            break;

        case MDM_DRAW_SYSIMAGE:
            retval = menudecor_draw_sysimage(cl, obj, (struct mdpDrawSysImage *)msg);
            break;

        case MDM_GETDEFSIZE_SYSIMAGE:
            retval = menudecor_getdefsizes(cl, obj, (struct mdpGetDefSizeSysImage *) msg);
            break;

        case MDM_DRAWBACKGROUND:
            retval = menudecor_renderbackground(cl, obj, (struct mdpDrawBackground *)msg);
            break;

        case MDM_INITMENU:
            retval = menudecor_initmenu(cl, obj, (struct mdpInitMenu *)msg);
            break;

        case MDM_EXITMENU:
            retval = menudecor_exitmenu(cl, obj, (struct mdpExitMenu *)msg);
            break;

        case MDM_GETMENUSPACES:
            retval = menudecor_getmenuspaces(cl, obj, (struct mdpGetMenuSpaces *)msg);
            break;

        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
}

/**************************************************************************************************/




void DisposeScreenSkinning(struct scrdecor_data *data)
{
    DisposeImageContainer(data->img_sdepth);
    DisposeImageContainer(data->img_sbarlogo);
    DisposeImageContainer(data->img_stitlebar);

    DisposeImageContainer(data->img_size);
    DisposeImageContainer(data->img_close);
    DisposeImageContainer(data->img_depth);
    DisposeImageContainer(data->img_zoom);
    DisposeImageContainer(data->img_mui);
    DisposeImageContainer(data->img_popup);
    DisposeImageContainer(data->img_snapshot);
    DisposeImageContainer(data->img_iconify);
    DisposeImageContainer(data->img_lock);
    DisposeImageContainer(data->img_up);
    DisposeImageContainer(data->img_down);
    DisposeImageContainer(data->img_left);
    DisposeImageContainer(data->img_right);
    DisposeImageContainer(data->img_winbar_normal);
    DisposeImageContainer(data->img_border_normal);
    DisposeImageContainer(data->img_border_deactivated);
    DisposeImageContainer(data->img_verticalcontainer);
    DisposeImageContainer(data->img_verticalknob);
    DisposeImageContainer(data->img_horizontalcontainer);
    DisposeImageContainer(data->img_horizontalknob);

    DisposeImageContainer(data->img_menu);
    DisposeImageContainer(data->img_menucheck);
    DisposeImageContainer(data->img_amigakey);
    DisposeImageContainer(data->img_submenu);

    data->img_size = NULL;
    data->img_close = NULL;
    data->img_depth = NULL;
    data->img_zoom = NULL;
    data->img_mui = NULL;
    data->img_popup = NULL;
    data->img_snapshot = NULL;
    data->img_iconify = NULL;
    data->img_lock = NULL;
    data->img_up = NULL;
    data->img_down = NULL;
    data->img_left = NULL;
    data->img_right = NULL;
    data->img_winbar_normal = NULL;
    data->img_border_normal = NULL;
    data->img_border_deactivated = NULL;
    data->img_verticalcontainer = NULL;
    data->img_verticalknob = NULL;
    data->img_horizontalcontainer = NULL;
    data->img_horizontalknob = NULL;

    data->img_sdepth = NULL;
    data->img_sbarlogo = NULL;
    data->img_stitlebar = NULL;
}

BOOL InitScreenSkinning(STRPTR path, struct scrdecor_data *data) {
    
    char    buffer[256];
    char    *line, *v;
    BPTR    file;
    BPTR    lock;
    BPTR    olddir = 0;
    lock = Lock(path, ACCESS_READ);
    if (lock)
    {
        olddir = CurrentDir(lock);
    }
    else return FALSE;

    data->leftborder = 4;
    data->rightborder = 4;
    data->bottomborder = 4;

    data->lut_col_a = 0x00cccccc;
    data->lut_col_d = 0x00888888;

    data->outline = FALSE;
    data->shadow = FALSE;

    data->text_col = 0x00cccccc;
    data->shadow_col = 0x00444444;

    file = Open("System/Config", MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "LeftBorder ")) == line) {
                    data->leftborder = GetInt(v);
                } else  if ((v = strstr(line, "RightBorder ")) == line) {
                    data->rightborder = GetInt(v);
                } else  if ((v = strstr(line, "BottomBorder ")) == line) {
                    data->bottomborder = GetInt(v);
                } else  if ((v = strstr(line, "LogoOffset ")) == line) {
                    data->slogo_off = GetInt(v);
                } else  if ((v = strstr(line, "TitleOffset ")) == line) {
                    data->stitle_off = GetInt(v);
                } else  if ((v = strstr(line, "SBarHeight ")) == line) {
                    data->sbarheight = GetInt(v);
                } else  if ((v = strstr(line, "BarHeight ")) == line) {
                    data->winbarheight = GetInt(v); //screen, window
                } else  if ((v = strstr(line, "LUTBaseColors ")) == line) {
                    GetColors(v, &data->lut_col_a, &data->lut_col_d);
                } else  if ((v = strstr(line, "ScreenTitleColors ")) == line) {
                    GetColors(v, &data->text_col, &data->shadow_col);
                } else if ((v = strstr(line, "ScreenTitleMode ")) == line) {
                    data->outline = GetBool(v, "Outline");
                    data->shadow = GetBool(v, "Shadow");
                }
            }
        }
        while(line);
        Close(file);
    }

    data->img_sdepth = GetImageFromFile(path, "System/SDepth/", TRUE);
    data->img_stitlebar = GetImageFromFile(path, "System/STitlebar/", TRUE);
    data->img_sbarlogo = GetImageFromFile(path, "System/SBarLogo/Default", FALSE);

    data->img_size = GetImageFromFile(path, "System/Size/", TRUE);
    data->img_close = GetImageFromFile(path, "System/Close/", TRUE);
    data->img_depth = GetImageFromFile(path, "System/Depth/", TRUE);
    data->img_zoom = GetImageFromFile(path, "System/Zoom/", TRUE);
    data->img_mui = GetImageFromFile(path, "System/MUI/", TRUE);
    data->img_popup = GetImageFromFile(path, "System/PopUp/", TRUE);
    data->img_snapshot = GetImageFromFile(path, "System/Snapshot/", TRUE);
    data->img_iconify = GetImageFromFile(path, "System/Iconify/", TRUE);
    data->img_lock = GetImageFromFile(path, "System/Lock/", TRUE);
    data->img_up = GetImageFromFile(path, "System/ArrowUp/", TRUE);
    data->img_down = GetImageFromFile(path, "System/ArrowDown/", TRUE);
    data->img_left = GetImageFromFile(path, "System/ArrowLeft/", TRUE);
    data->img_right = GetImageFromFile(path, "System/ArrowRight/", TRUE);
    data->img_winbar_normal = GetImageFromFile(path, "System/Titlebar/", TRUE);
    data->img_border_normal = GetImageFromFile(path, "System/Borders/Default", FALSE);
    data->img_border_deactivated = GetImageFromFile(path, "System/Borders/Default_Deactivated", FALSE);
    data->img_verticalcontainer = GetImageFromFile(path, "System/Container/Vertical", FALSE);
    data->img_verticalknob = GetImageFromFile(path, "System/Knob/Vertical", FALSE);
    data->img_horizontalcontainer = GetImageFromFile(path, "System/Container/Horizontal", FALSE);
    data->img_horizontalknob = GetImageFromFile(path, "System/Knob/Horizontal", FALSE);

    data->img_menu = GetImageFromFile(path, "Menu/Background/Default", FALSE);
    data->img_amigakey = GetImageFromFile(path, "Menu/AmigaKey/", TRUE);
    data->img_menucheck = GetImageFromFile(path, "Menu/Checkmark/", TRUE);
    data->img_submenu = GetImageFromFile(path, "Menu/SubMenu/", TRUE);

    if (data->img_stitlebar)
    {
        data->img_stitlebar->tile_left = 8;
        data->img_stitlebar->tile_right = 8;
        data->img_stitlebar->tile_top = 9;
        data->img_stitlebar->tile_bottom = 8;
    }

    if (olddir) CurrentDir(olddir);
    UnLock(lock);

    if (data->img_sdepth) return TRUE;
    DisposeScreenSkinning(data);
    return FALSE;
}

STRPTR __detached_name = "Decoration";

#define MAGIC_PRIVATE_SKIN      0x0001

struct SkinMessage {
    struct MagicMessage msg;
    UWORD  class;
    STRPTR  path;
    STRPTR  id;
};

void DeleteDecorator(struct NewDecorator *nd)
{
    if (nd == NULL) return;
    if (nd->nd_Menu != NULL) DisposeObject(nd->nd_Menu);
    if (nd->nd_Window != NULL) DisposeObject(nd->nd_Window);
    if (nd->nd_Screen != NULL) DisposeObject(nd->nd_Screen);
    FreeVec(nd);
}

struct NewDecorator *GetDecorator(STRPTR path)
{
    struct NewDecorator *nd = NULL;

    STRPTR newpath;

    if (path != NULL) newpath = path; else newpath = "Theme:";

    struct TagItem ScreenTags[] = { {SDA_UserBuffer, sizeof(struct ScreenData)}, {SDA_Configuration, (IPTR) newpath}, {TAG_DONE} };


    nd = AllocVec(sizeof(struct NewDecorator), MEMF_CLEAR | MEMF_ANY);
    if (nd)
    {
        nd->nd_Screen = NewObjectA(scrcl, NULL, ScreenTags);

        if (nd->nd_Screen)
        {
            APTR    screendata = NULL;

            get(nd->nd_Screen, SDA_ScreenData, &screendata);

            struct TagItem WindowTags[] = { {WDA_UserBuffer, sizeof(struct WindowData)}, {WDA_Configuration, (IPTR) newpath}, {WDA_ScreenData, (IPTR)screendata}, {TAG_DONE} };
            struct TagItem MenuTags[]   = { {MDA_UserBuffer, sizeof(struct MenuData)}, {MDA_Configuration, (IPTR) newpath}, {MDA_ScreenData, (IPTR)screendata}, {TAG_DONE} };


            nd->nd_Window = NewObjectA(cl, NULL, WindowTags);
            nd->nd_Menu = NewObjectA(menucl, NULL, MenuTags);
            if ((nd->nd_Menu == NULL ) || (nd->nd_Window == NULL) || (nd->nd_Screen == NULL))
            {
                DeleteDecorator(nd);
                nd = NULL;
            }
        }
        else
        {
            DeleteDecorator(nd);
            nd = NULL;
        }
    }
    return nd;
}

#define ARGUMENT_TEMPLATE "PATH,SCREENID=ID/K"

char usage[] =
    "Decoration:\n"
    "Arguments:\n";

int main(void)
{

    IPTR rd_Args[] = {0, 0, };

    struct RDArgs *args, *newargs;

    /* the 1M $$ Question why "Decoration ?" does not work in the shell? */
    
    newargs = (struct RDArgs*) AllocDosObject(DOS_RDARGS, NULL);

    if (newargs == NULL) return 0;

    newargs->RDA_ExtHelp = usage;
    newargs->RDA_Flags = 0;
    
    args = ReadArgs(ARGUMENT_TEMPLATE, rd_Args, newargs);

    if (args == NULL) {
        FreeDosObject (DOS_RDARGS, (APTR) newargs);
        return 0;
    }

    Forbid();
    if (FindPort("DECORATIONS")) {
        struct MsgPort *port = CreateMsgPort();
        if (port) {
            struct SkinMessage msg;
            msg.msg.mn_ReplyPort = port;
            msg.msg.mn_Magic = MAGIC_PRIVATE_SKIN;
            msg.class = 0;
            msg.path = (STRPTR) rd_Args[0];
            msg.id = (STRPTR) rd_Args[1];
            PutMsg(FindPort("DECORATIONS"), (struct Message *) &msg);
            WaitPort(port);
            GetMsg(port);
            Permit();
            DeleteMsgPort(port);
            FreeArgs(args);
            return 0;
        }
    }
    Permit();

    cl = MakeClass(NULL, WINDECORCLASS, NULL, sizeof(struct windecor_data), 0);
    if (cl)
    {
        cl->cl_Dispatcher.h_Entry    = HookEntry;
        cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)WinDecor_Dispatcher;

        scrcl = MakeClass(NULL, SCRDECORCLASS, NULL, sizeof(struct scrdecor_data), 0);
        if (scrcl)
        {
            scrcl->cl_Dispatcher.h_Entry    = HookEntry;
            scrcl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)scrdecor_dispatcher;
		
            menucl = MakeClass(NULL, MENUDECORCLASS, NULL, sizeof(struct menudecor_data), 0);
            if (menucl)
            {
                menucl->cl_Dispatcher.h_Entry    = HookEntry;
                menucl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)menudecor_dispatcher;

                struct MsgPort *port = CreateMsgPort();
                ULONG  skinSignal;
                if (port)
                {
                    skinSignal = 1 << port->mp_SigBit;
                    port->mp_Node.ln_Name="DECORATIONS";
                    AddPort(port);

                    struct  NewDecorator *decor = GetDecorator((STRPTR) rd_Args[0]);

                    if (decor != NULL)
                    {
                        decor->nd_Pattern = (STRPTR) rd_Args[1];
                        decor->nd_Port = port;
                        ChangeDecoration(DECORATION_SET, decor);
                    }

                    Detach();

                    BOOL running = TRUE;

                    while (running)
                    {
                        ULONG sigs = Wait(SIGBREAKF_CTRL_C | skinSignal);
                        if ((sigs & SIGBREAKF_CTRL_C) != 0)
                        {
                            //       running = FALSE; /* at the moment no exit */
                        }
                        else if ((sigs & skinSignal) != 0)
                        {
                            struct DecoratorMessage *dmsg;
                            struct SkinMessage * msg = (struct SkinMessage *) GetMsg(port);
                            while (msg)
                            {
                                switch(msg->msg.mn_Magic)
                                {
                                    case MAGIC_PRIVATE_SKIN:
                                        decor = GetDecorator(msg->path);
                                        if (decor != NULL)
                                        {
                                            decor->nd_Pattern = msg->id;
                                            decor->nd_Port = port;
                                            ChangeDecoration(DECORATION_SET, decor);
                                        }
                                        break;
                                    case MAGIC_DECORATOR:
                                        dmsg = (struct DecoratorMessage *) msg;
                                        DeleteDecorator((struct NewDecorator *) dmsg->dm_Object);
                                        break;
                                }
                                ReplyMsg((struct Message *) msg);
                                msg = (struct SkinMessage *) GetMsg(port);
                            }
                        }
                    }
                }
                FreeClass(menucl);
            }
            FreeClass(scrcl);
        }
        FreeClass(cl);
    }
    FreeDosObject (DOS_RDARGS, (APTR) newargs);
    FreeArgs(args);
    return 0;
}
