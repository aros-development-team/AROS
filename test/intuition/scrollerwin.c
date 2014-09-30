/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <stdio.h>
#include <stdlib.h>

enum
{
    GAD_UPARROW,
    GAD_DOWNARROW,
    GAD_LEFTARROW,
    GAD_RIGHTARROW,
    GAD_VERTSCROLL,
    GAD_HORIZSCROLL,
    NUM_GADGETS
};

enum
{
    IMG_UPARROW,
    IMG_DOWNARROW,
    IMG_LEFTARROW,
    IMG_RIGHTARROW,
    IMG_SIZE,
    NUM_IMAGES
};

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

static struct Screen *scr;
static struct Window *win;
static struct DrawInfo *dri;

static struct Gadget *gad[NUM_GADGETS], *firstgadget;
static struct Image *img[NUM_GADGETS];

static void Cleanup(char *msg)
{
    WORD rc, i;

    if (msg)
    {
	printf("scrollerwin: %s\n",msg);
	rc = RETURN_WARN;
    } else {
	rc = RETURN_OK;
    }

    if (win)
    {
	for(i = 0; i < NUM_GADGETS;i++)
	{
	    if (gad[i]) RemoveGadget(win,gad[i]);
	}

	CloseWindow(win);
    }

    for(i = 0; i < NUM_GADGETS;i++)
    {
	if (gad[i]) DisposeObject(gad[i]);
    }
    for(i = 0; i < NUM_IMAGES;i++)
    {
	if (img[i]) DisposeObject(img[i]);
    }

    if (dri) FreeScreenDrawInfo(scr,dri);
    if (scr) UnlockPubScreen(0,scr);

    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    exit(rc);
}

static void OpenLibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39)))
    {
	Cleanup("Can't open intuition.library V39!");
    }

    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",39)))
    {
	Cleanup("Can't open graphics.library V39!");
    }	
}

static void GetVisual(void)
{
    if (!(scr = LockPubScreen(0)))
    {
	Cleanup("Can't lock pub screen!");
    }

    if (!(dri = GetScreenDrawInfo(scr)))
    {
	Cleanup("Can't get drawinfo!");
    }
}

