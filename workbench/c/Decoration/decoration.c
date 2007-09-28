/*
    Copyright  1995-2007, The AROS Development Team.
    $Id$
*/

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

/**************************************************************************************************/

#define PUTIMAGE_WIN(id) data->img_##id=data->sd->img_##id
#define PUTIMAGE_MEN(id) data->img_##id=data->sd->img_##id

#define SETIMAGE_SCR(id) SetImage(data->img_##id, &sd->img_##id, truecolor, screen)
#define SETIMAGE_WIN(id) wd->img_##id=&sd->img_##id
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
#define WDA_Configuration 0x30002

#define MDA_ScreenData 0x10003
#define SDA_ScreenData 0x20003
#define WDA_ScreenData 0x30003

    struct IClass 	*cl, *scrcl, *menucl;

    struct	  NewImage
    {
        UWORD   w;
        UWORD   h;
        BOOL    istiled;
        ULONG  *data;
        UWORD   tile_left, tile_top, tile_bottom, tile_right;
        UWORD   inner_left, inner_top, inner_bottom, inner_right;
        APTR    mask;
        Object  *o;
        struct  BitMap  *bitmap;
        BOOL    ok;
        STRPTR  filename;

    };

    struct  NewLUT8Image
    {
        UWORD   w;
        UWORD   h;
        UBYTE  *data;
    };

