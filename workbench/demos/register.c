#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************************/

#define REGISTER_EXTRA_HEIGHT 4
#define REGISTERITEM_EXTRA_WIDTH 8
#define REGISTER_SPACE_LEFT 8
#define REGISTER_SPACE_RIGHT 8

struct Register
{
    struct RegisterItem *items;
    struct DrawInfo *dri;
    WORD numitems;
    WORD active;
    WORD left;
    WORD top;
    WORD width;
    WORD height;
    WORD fontw;
    WORD fonth;
    WORD fontb;
    WORD slopew;
};

struct RegisterItem
{
    STRPTR text;
    WORD textlen;
    WORD x1, y1, x2, y2, w, h;
    WORD tx, ty;
};

/****************************************************************************************/

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

struct Screen *scr;
struct DrawInfo *dri;
struct Window *win;

/****************************************************************************************/

struct RegisterItem myregisteritems[] =
{
    {"Timezone"},
    {"Country"},
    {"Language"},
    {"Something"},
    {"AROS"},
    {NULL}
};
struct Register myregister;

/****************************************************************************************/

static void cleanup(char *msg)
{
    if (msg) printf("register: %s\n", msg);
    
    if (win) CloseWindow(win);
    
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(NULL, scr);
    
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit (0);
}

/****************************************************************************************/

static void openlibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
    {
    	cleanup("Can't open intuition.library V39!");
    }
    
    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
    {
    	cleanup("Can't open graphics.library V39!");
    }
}

/****************************************************************************************/

static void getvisual(void)
{
    scr = LockPubScreen(NULL);
    if (!scr) cleanup("Can't lock pub screen!");
    
    dri = GetScreenDrawInfo(scr);
    if (!dri) cleanup("Can't get drawinfo!");
    
}

/****************************************************************************************/

static void initregister(struct Register *reg, struct RegisterItem *items)
{
    reg->items = items;
    reg->numitems = 0;

    while(items->text)
    {
    	reg->numitems++;
	items++;
    }
}

/****************************************************************************************/

static void layoutregister(struct Register *reg, struct Screen *scr,
    	    	    	   struct DrawInfo *dri, BOOL samewidth)
{
    struct RastPort temprp;
    WORD i, x, biggest_w = 0;
    
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);
    reg->dri = dri;
    reg->fonth = dri->dri_Font->tf_YSize;
    reg->fontb = dri->dri_Font->tf_Baseline;
    
    reg->height = ((reg->fonth + REGISTER_EXTRA_HEIGHT) + 3) & ~3; /* Multiple of 4 */
    reg->height += 4;
    
    reg->slopew = (reg->height - 4) / 2;
    
    for(i = 0; i < reg->numitems; i++)
    {
    	reg->items[i].textlen = strlen(reg->items[i].text);
    	reg->items[i].w = TextLength(&temprp, reg->items[i].text, reg->items[i].textlen);
	reg->items[i].w += REGISTERITEM_EXTRA_WIDTH + reg->slopew * 2;
	
	if (reg->items[i].w > biggest_w) biggest_w = reg->items[i].w;
    }
    
    if (samewidth)
    {
	for(i = 0; i < reg->numitems; i++)
	{
    	    reg->items[i].w = biggest_w;
	}
    }

   x = REGISTER_SPACE_LEFT;
   for(i = 0; i < reg->numitems; i++)
   {
       reg->items[i].x1 = x;
       reg->items[i].y1 = 0;
       reg->items[i].x2 = x + reg->items[i].w - 1;
       reg->items[i].y2 = reg->height - 1;
       reg->items[i].h  = reg->items[i].y2 - reg->items[i].y1 + 1;
       reg->items[i].tx = reg->items[i].w / 2 - TextLength(&temprp, reg->items[i].text, reg->items[i].textlen) / 2;
       reg->items[i].ty = reg->fontb + reg->items[i].h / 2 - reg->fonth / 2;
       
       x += reg->items[i].w - reg->slopew;
   }
   
   reg->width = x + reg->slopew + REGISTER_SPACE_RIGHT;
    
   DeinitRastPort(&temprp);
}

/****************************************************************************************/

