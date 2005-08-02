/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <proto/layers.h>
#include <graphics/gfxmacros.h>
#include <intuition/cghooks.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>
#include "intuition_intern.h"
#include "propgadgets.h"
#ifdef SKINS
#include "intuition_customize.h"
#include "intuition_customizesupport.h"
#include "morphos/proprender.h"
#endif
#include "gadgets.h"

extern ULONG HookEntry();

#undef DEBUG
#define DEBUG 0
#   include <aros/debug.h>

// static data moved to intuibase - Piru
//static WORD clickoffset_x, clickoffset_y;
#define clickoffset_x GetPrivIBase(IntuitionBase)->prop_clickoffset_x
#define clickoffset_y GetPrivIBase(IntuitionBase)->prop_clickoffset_y

BOOL isonborder(struct Gadget *gadget,struct Window *window);

static void RenderPropBackground(struct Window *win, struct DrawInfo *dri,
    	    	    	    	 struct Rectangle *rect, struct PropInfo *pi,
				 struct RastPort *rp, BOOL onborder, struct IntuitionBase *IntuitionBase)
{
    static UWORD    	pattern[] = {0x5555,0xAAAA};
    struct Rectangle 	r = *rect;
       
#if 0
    if (!(pi->Flags & PROPBORDERLESS))
    {
    	r.MinX++;
	r.MinY++;
	r.MaxX--;
	r.MaxY--;
    }
#endif
    
    SetDrMd(rp, JAM2);
    
    if (pi->Flags & PROPNEWLOOK)
    {
    	SetAfPt(rp, pattern, 1);
	SetAPen(rp, dri->dri_Pens[SHADOWPEN]);
	SetBPen(rp, dri->dri_Pens[(!onborder || !(win->Flags & WFLG_WINDOWACTIVE)) ?
	    	    	    	  BACKGROUNDPEN : FILLPEN]);
	
	RectFill(rp, r.MinX, r.MinY, r.MaxX, r.MaxY);
    	SetAfPt(rp, NULL, 0);
    }
    else
    {
    	SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
	RectFill(rp, r.MinX, r.MinY, r.MaxX, r.MaxY);
    }
}


VOID HandlePropSelectDown(struct Gadget *gadget, struct Window *w, struct Requester *req,
    	    	    	  UWORD mouse_x, UWORD mouse_y, struct IntuitionBase *IntuitionBase)
{
    struct BBox     	 knob;
    struct PropInfo 	*pi;
    UWORD   	    	 dx, dy, flags;

    pi = (struct PropInfo *)gadget->SpecialInfo;

    if (!pi)
        return;

    CalcBBox (w, req, gadget, &knob);

    /* This func gets mouse coords relative to gadget box */

    mouse_x += knob.Left;
    mouse_y += knob.Top;

    if (!CalcKnobSize (gadget, &knob))
        return;

    clickoffset_x = mouse_x - knob.Left;
    clickoffset_y = mouse_y - knob.Top;

    dx = pi->HorizPot;
    dy = pi->VertPot;

    if (pi->Flags & FREEHORIZ)
    {
        if (mouse_x < knob.Left)
        {
            if (dx > pi->HPotRes)
                dx -= pi->HPotRes;
            else
                dx = 0;
        }
        else if (mouse_x >= knob.Left + knob.Width)
        {
            if (dx < MAXPOT - pi->HPotRes)
                dx += pi->HPotRes;
            else
                dx = MAXPOT;
        }
    }

    if (pi->Flags & FREEVERT)
    {
        if (mouse_y < knob.Top)
        {
            if (dy > pi->VPotRes)
                dy -= pi->VPotRes;
            else
                dy = 0;
        }
        else if (mouse_y >= knob.Top + knob.Height)
        {
            if (dy < MAXPOT - pi->VPotRes)
                dy += pi->VPotRes;
            else
                dy = MAXPOT;
        }
    }

    flags = pi->Flags;

    /* jDc: when clicked on frame, prop gadget also gets activated! */
    if (!(flags & PROPBORDERLESS))
    {
        if (pi->Flags & FREEVERT)
        {
            knob.Left --;
            knob.Width += 2;
        } else {
            knob.Top ++;
            knob.Height += 2;
        }
    }

    if (mouse_x >= knob.Left &&
    	mouse_y >= knob.Top &&
	mouse_x < knob.Left + knob.Width &&
	mouse_y < knob.Top + knob.Height)
    {
        flags |= KNOBHIT;
    }
    else
    {
        flags &= ~KNOBHIT;
    }

    gadget->Flags |= GFLG_SELECTED;

    D(bug("New HPot: %d, new VPot: %d\n", dx, dy));

    NewModifyProp(gadget, w, req, flags, dx, dy, pi->HorizBody, pi->VertBody, 1);

    return;
}

