#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/alib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define SHOW_CHECKMARK	1
#define SHOW_SELECTFILL 0

#define WINWIDTH    	200
#define WINHEIGHT   	200
#define LVENTRIES   	40

struct IntuitionBase 	*IntuitionBase;
struct GfxBase      	*GfxBase;
struct Library	    	*GadToolsBase;
struct Screen	    	*scr;
struct DrawInfo     	*dri;
APTR	    	    	vi;
struct Window	    	*win;
struct Gadget	    	*gadlist, *gad, *lvgad;
struct RastPort     	*rp;
struct Node	    	lvnode[LVENTRIES];
struct List 	    	lvlist;
struct Hook 	    	myrenderhook;
WORD	    	    	greenpen = -1;

#define IS_SELECTED(node)   (node->ln_Pri == 1)
#define DO_SELECT(node)     node->ln_Pri = 1
#define DO_UNSELECT(node)   node->ln_Pri = 0

static void cleanup(char *msg)
{
    if(msg) printf("gtmultiselect: %s\n", msg);
    
    if (win) CloseWindow(win);
    if (gadlist) FreeGadgets(gadlist);
    
    if (greenpen != -1) ReleasePen(scr->ViewPort.ColorMap, greenpen);
    
    if (vi) FreeVisualInfo(vi);
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(0, scr);
    
    if (GadToolsBase) CloseLibrary(GadToolsBase);
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
   
    if (!(GadToolsBase = OpenLibrary("gadtools.library", 39)))
    {
    	cleanup("Can't open gadtools.library!");
    }
}

static void getvisual(void)
{
    if (!(scr = LockPubScreen(0))) cleanup("Can't lock pub screen!");
    if (!(dri = GetScreenDrawInfo(scr))) cleanup("Can't get drawinfo!");
    if (!(vi = GetVisualInfoA(scr, NULL))) cleanup("Can' get visualinfo!");
    
    greenpen = ObtainBestPen(scr->ViewPort.ColorMap, 0, 0x76767676, 0, OBP_FailIfBad, FALSE, TAG_DONE);
    
}