static void setregisterpos(struct Register *reg, WORD left, WORD top)
{
    reg->left = left;
    reg->top  = top;
}

/****************************************************************************************/

static void renderregisteritem(struct RastPort *rp, struct Register *reg, WORD item)
{
    struct RegisterItem *ri = &reg->items[item];
    WORD x, y;
    
    SetDrMd(rp, JAM1);
    SetFont(rp, reg->dri->dri_Font);
    
    x = reg->left + ri->x1;
    y = reg->top + ri->y1;

    
    SetAPen(rp, reg->dri->dri_Pens[(reg->active == item) ? TEXTPEN : BACKGROUNDPEN]);       
    Move(rp, x + ri->tx + 1, y + ri->ty);
    Text(rp, ri->text, ri->textlen);
    SetAPen(rp, reg->dri->dri_Pens[TEXTPEN]);       
    Move(rp, x + ri->tx, y + ri->ty);
    Text(rp, ri->text, ri->textlen);
    
    /* upper / at left side */
    
    SetAPen(rp, reg->dri->dri_Pens[SHINEPEN]);
    WritePixel(rp, x + reg->slopew, y + 2);
    Move(rp, x + reg->slopew / 2, y + 3 + reg->slopew - 1);
    Draw(rp, x + reg->slopew - 1, y + 3);
    
    /* --- at top side */
    
    RectFill(rp, x + reg->slopew + 1, y + 1, x + reg->slopew + 2, y + 1);
    RectFill(rp, x + reg->slopew + 3, y, x + ri->w - 1 - reg->slopew - 3, y);

    SetAPen(rp, reg->dri->dri_Pens[SHADOWPEN]);
    RectFill(rp, x + ri->w - 1 - reg->slopew - 2, y + 1, x + ri->w - 1 - reg->slopew - 1, y + 1);
    
    /* upper \ at right side */
    
    WritePixel(rp, x + ri->w - 1 - reg->slopew, y + 2);
    Move(rp, x + ri->w - 1 - reg->slopew + 1, y + 3);
    Draw(rp, x + ri->w - 1 - reg->slopew / 2, y + 3 + reg->slopew - 1);
    
    /* lower / at left side. */
    
    if ((item == 0) || (reg->active == item))
    {
    	SetAPen(rp, reg->dri->dri_Pens[SHINEPEN]);
    }
    else
    {
    	SetAPen(rp, reg->dri->dri_Pens[BACKGROUNDPEN]);
    }
    Move(rp, x, y + ri->h - 2);
    Draw(rp, x + reg->slopew / 2 - 1, y + ri->h - 2 - reg->slopew + 1);
    
    /* lower \ at the lefst side from the previous item */
    
    if (item > 0)
    {
    	if (reg->active == item)
	{
    	    SetAPen(rp, reg->dri->dri_Pens[BACKGROUNDPEN]);
	}
	else
	{
    	    SetAPen(rp, reg->dri->dri_Pens[SHADOWPEN]);
	}
	Move(rp, x + reg->slopew / 2, y + ri->h - 2 - reg->slopew + 1);
	Draw(rp, x + reg->slopew - 1, y + ri->h - 2);
    }
    
    /* lower \ at right side. */
    
    if (reg->active == item + 1)
    {
    	SetAPen(rp, reg->dri->dri_Pens[BACKGROUNDPEN]);
    }
    else
    {
    	SetAPen(rp, reg->dri->dri_Pens[SHADOWPEN]);
    }
    Move(rp, x + ri->w - 1 - reg->slopew / 2 + 1, y + ri->h - 2 - reg->slopew + 1);
    Draw(rp, x + ri->w - 1, y + ri->h - 2);
    
    if (reg->active == item)
    {
    	SetAPen(rp, reg->dri->dri_Pens[BACKGROUNDPEN]);
    	RectFill(rp, x, y + ri->h - 1, x + ri->w - 1, y + ri->h - 1);
    }
    else
    {
    	WORD x1, x2;
	
	x1 = x;
	x2 = x + ri->w - 1;
	
	if (reg->active == item - 1)
	    x1 += reg->slopew;
	else if (reg->active == item + 1)
	    x2 -= reg->slopew;
	    
    	SetAPen(rp, reg->dri->dri_Pens[SHINEPEN]);
    	RectFill(rp, x1, y + ri->h - 1, x2, y + ri->h - 1);
    }
    
    
    
#if 0
    /* \ at left side from previous item */
       
    if (item != 0)
    {
	if (reg->active == item)
	{
    	    SetAPen(rp, reg->dri->dri_Pens[BACKGROUNDPEN]);
	}
	else
	{
    	    SetAPen(rp, reg->dri->dri_Pens[SHADOWPEN]);
	}
    
	Move(rp, x + reg->slopew / 2, y + ri->h - 2 - reg->slopew + 1);
	Draw(rp, x + reg->slopew - 1, y + ri->h - 2);
    }
#endif

}