/**************************************************************************************************/
    struct scrdecor_data
    {

        struct NewImage *img_sdepth;
        struct NewImage *img_sbarlogo;
        struct NewImage *img_stitlebar;

        struct NewImage *img_size;
        struct NewImage *img_close;
        struct NewImage *img_depth;
        struct NewImage *img_zoom;
        struct NewImage *img_up;
        struct NewImage *img_down;
        struct NewImage *img_left;
        struct NewImage *img_right;
        struct NewImage *img_mui;
        struct NewImage *img_popup;
        struct NewImage *img_snapshot;
        struct NewImage *img_iconify;
        struct NewImage *img_lock;
        struct NewImage *img_winbar_normal;
        struct NewImage *img_border_normal;
        struct NewImage *img_border_deactivated;
        struct NewImage *img_verticalcontainer;
        struct NewImage *img_verticalknob;
        struct NewImage *img_horizontalcontainer;
        struct NewImage *img_horizontalknob;

        struct NewImage *img_menu;
        struct NewImage *img_amigakey;
        struct NewImage *img_menucheck;
        struct NewImage *img_submenu;

        UWORD            sbarheight;
        UWORD            slogo_off;
        UWORD            stitle_off;
        UWORD            winbarheight;

        BOOL             outline;
        BOOL             shadow;

        int              leftborder, bottomborder, rightborder;
        int              lut_col_a, lut_col_d;
        int              text_col, shadow_col;


    };


    struct windecor_data
    {
        struct scrdecor_data *sd;
        struct DrawInfo *dri;
        struct Screen   *scr;
        struct NewImage *img_size;
        struct NewImage *img_close;
        struct NewImage *img_depth;
        struct NewImage *img_zoom;
        struct NewImage *img_up;
        struct NewImage *img_down;
        struct NewImage *img_left;
        struct NewImage *img_right;
        struct NewImage *img_mui;
        struct NewImage *img_popup;
        struct NewImage *img_snapshot;
        struct NewImage *img_iconify;
        struct NewImage *img_lock;
        struct NewImage *img_winbar_normal;
        struct NewImage *img_border_normal;
        struct NewImage *img_border_deactivated;
        struct NewImage *img_verticalcontainer;
        struct NewImage *img_verticalknob;
        struct NewImage *img_horizontalcontainer;
        struct NewImage *img_horizontalknob;

        BOOL             outline;
        BOOL             shadow;
        BOOL             barmasking;
        BOOL             closeright;
        BOOL             threestate;
        BOOL             barvert;
        BOOL             usegradients;
        BOOL             rounded;
        BOOL             filltitlebar;

        UWORD            winbarheight;
        UWORD            txt_align;

        int              BarJoinTB_o;
        int              BarJoinTB_s;
        int              BarPreGadget_o;
        int              BarPreGadget_s;
        int              BarPre_o;
        int              BarPre_s;
        int              BarLGadgetFill_o;
        int              BarLGadgetFill_s;
        int              BarJoinGB_o;
        int              BarJoinGB_s;
        int              BarLFill_o;
        int              BarLFill_s;
        int              BarJoinBT_o;
        int              BarJoinBT_s;
        int              BarTitleFill_o;
        int              BarTitleFill_s;
        int              BarRFill_o;
        int              BarRFill_s;
        int              BarJoinBG_o;
        int              BarJoinBG_s;
        int              BarRGadgetFill_o;
        int              BarRGadgetFill_s;
        int              BarPostGadget_o;
        int              BarPostGadget_s;
        int              BarPost_o;
        int              BarPost_s;

        int              ContainerTop_o, ContainerTop_s;
        int              ContainerVertTile_o, ContainerVertTile_s;
        int              ContainerBottom_o, ContainerBottom_s;
        int              KnobTop_o, KnobTop_s;
        int              KnobTileTop_o, KnobTileTop_s;
        int              KnobVertGripper_o, KnobVertGripper_s;
        int              KnobTileBottom_o, KnobTileBottom_s;
        int              KnobBottom_o, KnobBottom_s;
        int              ContainerLeft_o, ContainerLeft_s;
        int              ContainerHorTile_o, ContainerHorTile_s;
        int              ContainerRight_o, ContainerRight_s;
        int              KnobLeft_o, KnobLeft_s;
        int              KnobTileLeft_o, KnobTileLeft_s;
        int              KnobHorGripper_o, KnobHorGripper_s;
        int              KnobTileRight_o, KnobTileRight_s;
        int              KnobRight_o, KnobRight_s;
        int              sizeaddx, sizeaddy;
        int              updownaddx, updownaddy;
        int              leftrightaddx, leftrightaddy;
        int              rightbordergads, bottombordergads;
        int              rightbordernogads, bottombordernogads;
        int              horscrollerheight;
        int              scrollerinnerspacing;
        int              a_arc, d_arc;
        int              a_col_s, a_col_e;
        int              d_col_s, d_col_e;
        int              b_col_a, b_col_d;
        int              light, middle, dark;
        int              text_col, shadow_col;
    };


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

    struct  WindowData
    {
        struct  NewImage        *ni;

        struct NewImage *img_size;
        struct NewImage *img_close;
        struct NewImage *img_depth;
        struct NewImage *img_zoom;
        struct NewImage *img_up;
        struct NewImage *img_down;
        struct NewImage *img_left;
        struct NewImage *img_right;
        struct NewImage *img_mui;
        struct NewImage *img_popup;
        struct NewImage *img_snapshot;
        struct NewImage *img_iconify;
        struct NewImage *img_lock;
        struct NewImage *img_winbar_normal;
        struct NewImage *img_border_normal;
        struct NewImage *img_border_deactivated;
        struct NewImage *img_verticalcontainer;
        struct NewImage *img_verticalknob;
        struct NewImage *img_horizontalcontainer;
        struct NewImage *img_horizontalknob;

        struct  RastPort        *rp;
        UWORD                    w, h;
        LONG   ActivePen;
        LONG   DeactivePen;

        WORD   closewidth, depthwidth, zoomwidth;
        BOOL   truecolor;

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


    struct ScreenData
    {
        struct NewImage  img_sdepth;
        struct NewImage  img_sbarlogo;
        struct NewImage  img_stitlebar;

        struct NewImage  img_size;
        struct NewImage  img_close;
        struct NewImage  img_depth;
        struct NewImage  img_zoom;
        struct NewImage  img_up;
        struct NewImage  img_down;
        struct NewImage  img_left;
        struct NewImage  img_right;
        struct NewImage  img_mui;
        struct NewImage  img_popup;
        struct NewImage  img_snapshot;
        struct NewImage  img_iconify;
        struct NewImage  img_lock;
        struct NewImage  img_winbar_normal;
        struct NewImage  img_border_normal;
        struct NewImage  img_border_deactivated;
        struct NewImage  img_verticalcontainer;
        struct NewImage  img_verticalknob;
        struct NewImage  img_horizontalcontainer;
        struct NewImage  img_horizontalknob;

        struct NewImage  img_menu;
        struct NewImage  img_amigakey;
        struct NewImage  img_menucheck;
        struct NewImage  img_submenu;
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
    void DrawPartialTitleBar(struct WindowData *wd, struct windecor_data * data, struct Window *win, struct RastPort *dst_rp, struct DrawInfo *dri, UWORD align, UWORD start, UWORD width, UWORD *pens);

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


    SetImage(struct NewImage *in, struct NewImage *out, BOOL truecolor, struct Screen* scr)
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
    struct	BitMapHeader       *bmhd;
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

char *SkipChars(char *v)
{
    char *c;
    c = strstr(v, "=");
    return ++c;
}

int GetInt(char *v)
{
    char *c;
    c = SkipChars(v);
    return (int) atol(c);
}

void GetIntegers(char *v, int *v1, int *v2)
{
    char *c;
    char va1[32], va2[32];
    int cnt;
    c = SkipChars(v);
    if (c)
    {
        cnt = sscanf(c, "%s %s", va1, va2);
        if (cnt == 1)
        {
            *v1 = -1;
            *v2 = atol(va1);
        }
        else if (cnt == 2)
        {
            *v1 = atol(va1);
            *v2 = atol(va2);
        }
    }
}

void GetTripleIntegers(char *v, int *v1, int *v2, int *v3)
{
    char *ch;
    int a, b, c;
    int cnt;
    ch = SkipChars(v);
    if (ch)
    {
        cnt = sscanf(ch, "%x %x %d", &a, &b, &c);
        if (cnt == 3)
        {
            *v1 = a;
            *v2 = b;
            *v3 = c;
        }
    }
}

void GetColors(char *v, int *v1, int *v2)
{
    char *ch;
    int a, b;
    int cnt;
    ch = SkipChars(v);
    if (ch)
    {
        cnt = sscanf(ch, "%x %x", &a, &b);
        if (cnt == 2)
        {
            *v1 = a;
            *v2 = b;
        }
    }
}


BOOL GetBool(char *v, char *id)
{
    if (strstr(v, id)) return TRUE; else return FALSE;
}

static IPTR windecor_new(Class *cl, Object *obj, struct opSet *msg)
{
    struct windecor_data *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj)
    {
         data = INST_DATA(cl, obj);

         STRPTR path = (STRPTR) GetTagData(WDA_Configuration, (IPTR) "Theme:", msg->ops_AttrList);
         data->sd = (struct scrdecor_data *) GetTagData(WDA_ScreenData, NULL, msg->ops_AttrList);

         if (!InitWindowSkinning(path, data))
         {
             CoerceMethod(cl,obj,OM_DISPOSE);
             obj = NULL;
         }
     }

    return (IPTR)obj;
}

static IPTR windecor_dispose(Class *cl, Object *obj, struct opSet *msg)
{
    struct windecor_data *data = INST_DATA(cl, obj);

    DisposeWindowSkinning(data);

    return 1;
}
/**************************************************************************************************/

static IPTR windecor_get(Class *cl, Object *obj, struct opGet *msg)
{
    switch(msg->opg_AttrID)
    {
        case WDA_TrueColorOnly:
            *msg->opg_Storage = TRUE;
            break;
        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }
    return 1;
}

/**************************************************************************************************/

/**************************************************************************************************/

IPTR windecor_draw_sysimage(Class *cl, Object *obj, struct wdpDrawSysImage *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct RastPort        *rp = msg->wdp_RPort;
    struct NewImage        *ni = NULL;
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;
    LONG                    state = msg->wdp_State;
    LONG                    left = msg->wdp_X;
    LONG                    top = msg->wdp_Y;
    LONG                    width = msg->wdp_Width;
    LONG                    height = msg->wdp_Height;
    WORD                    addx = 0;
    WORD                    addy = 0;
    BOOL                    isset = FALSE;
    BOOL                    titlegadget = FALSE;

    switch(msg->wdp_Which)
    {
        case SIZEIMAGE:
            if (wd->img_size->ok)
            {
                ni = wd->img_size;
                isset = TRUE;
                if (data->threestate) addx = (data->rightbordergads - (data->img_size->w / 3)) /2; else addx = (data->rightbordergads - (data->img_size->w >> 2)) /2;
                addy = (data->bottombordergads - data->img_size->h) / 2;
            }
            break;

        case CLOSEIMAGE:
            if (wd->img_close->ok)
            {
                ni = wd->img_close;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case MUIIMAGE:
            if (wd->img_mui->ok)
            {
                ni = wd->img_mui;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case POPUPIMAGE:
            if (wd->img_popup->ok)
            {
                ni = wd->img_popup;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case SNAPSHOTIMAGE:
            if (wd->img_snapshot->ok)
            {
                ni = wd->img_snapshot;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case LOCKIMAGE:
            if (wd->img_lock->ok)
            {
                ni = wd->img_lock;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case ICONIFYIMAGE:
            if (wd->img_iconify->ok)
            {
                ni = wd->img_iconify;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case DEPTHIMAGE:
            if (wd->img_depth->ok)
            {
                ni = wd->img_depth;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case ZOOMIMAGE:
            if (wd->img_zoom->ok)
            {
                ni = wd->img_zoom;
                isset = TRUE;
                titlegadget = TRUE;
            }
            break;

        case UPIMAGE:
            ni = wd->img_up;
            if (data->threestate) addx = (data->rightbordergads - (data->img_up->w / 3)) /2; else addx = (data->rightbordergads - (data->img_up->w >> 2)) /2;
            addy = data->updownaddy / 2;
            isset = TRUE;
            break;

        case DOWNIMAGE:
            ni = wd->img_down;
            if (data->threestate) addx = (data->rightbordergads - (data->img_down->w / 3)) /2; else addx = (data->rightbordergads - (data->img_down->w >> 2)) /2;
            addy = data->updownaddy / 2;
            isset = TRUE;
            break;

        case LEFTIMAGE:
            ni = wd->img_left;
            addx = data->leftrightaddx / 2;
            addy = (data->bottombordergads - data->img_left->h) / 2;
            isset = TRUE;
            break;

        case RIGHTIMAGE:
            ni = wd->img_right;
            addx = data->leftrightaddx / 2;
            addy = (data->bottombordergads - data->img_right->h) /2;
            isset = TRUE;
            break;

        default:
            return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    if (!isset) return DoSuperMethodA(cl, obj, (Msg)msg);

    if (wd && titlegadget) if (wd->rp) if (wd->rp->BitMap) BltBitMapRastPort(wd->rp->BitMap, left+addy, top+addy, rp, left+addy, top+addy, width, height, 0xc0);

    if (ni) DrawAlphaStateImageToRP(data, rp, ni, state, left+addx, top+addy, TRUE);

    return TRUE;
}

/**************************************************************************************************/

void getrightgadgetsdimensions(struct windecor_data *data, struct Window *win, int *xs, int *xe)
{
    struct Gadget *g;

    int     x0 = 1000000;
    int     x1 = 0;
    UWORD   type;

    for (g = win->FirstGadget; g; g = g->NextGadget)
    {
        if ((g->Flags & GFLG_RELRIGHT) == GFLG_RELRIGHT)
        {
            type = g->GadgetType & GTYP_SYSTYPEMASK;
            if (data->closeright)
            {
                if (((type & (GTYP_CLOSE | GTYP_WDEPTH | GTYP_WZOOM)) != 0) || (g->Activation & GACT_TOPBORDER))
                {
                    if (g->Width > 0)
                    {
                        if ((win->Width + g->LeftEdge) < x0) x0 = win->Width + g->LeftEdge;
                        if ((win->Width + g->LeftEdge + g->Width) > x1) x1 = win->Width + g->LeftEdge + g->Width;
                    }
                }
            }
            else
            {
                if (((type & (GTYP_WDEPTH | GTYP_WZOOM)) != 0)  || (g->Activation & GACT_TOPBORDER))
                {
                    if (g->Width > 0)
                    {
                        if ((win->Width + g->LeftEdge) < x0) x0 = win->Width + g->LeftEdge;
                        if ((win->Width + g->LeftEdge + g->Width) > x1) x1 = win->Width + g->LeftEdge + g->Width;
                    }
                }
            }
        }
    }
    if (x0 == 1000000) x0 = 0;
    *xs = x0;
    *xe = x1;
}

void getleftgadgetsdimensions(struct windecor_data *data, struct Window *win, int *xs, int *xe)
{
    struct Gadget *g;

    int w = 0;
    int x0 = 1000000;
    int x1 = 0;
    for (g = win->FirstGadget; g; g = g->NextGadget)
    {
        w++;
        if (((g->Flags & GFLG_RELRIGHT) == 0) && (g->Activation & GACT_TOPBORDER))
        {
            if ((g->GadgetType & GTYP_WDRAGGING) == 0)
            {
                if (g->Width > 0)
                {
                    if (g->LeftEdge < x0) x0 = g->LeftEdge;
                    if ((g->LeftEdge + g->Width) > x1) x1 = g->LeftEdge + g->Width;
                }
            }
        }
    }
    if (x0 == 1000000) x0 = 0;
    *xs = x0;
    *xe = x1;
}

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

int WriteTiledImageShape(BOOL fill, struct Window *win, struct NewLUT8Image *lut8, struct NewImage *ni, int sx, int sy, int sw, int sh, int xp, int yp, int dw, int dh)
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
        WriteAlphaPixelArray(ni, lut8, sx, sy, x, yp, ddw, dh);
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

IPTR windecor_draw_winborder(Class *cl, Object *obj, struct wdpDrawWinBorder *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct RastPort        *rp = msg->wdp_RPort;
    struct Window          *window = msg->wdp_Window;
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;
    struct NewImage        *ni = NULL;
    UWORD                  *pens = msg->wdp_Dri->dri_Pens;
    ULONG                   bc, color, s_col, e_col, arc;
    UWORD                   bl, bt, br, bb, ww, wh;
    LONG    pen = -1;
    int                     dy;

    if (wd->img_border_normal->ok) ni = wd->img_border_normal;

    if (ni == NULL) data->usegradients = TRUE;

    BOOL    tc = wd->truecolor;

    LONG    dpen = pens[SHADOWPEN];
    LONG    lpen = pens[SHINEPEN];
    LONG    mpen = pens[SHINEPEN];

    bl = window->BorderLeft;
    bt = window->BorderTop;
    bb = window->BorderBottom;
    br = window->BorderRight;
    ww = window->Width;
    wh = window->Height;

    color = 0x00cccccc;

    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        pen = wd->ActivePen;
        s_col = data->a_col_s;
        e_col = data->a_col_e;
        arc = data->a_arc;
        dy = 0;
        bc = data->b_col_a;
    } else {
        pen = wd->DeactivePen;
        s_col = data->d_col_s;
        e_col = data->d_col_e;
        arc = data->d_arc;
        dy = data->winbarheight;
        bc = data->b_col_d;
        if (!data->usegradients)
        {
            if (wd->img_border_deactivated->ok) ni = wd->img_border_deactivated;
        }
    }

//     if (data->filltitlebar)
//     {
//         if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width, window->Height, 0, 0, window->Width, window->BorderTop, s_col, e_col, arc);
//         else DrawTileToRP(rp, ni, color, 0, 0, 0, 0, window->Width, window->BorderTop);
//     }

    if (window->BorderTop == data->winbarheight) DrawPartialTitleBar(wd, data, window, rp, msg->wdp_Dri, data->txt_align, 0, window->Width, pens);
    if (!(msg->wdp_Flags & WDF_DWB_TOP_ONLY))
    {
        if (window->BorderLeft > 2)
        {
            if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1, 0, window->BorderTop, window->BorderLeft, window->Height - window->BorderTop, s_col, e_col, arc);
            else DrawTileToRP(rp, ni, color, 0, 0, 0, window->BorderTop, window->BorderLeft - 1, window->Height - window->BorderTop);
        }
        if (window->BorderRight > 2)
        {
            if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1, window->Width - window->BorderRight , window->BorderTop, window->BorderRight, window->Height - window->BorderTop, s_col, e_col, arc);
            else DrawTileToRP(rp, ni, color, 0, 0, window->Width - window->BorderRight , window->BorderTop, window->BorderRight, window->Height - window->BorderTop);
        }
        if (window->BorderBottom > 2)
        {
            if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1, 0, window->Height - window->BorderBottom , window->Width, window->BorderBottom, s_col, e_col, arc);
            else DrawTileToRP(rp, ni, color, 0, 0, 0, window->Height - window->BorderBottom , window->Width, window->BorderBottom);
        }

        int bbt = bt;

        if (bt != data->winbarheight) {
            int bq = 0;
            if (bt > 1) bq = bt - 1;
            if (window->BorderTop > 2)
            {
                if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1, 0, 0 , window->Width - 1, window->BorderTop - 1, s_col, e_col, arc);
                else DrawTileToRP(rp, ni, color, 0, 0, 0, 0 , window->Width, window->BorderTop);
            }
            if (bt > 0) ShadeLine(dpen, tc, data, rp, ni, bc, data->dark, 0, 0, 0, ww - 1, 0);
            if (bq > 0) ShadeLine(dpen, tc, data, rp, ni, bc, data->dark, bq, 0, bq, ww - 1, bq);
            if (bt > 1) ShadeLine(lpen, tc, data, rp, ni, bc, data->light, 1, 1, 1, ww - 2, 1);
            bbt = 0;
        }

        if (bl > 0) ShadeLine(dpen, tc, data, rp, ni, bc, data->dark, bbt, 0, bbt, 0, wh - 1);
        if (bb > 0) ShadeLine(dpen, tc, data, rp, ni, bc, data->dark, wh - 1, 0, wh - 1, ww - 1, wh - 1);
        if (br > 0) ShadeLine(dpen, tc, data, rp, ni, bc, data->dark, bbt , ww - 1, bbt , ww - 1, wh - 1);
        if (bl > 1) ShadeLine(dpen, tc, data, rp, ni, bc, data->dark, bbt, bl - 1, bbt, bl - 1, wh - bb);
        if (bb > 1) ShadeLine(dpen, tc, data, rp, ni, bc, data->dark, wh - bb, bl - 1, wh - bb, ww - br, wh - bb);
        if (br > 1) ShadeLine(dpen, tc, data, rp, ni, bc, data->dark, bbt , ww - br, bbt , ww - br, wh - bb);
        if (bl > 2) ShadeLine(lpen, tc, data, rp, ni, bc, data->light, bbt, 1, bbt, 1, wh - 2);
        if (bl > 3) {
            if (bb > 1) ShadeLine(mpen, tc, data, rp, ni, bc, data->middle, bbt, bl - 2, bbt, bl - 2, wh - bb + 1);
            else ShadeLine(mpen, tc, data, rp, ni, bc, data->middle, bbt, bl - 2, bbt, bl - 2, wh - bb);
        }
        if (br > 2) ShadeLine(mpen, tc, data, rp, ni, bc, data->middle, bbt, ww - 2, bbt, ww - 2, wh - 2);
        if (bb > 2) ShadeLine(mpen, tc, data, rp, ni, bc, data->middle, wh - 2, 1, wh - 2, ww - 2, wh - 2);
        if (bb > 3) {
            if ((bl > 0) && (br > 0)) ShadeLine(lpen, tc, data, rp, ni, bc, data->light, wh - bb + 1, bl, wh - bb + 1, ww - br, wh - bb + 1);
        }
        if (br > 3) {
            if (bb > 1) ShadeLine(lpen, tc, data, rp, ni, bc, data->light, bbt, ww - br + 1, bbt, ww - br + 1, wh - bb + 1);
        }
    }
    return TRUE;
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

void DrawPartialTitleBar(struct WindowData *wd, struct windecor_data *data, struct Window *window, struct RastPort *dst_rp, struct DrawInfo *dri, UWORD align, UWORD start, UWORD width, UWORD *pens)
{
    int                 xl0, xl1, xr0, xr1, defwidth;
    ULONG               textlen = 0, titlelen = 0, textpixellen = 0;
    struct TextExtent   te;
    struct RastPort    *rp;
    struct NewImage    *ni = NULL;

    BOOL                hasdepth;
    BOOL                haszoom;
    BOOL                hasclose;
    BOOL                hasdrag;
    BOOL                hastitle;
    BOOL                hastitlebar;
    UWORD               textstart = 0, barh, x;
    ULONG                   bc, color, s_col, e_col, arc;
    int                     dy;

    LONG    pen = -1;

    if ((wd->rp == NULL) || (window->Width != wd->w) || (window->BorderTop != wd->h))
    {
        if (wd->rp)
        {
            FreeBitMap(wd->rp->BitMap);
            FreeRastPort(wd->rp);
        }



        wd->h = window->BorderTop;
        wd->w = window->Width;
        wd->rp = NULL;


        rp = CreateRastPort();
        if (rp)
        {
            SetFont(rp, dri->dri_Font);
            rp->BitMap = AllocBitMap(window->Width, window->BorderTop, 1, 0, window->WScreen->RastPort.BitMap);
            if (rp->BitMap == NULL)
            {
                FreeRastPort(rp);
                return;
            }
        } else return;

        wd->rp = rp;

    } else rp = wd->rp;

    hastitle = window->Title != NULL ? TRUE : FALSE;
    hasclose = (window->Flags & WFLG_CLOSEGADGET) ? TRUE : FALSE;
    hasdepth = (window->Flags & WFLG_DEPTHGADGET) ? TRUE : FALSE;
    hasdrag = (window->Flags & WFLG_DRAGBAR) ? TRUE : FALSE;
    haszoom = ((window->Flags & WFLG_HASZOOM) || ((window->Flags & WFLG_SIZEGADGET) && hasdepth)) ? TRUE : FALSE;
    hastitlebar = (window->BorderTop == data->winbarheight) ? TRUE : FALSE;

    if (wd->img_border_normal->ok) ni = wd->img_border_normal;

    if (ni == NULL) data->usegradients = TRUE;

    color = 0x00cccccc;

    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        s_col = data->a_col_s;
        e_col = data->a_col_e;
        arc = data->a_arc;
        dy = 0;
        bc = data->b_col_a;
        pen = wd->ActivePen;
    } else {
        s_col = data->d_col_s;
        e_col = data->d_col_e;
        arc = data->d_arc;
        dy = data->winbarheight;
        bc = data->b_col_d;
        if (!data->usegradients)
        {
            if (wd->img_border_deactivated->ok) ni = wd->img_border_deactivated;
        }
        pen = wd->DeactivePen;
    }


    if (data->filltitlebar)
    {
        if (data->usegradients) FillPixelArrayGradient(pen, wd->truecolor, rp, 0, 0, window->Width - 1, window->Height - 1, 0, 0, window->Width, window->BorderTop, s_col, e_col, arc);
        else DrawTileToRP(rp, ni, color, 0, 0, 0, 0, window->Width, window->BorderTop);
    }
    
    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        dy = 0;
    }
    else
    {
        dy = data->winbarheight;
    }
    getleftgadgetsdimensions(data, window, &xl0, &xl1);
    getrightgadgetsdimensions(data, window, &xr0, &xr1);
    defwidth = (xl0 != xl1) ? data->BarPreGadget_s : data->BarPre_s;
    if(xr1 == 0)
    {
        xr1 = window->Width - data->BarPre_s;
        xr0 = window->Width - data->BarPre_s;
    }

    defwidth += (xl1 - xl0);

    defwidth += data->BarJoinGB_s;
    defwidth += data->BarJoinBT_s;
    defwidth += data->BarJoinTB_s;
    defwidth += data->BarJoinBG_s;
    defwidth += (xr1 - xr0);
    defwidth += (xr0 != xr1) ? data->BarPostGadget_s : data->BarPost_s;

    if (defwidth >= window->Width) hastitle = FALSE;

    if (hastitle)
    {
        titlelen = strlen(window->Title);
        textlen = TextFit(rp, window->Title, titlelen, &te, NULL, 1, window->Width-defwidth, window->BorderTop - 2);
        if (textlen)
        {
            textpixellen = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
        }
    }

    if (wd->img_winbar_normal->ok && hastitlebar)
    {
        barh =  data->img_winbar_normal->h;
        if (data->barvert)
        {
            if (barh > data->winbarheight) barh =  data->winbarheight;
        }
        x = 0;
        if (xl0 != xl1)
        {
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarPreGadget_o, dy, data->BarPreGadget_s, barh, x, 0, data->BarPreGadget_s, barh);
            if ((xl1-xl0) > 0) x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarLGadgetFill_o, dy, data->BarLGadgetFill_s, barh, x, 0, xl1-xl0, barh);
        }
        else
        {
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarPre_o, dy, data->BarPre_s, barh, x, 0, data->BarPreGadget_s, barh);
        }
        x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarJoinGB_o, dy, data->BarJoinGB_s, barh, x, 0, data->BarJoinGB_s, barh);
        if (hastitle && (textlen > 0))
        {
            switch(align)
            {
                case WD_DWTA_CENTER:
                    //BarLFill
                    x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarLFill_o, dy, data->BarLFill_s, barh, x, 0, 60, barh);
                    break;
                case WD_DWTA_RIGHT:
                    //BarLFill
                    break;
                default:
                case WD_DWTA_LEFT:
                    break;
            }
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarJoinBT_o, dy, data->BarJoinBT_s, barh, x, 0, data->BarJoinBT_s, barh);
            textstart = x;
            if (textpixellen > 0) x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarTitleFill_o, dy, data->BarTitleFill_s, barh, x, 0, textpixellen, barh);
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarJoinTB_o, dy, data->BarJoinTB_s, barh, x, 0, data->BarJoinTB_s, barh);
        }
        x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarRFill_o, dy, data->BarRFill_s, barh, x, 0, xr0 - x - data->BarJoinBG_s, barh);
        x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarJoinBG_o, dy, data->BarJoinBG_s, barh, x, 0, data->BarJoinBG_s, barh);
        if ((xr1-xr0) > 0) x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarRGadgetFill_o, dy, data->BarRGadgetFill_s, barh, x, 0, xr1-xr0, barh);
        if (xr0 != xr1)
        {
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarPostGadget_o, dy, data->BarPostGadget_s, barh, x, 0, data->BarPostGadget_s, barh);
        }
        else
        {
            x = WriteTiledImageTitle(data->filltitlebar, window, rp, wd->img_winbar_normal, data->BarPost_o, dy, data->BarPost_s, barh, x, 0, data->BarPost_s, barh);
        }
    }

    if ((textlen > 0) && hastitle)
    {
        SetAPen(rp, pens[(window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) ? FILLTEXTPEN : TEXTPEN]);
        SetDrMd(rp, JAM1);
        UWORD   tx = textstart;
        UWORD   ty = ((data->winbarheight - dri->dri_Font->tf_YSize) >> 1) + dri->dri_Font->tf_Baseline;

        if (!wd->truecolor || ((data->outline == FALSE) && (data->shadow == FALSE)))
        {
            Move(rp, tx, ty);
            Text(rp, window->Title, textlen);
        }
        else if (data->outline)
        {

                SetSoftStyle(rp, FSF_BOLD, AskSoftStyle(rp));
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->shadow_col, TAG_DONE);

                Move(rp, tx + 1, ty ); Text(rp, window->Title, textlen);
                Move(rp, tx + 2, ty ); Text(rp, window->Title, textlen);
                Move(rp, tx , ty ); Text(rp, window->Title, textlen);
                Move(rp, tx, ty + 1);  Text(rp, window->Title, textlen);
                Move(rp, tx, ty + 2);  Text(rp, window->Title, textlen);
                Move(rp, tx + 1, ty + 2);  Text(rp, window->Title, textlen);
                Move(rp, tx + 2, ty + 1);  Text(rp, window->Title, textlen);
                Move(rp, tx + 2, ty + 2);  Text(rp, window->Title, textlen);

                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->text_col, TAG_DONE);
                Move(rp, tx + 1, ty + 1);
                Text(rp, window->Title, textlen);
                SetSoftStyle(rp, FS_NORMAL, AskSoftStyle(rp));
        }
        else
        {
                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->shadow_col, TAG_DONE);
                Move(rp, tx + 1, ty + 1 );
                Text(rp, window->Title, textlen);

                SetRPAttrs(rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, data->text_col, TAG_DONE);
                Move(rp, tx, ty);
                Text(rp, window->Title, textlen);

        }
    }
    struct Gadget *g;

    for (g = window->FirstGadget; g; g = g->NextGadget)
    {
        if (g->Activation & GACT_TOPBORDER && (g->GadgetType & GTYP_SYSTYPEMASK) != GTYP_WDRAGGING)
        {
            int x, y;
            y = g->TopEdge;
            if (!(g->Flags & GFLG_RELRIGHT))
            {
                x = g->LeftEdge;
            }
            else
            {
                x = g->LeftEdge + window->Width - 1;
            }
            struct NewImage *ni = NULL;
            UWORD state = IDS_NORMAL;

            if ((window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) == 0)
            {
                state = IDS_INACTIVENORMAL;
            }
            else  if (g->Flags & GFLG_SELECTED) state = IDS_SELECTED;

            if (g->GadgetType & GTYP_SYSTYPEMASK) {
                switch(g->GadgetType & GTYP_SYSTYPEMASK)
                {
                    case GTYP_CLOSE:
                        ni = wd->img_close;
                        break;
                    case GTYP_WDEPTH:
                        ni = wd->img_depth;
                        break;
                    case GTYP_WZOOM:
                        ni = wd->img_zoom;
                        break;
                }
            }
            else
            {
                switch(g->GadgetID)
                {
                    case ETI_MUI:
                        ni = wd->img_mui;
                        break;

                    case ETI_PopUp:
                        ni = wd->img_popup;
                        break;

                    case ETI_Snapshot:
                        ni = wd->img_snapshot;
                        break;

                    case ETI_Iconify:
                        ni = wd->img_iconify;
                        break;

                    case ETI_Lock:
                        ni = wd->img_lock;
                        break;
                }
            }

            if (ni) DrawAlphaStateImageToRP(data, rp, ni, state, x, y, TRUE);
        }
    }
    BltBitMapRastPort(rp->BitMap, start, 0, dst_rp, start, 0, width, window->BorderTop, 0xc0);
}