VOID HandlePropSelectUp(struct Gadget *gadget, struct Window *w,
    	    	    	struct Requester *req, struct IntuitionBase *IntuitionBase)
{
    struct PropInfo * pi;

    pi = (struct PropInfo *)gadget->SpecialInfo;

    gadget->Flags &= ~GFLG_SELECTED;
    if (pi) pi->Flags &= ~KNOBHIT;

    if (pi) RefreshPropGadget (gadget, w, req, IntuitionBase);

/*    if (pi)
        NewModifyProp (gadget
                       , w
                       , req
                       , pi->Flags &= ~KNOBHIT
                       , pi->HorizPot
                       , pi->VertPot
                       , pi->HorizBody
                       , pi->VertBody
                       , 1
                      ); */

    return;
}

VOID HandlePropMouseMove(struct Gadget *gadget, struct Window *w,struct Requester *req,
    	    	    	 LONG dx, LONG dy, struct IntuitionBase *IntuitionBase)
{
    struct BBox      knob;
    struct PropInfo *pi;

    pi = (struct PropInfo *)gadget->SpecialInfo;

    /* Has propinfo and the mouse was over the knob */
    if (pi && (pi->Flags & KNOBHIT))
    {
        CalcBBox (w, req, gadget, &knob);

        if (!CalcKnobSize (gadget, &knob))
            return;

        /* Move the knob the same amount, ie.
        knob.Left += dx; knob.Top += dy;

        knob.Left = knob.Left
        + (pi->CWidth - knob.Width)
        * pi->HorizPot / MAXPOT;

        ie. dx = (pi->CWidth - knob.Width)
        * pi->HorizPot / MAXPOT;

        or

        pi->HorizPot = (dx * MAXPOT) /
        (pi->CWidth - knob.Width);
        */

        /* stegerg: dx and dy are not delta values
                anymore but relative to gadget
                box */

        dx = dx - clickoffset_x;
        dy = dy - clickoffset_y;

        if (pi->Flags & FREEHORIZ
                && pi->CWidth != knob.Width)
        {

            dx = (dx * MAXPOT) / (pi->CWidth - knob.Width);
            if (dx < 0)
            {
                dx = 0;
            }
            else if (dx > MAXPOT)
            {
                dx = MAXPOT;
            }

        } /* FREEHORIZ */

        if (pi->Flags & FREEVERT
                && pi->CHeight != knob.Height)
        {
            dy = (dy * MAXPOT) / (pi->CHeight - knob.Height);

            if (dy < 0)
            {
                dy = 0;
            }
            else if (dy > MAXPOT)
            {
                dy = MAXPOT;
            }

        } /* FREEVERT */

        if ( ((pi->Flags & FREEHORIZ) && (dx != pi->HorizPot)) ||
                ((pi->Flags & FREEVERT)  && (dy != pi->VertPot)) )

        {
            NewModifyProp (gadget, w, req, pi->Flags, dx, dy, pi->HorizBody, pi->VertBody, 1);
        }

    } /* Has PropInfo and Mouse is over knob */

    return;
}

