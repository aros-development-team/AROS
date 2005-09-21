/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$

    Common routines for Gadgets.
*/

/**********************************************************************************************/

#include <exec/types.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/cghooks.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <graphics/gfxmacros.h>

#include <string.h>

#include "intuition_intern.h"
#include "gadgets.h"

#define DEBUG_RENDERDISABLEPATTERN(x)   ;
#define DEBUG_ERASERELGADGETAREA(x) ;
#define DEBUG_SETUPGINFO(x)     ;


/**********************************************************************************************/

#define EG(o) ((struct ExtGadget *)o)
#define IM(o) ((struct Image *)o)

/**********************************************************************************************/

/* print the label of a gadget object */

void printgadgetlabel(Class *cl, Object *o, struct gpRender *msg,
                      struct IntuitionBase *IntuitionBase)
{
    struct RastPort *rp = msg->gpr_RPort;
    struct IBox      container;
    UWORD           *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
    ULONG            len, x, y;

    if (!(EG(o)->GadgetText)) return;

    GetGadgetIBox((struct Gadget *)o, msg->gpr_GInfo, &container);
    //dprintf("printgadgetlabel: o %p ibox %d %d %d %d\n",o,container.Left,container.Top,container.Width,container.Height);

    SetFont(rp, msg->gpr_GInfo->gi_DrInfo->dri_Font);

