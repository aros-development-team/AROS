/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "global.h"
#include "registertab.h"

#include <string.h>

/****************************************************************************************/

#define IMWIDTH(x)  (((struct Image *)(x))->Width)
#define IMHEIGHT(x) (((struct Image *)(x))->Height)

/****************************************************************************************/

void InitRegisterTab(struct RegisterTab *reg, struct RegisterTabItem *items)
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

void LayoutRegisterTab(struct RegisterTab *reg, struct Screen *scr,
    	    	       struct DrawInfo *dri, BOOL samewidth)
{
    struct RastPort temprp;
    WORD    	    i, x, h, biggest_w = 0;
    
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);
    
    reg->dri = dri;
    reg->fonth = dri->dri_Font->tf_YSize;
    reg->fontb = dri->dri_Font->tf_Baseline;
    
    h = 0;
    for(i = 0; i < reg->numitems; i++)
    {
    	if (reg->items[i].image)
	{
	    if (IMHEIGHT(reg->items[i].image) > h) h = IMHEIGHT(reg->items[i].image);
	}
    }
    
    if (h) h += REGISTERTAB_IMEXTRA_HEIGHT;
        
    i = reg->fonth + REGISTERTAB_EXTRA_HEIGHT;
    h = (i > h) ? i : h;
    
    reg->height = (h + 3) & ~3; /* Multiple of 4 */
    
    reg->height += 4;
    
    reg->slopew = (reg->height - 4) / 2;
    
    for(i = 0; i < reg->numitems; i++)
    {
    	reg->items[i].textlen = strlen(reg->items[i].text);
    	reg->items[i].w = TextLength(&temprp, reg->items[i].text, reg->items[i].textlen);
	reg->items[i].w += REGISTERTABITEM_EXTRA_WIDTH + reg->slopew * 2;
	
	if (reg->items[i].image)
	{
	    reg->items[i].w += REGISTERTAB_IMAGE_TEXT_SPACE +
	    	    	       ((struct Image *)reg->items[i].image)->Width;
	}
	
	if (reg->items[i].w > biggest_w) biggest_w = reg->items[i].w;
    }
    
    if (samewidth)
    {
	for(i = 0; i < reg->numitems; i++)
	{
    	    reg->items[i].w = biggest_w;
	}
    }

    x = REGISTERTAB_SPACE_LEFT;
    for(i = 0; i < reg->numitems; i++)
    {
	WORD itemwidth;
    	WORD to = 0;
	itemwidth = TextLength(&temprp, reg->items[i].text, reg->items[i].textlen);

	if (reg->items[i].image)
	{
	    to = IMWIDTH(reg->items[i].image) + REGISTERTAB_IMAGE_TEXT_SPACE;
            itemwidth += to;
    	}
	
	reg->items[i].x1 = x;
	reg->items[i].y1 = 0;
	reg->items[i].x2 = x + reg->items[i].w - 1;
	reg->items[i].y2 = reg->height - 1;
	reg->items[i].h  = reg->items[i].y2 - reg->items[i].y1 + 1;
    	reg->items[i].ix = (reg->items[i].w - itemwidth) / 2;
	if (reg->items[i].image)
	{
	    reg->items[i].iy = (reg->items[i].h - IMHEIGHT(reg->items[i].image)) / 2;
	}
	
	reg->items[i].tx = reg->items[i].ix + to;
	reg->items[i].ty = reg->fontb + (reg->items[i].h - reg->fonth) / 2;
	
	
	x += reg->items[i].w - reg->slopew;
    }

    reg->width = x + reg->slopew + REGISTERTAB_SPACE_RIGHT;

    DeinitRastPort(&temprp);
}

/****************************************************************************************/

void SetRegisterTabPos(struct RegisterTab *reg, WORD left, WORD top)
{
    reg->left = left;
    reg->top  = top;
}

/****************************************************************************************/

void SetRegisterTabFrameSize(struct RegisterTab *reg, WORD width, WORD height)
{
    reg->framewidth  = width;
    reg->frameheight = height;
}

/****************************************************************************************/

void RenderRegisterTabItem(struct RastPort *rp, struct RegisterTab *reg, WORD item)
{
    struct RegisterTabItem *ri = &reg->items[item];
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
    
    if (ri->image)
    {
    	DrawImageState(rp, (struct Image *) ri->image, x + ri->ix, y + ri->iy, IDS_NORMAL, dri);
    }
    
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

}

/****************************************************************************************/

void RenderRegisterTab(struct RastPort *rp, struct RegisterTab *reg, BOOL alsoframe)
{
    WORD i;

    SetDrMd(rp, JAM1);
    
    SetAPen(rp, reg->dri->dri_Pens[SHINEPEN]);   
     
    RectFill(rp, reg->left,
    	    	 reg->top + reg->height - 1,
		 reg->left + REGISTERTAB_SPACE_LEFT - 1,
		 reg->top + reg->height - 1);
		 
    RectFill(rp, reg->left + reg->width - REGISTERTAB_SPACE_RIGHT,
    	    	 reg->top + reg->height - 1,
		 reg->left + reg->width - 1,
		 reg->top + reg->height - 1); 
		 
    for(i = 0; i < reg->numitems; i++)
    {
    	RenderRegisterTabItem(rp, reg, i); 
    }
    
    if (alsoframe)
    {
    	SetAPen(rp, reg->dri->dri_Pens[SHINEPEN]);
	
	RectFill(rp, reg->left,
	    	     reg->top + reg->height - 1,
		     reg->left,
		     reg->top + reg->height + reg->frameheight - 1);
		     		     
	RectFill(rp, reg->left + reg->width,
	    	     reg->top + reg->height - 1,
		     reg->left + reg->framewidth - 2,
		     reg->top + reg->height - 1);
		     
	SetAPen(rp, reg->dri->dri_Pens[SHADOWPEN]);
	
	RectFill(rp, reg->left + reg->framewidth - 1,
	    	     reg->top + reg->height - 1,
		     reg->left + reg->framewidth - 1,
		     reg->top + reg->height + reg->frameheight - 1);
		     
	RectFill(rp, reg->left + 1,
	    	     reg->top + reg->height + reg->frameheight - 1,
		     reg->left + reg->framewidth - 2,
		     reg->top + reg->height + reg->frameheight - 1);
    }
}

/****************************************************************************************/

BOOL HandleRegisterTabInput(struct RegisterTab *reg, struct IntuiMessage *msg)
{
    struct Window *win = msg->IDCMPWindow;
    BOOL    	   retval = FALSE;
    
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
	    	retval = TRUE;
		
	    	if (reg->active != i)
		{
		    WORD oldactive = reg->active;
		    
		    reg->active = i;
		    RenderRegisterTabItem(win->RPort, reg, oldactive);
		    RenderRegisterTabItem(win->RPort, reg, i);
		}
	    	break;
	    }
	}
    }
    
    return retval;
    
}


/****************************************************************************************/
/****************************************************************************************/
