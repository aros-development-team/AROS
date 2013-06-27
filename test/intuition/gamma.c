/*
    Copyright © 2000-2013, The AROS Development Team. All rights reserved.
    $Id$

*/

/*****************************************************************************************/

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <gadgets/gradientslider.h>
#include <gadgets/colorwheel.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/colorwheel.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * We won't allow to decrease gamma more than down to 128.
 * Otherwise the playing user might end up in a completely
 * black screen with zeroed out gamma.
 */
#define BOTTOM_LIMIT 128 

struct Library         *GradientSliderBase;
struct Library         *ColorWheelBase;

static struct RDArgs    *rda;
static struct Screen    *scr;
static struct DrawInfo  *dri;
static struct ViewPort        *vp;
static struct ColorMap        *cm;
static struct Window    *win;
static WORD penr = -1, peng = -1, penb = -1;
static struct Gadget *rgad, *ggad, *bgad, *wheelgad;

static WORD pensr[] = {2, 1, ~0};
static WORD pensg[] = {2, 1, ~0};
static WORD pensb[] = {2, 1, ~0};

struct myargs
{
    STRPTR mode;
    IPTR nogamma;
};

static struct myargs args =
{
    NULL,
    FALSE
};

static UBYTE gamma_r[256];
static UBYTE gamma_b[256];
static UBYTE gamma_g[256];

static void gen_gamma(UBYTE r, UBYTE b, UBYTE g)
{
     UWORD i;

     printf("Generating gamma: %02X %02X %02X\n", r, g, b);

     for (i = 0; i < 256; i++)
     {
         gamma_r[i] = (i < r) ? i : r;
         gamma_b[i] = (i < b) ? i : b;
         gamma_g[i] = (i < g) ? i : g;
     }
}

/*****************************************************************************************/

static void cleanup(char *msg)
{
    if (msg) printf("colorwheel: %s\n", msg);
    
    if (win)
    {
        if (wheelgad) RemoveGadget(win, wheelgad);
        if (rgad) RemoveGadget(win, rgad);
        if (ggad) RemoveGadget(win, ggad);
        if (bgad) RemoveGadget(win, bgad);
        CloseWindow(win);
    }
    
    if (wheelgad) DisposeObject((Object *)wheelgad);
    if (rgad) DisposeObject((Object *)rgad);
    if (ggad) DisposeObject((Object *)ggad);
    if (bgad) DisposeObject((Object *)bgad);

    if (penr != -1) ReleasePen(cm, penr);
    if (peng != -1) ReleasePen(cm, peng);
    if (penb != -1) ReleasePen(cm, penb);

    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) CloseScreen(scr);

    if (rda) FreeArgs(rda);
    
    if (ColorWheelBase) CloseLibrary(ColorWheelBase);
    if (GradientSliderBase) CloseLibrary(GradientSliderBase);
    
    exit(0);
}

/*****************************************************************************************/

static void openlibs(void)
{
    if (!(GradientSliderBase = OpenLibrary("Gadgets/gradientslider.gadget", 0)))
    {
        cleanup("Can't open gradientslider.gadget!");
    }
    
    if (!(ColorWheelBase = OpenLibrary("Gadgets/colorwheel.gadget", 0)))
    {
        cleanup("Can't open colorwheel.gadget!");
    }
}

/*****************************************************************************************/