AROS_UFH3(IPTR, MyRenderFunc,
    AROS_UFHA(struct Hook *,            hook,     	A0),
    AROS_UFHA(struct Node *,    	node,           A2),
    AROS_UFHA(struct LVDrawMsg *,	msg,	        A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval;
    
    if (msg->lvdm_MethodID == LV_DRAW)
    {
    	struct DrawInfo *dri = msg->lvdm_DrawInfo;
    	struct RastPort *rp  = msg->lvdm_RastPort;
    	struct TextExtent te;
    	WORD numfit;
    	
    	WORD min_x = msg->lvdm_Bounds.MinX;
    	WORD min_y = msg->lvdm_Bounds.MinY;
    	WORD max_x = msg->lvdm_Bounds.MaxX;
    	WORD max_y = msg->lvdm_Bounds.MaxY;
    	WORD width, height;
	
        UWORD erasepen;

    	if (msg->lvdm_State == LVR_SELECTED)
	{
	    if (IS_SELECTED(node))
	    {
	    	DO_UNSELECT(node);
	    }
	    else
	    {
	    	DO_SELECT(node);
	    }
	}

#if SHOW_SELECTFILL
	erasepen = IS_SELECTED(node) ? FILLPEN : BACKGROUNDPEN;
#else
    	erasepen = BACKGROUNDPEN;
#endif

     	SetDrMd(rp, JAM1);

	SetAPen(rp, dri->dri_Pens[erasepen]);
     	RectFill(rp, min_x, min_y, max_x, max_y);

#if SHOW_CHECKMARK
    	numfit = TextFit(rp, node->ln_Name, strlen(node->ln_Name),
    	    	&te, NULL, 1, max_x - min_x + 1 - 16, max_y - min_y + 1);
#else
    	numfit = TextFit(rp, node->ln_Name, strlen(node->ln_Name),
    	    	&te, NULL, 1, max_x - min_x + 1, max_y - min_y + 1);
#endif

	SetAPen(rp, dri->dri_Pens[TEXTPEN]);

    	/* Render text */
#if SHOW_CHECKMARK
    	Move(rp, min_x + 16, min_y + rp->Font->tf_Baseline);
#else
    	Move(rp, min_x, min_y + rp->Font->tf_Baseline);
#endif
    	Text(rp, node->ln_Name, numfit);

#if SHOW_CHECKMARK	    	
     	if (IS_SELECTED(node))
	{
	    min_y += 1;
	    max_y -= 1;
	    min_x += 1;
	    height = max_y - min_y + 1;
	    width = height;
	    max_x = min_x + width - 1;
	    
	    SetAPen(rp, greenpen);
    	    Move(rp, min_x, min_y + height / 3); Draw(rp, min_x, max_y);
    	    Move(rp, min_x + 1, min_y + height / 3); Draw(rp, min_x + 1, max_y);
	    
	    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
    	    Move(rp, min_x + 2, min_y + height / 3); Draw(rp, min_x + 2, max_y);
	    
	    SetAPen(rp, greenpen);	    
	    Move(rp, min_x + 1, max_y); Draw(rp, max_x - 1, min_y);
	    Move(rp, min_x + 2, max_y); Draw(rp, max_x, min_y);
	    
	    SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
	    Move(rp, min_x + 3, max_y); Draw(rp, max_x + 1, min_y);
	    
	}	
#endif
     	
     	retval = LVCB_OK;
     }
     else
     {
     	retval = LVCB_UNKNOWN;
     }
    
    return TRUE;
    
     AROS_USERFUNC_EXIT
}

static void makegadgets(void)
{
    struct NewGadget ng;
    WORD i;
    
    gad = CreateContext(&gadlist);
    
    memset(&ng, 0, sizeof(ng));
    
    ng.ng_VisualInfo = vi;
    ng.ng_LeftEdge = scr->WBorLeft + 4;
    ng.ng_TopEdge = scr->WBorTop + scr->Font->ta_YSize + 1 + 4;
    ng.ng_Width = WINWIDTH - scr->WBorRight - scr->WBorLeft - 8;
    ng.ng_Height = WINHEIGHT - scr->WBorTop - scr->Font->ta_YSize - 1 - scr->WBorBottom - 8;
    
    NewList(&lvlist);
    for(i = 0; i < LVENTRIES; i++)
    {
        char s[30];
	
	sprintf(s, "Item %d", i);
	
    	lvnode[i].ln_Name = strdup(s);
	AddTail(&lvlist, &lvnode[i]);
    }
    
    {
    	struct TagItem lv_tags[] =
	{
	    {GTLV_Labels    , (IPTR)&lvlist   	    	    },
	    {GTLV_CallBack  , (IPTR)&myrenderhook   	    },
	    {GTLV_ItemHeight, dri->dri_Font->tf_YSize + 4   },
	    {TAG_DONE	    	    	    	    	    }
	};
	
	myrenderhook.h_Entry = AROS_ASMSYMNAME(MyRenderFunc);
	
    	gad = lvgad = CreateGadgetA(LISTVIEW_KIND, gad, &ng, lv_tags);
    }
    
    if (!gad) cleanup("Can't create gadgets!");
}

static void makewin(void)
{
    win = OpenWindowTags(0, WA_PubScreen, (IPTR)scr,
    	    	    	    WA_Left, 20,
    	    	    	    WA_Top, 20,
			    WA_Width, WINWIDTH,
			    WA_Height, WINHEIGHT,
			    WA_Title, (IPTR)"GadTools MultiSelect Listview",
			    WA_CloseGadget, TRUE,
			    WA_DragBar, TRUE,
			    WA_DepthGadget, TRUE,
			    WA_IDCMP, IDCMP_CLOSEWINDOW | LISTVIEWIDCMP,
			    WA_Activate, TRUE,
			    WA_Gadgets, (IPTR)gadlist,
			    TAG_DONE);


    if (!win) cleanup("Can't create parent window!");
    
    GT_RefreshWindow(win, NULL);
}

static void handleall(void)
{
    struct IntuiMessage *imsg;
    BOOL quitme = FALSE;
    
    while(!quitme)
    {
    	WaitPort(win->UserPort);
	while((imsg = GT_GetIMsg(win->UserPort)))
	{
	    switch(imsg->Class)
	    {
	    	case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
	    }
	    GT_ReplyIMsg(imsg);
	}
	
    }
    
}

int main(void)
{
    openlibs();
    getvisual();
    makegadgets();
    makewin();
    handleall();
    cleanup(0);
    return 0;
}