/**************************************************************************************************/


IPTR windecor_layout_bordergadgets(Class *cl, Object *obj, struct wdpLayoutBorderGadgets *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct Window          *window = msg->wdp_Window;
    struct Gadget          *gadget = msg->wdp_Gadgets;
    struct Gadget          *draggadget = NULL;
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;
    ULONG                   eb = msg->wdp_ExtraButtons;

    BOOL                    hasdepth;
    BOOL                    haszoom;
    BOOL                    hasclose;
    BOOL                    hasdrag;
    BOOL                    hastitle;
    BOOL                    hassize;
    BOOL                    borderless;
    LONG                    width;
    LONG                    rightborder = 0;
    LONG                    bottomborder = 0;

    DoSuperMethodA(cl, obj, (Msg)msg);

    hastitle = window->Title != NULL ? TRUE : FALSE;
    hasclose = (window->Flags & WFLG_CLOSEGADGET) ? TRUE : FALSE;
    hassize = (window->Flags & WFLG_SIZEGADGET) ? TRUE : FALSE;
    hasdepth = (window->Flags & WFLG_DEPTHGADGET) ? TRUE : FALSE;
    hasdrag = (window->Flags & WFLG_DRAGBAR) ? TRUE : FALSE;
    borderless = (window->Flags & WFLG_BORDERLESS) ? TRUE : FALSE;
    haszoom = ((window->Flags & WFLG_HASZOOM) || ((window->Flags & WFLG_SIZEGADGET) && hasdepth)) ? TRUE : FALSE;


    if ((msg->wdp_Flags & WDF_LBG_SYSTEMGADGET) != 0)
    {
        if (gadget->GadgetType == GTYP_CUSTOMGADGET)
        {
            switch(gadget->GadgetID)
            {
                case ETI_MUI:
                    if (wd->img_mui->ok)
                    {
                        if (data->threestate) width = (wd->img_mui->w / 3); else width = (wd->img_mui->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_mui->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }
                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }
                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

                case ETI_PopUp:
                    if (wd->img_popup->ok)
                    {
                        if (data->threestate) width = (wd->img_popup->w / 3); else width = (wd->img_popup->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_popup->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if ((eb & ETG_MUI) != 0)
                        {
                            if (wd->img_mui->ok)
                            {
                                if (data->threestate) width += (wd->img_mui->w / 3); else width += (wd->img_mui->w >> 2);
                            }
                        }

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }

                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }
                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

                case ETI_Snapshot:
                    if (wd->img_snapshot->ok)
                    {
                        if (data->threestate) width = (wd->img_snapshot->w / 3); else width = (wd->img_snapshot->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_snapshot->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if ((eb & ETG_MUI) != 0)
                        {

                            if (wd->img_mui->ok)
                            {
                                if (data->threestate) width += (wd->img_mui->w / 3); else width += (wd->img_mui->w >> 2);
                            }
                        }

                        if ((eb & ETG_POPUP) != 0)
                        {
                            if (wd->img_popup->ok)
                            {
                                if (data->threestate) width += (wd->img_popup->w / 3); else width += (wd->img_popup->w >> 2);
                            }
                        }

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }

                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }

                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }

                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

                case ETI_Iconify:
                    if (wd->img_iconify->ok)
                    {
                        if (data->threestate) width = (wd->img_iconify->w / 3); else width = (wd->img_iconify->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_iconify->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if ((eb & ETG_MUI) != 0)
                        {
                            if (wd->img_mui->ok)
                            {
                                if (data->threestate) width += (wd->img_mui->w / 3); else width += (wd->img_mui->w >> 2);
                            }
                        }

                        if ((eb & ETG_POPUP) != 0)
                        {
                            if (wd->img_popup->ok)
                            {
                                if (data->threestate) width += (wd->img_popup->w / 3); else width += (wd->img_popup->w >> 2);
                            }
                        }

                        if ((eb & ETG_SNAPSHOT) != 0)
                        {
                            if (wd->img_snapshot->ok)
                            {
                                if (data->threestate) width += (wd->img_snapshot->w / 3); else width += (wd->img_snapshot->w >> 2);
                            }
                        }

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }

                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }
                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

                case ETI_Lock:
                    if (wd->img_lock->ok)
                    {
                        if (data->threestate) width = (wd->img_lock->w / 3); else width = (wd->img_lock->w >> 2);

                        gadget->Width = width;
                        gadget->Height = wd->img_lock->h;
                        gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;

                        if ((eb & ETG_MUI) != NULL)
                        {
                            if (wd->img_mui->ok)
                            {
                                if (data->threestate) width += (wd->img_mui->w / 3); else width += (wd->img_mui->w >> 2);
                            }
                        }

                        if ((eb & ETG_POPUP) != NULL)
                        {
                            if (wd->img_popup->ok)
                            {
                                if (data->threestate) width += (wd->img_popup->w / 3); else width += (wd->img_popup->w >> 2);
                            }
                        }

                        if ((eb & ETG_SNAPSHOT) != NULL)
                        {
                            if (wd->img_snapshot->ok)
                            {
                                if (data->threestate) width += (wd->img_snapshot->w / 3); else width += (wd->img_snapshot->w >> 2);
                            }
                        }

                        if ((eb & ETG_ICONIFY) != NULL)
                        {
                            if (wd->img_iconify->ok)
                            {
                                if (data->threestate) width += (wd->img_iconify->w / 3); else width += (wd->img_iconify->w >> 2);
                            }
                        }

                        if (haszoom)
                        {
                            if (data->threestate) width += (wd->img_zoom->w / 3); else width += (wd->img_zoom->w >> 2);
                        }

                        if (hasclose && data->closeright)
                        {
                            if (data->threestate) width += (wd->img_close->w / 3); else width += (wd->img_close->w >> 2);
                        }
                        if (hasdepth)
                        {
                            if (data->threestate) width += (wd->img_depth->w / 3); else width += (wd->img_depth->w >> 2);
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        else
                        {
                            gadget->LeftEdge = -data->BarPostGadget_s - width;
                        }
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;

                    }
                    break;

            }
        }
        else
        {
            switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
            {
                case GTYP_CLOSE:
                    if (data->threestate) width = (data->img_close->w / 3); else width = (data->img_close->w >> 2);
                    gadget->Width = width;
                    wd->closewidth = width;
                    gadget->Height = data->img_close->h;
                    if (data->closeright)
                    {
                        gadget->Flags &= ~GFLG_RELWIDTH;
                        gadget->Flags |= GFLG_RELRIGHT;
                        gadget->LeftEdge = -data->BarPostGadget_s - width;
                    }
                    else
                    {
                        gadget->LeftEdge = data->BarPreGadget_s;
                    }
                    gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;
                    break;
    
                case GTYP_WDEPTH:
                    if (data->threestate) width = (data->img_depth->w / 3); else width = (data->img_depth->w >> 2);
                    gadget->Width = width;
                    wd->depthwidth = width;
                    gadget->Height = data->img_depth->h;
                    if (hasclose && data->closeright)
                    {
                        if (data->threestate) width += (data->img_close->w / 3); else width += (data->img_close->w >> 2);
                    }
                    gadget->LeftEdge = -data->BarPostGadget_s - width;
                    gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;
                    gadget->Flags &= ~GFLG_RELWIDTH;
                    gadget->Flags |= GFLG_RELRIGHT;
                    break;
    
                case GTYP_WZOOM:
                    if (data->threestate) width = (data->img_zoom->w / 3); else width = (data->img_zoom->w >> 2);
                    gadget->Width = width;
                    wd->zoomwidth = width;
                    gadget->Height = data->img_zoom->h;
                    gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;
                    if (hasclose && data->closeright)
                    {
                        if (data->threestate) width += (data->img_close->w / 3); else width += (data->img_close->w >> 2);
                    }
                    if (hasdepth)
                    {
                        if (data->threestate) width += (data->img_depth->w / 3); else width += (data->img_depth->w >> 2);
                        gadget->LeftEdge = -data->BarPostGadget_s - width;
                    }
                    else
                    {
                        gadget->LeftEdge = -data->BarPostGadget_s - width;
                    }
                    gadget->Flags &= ~GFLG_RELWIDTH;
                    gadget->Flags |= GFLG_RELRIGHT;
    
                    break;
    
                case GTYP_SIZING:
                    rightborder = data->rightbordergads;
                    if ((gadget->Flags & WFLG_SIZEBBOTTOM) != 0) bottomborder = data->bottombordergads;
                    break;
    
                case GTYP_WDRAGGING:
                    break;
    
            }
        }
        return TRUE;
    }

    int sysrgad = -data->BarPostGadget_s - 1;

    if (data->closeright && hasclose) sysrgad -= wd->closewidth;
    if (hasdepth) sysrgad -= wd->depthwidth;
    if (haszoom) sysrgad -= wd->zoomwidth;
    while(gadget)
    {
        if ((gadget->GadgetType & GTYP_SYSTYPEMASK) == 0)
        {
        switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
        {
                case GTYP_WDRAGGING:
                   break;

                default:
                if ((gadget->Flags & GFLG_EXTENDED) != 0)
                {
                    if ((((struct ExtGadget *) gadget)->MoreFlags & GMORE_BOOPSIGADGET) != 0)
                    {
                        ULONG   rtsm;
                        get((Object *) gadget, GA_RightBorder, &rtsm);
                        if (rtsm)
                        {
                            if (get((Object *) gadget, PGA_Top, &rtsm))
                            {
                                SetAttrs((Object *) gadget, GA_RelRight, - data->rightbordergads + ((data->rightbordergads - (data->img_verticalcontainer->w >> 1) + 1) >> 1) + 1, GA_Width, data->img_verticalcontainer->w >> 1, TAG_DONE);
                            }
                            else
                            {
                                get((Object *) gadget, GA_Width, &rtsm);
                                SetAttrs((Object *) gadget, GA_RelRight, - data->rightbordergads + ((data->rightbordergads - rtsm + 1) >> 1) + 1, TAG_DONE);
                            }
                        }
                        else
                        {
                            get((Object *) gadget, GA_BottomBorder, &rtsm);
                            if (rtsm)
                            {
                                if (get((Object *) gadget, PGA_Top, &rtsm))
                                {
                                    SetAttrs((Object *) gadget, GA_RelBottom, - data->bottombordergads + ((data->bottombordergads - (data->img_horizontalcontainer->h >> 1) + 1)  >> 1) +1, GA_Height, (data->img_horizontalcontainer->h >> 1), TAG_DONE);
                                }
                                else
                                {
                                    get((Object *) gadget, GA_Height, &rtsm);
                                    SetAttrs((Object *) gadget, GA_RelBottom, - data->bottombordergads + ((data->bottombordergads - rtsm + 1) >> 1) + 1, TAG_DONE);
                                }
                            }
                        }
                    }
                }
                break;
                }
        }
        if (msg->wdp_Flags & WDF_LBG_MULTIPLE)
        {
            gadget = gadget->NextGadget;
        }
        else
        {
            gadget = NULL;
        }
   }
    gadget = msg->wdp_Gadgets;

    while(gadget)
    {
        if ((gadget->GadgetType & GTYP_SYSTYPEMASK) == 0)
        {
            if ((gadget->Activation & GACT_TOPBORDER) != 0)
            {
                if ((gadget->Flags & GFLG_RELRIGHT) != 0)
                {
                    gadget->TopEdge = (data->winbarheight - gadget->Height) / 2;
                    sysrgad -= gadget->Width;
                    gadget->LeftEdge = sysrgad;
                }
            }
        }
        gadget = gadget->NextGadget;
    }

    gadget = msg->wdp_Gadgets;

    if ((msg->wdp_Flags & WDF_LBG_SYSTEMGADGET) != 0) while(gadget)
    {
        switch(gadget->GadgetType & GTYP_SYSTYPEMASK)
        {
            case GTYP_WDRAGGING:
                gadget->Width = sysrgad;
                if (hasclose && !data->closeright)
                {
                    gadget->Width -= data->BarPreGadget_s;
                    if (data->threestate) gadget->Width -= (data->img_close->w / 3); else gadget->Width -= (data->img_close->w >> 2);
                }
                break;
        }
        gadget = gadget->NextGadget;
    }

    if (draggadget)
    {
    }

    return TRUE;
}