    switch (EG(o)->Flags & GFLG_LABELMASK)
    {
	case GFLG_LABELITEXT:
            /* ITexts must not to be centered! */

    	    /* frbuttonclass sets fg/bg/drawmode attributes */
            int_PrintIText(rp, EG(o)->GadgetText, container.Left,container.Top, TRUE, IntuitionBase);
            break;

	case GFLG_LABELSTRING:
            len = strlen ((STRPTR) EG(o)->GadgetText);

            if (len > 0UL)
            {
        	ULONG x;
        	ULONG y;

        	x = container.Left + (container.Width / 2);
        	x -= LabelWidth (rp,
                        	 (STRPTR)EG(o)->GadgetText, len, IntuitionBase) / 2;
        	y = container.Top + (container.Height / 2) +
                    rp->Font->tf_Baseline;
        	y -= rp->Font->tf_YSize / 2;
        	SetAPen (rp, pens[TEXTPEN]);
        	Move (rp, x, y);
        	RenderLabel (rp,
                             (STRPTR) EG(o)->GadgetText, len,
                             IntuitionBase);
            }
            break;

	case GFLG_LABELIMAGE:
            /* center image position */

            x = container.Left + ((container.Width / 2) -
                        	  (IM(EG(o)->GadgetText)->Width / 2)) - IM(EG(o)->GadgetText)->LeftEdge;

            y = container.Top + ((container.Height / 2) -
                        	 (IM(EG(o)->GadgetText)->Height / 2)) - IM(EG(o)->GadgetText)->TopEdge;
            //dprintf("printgadgetlabel: o %p w %d h %d x %d y %d l %d t %d\n", o,IM(EG(o)->GadgetText)->Width,IM(EG(o)->GadgetText)->Height,x,y,IM(EG(o)->GadgetText)->LeftEdge,IM(EG(o)->GadgetText)->TopEdge);

            DrawImageState(rp, IM(EG(o)->GadgetText),
                	   x, y,
                	   ((EG(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL ),
                	   msg->gpr_GInfo->gi_DrInfo);
            break;

    } /* switch (EG(o)->Flags & GFLG_LABELMASK) */

}

/**********************************************************************************************/

/* Calculate the size of the Bounding Box of the gadget */
void CalcBBox (struct Window * window, struct Requester * req, struct Gadget * gadget,
               struct BBox * bbox)
{

    GetDomGadgetIBox(gadget, window->WScreen, window, req, (struct IBox *)bbox);

} /* CalcBBox */

/**********************************************************************************************/

/* Figure out the size of the gadget rectangle, taking relative
 * positioning into account.
 */
VOID GetGadgetIBox(struct Gadget *g, struct GadgetInfo *gi, struct IBox *ibox)
{
    ibox->Left   = g->LeftEdge;
    ibox->Top    = g->TopEdge;
    ibox->Width  = g->Width;
    ibox->Height = g->Height;

    if (gi)
    {
        if (g->Flags & GFLG_RELRIGHT)
            ibox->Left   += gi->gi_Domain.Width - 1;

        if (g->Flags & GFLG_RELBOTTOM)
            ibox->Top    += gi->gi_Domain.Height - 1;

        if (g->Flags & GFLG_RELWIDTH)
            ibox->Width  += gi->gi_Domain.Width;

        if (g->Flags & GFLG_RELHEIGHT)
            ibox->Height += gi->gi_Domain.Height;
    }

    DEBUG_GETGADGETIBOX(dprintf("GetGadgetIBox: Gadget 0x%lx GInfo 0x%lx Flags 0x%lx Left %ld Top %ld Width %ld Height %ld\n",
                                o, gi, g->Flags, ibox->Left, ibox->Top, ibox->Width, ibox->Height));
}

/**********************************************************************************************/

ULONG LabelWidth (struct RastPort * rp, STRPTR label, ULONG len,
                  struct IntuitionBase * IntuitionBase)
{
    ULONG totalwidth, uscorewidth;

    totalwidth  = TextLength (rp, label, len);
    uscorewidth = TextLength (rp, "_", 1);

    while (len && *label)
    {
        if (*label == '_')
            totalwidth -= uscorewidth;

        label ++;
        len --;
    }

    return totalwidth;
}

/**********************************************************************************************/

void RenderLabel (struct RastPort * rp, STRPTR label, ULONG len,
                  struct IntuitionBase * IntuitionBase)
{
    ULONG renderlen;
    ULONG uscorewidth;

    while (*label)
    {
        renderlen = 0;

        while (label[renderlen] && label[renderlen] != '_')
            renderlen ++;

        Text (rp, label, renderlen);

        label += renderlen;

        if (*label) /* '_' ? */
        {
            WORD cx, cy;

            label ++; /* Skip */

            uscorewidth = TextLength (rp, label, 1);

            cx = rp->cp_x;
            cy = rp->cp_y;

            Move (rp, cx, cy+2);
            Draw (rp, cx+uscorewidth-1, cy+2);
            Move (rp, cx, cy);
        }
    }
}

/**********************************************************************************************/

VOID drawrect(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
              struct IntuitionBase *IntuitionBase)
{
    Move(rp, x1, y1);

    /* We RectFill() because it is generally faster than Draw()
       (Draw() uses Set/GetPixel() while RectFill() can do higherlevel
       clipping. Also it is MUCH faster in the x11gfx hidd.
    */

    RectFill(rp, x1, y1, x2 - 1, y1);   /* Top      */
    RectFill(rp, x2, y1, x2, y2 - 1);   /* Right    */
    RectFill(rp, x1 + 1, y2, x2, y2);   /* Bottom   */
    RectFill(rp, x1, y1 + 1, x1, y2);   /* Left     */

    return;
}

/**********************************************************************************************/

void GetGadgetDomain(struct Gadget *gad, struct Screen *scr, struct Window *win,
                     struct Requester *req, struct IBox *box)
{
    switch (gad->GadgetType & (GTYP_GADGETTYPE & ~GTYP_SYSGADGET))
    {
	case GTYP_SCRGADGET:
            box->Left   = 0;
            box->Top    = 0;
            box->Width  = scr->Width;
            box->Height = scr->Height;

            break;

	case GTYP_GZZGADGET:
            /* stegerg: this means gadget is in window border! */

            box->Left   = 0;
            box->Top    = 0;
            box->Width  = win->Width;
            box->Height = win->Height;

            break;

	case GTYP_REQGADGET:
            box->Left   = req->LeftEdge + win->BorderLeft;
            box->Top    = req->TopEdge + win->BorderTop;
            box->Width  = req->Width;
            box->Height = req->Height;

            break;

	default:
            if (win->Flags & WFLG_GIMMEZEROZERO)
            {
        	/* stegerg: domain.left and domain.top must not be added
        	   to gadget position when it is rendered, because gadgets
        	   in the innerlayer of a gzz gadget are already shifted
        	   thanks to the innerlayer. */

        	box->Left   = win->BorderLeft;
        	box->Top    = win->BorderTop;

        	box->Width  = win->Width - win->BorderLeft - win->BorderRight;
        	box->Height = win->Height - win->BorderTop - win->BorderBottom;

            }
            else
            {
        	box->Left   = 0;
        	box->Top    = 0;
        	box->Width  = win->Width;
        	box->Height = win->Height;

            }

            break;

    } /* switch (gadgettype) */
}

/**********************************************************************************************/

WORD GetGadgetLeft(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req)
{
    struct IBox box;

    GetGadgetDomain(gad, scr, win, req, &box);

    return gad->LeftEdge + ADDREL(gad, GFLG_RELRIGHT, (&box), Width - 1);
}

/**********************************************************************************************/

WORD GetGadgetTop(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req)
{
    struct IBox box;

    GetGadgetDomain(gad, scr, win, req, &box);

    return gad->TopEdge + ADDREL(gad, GFLG_RELBOTTOM, (&box), Height - 1);
}

/**********************************************************************************************/

WORD GetGadgetWidth(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req)
{
    struct IBox box;

    GetGadgetDomain(gad, scr, win, req, &box);

    return gad->Width + ADDREL(gad, GFLG_RELWIDTH, (&box), Width);
}

/**********************************************************************************************/

WORD GetGadgetHeight(struct Gadget *gad, struct Screen *scr, struct Window *win, struct Requester *req)
{
    struct IBox box;

    GetGadgetDomain(gad, scr, win, req, &box);

    return gad->Height + ADDREL(gad, GFLG_RELHEIGHT, (&box), Height);
}

/**********************************************************************************************/

/* gadget box in screen coords */
void GetScrGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
                      struct Requester *req, struct IBox *box)
{
    GetWinGadgetIBox(gad, scr, win, req, box);