int CalcKnobSize (struct Gadget * propGadget, struct BBox * knobbox)
{
    struct PropInfo *pi;
    WORD    	     x, y;
    
    pi = (struct PropInfo *)propGadget->SpecialInfo;

    //dprintf("CalcKnobSize(%lx,%d,%d,%d,%d)\n", propGadget,
    //      propGadget->LeftEdge, propGadget->TopEdge, propGadget->Width, propGadget->Height);
    //dprintf("knob(%d,%d,%d,%d)\n", knobbox->Left, knobbox->Top, knobbox->Width, knobbox->Height);

    if (pi->Flags & PROPBORDERLESS)
    {
        pi->LeftBorder = 0;
        pi->TopBorder  = 0;
    }
    else
    {
        x = y = 1;

        knobbox->Left += x;
        knobbox->Top += y;
        knobbox->Width -= x * 2;
        knobbox->Height -= y * 2;
        pi->LeftBorder = x;
        pi->TopBorder  = y;
    }
    //dprintf("knob(%d,%d,%d,%d)\n", knobbox->Left, knobbox->Top, knobbox->Width, knobbox->Height);

    pi->CWidth     = knobbox->Width;
    pi->CHeight    = knobbox->Height;

    if ((!(pi->Flags & AUTOKNOB)) && propGadget->GadgetRender)
    {
        struct Image *im = propGadget->GadgetRender;

        knobbox->Width = im->Width;
        knobbox->Height = im->Height;
    }

    if (pi->Flags & FREEHORIZ)
    {
        if (pi->Flags & AUTOKNOB)
        {
            knobbox->Width = pi->CWidth * pi->HorizBody / MAXBODY;
            if (knobbox->Width < KNOBHMIN) knobbox->Width = KNOBHMIN;
        }

        knobbox->Left = knobbox->Left + (pi->CWidth - knobbox->Width)
                        * pi->HorizPot / MAXPOT;

        if (pi->HorizBody)
        {
            if (pi->HorizBody < MAXBODY/2)
                pi->HPotRes = MAXPOT * 32768 / ((MAXBODY * 32768 / pi->HorizBody) - 32768);
            else
                pi->HPotRes = MAXPOT;
        }
        else
            pi->HPotRes = 1;
    }

    if (pi->Flags & FREEVERT)
    {
        if (pi->Flags & AUTOKNOB)
        {
            knobbox->Height = pi->CHeight * pi->VertBody / MAXBODY;
            if (knobbox->Height < KNOBVMIN) knobbox->Height = KNOBVMIN;
        }

        knobbox->Top = knobbox->Top + (pi->CHeight - knobbox->Height)
                       * pi->VertPot / MAXPOT;

        if (pi->VertBody)
        {
            if (pi->VertBody < MAXBODY/2)
                pi->VPotRes = MAXPOT * 32768 / ((MAXBODY * 32768 / pi->VertBody) - 32768);
            else
                pi->VPotRes = MAXPOT;
        }
        else
            pi->VPotRes = 1;
    }
    //dprintf("knob(%d,%d,%d,%d)\n", knobbox->Left, knobbox->Top, knobbox->Width, knobbox->Height);

    return TRUE;
} /* CalcKnobSize */


void RefreshPropGadget (struct Gadget * gadget, struct Window * window,
                        struct Requester * req, struct IntuitionBase * IntuitionBase)
{
    struct PropInfo 	*pi;
    struct DrawInfo  	*dri;
    struct GadgetInfo 	 gi;
    struct RastPort 	*rp = 0;
    struct BBox     	 bbox, kbox;
    BOOL    	    	 onborder;

    D(bug("RefreshPropGadget(gad=%p, win=%s, req=%p)\n", gadget, window->Title, req));

    onborder = (IS_BORDER_GADGET(gadget) || isonborder(gadget,window));

    if ((dri = GetScreenDrawInfo(window->WScreen)))
    {

        SetupGInfo(&gi, window, req, gadget, IntuitionBase);

        if ((rp = ObtainGIRPort(&gi)))
        {
            for (;;)
            {
                CalcBBox (window, req, gadget, &bbox);
                kbox = bbox;

                if (bbox.Width <= 0 || bbox.Height <= 0)
                    break;

                if (CalcKnobSize (gadget, &kbox))
                {
                    pi = (struct PropInfo *)gadget->SpecialInfo;

                    if (!pi)
                        break;

                    SetDrMd (rp, JAM2);

                    if (!(pi->Flags & PROPBORDERLESS))
                    {
                        SetAPen(rp,dri->dri_Pens[SHADOWPEN]);
                        drawrect(rp,bbox.Left,
                                 bbox.Top,
                                 bbox.Left + bbox.Width - 1,
                                 bbox.Top + bbox.Height - 1,
                                 IntuitionBase);
                    }

                    RefreshPropGadgetKnob (gadget, &bbox, &kbox, window, req, IntuitionBase);
                }
                else
                {

                    pi = (struct PropInfo *)gadget->SpecialInfo;

                    if (!pi)
                        break;

                    SetDrMd (rp, JAM2);

                    if (!(pi->Flags & PROPBORDERLESS))
                    {
                        SetAPen(rp,dri->dri_Pens[SHADOWPEN]);
                        drawrect(rp,bbox.Left,
                                 bbox.Top,
                                 bbox.Left + bbox.Width - 1,
                                 bbox.Top + bbox.Height - 1,
                                 IntuitionBase);

                        bbox.Left ++; bbox.Top ++;
                        bbox.Width -= 2; bbox.Height -= 2;

                    }

                    {
                        struct Rectangle tmprect;
			
                        tmprect.MinX = bbox.Left;
                        tmprect.MaxX = bbox.Left + bbox.Width - 1;
                        tmprect.MinY = bbox.Top;
                        tmprect.MaxY = bbox.Top + bbox.Height - 1;

                        RenderPropBackground(window,dri,&tmprect,pi,rp,onborder,IntuitionBase);
                    }
                } // if (CalcKnob
                break;
            }

        } /* if ((rp = ObtainGIRPort(&gi))) */

        if (rp && gadget->Flags & GFLG_DISABLED)
        {
            CalcBBox (window, req, gadget, &bbox);

            RenderDisabledPattern(rp, (struct DrawInfo *)dri, bbox.Left,
                                  bbox.Top,
                                  bbox.Left + bbox.Width - 1,
                                  bbox.Top + bbox.Height - 1,
                                  IntuitionBase);
        }

        if (rp) ReleaseGIRPort(rp);

        FreeScreenDrawInfo(window->WScreen, (struct DrawInfo *)dri);

    } /* if ((dri = GetScreenDrawInfo(window->WScreen))) */