/**************************************************************************************************/

IPTR windecor_draw_borderpropback(Class *cl, Object *obj, struct wdpDrawBorderPropBack *msg)
{
    /* Simply return, we need to render the back in the knob method    */
    /* because we want to use irregular (alpha images) for the sliders */
    return TRUE;
}

/**************************************************************************************************/

IPTR windecor_draw_borderpropknob(Class *cl, Object *obj, struct wdpDrawBorderPropKnob *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct Window          *window = msg->wdp_Window;
    struct RastPort        *winrp = msg->wdp_RPort;
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;

    struct RastPort        *rp;
    struct Gadget          *gadget = msg->wdp_Gadget;
    struct Rectangle       *r;
    struct PropInfo        *pi = ((struct PropInfo *)gadget->SpecialInfo);
    struct NewImage        *ni = NULL;
    BOOL                    hit = (msg->wdp_Flags & WDF_DBPK_HIT) ? TRUE : FALSE;
    ULONG                   y, x, bx0, bx1, by0, by1;
    int                     size, is, pos;
    ULONG                   bc, color, s_col, e_col, arc;
    LONG    pen = -1;

    if (!(pi->Flags & PROPNEWLOOK) || (gadget->Activation && (GACT_RIGHTBORDER | GACT_BOTTOMBORDER) == 0))
    {
        return DoSuperMethodA(cl, obj, (Msg)msg);
    }

    r = msg->wdp_PropRect;

    bx0 = r->MinX;
    by0 = r->MinY;
    bx1 = r->MaxX;
    by1 = r->MaxY;

    rp = CreateRastPort();
    if (rp)
    {
        rp->BitMap = AllocBitMap(bx1 - bx0 + 1, by1 - by0 + 1, 1, 0, window->WScreen->RastPort.BitMap);
        if (rp->BitMap == NULL)
        {
            FreeRastPort(rp);
            return FALSE;
        }
    }
    else return FALSE;

    color = 0x00cccccc;

    if (wd->img_border_normal->ok) ni = wd->img_border_normal;

    if (ni == NULL) data->usegradients = TRUE;

    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        s_col = data->a_col_s;
        e_col = data->a_col_e;
        arc = data->a_arc;
        bc = data->b_col_a;
        pen = wd->ActivePen;
    }
    else
    {
        s_col = data->d_col_s;
        e_col = data->d_col_e;
        arc = data->d_arc;
        bc = data->b_col_d;
        if (!data->usegradients)
        {
            if (wd->img_border_deactivated->ok) ni = wd->img_border_deactivated;
        }
        pen = wd->DeactivePen;
    }

    if (data->usegradients)
    {

        FillPixelArrayGradientDelta(pen, wd->truecolor, rp, 0, 0, window->Width-1, window->Height-1,  0, 0, bx1 - bx0 + 1, by1 - by0 + 1, s_col, e_col, arc, 0, 0);

    }
    else
    {


        if (ni->ok != NULL)
        {
            ULONG   color = 0x00cccccc;

            DrawTileToRPRoot(rp, ni, color, 0, 0, bx0, by0, bx1 - bx0 + 1, by1 - by0 + 1);
        }

    }

    r = msg->wdp_PropRect;

    bx0 = 0;
    by0 = 0;
    bx1 = r->MaxX - r->MinX;
    by1 = r->MaxY - r->MinY;

    if ((pi->Flags & FREEVERT) != 0)
    {

        is = data->img_verticalcontainer->w >> 1;
        if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) pos = 0; else pos = is;
        y = by0;
        size = by1 - by0 - data->ContainerTop_s - data->ContainerBottom_s + 1;
        y = WriteTiledImageVertical(rp, wd->img_verticalcontainer, pos, data->ContainerTop_o, is, data->ContainerTop_s, bx0, y, is, data->ContainerTop_s);
        if (size > 0) y = WriteTiledImageVertical(rp, wd->img_verticalcontainer, pos, data->ContainerVertTile_o, is, data->ContainerVertTile_s, bx0, y, is, size);

        y = WriteTiledImageVertical(rp, wd->img_verticalcontainer, pos, data->ContainerBottom_o, is, data->ContainerBottom_s, bx0, y, is, data->ContainerBottom_s);

    }
    else if ((pi->Flags & FREEHORIZ) != 0)
    {

        is = data->img_horizontalcontainer->h >> 1;
        if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) pos = 0; else pos = is;
        x = bx0;
        size = bx1 - bx0 - data->ContainerLeft_s - data->ContainerRight_s + 1;
        x = WriteTiledImageHorizontal(rp, wd->img_horizontalcontainer, data->ContainerLeft_o, pos, data->ContainerLeft_s, is, x, by0, data->ContainerLeft_s, is);
        if (size > 0) x = WriteTiledImageHorizontal(rp, wd->img_horizontalcontainer, data->ContainerHorTile_o, pos, data->ContainerHorTile_s, is, x, by0, size, is);
        x = WriteTiledImageHorizontal(rp, wd->img_horizontalcontainer, data->ContainerRight_o, pos, data->ContainerRight_s, is, x, by0, data->ContainerRight_s, is);
    }

    bx0 = msg->wdp_PropRect->MinX;
    by0 = msg->wdp_PropRect->MinY;
    bx1 = msg->wdp_PropRect->MaxX;
    by1 = msg->wdp_PropRect->MaxY;

    r = msg->wdp_RenderRect;
    if ((pi->Flags & FREEVERT) != 0)
    {
        is = data->img_verticalknob->w / 3;
        if (hit) pos = is; else if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) pos = 0; else pos = is * 2;
        y = r->MinY - by0;
        size = r->MaxY - r->MinY - data->KnobTop_s - data->KnobBottom_s + 1;

        y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobTop_o, is, data->KnobTop_s, r->MinX - bx0, y, is, data->KnobTop_s);
        if (size > 0)
        {
            if (size > data->KnobVertGripper_s)
            {
                size = size - data->KnobVertGripper_s;
                int size_bak = size;
                size = size / 2;
                if (size > 0) y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobTileTop_o, is, data->KnobTileTop_s, r->MinX - bx0, y, is, size);
                y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobVertGripper_o, is, data->KnobVertGripper_s, r->MinX - bx0, y, is, data->KnobVertGripper_s);
                size = size_bak - size;
                if (size > 0) y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobTileBottom_o, is, data->KnobTileBottom_s, r->MinX - bx0, y, is, size);
            }
            else
            {
                y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobTileTop_o, is, data->KnobTileTop_s, r->MinX - bx0, y, is, size);
            }
        }
        y = WriteTiledImageVertical(rp, wd->img_verticalknob, pos, data->KnobBottom_o, is, data->KnobBottom_s, r->MinX - bx0, y, is, data->KnobBottom_s);
    }
    else if ((pi->Flags & FREEHORIZ) != 0)
    {

        is = data->img_horizontalknob->h / 3;
        if (hit) pos = is; else if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX)) pos = 0; else pos = is * 2;
        x = r->MinX - bx0;
        size = r->MaxX - r->MinX - data->KnobLeft_s - data->KnobRight_s + 1;
        x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobLeft_o, pos, data->KnobLeft_s, is, x, r->MinY - by0, data->KnobLeft_s, is);

        if (size > 0)
        {
            if (size > data->KnobHorGripper_s)
            {
                size = size - data->KnobHorGripper_s;
                int size_bak = size;
                size = size / 2;
                if (size > 0) x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobTileLeft_o, pos, data->KnobTileLeft_s, is, x, r->MinY - by0, size, is);
                x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobHorGripper_o, pos, data->KnobHorGripper_s, is, x, r->MinY - by0, data->KnobHorGripper_s, is);
                size = size_bak - size;
                if (size > 0) x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobTileRight_o, pos, data->KnobTileRight_s, is, x, r->MinY - by0, size, is);
            }
            else
            {
                x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobTileRight_o, pos, data->KnobTileRight_s, is, x, r->MinY - by0, size, is);
            }
        }
        x = WriteTiledImageHorizontal(rp, wd->img_horizontalknob, data->KnobRight_o, pos, data->KnobRight_s, is, x, r->MinY - by0, data->KnobRight_s, is);
    }

    BltBitMapRastPort(rp->BitMap, 0, 0, winrp, msg->wdp_PropRect->MinX, msg->wdp_PropRect->MinY, bx1 - bx0 + 1, by1 - by0 + 1, 0xc0);

    FreeBitMap(rp->BitMap);
    FreeRastPort(rp);

    return TRUE;
}