static void getvisual(void)
{
    struct TagItem tags[] =
    {
        {SA_LikeWorkbench, TRUE              },
        {SA_GammaControl , TRUE              },
        {SA_GammaRed     , (IPTR)gamma_r     },
        {SA_GammaBlue    , (IPTR)gamma_b     },
        {SA_GammaGreen   , (IPTR)gamma_g     },
        {SA_Title        , (IPTR)"Gamma test"},
        {TAG_DONE        , 0                 }
    };

    rda = ReadArgs("MODEID/K,NOGAMMA/S", (IPTR *)&args, NULL);
    if (!rda)
        cleanup("Failed to parse arguments!");

    gen_gamma(255, 255, 255);

    if (args.mode)
    {
        tags[0].ti_Tag  = SA_DisplayID;
        tags[0].ti_Data = strtoul(args.mode, NULL, 16);
    }
    tags[1].ti_Data = !args.nogamma;

    scr = OpenScreenTagList(NULL, tags);
    if (!scr)
    {
        cleanup("Can't open screen!");
    }
       
    if (!(dri = GetScreenDrawInfo(scr)))
    {
        cleanup("Can't get screen drawinfo!");
    }

    vp = &scr->ViewPort;
    cm = vp->ColorMap;
    
    penr = ObtainBestPen(cm, 0xFFFFFFFF, 0, 0, OBP_Precision, PRECISION_GUI);
    peng = ObtainBestPen(cm, 0, 0xFFFFFFFF, 0, OBP_Precision, PRECISION_GUI);
    penb = ObtainBestPen(cm, 0, 0, 0xFFFFFFFF, OBP_Precision, PRECISION_GUI);

    if (penr != -1) pensr[0] = penr;
    if (peng != -1) pensg[0] = peng;
    if (penb != -1) pensb[0] = penb;
}

/*****************************************************************************************/

#define BORDERX     4
#define BORDERY     4
#define SPACINGX    4
#define SPACINGY    4
#define GRADWIDTH   20

static void makegads(void)
{
    Object                  *im;
    WORD                    sizeheight = 14;
    WORD                    gradx, grady, gradw, gradh;
    WORD                    wheelx, wheely, wheelw, wheelh;
    
    im = NewObject(NULL, SYSICLASS, SYSIA_DrawInfo, (IPTR)dri, SYSIA_Which, SIZEIMAGE, TAG_DONE);
    if (im)
    {
        sizeheight = ((struct Image *)im)->Height;
    DisposeObject(im);
    }
    
    wheelx = scr->WBorLeft + BORDERX;
    wheely = scr->WBorTop + scr->Font->ta_YSize + 1 + BORDERY;
    wheelw = -(scr->WBorLeft + scr->WBorRight + BORDERX * 2 + SPACINGX + GRADWIDTH * 3);
    wheelh = -(scr->WBorTop + scr->Font->ta_YSize + 1 + sizeheight + BORDERY * 2);
    
    gradx = -(scr->WBorRight + BORDERX + GRADWIDTH) + 1;
    grady = scr->WBorTop + scr->Font->ta_YSize + 1 + BORDERY;
    gradw = GRADWIDTH;
    gradh = -(scr->WBorTop + scr->Font->ta_YSize + 1 + sizeheight + BORDERY * 2);

    rgad = (struct Gadget *)NewObject(0, "gradientslider.gadget",
                                      GA_RelRight    , gradx - gradw * 2,
                                      GA_Top         , grady,
                                      GA_Width       , gradw,
                                      GA_RelHeight   , gradh,
                                      GRAD_PenArray  , (IPTR)pensr,
                                      GRAD_KnobPixels, 10,
                                      GRAD_MaxVal    , BOTTOM_LIMIT,
                                      PGA_Freedom    , LORIENT_VERT,
                                      ICA_TARGET     , ICTARGET_IDCMP,
                                      TAG_DONE);
                     
    if (!rgad) cleanup("Can't create red slider!");

    ggad = (struct Gadget *)NewObject(0, "gradientslider.gadget",
                                      GA_RelRight    , gradx - gradw,
                                      GA_Top         , grady,
                                      GA_Width       , gradw,
                                      GA_RelHeight   , gradh,
                                      GRAD_PenArray  , (IPTR)pensg,
                                      GRAD_KnobPixels, 10,
                                      GRAD_MaxVal    , BOTTOM_LIMIT,
                                      PGA_Freedom    , LORIENT_VERT,
                                      ICA_TARGET     , ICTARGET_IDCMP,
                                      GA_Previous    , rgad,
                                      TAG_DONE);

    if (!ggad) cleanup("Can't create green slider!");

    bgad = (struct Gadget *)NewObject(0, "gradientslider.gadget",
                                      GA_RelRight    , gradx,
                                      GA_Top         , grady,
                                      GA_Width       , gradw,
                                      GA_RelHeight   , gradh,
                                      GRAD_PenArray  , (IPTR)pensb,
                                      GRAD_KnobPixels, 10,
                                      GRAD_MaxVal    , BOTTOM_LIMIT,
                                      PGA_Freedom    , LORIENT_VERT,
                                      ICA_TARGET     , ICTARGET_IDCMP,
                                      GA_Previous    , ggad,
                                      TAG_DONE);
                     
    if (!bgad) cleanup("Can't create blue slider!");

    
    wheelgad = (struct Gadget *)NewObject(0, "colorwheel.gadget",
                                          GA_Left       , wheelx,
                                          GA_Top        , wheely,
                                          GA_RelWidth   , wheelw,
                                          GA_RelHeight  , wheelh,
                                          GA_RelVerify  , TRUE,
                                          WHEEL_Screen  , scr,
                                          WHEEL_BevelBox, TRUE,
                                          GA_Previous   , bgad,
                                          TAG_DONE);
                                  
    if (!wheelgad) cleanup("Can't create colorwheel gadget!");
   
}

