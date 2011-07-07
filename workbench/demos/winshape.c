
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <aros/debug.h>

#include <stdio.h>
#include <stdlib.h>

#define WINWIDTH    	260
#define WINHEIGHT   	260
#define WINCX	    	(WINWIDTH / 2)
#define WINCY	    	(WINHEIGHT / 2)

struct IntuitionBase 	*IntuitionBase;
struct GfxBase      	*GfxBase;
struct Library	    	*LayersBase;
struct Screen	    	*scr;
struct DrawInfo     	*dri;
struct Window	    	*win;
struct RastPort     	*rp;
struct Region	    	*shape;
WORD	    	    	actshape;


static void cleanup(char *msg)
{
    if(msg) printf("winshape: %s\n", msg);

    if (win) CloseWindow(win);

//    if (shape) DisposeRegion(shape);

    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(0, scr);

    if (LayersBase) CloseLibrary(LayersBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    exit(0);
}


static void openlibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
    {
    	cleanup("Can't open intuition.library!");
    }

    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
    {
    	cleanup("Can't open graphics.library!");
    }
    
    if (!(LayersBase = OpenLibrary("layers.library", 39)))
    {
    	cleanup("Can't open layers.library!");
    }
   
}


static void getvisual(void)
{
    if (!(scr = LockPubScreen(0))) cleanup("Can't lock pub screen!");
    if (!(dri = GetScreenDrawInfo(scr))) cleanup("Can't get drawinfo!");
}


static void makeshape(void)
{
    struct Rectangle rect;

#if 1
    if (!(shape = NewRectRegion(0, 0, WINWIDTH - 1, scr->WBorTop + scr->Font->ta_YSize + 1 - 1)))
    	cleanup("Can't create region!\n");
#else
    if (!(shape = NewRegion())) cleanup("Can't create region!\n");

    rect.MinX = 0;
    rect.MinY = 0;
    rect.MaxX = WINWIDTH - 1;
    rect.MaxY = scr->WBorTop + scr->Font->ta_YSize + 1 - 1;

    if (!(OrRectRegion(shape, &rect))) cleanup("Can't create region!\n");
#endif

    rect.MinX = WINCX - 20;
    rect.MinY = 20;
    rect.MaxX = WINCX + 20;
    rect.MaxY = WINHEIGHT - 1 - 20;

    if (!(OrRectRegion(shape, &rect))) cleanup("Can't create region!\n");

    rect.MinX = 20;
    rect.MinY = WINCY - 20;
    rect.MaxX = WINWIDTH - 1 - 20;
    rect.MaxY = WINCY + 20;

    if (!(OrRectRegion(shape, &rect))) cleanup("Can't create region!\n");

}


static void makewin(void)
{
    UWORD pattern[] = {0x5555, 0xAAAA};
    struct TagItem win_tags[] =
    {
    	{ WA_Left    	, 0	    	    	    	    	},
	{ WA_Top     	, 0	    	    	    	    	},
	{ WA_Width   	, WINWIDTH  	    	    	    	},
	{ WA_Height  	, WINHEIGHT 	    	    	    	},
	{ WA_Title   	, (IPTR)"Irregular shaped window"   	},
	{ WA_CloseGadget , TRUE	    	    	    	    	},
	{ WA_DepthGadget , TRUE	    	    	    	    	},
	{ WA_DragBar 	, TRUE	    	    	    	    	},
	{ WA_IDCMP   	, IDCMP_CLOSEWINDOW | IDCMP_VANILLAKEY  },
	{ WA_Activate	, TRUE	    	    	    	    	},
	{ WA_ShapeRegion   	, (IPTR)shape	    	    	    	},
	{ TAG_DONE   	    	    	    	    	    	}
    };

    win = OpenWindowTagList(0, win_tags);
    if (!win) cleanup("Can't create window!");

    rp = win->RPort;
    SetAPen(rp, dri->dri_Pens[FILLPEN]);
    SetBPen(rp, dri->dri_Pens[SHINEPEN]);
    SetDrMd(rp, JAM2);

    SetAfPt(rp, pattern, 1);
    RectFill(rp, win->BorderLeft,
    	    	 win->BorderTop,
		 WINWIDTH - 1 - win->BorderRight,
		 WINHEIGHT - 1 - win->BorderBottom);
    SetAfPt(rp, 0, 0);

    SetAPen(rp, dri->dri_Pens[SHINEPEN]);
    RectFill(rp, WINCX - 20, 20, WINCX + 20, 20);
    RectFill(rp, WINCX - 20, 21, WINCX - 20, WINCY - 20);
    RectFill(rp, 20, WINCY - 20, WINCX - 20, WINCY - 20);
    RectFill(rp, 20, WINCY - 20, 20, WINCY + 20);
    RectFill(rp, WINCX - 20, WINCY + 20, WINCX - 20, WINHEIGHT - 1 - 20);
    RectFill(rp, WINCX + 20, WINCY - 20, WINWIDTH - 1 - 20, WINCY - 20);

    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
    RectFill(rp, WINCX + 20, 20, WINCX + 20, WINCY - 20);
    RectFill(rp, WINCX + 20, WINCY + 20, WINWIDTH - 1 - 20, WINCY + 20);
    RectFill(rp, WINWIDTH - 1 - 20, WINCY - 20, WINWIDTH - 1 - 20, WINCY + 20);
    RectFill(rp, 20, WINCY + 20, WINCX - 20, WINCY + 20);
    RectFill(rp, WINCX - 20, WINHEIGHT - 1 - 20, WINCX + 20, WINHEIGHT - 1 - 20);
    RectFill(rp, WINCX + 20, WINCY + 20, WINCX + 20, WINHEIGHT - 1 - 20);
}

static void handleall(void)
{
    struct IntuiMessage *imsg;
    BOOL quitme = FALSE;

    struct TagItem taglist[] = {
        {LA_DESTWIDTH  , 0                              },
        {LA_DESTHEIGHT , 0                              },
        {TAG_END       , 0                              }
    };
    
    while (!quitme)
    {
    	WaitPort(win->UserPort);

	while ((imsg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch (imsg->Class)
	    {
	    	case IDCMP_CLOSEWINDOW: 
		    quitme = TRUE;
		    break;
	    	
		case IDCMP_VANILLAKEY:
		    switch (imsg->Code)
		    {
		    
		      case 43:
		         /*
		          * '+': enlarge the layer.
		          */
		        taglist[0].ti_Data = (win->Width*3)/2;
		        taglist[1].ti_Data = (win->Height*3)/2;
		        ScaleLayer(win->WLayer, taglist);
                      break;

                      case 45:
		         /*
		          * '-': make layer smaller.
		          */
		        taglist[0].ti_Data = (win->Width*2)/3;
		        taglist[1].ti_Data = (win->Height*2)/3;
		        ScaleLayer(win->WLayer, taglist);
                      break;

                      default:
		        actshape = 1 - actshape;
		        ChangeWindowShape(win, (actshape ? NULL : shape), NULL);
			break;
		    }
	    }

	    ReplyMsg((struct Message *)imsg);
	}
    }
}


int main(void)
{
    openlibs();
    getvisual();
    makeshape();
    makewin();
    handleall();
    cleanup(0);

    return 0;
}