IPTR windecor_getdefsizes(Class *cl, Object *obj, struct wdpGetDefSizeSysImage *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct NewImage        *n = NULL;
    WORD                    w = 0, h = 0;
    BOOL                    isset = FALSE;
    switch(msg->wdp_Which)
    {
        case SIZEIMAGE:
            n = NULL;
            w = data->rightbordergads;
            h = data->bottombordergads;
            isset = TRUE;
            break;

        case CLOSEIMAGE:
            n = data->img_close;
            isset = TRUE;
            break;

        case MUIIMAGE:
            n = data->img_mui;
            isset = TRUE;
            break;

        case POPUPIMAGE:
            n = data->img_popup;
            isset = TRUE;
            break;

        case SNAPSHOTIMAGE:
            n = data->img_snapshot;
            isset = TRUE;
            break;

        case ICONIFYIMAGE:
            n = data->img_iconify;
            isset = TRUE;
            break;

        case LOCKIMAGE:
            n = data->img_lock;
            isset = TRUE;
            break;

        case UPIMAGE:
            n = NULL;
            w = data->rightbordergads;
            h = data->img_up->h + data->updownaddy;
            isset = TRUE;
            break;

        case DOWNIMAGE:
            n = NULL;
            w = data->rightbordergads;
            h = data->img_down->h + data->updownaddy;
            isset = TRUE;
            break;

        case LEFTIMAGE:
            n = NULL;
            if (data->threestate) w = (data->img_left->w / 3); else w = (data->img_left->w >> 2);
            w += data->leftrightaddx;
            h = data->bottombordergads;
            isset = TRUE;
            break;

        case RIGHTIMAGE:
            n = NULL;
            if (data->threestate) w = (data->img_right->w / 3); else w = (data->img_right->w >> 2);
            w += data->leftrightaddx;
            h = data->bottombordergads;
            isset = TRUE;
            break;

        case DEPTHIMAGE:
            n = data->img_depth;
            isset = TRUE;
            break;

        case ZOOMIMAGE:
            n = data->img_zoom;
            isset = TRUE;
            break;

        default:
            return FALSE;
    }

    if (!isset) return DoSuperMethodA(cl, obj, (Msg) msg);

    if (n == NULL)
    {
        *msg->wdp_Width = w;
        *msg->wdp_Height = h;
    }
    else
    {
        if (n->ok) {
            if (data->threestate)
            {
                *msg->wdp_Width = (n->w / 3);
                *msg->wdp_Height = n->h;
            }
            else
            {
                *msg->wdp_Width = (n->w >> 2);
                *msg->wdp_Height = n->h;
            }
        } else return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    return TRUE;
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


void DrawShapePartialTitleBar(struct WindowData *wd, struct NewLUT8Image *shape, struct windecor_data *data, struct Window *window, UWORD align, UWORD start, UWORD width)
{
    int                 xl0, xl1, xr0, xr1, defwidth;
    ULONG               textlen = 0, titlelen = 0, textpixellen = 0;
    struct TextExtent   te;
    struct NewImage    *ni;

    BOOL                hastitle;
    BOOL                hastitlebar;
    UWORD               textstart = 0, barh, x;
    int                     dy;

    struct RastPort    *rp = &window->WScreen->RastPort;
    hastitle = window->Title != NULL ? TRUE : FALSE;
    hastitlebar = (window->BorderTop == data->winbarheight) ? TRUE : FALSE;

    if (window->Flags & (WFLG_WINDOWACTIVE | WFLG_TOOLBOX))
    {
        dy = 0;
    }
    else
    {
        dy = data->winbarheight;
    }
    getleftgadgetsdimensions(data, window, &xl0, &xl1);
    getrightgadgetsdimensions(data, window, &xr0, &xr1);

    defwidth = (xl0 != xl1) ? data->BarPreGadget_s : data->BarPre_s;
    if(xr1 == 0)
    {
        xr1 = window->Width - data->BarPre_s;
        xr0 = window->Width - data->BarPre_s;
    }

    defwidth += (xl1 - xl0);

    defwidth += data->BarJoinGB_s;
    defwidth += data->BarJoinBT_s;
    defwidth += data->BarJoinTB_s;
    defwidth += data->BarJoinBG_s;
    defwidth += (xr1 - xr0);
    defwidth += (xr0 != xr1) ? data->BarPostGadget_s : data->BarPost_s;

    if (defwidth >= window->Width) hastitle = FALSE;
 
    if (hastitle)
    {
        titlelen = strlen(window->Title);
        textlen = TextFit(rp, window->Title, titlelen, &te, NULL, 1, window->Width-defwidth, window->BorderTop - 2);
        if (textlen)
        {
            textpixellen = te.te_Extent.MaxX - te.te_Extent.MinX + 1;
        }
    }

 
    if (data->img_winbar_normal->ok && hastitlebar)
    {
        barh =  data->img_winbar_normal->h;
        if (data->barvert)
        {
            if (barh > data->winbarheight) barh =  data->winbarheight;
        }
        x = 0;
        if (xl0 != xl1)
        {
            x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarPreGadget_o, dy, data->BarPreGadget_s, barh, x, 0, data->BarPreGadget_s, barh);
            if ((xl1-xl0) > 0) x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarLGadgetFill_o, dy, data->BarLGadgetFill_s, barh, x, 0, xl1-xl0, barh);
        }
        else
        {
            x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarPre_o, dy, data->BarPre_s, barh, x, 0, data->BarPreGadget_s, barh);
        }
        x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarJoinGB_o, dy, data->BarJoinGB_s, barh, x, 0, data->BarJoinGB_s, barh);
        if (hastitle && (textlen > 0))
        {
            switch(align)
            {
                case WD_DWTA_CENTER:
                    //BarLFill
                    x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarLFill_o, dy, data->BarLFill_s, barh, x, 0, 60, barh);
                    break;
                case WD_DWTA_RIGHT:
                    //BarLFill
                    break;
                default:
                case WD_DWTA_LEFT:
                    break;
            }
            x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarJoinBT_o, dy, data->BarJoinBT_s, barh, x, 0, data->BarJoinBT_s, barh);
            textstart = x;
            if (textpixellen > 0) x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarTitleFill_o, dy, data->BarTitleFill_s, barh, x, 0, textpixellen, barh);
            x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarJoinTB_o, dy, data->BarJoinTB_s, barh, x, 0, data->BarJoinTB_s, barh);
        }
        x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarRFill_o, dy, data->BarRFill_s, barh, x, 0, xr0 - x - data->BarJoinBG_s, barh);
        x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarJoinBG_o, dy, data->BarJoinBG_s, barh, x, 0, data->BarJoinBG_s, barh);
        if ((xr1-xr0) > 0) x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarRGadgetFill_o, dy, data->BarRGadgetFill_s, barh, x, 0, xr1-xr0, barh);
        if (xr0 != xr1)
        {
            x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarPostGadget_o, dy, data->BarPostGadget_s, barh, x, 0, data->BarPostGadget_s, barh);
        }
        else
        {
            x = WriteTiledImageShape(data->filltitlebar, window, shape, data->img_winbar_normal, data->BarPost_o, dy, data->BarPost_s, barh, x, 0, data->BarPost_s, barh);
        }
    }
}