static void MakeGadgets(void)
{
    static WORD img2which[] =
    {
   	UPIMAGE,
   	DOWNIMAGE,
   	LEFTIMAGE,
   	RIGHTIMAGE,
   	SIZEIMAGE
    };

    IPTR imagew[NUM_IMAGES],imageh[NUM_IMAGES];
    WORD v_offset,h_offset, btop, i;

    for(i = 0;i < NUM_IMAGES;i++)
    {
	img[i] = NewObject(0,SYSICLASS,SYSIA_DrawInfo,(IPTR)dri,
				       SYSIA_Which,img2which[i],
				       TAG_DONE);

	if (!img[i]) Cleanup("Can't create SYSICLASS image!");

	GetAttr(IA_Width,(Object *)img[i],&imagew[i]);
	GetAttr(IA_Height,(Object *)img[i],&imageh[i]);
    }

    btop = scr->WBorTop + dri->dri_Font->tf_YSize + 1;

    v_offset = imagew[IMG_DOWNARROW] / 4;
    h_offset = imageh[IMG_LEFTARROW] / 4;

    firstgadget = gad[GAD_UPARROW] = NewObject(0,BUTTONGCLASS,
	    GA_Image,(IPTR)img[IMG_UPARROW],
	    GA_RelRight,-imagew[IMG_UPARROW] + 1,
	    GA_RelBottom,-imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1,
	    GA_ID,GAD_UPARROW,
	    GA_RightBorder,TRUE,
	    GA_Immediate,TRUE,
	    TAG_DONE);

    gad[GAD_DOWNARROW] = NewObject(0,BUTTONGCLASS,
	    GA_Image,(IPTR)img[IMG_DOWNARROW],
	    GA_RelRight,-imagew[IMG_UPARROW] + 1,
	    GA_RelBottom,-imageh[IMG_UPARROW] - imageh[IMG_SIZE] + 1,
	    GA_ID,GAD_DOWNARROW,
	    GA_RightBorder,TRUE,
	    GA_Previous,(IPTR)gad[GAD_UPARROW],
	    GA_Immediate,TRUE,
	    TAG_DONE);

    gad[GAD_VERTSCROLL] = NewObject(0,PROPGCLASS,
	    GA_Top,btop + 1,
	    GA_RelRight,-imagew[IMG_DOWNARROW] + v_offset + 1,
	    GA_Width,imagew[IMG_DOWNARROW] - v_offset * 2,
	    GA_RelHeight,-imageh[IMG_DOWNARROW] - imageh[IMG_UPARROW] - imageh[IMG_SIZE] - btop -2,
	    GA_ID,GAD_VERTSCROLL,
	    GA_Previous,(IPTR)gad[GAD_DOWNARROW],
	    GA_RightBorder,TRUE,
	    GA_RelVerify,TRUE,
	    GA_Immediate,TRUE,
	    PGA_NewLook,TRUE,
	    PGA_Borderless,TRUE,
	    PGA_Total,100,
	    PGA_Visible,20,
	    PGA_Freedom,FREEVERT,
	    TAG_DONE);

    gad[GAD_RIGHTARROW] = NewObject(0,BUTTONGCLASS,
	    GA_Image,(IPTR)img[IMG_RIGHTARROW],
	    GA_RelRight,-imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] + 1,
	    GA_RelBottom,-imageh[IMG_RIGHTARROW] + 1,
	    GA_ID,GAD_RIGHTARROW,
	    GA_BottomBorder,TRUE,
	    GA_Previous,(IPTR)gad[GAD_VERTSCROLL],
	    GA_Immediate,TRUE,
	    TAG_DONE);

    gad[GAD_LEFTARROW] = NewObject(0,BUTTONGCLASS,
	    GA_Image,(IPTR)img[IMG_LEFTARROW],
	    GA_RelRight,-imagew[IMG_SIZE] - imagew[IMG_RIGHTARROW] - imagew[IMG_LEFTARROW] + 1,
	    GA_RelBottom,-imageh[IMG_RIGHTARROW] + 1,
	    GA_ID,GAD_LEFTARROW,
	    GA_BottomBorder,TRUE,
	    GA_Previous,(IPTR)gad[GAD_RIGHTARROW],
	    GA_Immediate,TRUE,
	    TAG_DONE);

    gad[GAD_HORIZSCROLL] = NewObject(0,PROPGCLASS,
	    GA_Left,scr->WBorLeft,
	    GA_RelBottom,-imageh[IMG_LEFTARROW] + h_offset + 1,
	    GA_RelWidth,-imagew[IMG_LEFTARROW] - imagew[IMG_RIGHTARROW] - imagew[IMG_SIZE] - scr->WBorRight - 2,
	    GA_Height,imageh[IMG_LEFTARROW] - (h_offset * 2),
	    GA_ID,GAD_HORIZSCROLL,
	    GA_Previous,(IPTR)gad[GAD_LEFTARROW],
	    GA_BottomBorder,TRUE,
	    GA_RelVerify,TRUE,
	    GA_Immediate,TRUE,
	    PGA_NewLook,TRUE,
	    PGA_Borderless,TRUE,
	    PGA_Total,100,
	    PGA_Visible,20,
	    PGA_Freedom,FREEHORIZ,
	    TAG_DONE);

    for(i = 0;i < NUM_GADGETS;i++)
    {
	if (!gad[i]) Cleanup("Can't create gadget!");
    }
}

static void MakeWin(void)
{	
    if (!(win = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
				 WA_Left,10,
				 WA_Top,10,
				 WA_Width,300,
				 WA_Height,150,
				 WA_Title,(IPTR)"Scroller Window",
				 WA_SimpleRefresh,TRUE,
				 WA_CloseGadget,TRUE,
				 WA_DepthGadget,TRUE,
				 WA_DragBar,TRUE,
				 WA_SizeGadget,TRUE,
				 WA_SizeBBottom,TRUE,
				 WA_SizeBRight,TRUE,
				 WA_Gadgets,(IPTR)firstgadget,
				 WA_MinWidth,50,
				 WA_MinHeight,50,
				 WA_MaxWidth,scr->Width,
				 WA_MaxHeight,scr->Height,
				 WA_IDCMP,IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW,
				 TAG_DONE)))
    {
	    Cleanup("Can't open window!");
    }

    ScreenToFront(win->WScreen);

}

static void HandleAll(void)
{
    struct IntuiMessage *msg;

    BOOL quitme = FALSE;

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

		case IDCMP_REFRESHWINDOW:
		    BeginRefresh(win);
		    EndRefresh(win,TRUE);
		    break;
	    }
	    ReplyMsg((struct Message *)msg);
	}
    }
}

int main(void)
{
    OpenLibs();
    GetVisual();
    MakeGadgets();
    MakeWin();
    HandleAll();
    Cleanup(0);
    return 0;
}