    ReturnVoid("RefreshPropGadget");
} /* RefreshPropGadget */


void RefreshPropGadgetKnob (struct Gadget * gadget, struct BBox * clear,
                            struct BBox * knob, struct Window * window, struct Requester * req,
                            struct IntuitionBase * IntuitionBase)
{
    struct DrawInfo  *dri;
    struct RastPort 	*rp;
    struct PropInfo 	*pi;
    struct GadgetInfo    gi;
    UWORD   	    	 flags;
    BOOL    	    	 onborder;

    D(bug("RefresPropGadgetKnob(flags=%d, clear=%p, knob = %p, win=%s)\n",
          flags, clear, knob, window->Title));

    onborder = (IS_BORDER_GADGET(gadget) || isonborder(gadget,window));

    pi = (struct PropInfo *)gadget->SpecialInfo;
    flags = pi->Flags;

    if ((dri = GetScreenDrawInfo(window->WScreen)))
    {
        SetupGInfo(&gi, window, req, gadget, IntuitionBase);

        if ((rp = ObtainGIRPort(&gi)))
        {
            SetDrMd (rp, JAM2);

            if (clear)
            {
                struct Rectangle a, b, clearrects[4];
                WORD         	 i, nrects;

    	    #if (!(PROP_RENDER_OPTIMIZATION))
                if (!(flags & PROPBORDERLESS))
                {
                    clear->Left ++; clear->Top ++;
                    clear->Width -= 2; clear->Height -= 2;
                }
    	    #endif

    	    #if 0
                D(bug("RefresPropGadgetKnob: clear Left %d Top %d Width %d Height %d\n",
                      clear->Left,
                      clear->Top,
                      clear->Width,
                      clear->Height));

                D(bug("RefresPropGadgetKnob: knob Left %d Top %d Width %d Height %d\n",
                      knob->Left,
                      knob->Top,
                      knob->Width,
                      knob->Height));
    	    #endif
	    
                a.MinX = clear->Left;
                a.MinY = clear->Top;
                a.MaxX = clear->Left + clear->Width - 1;
                a.MaxY = clear->Top + clear->Height - 1;

                b.MinX = knob->Left;
                b.MinY = knob->Top;
                b.MaxX = knob->Left + knob->Width - 1;
                b.MaxY = knob->Top + knob->Height - 1;
		
    	    #if 0
                D(bug("RefresPropGadgetKnob: a MinX %d MinY %d MaxX %d MaxY %d\n",
                      a.MinX,
                      a.MinY,
                      a.MaxX,
                      a.MaxY));

                D(bug("RefresPropGadgetKnob: a MinX %d MinY %d MaxX %d MaxY %d\n",
                      b.MinX,
                      b.MinY,
                      b.MaxX,
                      b.MaxY));
    	    #endif
	    
                nrects = SubtractRectFromRect(&a, &b, clearrects);

                D(bug("RefresPropGadgetKnob: nrects %d\n",
                      nrects));
    	    #if 0
                D(bug("RefresPropGadgetKnob: clearrects[0] MinX %d MinY %d MaxX %d MaxY %d\n",
                      clearrects[0].MinX,
                      clearrects[0].MinY,
                      clearrects[0].MaxX,
                      clearrects[0].MaxY));

                D(bug("RefresPropGadgetKnob: clearrects[1] MinX %d MinY %d MaxX %d MaxY %d\n",
                      clearrects[1].MinX,
                      clearrects[1].MinY,
                      clearrects[1].MaxX,
                      clearrects[1].MaxY));

                D(bug("RefresPropGadgetKnob: clearrects[2] MinX %d MinY %d MaxX %d MaxY %d\n",
                      clearrects[2].MinX,
                      clearrects[2].MinY,
                      clearrects[2].MaxX,
                      clearrects[2].MaxY));

                D(bug("RefresPropGadgetKnob: clearrects[3] MinX %d MinY %d MaxX %d MaxY %d\n",
                      clearrects[3].MinX,
                      clearrects[3].MinY,
                      clearrects[3].MaxX,
                      clearrects[3].MaxY));
    	    #endif

                /*kprintf("\n=== oldknob = %d,%d-%d,%d   newknob = %d,%d-%d,%d\n",
                    a.MinX,
                    a.MinY,
                    a.MaxX,
                    a.MaxY,
                    b.MinX,
                    b.MinY,
                    b.MaxX,
                    b.MaxY);*/

                for(i = 0; i < nrects; i++)
                {
                    RenderPropBackground(window,dri,&clearrects[i],pi,rp,onborder,IntuitionBase);
                }

            } /* if (clear) */

            if (flags & AUTOKNOB)
            {
                int hit = ((flags & KNOBHIT) != 0);

                D(bug("RefresPropGadgetKnob: draw knob Left %d Top %d Width %d Height %d\n",
                      knob->Left,
                      knob->Top,
                      knob->Width,
                      knob->Height));

                if (onborder)
                {
                    BOOL stdlook = TRUE;

                    #ifdef SKINS
                    stdlook = RenderOnBorderPropKnob(window,dri,rp,pi,knob,hit,IntuitionBase);
                    #endif

                    if (stdlook)
                    {
                        if (flags & PROPBORDERLESS)
                        {
                            SetAPen(rp,dri->dri_Pens[SHINEPEN]);

                            /* Top edge */
                            RectFill(rp,knob->Left,
                                     knob->Top,
                                     knob->Left + knob->Width - 2,
                                     knob->Top);

                            /* Left edge */
                            RectFill(rp,knob->Left,
                                     knob->Top + 1,
                                     knob->Left,
                                     knob->Top + knob->Height - 2);

                            SetAPen(rp,dri->dri_Pens[SHADOWPEN]);

                            /* Right edge */
                            RectFill(rp,knob->Left + knob->Width - 1,
                                     knob->Top,
                                     knob->Left + knob->Width - 1,
                                     knob->Top + knob->Height - 1);

                            /* Bottom edge */
                            RectFill(rp,knob->Left,
                                     knob->Top + knob->Height - 1,
                                     knob->Left + knob->Width - 2,
                                     knob->Top + knob->Height - 1);

                            knob->Left++;
                            knob->Top++;
                            knob->Width -= 2;
                            knob->Height -= 2;

                        } /* PROPBORDERLESS */
                        else
                        {
                            SetAPen(rp,dri->dri_Pens[SHADOWPEN]);

                            if (flags & FREEHORIZ)
                            {
                                /* black line at the left and at the right */

                                RectFill(rp,knob->Left,
                                         knob->Top,
                                         knob->Left,
                                         knob->Top + knob->Height - 1);

                                RectFill(rp,knob->Left + knob->Width - 1,
                                         knob->Top,
                                         knob->Left + knob->Width - 1,
                                         knob->Top + knob->Height - 1);

                                knob->Left++,
                                knob->Width -= 2;
                            }

                            if (flags & FREEVERT)
                            {
                                /* black line at the top and at the bottom */

                                RectFill(rp,knob->Left,
                                         knob->Top,
                                         knob->Left + knob->Width - 1,
                                         knob->Top);

                                RectFill(rp,knob->Left,
                                         knob->Top + knob->Height - 1,
                                         knob->Left + knob->Width - 1,
                                         knob->Top + knob->Height - 1);

                                knob->Top++;
                                knob->Height -= 2;
                            }


                        } /* not PROPBORDERLESS */


                        SetAPen(rp, dri->dri_Pens[(window->Flags & WFLG_WINDOWACTIVE) ? FILLPEN : BACKGROUNDPEN]);

                        /* interior */
                        RectFill(rp,knob->Left,
                                 knob->Top,
                                 knob->Left + knob->Width - 1,
                                 knob->Top + knob->Height - 1);
                    } /* stdlook */

                } /* gadget inside window border */
                else
                {
                    BOOL stdlook = TRUE;

                    #ifdef SKINS
                    stdlook = RenderPropKnob(window,dri,rp,pi,knob,hit,IntuitionBase);
                    #endif

                    if (stdlook)
                    {
                        if (flags & PROPBORDERLESS)
                        {
                            SetAPen(rp,dri->dri_Pens[SHADOWPEN]);

                            /* paint black right and bottom edges */

                            RectFill(rp,knob->Left + knob->Width - 1,
                                     knob->Top,
                                     knob->Left + knob->Width - 1,
                                     knob->Top + knob->Height - 1);

                            RectFill(rp,knob->Left,
                                     knob->Top + knob->Height - 1,
                                     knob->Left + knob->Width - 2,
                                     knob->Top + knob->Height - 1);

                            knob->Width--;
                            knob->Height--;

                        } /* propborderless */
                        else
                        {
                            SetAPen(rp,dri->dri_Pens[SHADOWPEN]);

                            if (flags & FREEHORIZ)
                            {
                                /* black line at the left and at the right */

                                RectFill(rp,knob->Left,
                                         knob->Top,
                                         knob->Left,
                                         knob->Top + knob->Height - 1);

                                RectFill(rp,knob->Left + knob->Width - 1,
                                         knob->Top,
                                         knob->Left + knob->Width - 1,
                                         knob->Top + knob->Height - 1);

                                knob->Left++,
                                knob->Width -= 2;
                            }
                            if (flags & FREEVERT)
                            {
                                /* black line at the top and at the bottom */

                                RectFill(rp,knob->Left,
                                         knob->Top,
                                         knob->Left + knob->Width - 1,
                                         knob->Top);

                                RectFill(rp,knob->Left,
                                         knob->Top + knob->Height - 1,
                                         knob->Left + knob->Width - 1,
                                         knob->Top + knob->Height - 1);

                                knob->Top++;
                                knob->Height -= 2;
                            }

                        } /* not propborderless */


                        SetAPen(rp, dri->dri_Pens[SHINEPEN]);

                        /* interior */
                        RectFill(rp,knob->Left,
                                 knob->Top,
                                 knob->Left + knob->Width - 1,
                                 knob->Top + knob->Height - 1);
                    } /* stdlook */

                } /* gadget not inside window border */

            } /* if (flags & AUTOKNOB) */
            else
            {
                struct Image *image = (struct Image *)gadget->GadgetRender;
                struct BBox   bbox;

                CalcBBox (window, req, gadget, &bbox);

                if (knob->Top + image->Height <= bbox.Top + bbox.Height &&
                        knob->Left + image->Width <= bbox.Left + bbox.Width)
                {
                    image->LeftEdge = 0;
                    image->TopEdge = 0;

                    DrawImageState(rp,
                                   image,
                                   knob->Left,
                                   knob->Top,
                                   IDS_NORMAL,
                                   (struct DrawInfo *)dri);
                }
            }

            if (gadget->Flags & GFLG_DISABLED)
            {
                struct BBox bbox;

                CalcBBox (window, req, gadget, &bbox);

                RenderDisabledPattern(rp, (struct DrawInfo *)dri, bbox.Left,
                                      bbox.Top,
                                      bbox.Left + bbox.Width - 1,
                                      bbox.Top + bbox.Height - 1,
                                      IntuitionBase);
            }

            ReleaseGIRPort(rp);

        } /* if ((rp = ObtainGIRPort(&gi))) */

        FreeScreenDrawInfo(window->WScreen, (struct DrawInfo *)dri);

    } /* if ((dri = GetScreenDrawInfo(window->WScreen))) */

    ReturnVoid("RefreshPropGadgetKnob");

} /* RefreshPropGadgetKnob */

BOOL isonborder(struct Gadget *gadget, struct Window *window)
{
    if ((window->BorderBottom > 2) && (gadget->Flags & GFLG_RELBOTTOM))
    if (window->Height + gadget->TopEdge >= window->Height - window->BorderBottom) return TRUE;

    if ((window->BorderRight > 2) && (gadget->Flags & GFLG_RELRIGHT))
    if (window->Width + gadget->LeftEdge >= window->Width - window->BorderRight) return TRUE;

    return FALSE;
}