/**************************************************************************************************/
IPTR windecor_windowshape(Class *cl, Object *obj, struct wdpWindowShape *msg)
{
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct WindowData      *wd = (struct WindowData *) msg->wdp_UserBuffer;
    struct Window          *window = msg->wdp_Window;

    if (data->barmasking)
    {
        struct  NewLUT8ImageContainer *shape;
        IPTR    back = NULL;
        shape = NewLUT8ImageContainer(window->Width, window->BorderTop);
        if (shape)
        {
            if (window->BorderTop == data->winbarheight) DrawShapePartialTitleBar(wd, shape, data, window, data->txt_align, 0, window->Width);
            back =(IPTR) RegionFromLUT8Image(msg->wdp_Width, msg->wdp_Height, shape);

            DisposeLUT8ImageContainer(shape);
            return back;

        }

    }

    if (!data->rounded) return (IPTR) NULL;

    struct  Region *newshape;

    int x2 = msg->wdp_Width-1;
    int y2 = msg->wdp_Height-1;
    
    if ((newshape = NewRegion()))
    {
        struct Rectangle rect;
        BOOL success = TRUE;

        rect.MinX = 9;
        rect.MinY = 0;
        rect.MaxX = x2 - 9;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 6;
        rect.MinY = 1;
        rect.MaxX = x2 - 6;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 4;
        rect.MinY = 2;
        rect.MaxX = x2 - 4;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 3;
        rect.MinY = 3;
        rect.MaxX = x2 - 3;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 2;
        rect.MinY = 4;
        rect.MaxX = x2 - 2;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 1;
        rect.MinY = 6;
        rect.MaxX = x2 - 1;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);

        rect.MinX = 0;
        rect.MinY = 9;
        rect.MaxX = x2;
        rect.MaxY = y2;
        success &= OrRectRegion(newshape, &rect);
    }
    return (IPTR) newshape;
}