/*****************************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(0,
                         WA_CustomScreen, scr,
                         WA_Left        , 10,
                         WA_Top         , 20,
                         WA_Width       , 220,
                         WA_Height      , 190,
                         WA_MinWidth    , 50,
                         WA_MinHeight   , 50,
                         WA_MaxWidth    , 4000,
                         WA_MaxHeight   , 4000,
                         WA_AutoAdjust  , TRUE,
                         WA_Title       , "Gamma test",
                         WA_CloseGadget , TRUE,
                         WA_DragBar     , TRUE,
                         WA_DepthGadget , TRUE,
                         WA_SizeGadget  , TRUE,
                         WA_SizeBBottom , TRUE,
                         WA_Activate    , TRUE,
                         WA_ReportMouse , TRUE,
                         WA_IDCMP       , IDCMP_CLOSEWINDOW | IDCMP_IDCMPUPDATE,
                         WA_Gadgets     , rgad,
                         TAG_DONE);

    if (!win) cleanup("Can't open window!");                
}

/*****************************************************************************************/

static void handleall(void)
{
    struct IntuiMessage *msg;
    BOOL quitme = FALSE;
    IPTR r, g, b;

    while(!quitme)
    {
        WaitPort(win->UserPort);
    
        while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
        {
            switch(msg->Class)
            {
            case IDCMP_CLOSEWINDOW:
                quitme = TRUE;
                break;
        
            case IDCMP_IDCMPUPDATE:
               GetAttr(GRAD_CurVal, (Object *)rgad, &r);
               GetAttr(GRAD_CurVal, (Object *)ggad, &g);
               GetAttr(GRAD_CurVal, (Object *)bgad, &b);

               /* Values are reversed (largest down, smallest up) */
               gen_gamma(255 - r, 255 - g, 255 - b);

               SetAttrs(scr,
                        SA_GammaRed  , gamma_r,
                        SA_GammaBlue , gamma_b,
                        SA_GammaGreen, gamma_g,
                        TAG_DONE);
               break;
            } /* switch(msg->Class) */
        
            ReplyMsg((struct Message *)msg);
        
        } /* while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
    
    } /* while(!quitme) */
    
}

/*****************************************************************************************/

int main(void)
{
    openlibs();
    getvisual();
    makegads();
    makewin();
    handleall();
    cleanup(0);
    
    return 0;
}

/*****************************************************************************************/