    if (win)
    {
        box->Left += win->LeftEdge;
        box->Top += win->TopEdge;
    }
}

/**********************************************************************************************/

/* gadget box relative to upper left window edge */
void GetWinGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
                      struct Requester *req, struct IBox *box)
{
    struct IBox domain;

    GetGadgetDomain(gad, scr, win, req, &domain);

    box->Left   = gad->LeftEdge;
    box->Top    = gad->TopEdge;
    box->Width  = gad->Width;
    box->Height = gad->Height;

    if (gad->Flags & GFLG_RELRIGHT)
        box->Left += domain.Width - 1;

    if (gad->Flags & GFLG_RELBOTTOM)
        box->Top += domain.Height - 1;

    if (gad->Flags & GFLG_RELWIDTH)
        box->Width += domain.Width;

    if (gad->Flags & GFLG_RELHEIGHT)
        box->Height += domain.Height;

    box->Left += domain.Left;
    box->Top += domain.Top;
}

/**********************************************************************************************/

/* gadget box in domain coords */
void GetDomGadgetIBox(struct Gadget *gad, struct Screen *scr, struct Window *win,
                      struct Requester *req, struct IBox *box)
{
    struct IBox domain;

    GetWinGadgetIBox(gad, scr, win, req, box);
    GetGadgetDomain(gad, scr, win, req, &domain);

    box->Left -= domain.Left;
    box->Top  -= domain.Top;
}

/**********************************************************************************************/

/* gadget bounds in screen coords */
void GetScrGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
                        struct Requester *req, struct IBox *box)
{
    struct IBox domain;

    if (gad->Flags & GFLG_EXTENDED)
    {
        if (EG(gad)->MoreFlags & GMORE_BOUNDS)
        {
            GetGadgetDomain(gad, scr, win, req, &domain);

            box->Left = domain.Left +
                        EG(gad)->BoundsLeftEdge + ADDREL(gad, GFLG_RELRIGHT, (&domain), Width - 1) +
                        (win ? win->LeftEdge : 0);

            box->Top  = domain.Top +
                        EG(gad)->BoundsTopEdge + ADDREL(gad, GFLG_RELBOTTOM, (&domain), Height - 1) +
                        (win ? win->TopEdge : 0);

            box->Width = EG(gad)->BoundsWidth + ADDREL(gad, GFLG_RELWIDTH, (&domain), Width);

            box->Height = EG(gad)->BoundsHeight + ADDREL(gad, GFLG_RELHEIGHT, (&domain), Height);

            return;

        } /* if (gad->MoreFlags & GMORE_BOUNDS) */

    } /* if (gad->Flags & GFLG_EXTENDED) */

    /* if gadget does not have bounds return box */

    GetScrGadgetIBox(gad, scr, win, req, box);
}

/**********************************************************************************************/