/**************************************************************************************************/

IPTR windecor_initwindow(Class *cl, Object *obj, struct wdpInitWindow *msg)
{
    struct WindowData *wd = msg->wdp_UserBuffer;
    struct ScreenData *sd = msg->wdp_ScreenUserBuffer;
    struct windecor_data   *data = INST_DATA(cl, obj);
    struct Screen *screen = msg->wdp_Screen;

    wd->truecolor = msg->wdp_TrueColor;

    wd->ActivePen = sd->ActivePen;
    wd->DeactivePen = sd->DeactivePen;

    SETIMAGE_WIN(size);
    SETIMAGE_WIN(close);
    SETIMAGE_WIN(depth);
    SETIMAGE_WIN(zoom);
    SETIMAGE_WIN(up);
    SETIMAGE_WIN(down);
    SETIMAGE_WIN(left);
    SETIMAGE_WIN(right);
    SETIMAGE_WIN(mui);
    SETIMAGE_WIN(popup);
    SETIMAGE_WIN(snapshot);
    SETIMAGE_WIN(iconify);
    SETIMAGE_WIN(lock);
    SETIMAGE_WIN(winbar_normal);
    SETIMAGE_WIN(border_normal);
    SETIMAGE_WIN(border_deactivated);
    SETIMAGE_WIN(verticalcontainer);
    SETIMAGE_WIN(verticalknob);
    SETIMAGE_WIN(horizontalcontainer);
    SETIMAGE_WIN(horizontalknob);

    return TRUE;
}

/**************************************************************************************************/

IPTR windecor_exitwindow(Class *cl, Object *obj, struct wdpExitWindow *msg)
{
    struct WindowData *wd = (struct WindowData *) msg->wdp_UserBuffer;

    if (wd->rp)
    {
        if (wd->rp->BitMap) FreeBitMap(wd->rp->BitMap);
        FreeRastPort(wd->rp);
    }

    return TRUE;
}

