/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

/****************************************************************************************/

#define USE_BOOPSI_STUBS

#include <exec/execbase.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <graphics/gfx.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <aros/asmcall.h>
#include <clib/boopsistubs.h>
#include <string.h>

#include "coolimages.h"

/****************************************************************************************/

struct CoolButtonData
{
    struct CoolImage *image;
    ULONG   	     *pal;
    Object  	     *frame;
    ULONG   	     bgcol;
    ULONG   	     selcol;
};

/****************************************************************************************/

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase 	    *GfxBase;
extern struct UtilityBase   *UtilityBase;

struct IClass 	    	    *cool_buttonclass;

/****************************************************************************************/

#define CyberGfxBase	    cool_cybergfxbase
#define G(x)	    	    ((struct Gadget *)(x))

#define BORDERIMAGESPACINGX 	4

/****************************************************************************************/

static struct Library       *cool_cybergfxbase;

/****************************************************************************************/

static void getgadgetcoords(struct Gadget *gad, struct GadgetInfo *gi,
    	    	    	    WORD *x, WORD *y, WORD *w, WORD *h)
{
    *x = gad->LeftEdge + ((gad->Flags & GFLG_RELRIGHT)  ? gi->gi_Domain.Width  - 1 : 0);
    *y = gad->TopEdge  + ((gad->Flags & GFLG_RELBOTTOM) ? gi->gi_Domain.Height - 1 : 0);
    *w = gad->Width    + ((gad->Flags & GFLG_RELWIDTH)  ? gi->gi_Domain.Width  : 0);
    *h = gad->Height   + ((gad->Flags & GFLG_RELHEIGHT) ? gi->gi_Domain.Height : 0);
}

/****************************************************************************************/

static IPTR coolbutton_new(Class * cl, Object * o, struct opSet * msg)
{
    struct CoolButtonData *data;
    struct TagItem fitags[] =
    {
	{ IA_FrameType , FRAME_BUTTON },
	{ IA_EdgesOnly , FALSE	      },
	{ TAG_DONE   	    	      }
    };
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
    	data = INST_DATA(cl, o);
	
	data->image = (struct CoolImage *)GetTagData(COOLBT_CoolImage, 0, msg->ops_AttrList);
	data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);

	if (!data->frame)
	{
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	} else {
	    if (CyberGfxBase && data->image)
	    {
	        if ((data->pal = AllocVec(data->image->numcolors * sizeof(ULONG), MEMF_PUBLIC)))
		{
		    ULONG *p = data->pal;
		    WORD  i;
		    
		    for(i = 0; i < data->image->numcolors; i++)
		    {
		        *p++ = (data->image->pal[i * 3] << 16) |
			       (data->image->pal[i * 3 + 1] << 8) |
			       (data->image->pal[i * 3 + 2]);
		    }
		    
		} else {
		    data->image = NULL;
		}
	    } else {
	        data->image = NULL;
	    }
	}
	
    } /* if (o) */
    
    return (IPTR)o;
}

/****************************************************************************************/

static IPTR coolbutton_dispose(Class * cl, Object * o, Msg msg)
{
    struct CoolButtonData *data;
    
    data = INST_DATA(cl, o);
    
    if (data->frame) DisposeObject(data->frame);
    FreeVec(data->pal);
    
    return DoSuperMethodA(cl, o, msg);
}

/****************************************************************************************/

static IPTR coolbutton_hittest(Class *cl, Object *o, struct gpHitTest *msg)
{
    WORD gadx, gady, gadw, gadh;
    
    getgadgetcoords(G(o), msg->gpht_GInfo, &gadx, &gady, &gadw, &gadh);

    return ((msg->gpht_Mouse.X >= 0) &&
    	    (msg->gpht_Mouse.Y >= 0) &&
	    (msg->gpht_Mouse.X < gadw) &&
	    (msg->gpht_Mouse.Y < gadh)) ? GMR_GADGETHIT : 0;
}

/****************************************************************************************/