/****************************************************************************************/

static void renderregister(struct RastPort *rp, struct Register *reg)
{
    WORD i;
    
    SetDrMd(rp, JAM1);
    
    SetAPen(rp, reg->dri->dri_Pens[SHINEPEN]);   
     
    RectFill(rp, reg->left,
    	    	 reg->top + reg->height - 1,
		 reg->left + REGISTER_SPACE_LEFT - 1,
		 reg->top + reg->height - 1);
		 
    RectFill(rp, reg->left + reg->width - REGISTER_SPACE_RIGHT,
    	    	 reg->top + reg->height - 1,
		 reg->left + reg->width - 1,
		 reg->top + reg->height - 1); 
		 
    for(i = 0; i < reg->numitems; i++)
    {
    	renderregisteritem(rp, reg, i); 
    }
    

}

/****************************************************************************************/

static BOOL handleregisterinput(struct Register *reg, struct IntuiMessage *msg)
{
    struct Window *win = msg->IDCMPWindow;
    BOOL retval = FALSE;
    
    if ((msg->Class == IDCMP_MOUSEBUTTONS) &&
    	(msg->Code == SELECTDOWN))
    {
    	WORD i;
	WORD x = win->MouseX - reg->left;
	WORD y = win->MouseY - reg->top;
	
	for(i = 0; i < reg->numitems; i++)
	{
	    if ((x >= reg->items[i].x1) &&
	    	(y >= reg->items[i].y1) &&
		(x <= reg->items[i].x2) &&
		(y <= reg->items[i].y2))
	    {
	    	if (reg->active != i)
		{
		    WORD oldactive = reg->active;
		    
		    reg->active = i;
		    renderregisteritem(win->RPort, reg, oldactive);
		    renderregisteritem(win->RPort, reg, i);
		}
	    	break;
	    }
	}
    }
    
    return retval;
    
}


/****************************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(0, WA_PubScreen, (IPTR)scr,
    	    	    	    WA_Title, (IPTR) "Register Tabs",
			    WA_InnerWidth, myregister.width + 8,
			    WA_InnerHeight, myregister.height + 8,
			    WA_CloseGadget, TRUE,
			    WA_DepthGadget, TRUE,
			    WA_Activate, TRUE,
			    WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MOUSEBUTTONS,
			    TAG_DONE);
			    
    if (!win) cleanup("Can't open window!");
}

/****************************************************************************************/

static void handleall(void)
{
    struct IntuiMessage *msg;
    
    BOOL quitme = FALSE;
    
    while(!quitme)
    {
    	WaitPort(win->UserPort);
	
	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    if (!handleregisterinput(&myregister, msg))
	    {
	    	switch(msg->Class)
		{
		    case IDCMP_CLOSEWINDOW:
		    	quitme = TRUE;
			break;
		}
	    }
	    ReplyMsg((struct Message *)msg);
	    
	} /* while((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
	
    } /* while(!quitme) */
}

/****************************************************************************************/

int main(void)
{
    openlibs();
    getvisual();
    initregister(&myregister, myregisteritems);
    layoutregister(&myregister, scr, dri, TRUE);
    makewin();
    setregisterpos(&myregister, win->BorderLeft + 4, win->BorderTop + 4);
    renderregister(win->RPort, &myregister);
    handleall();
    cleanup(0);
    return 0;
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