IPTR windecor_dispatcher(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR retval;

    switch(msg->MethodID)
    {
        case OM_NEW:
            retval = windecor_new(cl, obj, (struct opSet *)msg);
            break;

        case OM_DISPOSE:
            retval = windecor_dispose(cl, obj, (struct opSet *)msg);
            break;

        case OM_GET:
            retval = windecor_get(cl, obj, (struct opGet *)msg);
            break;

        case WDM_DRAW_SYSIMAGE:
            retval = windecor_draw_sysimage(cl, obj, (struct wdpDrawSysImage *)msg);
            break;

        case WDM_DRAW_WINBORDER:
            retval = windecor_draw_winborder(cl, obj, (struct wdpDrawWinBorder *)msg);
            break;

        case WDM_LAYOUT_BORDERGADGETS:
            retval = windecor_layout_bordergadgets(cl, obj, (struct wdpLayoutBorderGadgets *)msg);
            break;

        case WDM_DRAW_BORDERPROPBACK:
            retval = windecor_draw_borderpropback(cl, obj, (struct wdpDrawBorderPropBack *)msg);
            break;

        case WDM_DRAW_BORDERPROPKNOB:
            retval = windecor_draw_borderpropknob(cl, obj, (struct wdpDrawBorderPropKnob *)msg);
            break;

        case WDM_GETDEFSIZE_SYSIMAGE:
            retval = windecor_getdefsizes(cl, obj, (struct wdpGetDefSizeSysImage *) msg);
            break;

        case WDM_WINDOWSHAPE:
            retval = windecor_windowshape(cl, obj, (struct wdpWindowShape *) msg);
            break;

        case WDM_INITWINDOW:
            retval = windecor_initwindow(cl, obj, (struct wdpInitWindow *) msg);
            break;

        case WDM_EXITWINDOW:
            retval = windecor_exitwindow(cl, obj, (struct wdpExitWindow *) msg);
            break;

        default:
            retval = DoSuperMethodA(cl, obj, msg);
            break;
    }

    return retval;
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
            *msg->opg_Storage = (APTR) INST_DATA(cl, obj);
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
    LONG                    left, right, titlelen = 0;
    BOOL                    hastitle = TRUE;

    if (sd->img_stitlebar.ok) WriteTiledImage(NULL, rp, &sd->img_stitlebar, 0, 0, data->img_stitlebar->w, data->img_stitlebar->h, 0, 0, scr->Width, data->img_stitlebar->h);
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
    struct ScreenData      *sd = (struct ScreenData *) msg->sdp_UserBuffer;

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
    struct ScreenData *sd = msg->sdp_UserBuffer;
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
    struct scrdecor_data *data = INST_DATA(cl, obj);
    struct ScreenData *sd = msg->sdp_UserBuffer;

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
    struct menudecor_data  *data = INST_DATA(cl, obj);
    struct ScreenData        *md = (struct ScreenData *) msg->mdp_UserBuffer;
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
            if (md->img_amigakey.ok)
            {
                ni = &md->img_amigakey;
                isset = TRUE;
            }
            break;

        case MENUCHECK:
            if (md->img_amigakey.ok)
            {
                ni = &md->img_menucheck;
                isset = TRUE;
            }
            break;

        case SUBMENUIMAGE:
            if (md->img_submenu.ok)
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

    struct menudecor_data  *data = INST_DATA(cl, obj);

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
        data->sd = (struct scrdecor_data *) GetTagData(MDA_ScreenData, NULL, msg->ops_AttrList);

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

void DisposeWindowSkinning(struct windecor_data *data)
{

}

BOOL InitWindowSkinning(STRPTR path, struct windecor_data *data) {
    char    buffer[256];
    char    *line, *v;
    BPTR    file;
    BPTR    lock;
    BPTR    olddir = 0;
    struct  screendecor_data *sd = data->sd;

    lock = Lock(path, ACCESS_READ);
    if (lock)
    {
        olddir = CurrentDir(lock);
    }
    else return FALSE;

    data->rounded = FALSE;
    data->threestate = FALSE;
    data->barmasking = FALSE;
    data->winbarheight = 0; //screen, window

    data->sizeaddx = 2;
    data->sizeaddy = 2;
    data->BarJoinTB_o = 0;
    data->BarJoinTB_s = 0;
    data->BarPreGadget_o = 0;
    data->BarPreGadget_s = 0;
    data->BarPre_o = 0;
    data->BarPre_s = 0;
    data->BarLGadgetFill_o = 0;
    data->BarLGadgetFill_s = 0;
    data->BarJoinGB_o = 0;
    data->BarJoinGB_s = 0;
    data->BarLFill_o = 0;
    data->BarLFill_s = 0;
    data->BarJoinBT_o = 0;
    data->BarJoinBT_s = 0;
    data->BarTitleFill_o = 0;
    data->BarTitleFill_s = 0;
    data->BarRFill_o = 0;
    data->BarRFill_s = 0;
    data->BarJoinBG_o = 0;
    data->BarJoinBG_s = 0;
    data->BarRGadgetFill_o = 0;
    data->BarRGadgetFill_s = 0;
    data->BarPostGadget_o = 0;
    data->BarPostGadget_s = 0;
    data->BarPost_o = 0;
    data->BarPost_s = 0;
    data->txt_align = WD_DWTA_LEFT;
    data->usegradients = FALSE;
    data->closeright = FALSE;
    data->barvert = FALSE;
    data->filltitlebar = FALSE;
    data->outline = FALSE;
    data->shadow = FALSE;

    data->a_col_s = 0xaaaaaaaa;
    data->a_col_e = 0xeeeeeeff;
    data->d_col_s = 0x66666666;
    data->d_col_e = 0xaaaaaabb;

    data->text_col = 0x00cccccc;
    data->shadow_col = 0x00444444;

    data->a_arc = 0;
    data->d_arc = 0;
    data->light = 320;
    data->middle = 240;
    data->dark = 128;

    file = Open("System/Config", MODE_OLDFILE);
    if (file)
    {
        do
        {
            line = FGets(file, buffer, 256);
            if (line)
            {
                if ((v = strstr(line, "NoInactiveSelected ")) == line) {
                    data->threestate = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarRounded ")) == line) {
                    data->rounded = GetBool(v, "Yes");
                } else if ((v = strstr(line, "WindowTitleMode ")) == line) {
                    data->outline = GetBool(v, "Outline");
                    data->shadow = GetBool(v, "Shadow");
                } else if ((v = strstr(line, "FillTitleBar ")) == line) {
                    data->filltitlebar = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarMasking ")) == line) {
                    data->barmasking = GetBool(v, "Yes");
                } else if ((v = strstr(line, "CloseRight ")) == line) {
                    data->closeright = GetBool(v, "Yes");
                } else if ((v = strstr(line, "UseGradients ")) == line) {
                    data->usegradients = GetBool(v, "Yes");
                } else if ((v = strstr(line, "BarLayout ")) == line) {
                    data->barvert = GetBool(v, "Vertical");
                } else  if ((v = strstr(line, "RightBorderGads ")) == line) {
                    data->rightbordergads = GetInt(v);
                } else  if ((v = strstr(line, "HorScrollerHeight ")) == line) {
                    data->horscrollerheight = GetInt(v);
                } else  if ((v = strstr(line, "ScrollerInnerSpacing ")) == line) {
                    data->scrollerinnerspacing = GetInt(v);
                } else  if ((v = strstr(line, "BottomBorderGads ")) == line) {
                    data->bottombordergads = GetInt(v);
                } else  if ((v = strstr(line, "RightBorderNoGads ")) == line) {
                    data->rightbordernogads = GetInt(v);
                } else  if ((v = strstr(line, "BottomBorderNoGads ")) == line) {
                    data->bottombordernogads = GetInt(v);
                } else  if ((v = strstr(line, "BarHeight ")) == line) {
                    data->winbarheight = GetInt(v); //screen, window
                } else  if ((v = strstr(line, "BarJoinTB ")) == line) {
                    GetIntegers(v, &data->BarJoinTB_o, &data->BarJoinTB_s);
                } else  if ((v = strstr(line, "BarPreGadget ")) == line) {
                    GetIntegers(v, &data->BarPreGadget_o, &data->BarPreGadget_s);
                } else  if ((v = strstr(line, "BarPre ")) == line) {
                    GetIntegers(v, &data->BarPre_o, &data->BarPre_s);
                } else  if ((v = strstr(line, "BarLGadgetFill ")) == line) {
                    GetIntegers(v, &data->BarLGadgetFill_o, &data->BarLGadgetFill_s);
                } else  if ((v = strstr(line, "BarJoinGB ")) == line) {
                    GetIntegers(v, &data->BarJoinGB_o, &data->BarJoinGB_s);
                } else  if ((v = strstr(line, "BarLFill ")) == line) {
                    GetIntegers(v, &data->BarLFill_o, &data->BarLFill_s);
                } else  if ((v = strstr(line, "BarJoinBT ")) == line) {
                    GetIntegers(v, &data->BarJoinBT_o, &data->BarJoinBT_s);
                } else  if ((v = strstr(line, "BarTitleFill ")) == line) {
                    GetIntegers(v, &data->BarTitleFill_o, &data->BarTitleFill_s);
                } else  if ((v = strstr(line, "BarRFill ")) == line) {
                    GetIntegers(v, &data->BarRFill_o, &data->BarRFill_s);
                } else  if ((v = strstr(line, "BarJoinBG ")) == line) {
                    GetIntegers(v, &data->BarJoinBG_o, &data->BarJoinBG_s);
                } else  if ((v = strstr(line, "BarRGadgetFill ")) == line) {
                    GetIntegers(v, &data->BarRGadgetFill_o, &data->BarRGadgetFill_s);
                } else  if ((v = strstr(line, "BarPostGadget ")) == line) {
                    GetIntegers(v, &data->BarPostGadget_o, &data->BarPostGadget_s);
                } else  if ((v = strstr(line, "BarPost ")) == line) {
                    GetIntegers(v, &data->BarPost_o, &data->BarPost_s);
                } else  if ((v = strstr(line, "ContainerTop ")) == line) {
                    GetIntegers(v, &data->ContainerTop_o, &data->ContainerTop_s);
                } else  if ((v = strstr(line, "ContainerVertTile ")) == line) {
                    GetIntegers(v, &data->ContainerVertTile_o, &data->ContainerVertTile_s);
                } else  if ((v = strstr(line, "KnobTop ")) == line) {
                    GetIntegers(v, &data->KnobTop_o, &data->KnobTop_s);
                } else  if ((v = strstr(line, "KnobTileTop ")) == line) {
                    GetIntegers(v, &data->KnobTileTop_o, &data->KnobTileTop_s);
                } else  if ((v = strstr(line, "KnobVertGripper ")) == line) {
                    GetIntegers(v, &data->KnobVertGripper_o, &data->KnobVertGripper_s);
                } else  if ((v = strstr(line, "KnobTileBottom ")) == line) {
                    GetIntegers(v, &data->KnobTileBottom_o, &data->KnobTileBottom_s);
                } else  if ((v = strstr(line, "KnobBottom ")) == line) {
                    GetIntegers(v, &data->KnobBottom_o, &data->KnobBottom_s);
                } else  if ((v = strstr(line, "ContainerBottom ")) == line) {
                    GetIntegers(v, &data->ContainerBottom_o, &data->ContainerBottom_s);
                } else  if ((v = strstr(line, "ContainerLeft ")) == line) {
                    GetIntegers(v, &data->ContainerLeft_o, &data->ContainerLeft_s);
                } else  if ((v = strstr(line, "ContainerHorTile ")) == line) {
                    GetIntegers(v, &data->ContainerHorTile_o, &data->ContainerHorTile_s);
                } else  if ((v = strstr(line, "KnobLeft ")) == line) {
                    GetIntegers(v, &data->KnobLeft_o, &data->KnobLeft_s);
                } else  if ((v = strstr(line, "KnobTileLeft ")) == line) {
                    GetIntegers(v, &data->KnobTileLeft_o, &data->KnobTileLeft_s);
                } else  if ((v = strstr(line, "KnobHorGripper ")) == line) {
                    GetIntegers(v, &data->KnobHorGripper_o, &data->KnobHorGripper_s);
                } else  if ((v = strstr(line, "KnobTileRight ")) == line) {
                    GetIntegers(v, &data->KnobTileRight_o, &data->KnobTileRight_s);
                } else  if ((v = strstr(line, "KnobRight ")) == line) {
                    GetIntegers(v, &data->KnobRight_o, &data->KnobRight_s);
                } else  if ((v = strstr(line, "ContainerRight ")) == line) {
                    GetIntegers(v, &data->ContainerRight_o, &data->ContainerRight_s);
                } else  if ((v = strstr(line, "AddSize ")) == line) {
                    GetIntegers(v, &data->sizeaddx, &data->sizeaddy);
                } else  if ((v = strstr(line, "AddUpDown ")) == line) {
                    GetIntegers(v, &data->updownaddx, &data->updownaddy);
                } else  if ((v = strstr(line, "AddLeftRight ")) == line) {
                    GetIntegers(v, &data->leftrightaddx, &data->leftrightaddy);
                } else  if ((v = strstr(line, "ActivatedGradient ")) == line) {
                    GetTripleIntegers(v, &data->a_col_s, &data->a_col_e, &data->a_arc);
                } else  if ((v = strstr(line, "DeactivatedGradient ")) == line) {
                    GetTripleIntegers(v, &data->d_col_s, &data->d_col_e, &data->d_arc);
                } else  if ((v = strstr(line, "ShadeValues ")) == line) {
                    GetTripleIntegers(v, &data->light, &data->middle, &data->dark);
                } else  if ((v = strstr(line, "BaseColors ")) == line) {
                    GetColors(v, &data->b_col_a, &data->b_col_d);
                } else  if ((v = strstr(line, "WindowTitleColors ")) == line) {
                    GetColors(v, &data->text_col, &data->shadow_col);
                }
            }
        }
        while(line);
        Close(file);
    }

    PUTIMAGE_WIN(size);
    PUTIMAGE_WIN(close);
    PUTIMAGE_WIN(depth);
    PUTIMAGE_WIN(zoom);
    PUTIMAGE_WIN(up);
    PUTIMAGE_WIN(down);
    PUTIMAGE_WIN(left);
    PUTIMAGE_WIN(right);
    PUTIMAGE_WIN(mui);
    PUTIMAGE_WIN(popup);
    PUTIMAGE_WIN(snapshot);
    PUTIMAGE_WIN(iconify);
    PUTIMAGE_WIN(lock);
    PUTIMAGE_WIN(winbar_normal);
    PUTIMAGE_WIN(border_normal);
    PUTIMAGE_WIN(border_deactivated);
    PUTIMAGE_WIN(verticalcontainer);
    PUTIMAGE_WIN(verticalknob);
    PUTIMAGE_WIN(horizontalcontainer);
    PUTIMAGE_WIN(horizontalknob);

    if (olddir) CurrentDir(olddir);
    UnLock(lock);

    if (data->img_horizontalcontainer && data->img_horizontalknob && data->img_verticalcontainer && data->img_verticalknob && data->img_size && data->img_close && data->img_depth && data->img_zoom && data->img_up && data->img_down && data->img_left && data->img_right && data->img_winbar_normal) return TRUE;
    DisposeWindowSkinning(data);
    return FALSE;
}


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

    struct   TagItem        ScreenTags[] = { SDA_UserBuffer, sizeof(struct ScreenData), SDA_Configuration, (ULONG) newpath, TAG_DONE, };


    nd = AllocVec(sizeof(struct NewDecorator), MEMF_CLEAR | MEMF_ANY);
    if (nd)
    {
        nd->nd_Screen = NewObjectA(scrcl, NULL, ScreenTags);

        if (nd->nd_Screen)
        {
            APTR    screendata;

            get(nd->nd_Screen, SDA_ScreenData, &screendata);

            struct   TagItem        WindowTags[] = { WDA_UserBuffer, sizeof(struct WindowData), WDA_Configuration, (ULONG) newpath, WDA_ScreenData, screendata, TAG_DONE, };
            struct   TagItem        MenuTags[] = { MDA_UserBuffer, sizeof(struct MenuData), MDA_Configuration, (ULONG) newpath, MDA_ScreenData, screendata, TAG_DONE, };


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
        FreeDosObject (DOS_RDARGS, (IPTR) newargs);
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
        cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)windecor_dispatcher;

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
    FreeDosObject (DOS_RDARGS, (IPTR) newargs);
    FreeArgs(args);
    return 0;
}