static IPTR coolbutton_render(Class *cl, Object *o, struct gpRender *msg)
{
    struct CoolButtonData   *data;
    STRPTR  	    	    text = (STRPTR)G(o)->GadgetText;
    struct TagItem  	    im_tags[] =
    {
	{IA_Width   , 0	},
	{IA_Height  , 0	},
	{TAG_DONE	}
    };
    WORD    	    	    x, y, w, h;

    getgadgetcoords(G(o), msg->gpr_GInfo, &x, &y, &w, &h);
    
    data = INST_DATA(cl, o);
    
    im_tags[0].ti_Data = w;
    im_tags[1].ti_Data = h;

    SetAttrsA(data->frame, im_tags);

    DrawImageState(msg->gpr_RPort,
		   (struct Image *)data->frame,
		   x,
		   y,
		   (G(o)->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
		   msg->gpr_GInfo->gi_DrInfo);
    
    if (GetBitMapAttr(msg->gpr_RPort->BitMap, BMA_DEPTH) < 15)
    {
    	data->image = NULL;
    }
    
    if (text)
    {
	WORD len = strlen(text);
	WORD tx = x, ty = y;
	
	SetFont(msg->gpr_RPort, msg->gpr_GInfo->gi_DrInfo->dri_Font);

	if (data->image)
	{
	    tx += BORDERIMAGESPACINGX + data->image->width + BORDERIMAGESPACINGX;
	    w  -= (BORDERIMAGESPACINGX + data->image->width + BORDERIMAGESPACINGX + BORDERIMAGESPACINGX);
	}
	
	tx += (w - TextLength(msg->gpr_RPort, text, len)) / 2; 
	ty += (h - msg->gpr_RPort->TxHeight) / 2 + msg->gpr_RPort->TxBaseline;
	
	SetABPenDrMd(msg->gpr_RPort,
		     msg->gpr_GInfo->gi_DrInfo->dri_Pens[(G(o)->Flags & GFLG_SELECTED) ? FILLTEXTPEN : TEXTPEN],
		     0,
		     JAM1);
	Move(msg->gpr_RPort, tx, ty);
	Text(msg->gpr_RPort, text, len);
    } else {
    	x += 3; w -= 6;
	y += 3; h -= 6;
		 
	SetABPenDrMd(msg->gpr_RPort,
		     msg->gpr_GInfo->gi_DrInfo->dri_Pens[(G(o)->Flags & GFLG_SELECTED) ? FILLPEN : BACKGROUNDPEN],
		     0,
		     JAM1);
        
	RectFill(msg->gpr_RPort, x, y, x + w - 1, y + h - 1);
    }

    if (data->image)
    {
	if ((data->selcol == 0) && (data->bgcol == 0))
	{
	    ULONG col[3];
	    
	    GetRGB32(msg->gpr_GInfo->gi_Screen->ViewPort.ColorMap,
	    	     msg->gpr_GInfo->gi_DrInfo->dri_Pens[FILLPEN],
		     1,
		     col);
		     
	    data->selcol = ((col[0] & 0xFF000000) >> 8) +
	    	    	   ((col[1] & 0xFF000000) >> 16) +
			   ((col[2] & 0xFF000000) >> 24);
	    

	    GetRGB32(msg->gpr_GInfo->gi_Screen->ViewPort.ColorMap,
	    	     msg->gpr_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN],
		     1,
		     col);
		     
	    data->bgcol = ((col[0] & 0xFF000000) >> 8) +
	    	    	  ((col[1] & 0xFF000000) >> 16) +
			  ((col[2] & 0xFF000000) >> 24);
	    
	}
	
	
	data->pal[0] = (G(o)->Flags & GFLG_SELECTED) ? data->selcol : data->bgcol;
	
	WriteLUTPixelArray((APTR)data->image->data,
			    0,
			    0,
			    data->image->width,
			    msg->gpr_RPort,
			    data->pal,
			    x + BORDERIMAGESPACINGX,
			    y + (h - data->image->height) / 2,
			    data->image->width,
			    data->image->height,
			    CTABFMT_XRGB8);
        
    }
    	    
    return 0;
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/

AROS_UFH3S(IPTR, cool_buttonclass_dispatcher,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(Object *, obj, A2),
	  AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    IPTR retval;

    switch (msg->MethodID)
    {
        case OM_NEW:
	    retval = coolbutton_new(cl, obj, (struct opSet *)msg);
	    break;

	case OM_DISPOSE:
	    retval = coolbutton_dispose(cl, obj, msg);
	    break;

	case GM_HITTEST:
	    retval = coolbutton_hittest(cl, obj, (struct gpHitTest *)msg);
	    break;

	case GM_RENDER:
	    retval = coolbutton_render(cl, obj, (struct gpRender *)msg);
	    break;
	
	default:
	    retval = DoSuperMethodA(cl, obj, msg);
	    break;

    } /* switch (msg->MethodID) */

    return retval;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

#undef CyberGfxBase

/****************************************************************************************/

BOOL InitCoolButtonClass(struct Library *CyberGfxBase)
{
    BOOL retval = FALSE;
    
    cool_cybergfxbase = CyberGfxBase;
    
    if (IntuitionBase && GfxBase && UtilityBase) // && SysBase)
    {
    	if (!cool_buttonclass)
	{
   	    cool_buttonclass = MakeClass(NULL, BUTTONGCLASS, NULL, sizeof(struct CoolButtonData), 0UL);
	}
	
    	if (cool_buttonclass)
	{
    	    cool_buttonclass->cl_Dispatcher.h_Entry = (HOOKFUNC)cool_buttonclass_dispatcher;
	    retval = TRUE;
	}
    }
    
    return retval;
}

/****************************************************************************************/

void CleanupCoolButtonClass(void)
{
    if (cool_buttonclass)
    {
    	FreeClass(cool_buttonclass);
	cool_buttonclass = NULL;
    }
}

/****************************************************************************************/