/* gadget bounds relative to upper left window edge */
void GetWinGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
                        struct Requester *req, struct IBox *box)
{
    GetScrGadgetBounds(gad, scr, win, req, box);

    if (win)
    {
        box->Left -= win->LeftEdge;
        box->Top  -= win->TopEdge;
    }
}

/**********************************************************************************************/

/* gadget bounds in domain coords */
void GetDomGadgetBounds(struct Gadget *gad, struct Screen *scr, struct Window *win,
                        struct Requester *req, struct IBox *box)
{
    struct IBox domain;

    GetWinGadgetBounds(gad, scr, win, req, box);
    GetGadgetDomain(gad, scr, win, req, &domain);

    box->Left -= domain.Left;
    box->Top  -= domain.Top;
}

/**********************************************************************************************/

void EraseRelGadgetArea(struct Window *win, struct Rectangle *clipto,
    	    	    	BOOL onlydamagelist, struct IntuitionBase *IntuitionBase)
{
    struct Gadget   *gad;
    struct Region   *old_clipregion = NULL; /* shut up the compiler */
    struct RastPort *rp, *rp2;
    struct Layer    *lay;
    struct IBox      box;
    WORD             old_scroll_x = 0, old_scroll_y = 0, i, num_loops;

    DEBUG_ERASERELGADGETAREA(dprintf("EraseRelGadgetArea: win 0x%lx onlydamage %d\n",
                                     win, onlydamagelist));

    rp  = win->RPort;
    rp2 = win->BorderRPort;

    if (rp->Layer == rp2->Layer)
    {
        num_loops = 1;
    }
    else
    {
        num_loops = 2;
	clipto = NULL; /* Don't do any border clipping for GZZ windows */
    }

    for(i = 0; i < num_loops; i++, rp = rp2)
    {
        lay = rp->Layer;

        LockLayer(NULL,lay);

        if (!onlydamagelist)
        {
            old_scroll_x = lay->Scroll_X;
            old_scroll_y = lay->Scroll_Y;
            old_clipregion = InstallClipRegion(lay, NULL);
        }

        gad = win->FirstGadget;
        while(gad)
        {
            BOOL checkgad = FALSE;

            DEBUG_ERASERELGADGETAREA(dprintf("EraseRelGadgetArea: Gadget 0x%lx Flags 0x%lx Type 0x%lx Activation 0x%lx %d,%d %d×%d\n",
                                             gad, gad->Flags, gad->GadgetType, gad->Activation,
                                             gad->LeftEdge, gad->TopEdge, gad->Width, gad->Height));

            if (num_loops == 1)
            {
                checkgad = TRUE;
            }
            else
            {
                if (i == 0)
                {
                    if (!IS_BORDER_GADGET(gad)) checkgad = TRUE;
                }
                else
                {
                    if (IS_BORDER_GADGET(gad)) checkgad = TRUE;
                }
            }

            if (checkgad)
            {
                if (gad->Flags & (GFLG_RELRIGHT | GFLG_RELBOTTOM |
                                  GFLG_RELWIDTH | GFLG_RELHEIGHT | GFLG_RELSPECIAL))
                {
                    struct Rectangle rect;

                    GetDomGadgetBounds(gad, win->WScreen, win, NULL, &box);

                    if ((box.Width > 0) && (box.Height > 0))
                    {
                        rect.MinX = box.Left;
                        rect.MinY = box.Top;
                        rect.MaxX = box.Left + box.Width  - 1;
                        rect.MaxY = box.Top  + box.Height - 1;

                        if (!(lay->Flags & LAYERSUPER))
                        {
                            DEBUG_ERASERELGADGETAREA(dprintf("EraseRelGadgetArea: gad 0x%lx add (%d,%d)-(%d,%d)\n", gad, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY));

                            OrRectRegion(lay->DamageList, &rect);
                            lay->Flags |= LAYERREFRESH;
                        }

                        if (!onlydamagelist)
			{
    	    	    	    if (!clipto || AndRectRect(&rect, clipto, &rect))
	    	    	    {
 			    	EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
			    }
			}

                    }

                }

            } /* if (checkgad) */

            gad = gad->NextGadget;

        } /* while(gad) */

        if (!onlydamagelist)
        {
            InstallClipRegion(lay, old_clipregion);
            lay->Scroll_X = old_scroll_x;
            lay->Scroll_Y = old_scroll_y;
        }

        UnlockLayer(lay);

    } /* for(i = 0; i < num_loops; i++, rp = rp2) */

}

/**********************************************************************************************/

void RenderDisabledPattern(struct RastPort *rp, struct DrawInfo *dri, WORD x1, WORD y1,
                           WORD x2, WORD y2, struct IntuitionBase *IntuitionBase)
{
    UWORD pen;
    UWORD pattern[] = { 0x8888, 0x2222 };

    DEBUG_RENDERDISABLEPATTERN(dprintf("RenderDisabledPattern: rp 0x%lx dri 0x%lx x1 %ld y1 %ld x2 %ld y2 %ld\n",
                                       rp,dri,x1,y1,x2,y2));

    if (dri)
    {
        pen = dri->dri_Pens[SHADOWPEN];
    }
    else
    {
        pen = 1;
    }

    SetDrMd( rp, JAM1 );
    SetAPen( rp, pen );
    SetAfPt( rp, pattern, 1);

    /* render disable pattern */
    RectFill(rp, x1, y1, x2, y2);

    SetAfPt (rp, NULL, 0);

}

/**********************************************************************************************/

ULONG GetGadgetState(struct Window *window, struct Gadget *gadget)
{
    ULONG state = IDS_NORMAL;

    if (IS_BORDER_GADGET(gadget) && !(window->Flags & WFLG_WINDOWACTIVE))
    {
        if (gadget->Flags & GFLG_DISABLED)
        {
            if (gadget->Flags & GFLG_SELECTED)
	    {
                state = IDS_INACTIVEDISABLED; /* Hmm ... there's no INACTIVEDISABLEDSELECTED */
	    }
            else
	    {
                state = IDS_INACTIVEDISABLED;
	    }
        }
        else
        {
            if (gadget->Flags & GFLG_SELECTED)
	    {
                state = IDS_INACTIVESELECTED;
	    }
            else
	    {
                state = IDS_INACTIVENORMAL;
	    }
        }
    }
    else
    {
        if (gadget->Flags & GFLG_DISABLED)
        {
            if (gadget->Flags & GFLG_SELECTED)
	    {
                state = IDS_SELECTEDDISABLED;
	    }
            else
	    {
                state = IDS_DISABLED;
	    }
        }
        else
        {
            if (gadget->Flags & GFLG_SELECTED)
	    {
                state = IDS_SELECTED;
	    }
            else
	    {
                state = IDS_NORMAL;
	    }
        }
    }

    return state;
}

/**********************************************************************************************/

void SetupGInfo(struct GadgetInfo *gi, struct Window *win, struct Requester *req,
                struct Gadget *gad, struct IntuitionBase *IntuitionBase)
{
    struct Window *tw = 0;

    DEBUG_SETUPGINFO(dprintf("SetupGInfo: gi %p win %p req %p gadget %p (Type 0x%x)\n",
                             gi, win, req, gad, gad->GadgetType));

    memset(gi, 0, sizeof(*gi));

    if (req)
    {
        tw = req->RWindow;
    }
    else
    {
        tw = win;
    }

    if (tw)
    {
        if (IS_SCREEN_GADGET(gad))
        {
            gi->gi_Window         = NULL;
            gi->gi_Screen         = (struct Screen *)tw;
            gi->gi_Pens.DetailPen = 0;
            gi->gi_Pens.BlockPen  = 0;
        }
        else
        {
            gi->gi_Window         = tw;
            gi->gi_Screen         = tw->WScreen;
            gi->gi_Pens.DetailPen = tw->DetailPen;
            gi->gi_Pens.BlockPen  = tw->BlockPen;
        }
        gi->gi_Requester = req;
        gi->gi_DrInfo    = GetScreenDrawInfo (gi->gi_Screen);

        SET_GI_RPORT(gi, tw, req, gad);

        gi->gi_Layer = gi->gi_RastPort ? gi->gi_RastPort->Layer : NULL;

        GetGadgetDomain(gad, gi->gi_Screen, gi->gi_Window, req, &gi->gi_Domain);
    } /* if (tw) */

    DEBUG_SETUPGINFO(dprintf("SetGInfo: Win %p Req %p RP %p Layer %p\n",
                             gi->gi_Window, gi->gi_Requester, gi->gi_RastPort, gi->gi_Layer));
}

/**********************************************************************************************/
