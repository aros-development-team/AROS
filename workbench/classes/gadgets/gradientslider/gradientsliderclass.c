/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS gradientslider gadget.
    Lang: english
*/

#include <exec/types.h>

#ifdef __AROS__
//#define USE_BOOPSI_STUBS
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <utility/tagitem.h>
#include <gadgets/gradientslider.h>

#include <aros/asmcall.h>

#include <stdlib.h> /* abs() */
#include "gradientslider_intern.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

#else /* !AROS */

#include <intuition/icclass.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <utility/tagitem.h>
#include <gadgets/gradientslider.h>

#include <inline/graphics.h>
#include <inline/intuition.h>
#include <inline/exec.h>
#include <inline/utility.h>
#include <inline/cybergraphics.h>

#include "gradientslider_intern.h"
#include "BoopsiStubs.h"

#endif

#ifndef __AROS__

#if 1
#undef SysBase
void kprintf( STRPTR FormatStr, ... )
{
	TEXT	PutChData[64];
	STRPTR	p = PutChData;
	struct Library *SysBase = (*(struct Library **)4L);
	RawDoFmt(FormatStr, ((STRPTR)(&FormatStr))+4, (void (*)())"\x16\xc0\x4e\x75", PutChData);

	do RawPutChar( *p );
	while( *p++ );
}
#define SysBase		GSB(GradientSliderBase)->sysbase
#endif

#ifdef __STORMGCC__
LONG myBltBitMap( struct BitMap *srcBitMap, long xSrc, long ySrc,
	struct BitMap *destBitMap, long xDest, long yDest, long xSize,
	long ySize, unsigned long minterm, unsigned long mask,
	PLANEPTR tempA, struct Library *Graphics );
#endif

#endif

/***************************************************************************************************/

STATIC VOID notify_curval(Class *cl, Object *o, struct GadgetInfo *gi, BOOL interim, BOOL userinput)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    struct opUpdate		opu;
    struct TagItem		tags[] =
    {
        {GA_ID			, EG(o)->GadgetID	},
	{GA_UserInput		, userinput		},
        {GRAD_CurVal		, data->curval		},
	{TAG_DONE					}
    };
    
    opu.MethodID     = OM_NOTIFY;
    opu.opu_AttrList = tags;
    opu.opu_GInfo    = gi; 
    opu.opu_Flags    = interim ? OPUF_INTERIM : 0;
    
    DoMethodA(o, (Msg)&opu);
}

/***************************************************************************************************/

IPTR GradientSlider__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct TagItem 		*tag, *tstate;
    IPTR 			retval;
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    BOOL    	    	    	redraw_all = data->savebm == NULL;
    LONG    	    	    	flags = EG(o)->Flags;
    
    EnterFunc(bug("GradientSlider::Set()\n"));

    retval = 0;

    if (msg->MethodID != OM_NEW) retval = DoSuperMethodA(cl, o, (Msg)msg);
    
    tstate = msg->ops_AttrList;
    while((tag = NextTagItem(&tstate)))
    {
    	IPTR tidata = tag->ti_Data;
    	   	
    	switch (tag->ti_Tag)
    	{
	    case GRAD_CurVal:
	        data->curval = (ULONG)tidata;
		notify_curval(cl, o, msg->ops_GInfo, FALSE, FALSE);
		retval += 1UL;
		break;
		
	    case GA_Disabled:
	        if ((EG(o)->Flags&GFLG_DISABLED) != (flags&GFLG_DISABLED))
		{
		    retval += 1UL;
		    redraw_all = TRUE;
		}
		break;
		
	    case GRAD_MaxVal:
	        if (tidata > 0 && tidata < 65536) // ReAction fix.
		{
		    ULONG new_curval;
		    
		    new_curval = ( tidata > 0L ) ? (((ULONG)tidata) * data->curval) / data->maxval : 0L;
	            data->maxval = (ULONG)tidata;

		    if (new_curval != data->curval)
		    {
		        data->curval = new_curval;
			notify_curval(cl, o, msg->ops_GInfo, FALSE, FALSE);
		    }
		    retval += 1UL;
		}
		break;
		
	    case GRAD_SkipVal:
	        data->skipval = (ULONG)tidata;
		break;
	
	    case GRAD_PenArray:
	    {
	    	if( tidata ) // REACTION_TextAttr collides with this one
	    	{
	    	    UWORD *pen = (UWORD *) tidata;

	    	    if( ( pen[0] & 0xff00 ) && ( pen[0] != 0xffff ) )
	    	    	break;

	    	    if( ( pen[1] & 0xff00 ) && ( pen[1] != 0xffff ) )
	    	    	break;
	    	}

	        data->penarray = (UWORD *)tidata;
		data->numpens = 0;

		if (data->penarray)
		{
		    UWORD *pen = data->penarray;

		    while(*pen++ != (UWORD)~0) data->numpens++;
		}

		retval += 1UL;
		redraw_all = TRUE;
	    }
	    break;
		
	    default:
	        break;
		
    	} /* switch (tag->ti_Tag) */
    	
    } /* for (each attr in attrlist) */
        
    if (retval)
    {
        struct RastPort *rp;
	
	if ((rp = ObtainGIRPort(msg->ops_GInfo)))
	{
	    DoMethod(o, GM_RENDER, (IPTR)msg->ops_GInfo, (IPTR)rp, redraw_all ? GREDRAW_REDRAW : GREDRAW_UPDATE);
	    ReleaseGIRPort(rp);
	    retval = 0L;
	}
    }
    
    ReturnPtr ("GradientSlider::Set", IPTR, retval);
}

/***************************************************************************************************/

Object *GradientSlider__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    EnterFunc(bug("GradientSlider::New()\n"));
    
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct GradientSliderData	*data = INST_DATA(cl, o);
	static struct TagItem 		fitags[]=
	{
	   {IA_FrameType	, FRAME_BUTTON	},
           {TAG_DONE				}
	};
	
	if ((data->frame = NewObjectA(NULL, FRAMEICLASS, fitags)))
	{
	    data->maxval     = 0xFFFF;
	    data->curval     = 0;
	    data->skipval    = 0x1111;
	    data->knobpixels = GetTagData(GRAD_KnobPixels, 5, msg->ops_AttrList);
	    data->freedom    = GetTagData(PGA_Freedom, LORIENT_HORIZ, msg->ops_AttrList);

	    InitRastPort( &data->trp );
	    
    	    GradientSlider__OM_SET(cl, o, msg);
	    
	} else {
	    CoerceMethod(cl, o, OM_DISPOSE);
	    o = NULL;
	}
    	   
    }
    ReturnPtr ("GradientSlider::New", Object *, o);
}

/***************************************************************************************************/

VOID GradientSlider__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    
    if (data->frame) DisposeObject(data->frame);

    if (data->savebm)
    {
        WaitBlit();
	FreeBitMap(data->savebm);
    }

    if (data->knobbm)
    {
    	WaitBlit();
    	FreeBitMap(data->knobbm);
    }

    if (data->buffer)
    {
    	FreeVec(data->buffer);
    }

    DeinitRastPort( &data->trp );
     
    DoSuperMethodA(cl, o, msg);
}

/***************************************************************************************************/

IPTR GradientSlider__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    IPTR 			retval = 1UL;
   
    switch(msg->opg_AttrID)
    {
        case GRAD_MaxVal:
	    *msg->opg_Storage = data->maxval;
	    break;
	    
	case GRAD_CurVal:
	    *msg->opg_Storage = data->curval;
	    break;
	    
	case GRAD_SkipVal:
	    *msg->opg_Storage = data->skipval;
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, (Msg)msg);
	    break;
	    
    } /* switch(msg->opg_AttrID) */
   
    return retval;
}

/***************************************************************************************************/

VOID GradientSlider__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);    
    struct DrawInfo 		*dri = msg->gpr_GInfo->gi_DrInfo;
    struct RastPort 		*rp = msg->gpr_RPort;
    struct IBox			gbox, sbox, kbox;
    LONG			redraw = msg->gpr_Redraw;
    
    EnterFunc(bug("GradientSlider::Render()\n"));    

    GetGadgetIBox(o, msg->gpr_GInfo, &gbox);
    GetSliderBox(&gbox, &sbox);
    sbox.Left -= gbox.Left;
    sbox.Top -= gbox.Top;
    GetKnobBox(data, &sbox, &kbox);
    
    switch (redraw)
    {
    	case GREDRAW_UPDATE:
	    if ((kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height) && (data->savebm))
	    {
	    	if( gbox.Width == data->savebmwidth && gbox.Height == data->savebmheight )
	    	{
		    if ((kbox.Left != data->savefromx) || (kbox.Top != data->savefromy))
		    {
			 /* Restore old area behind knob */
    	    	    #ifdef __STORMGCC__
			 myBltBitMap( data->knobbm, 0,0, data->savebm, data->savefromx,data->savefromy, kbox.Width,kbox.Height, 0xc0, 0xff, NULL, GfxBase);
    	    	    #else
			 BltBitMap( data->knobbm, 0,0, data->savebm, data->savefromx,data->savefromy, kbox.Width,kbox.Height, 0xc0, 0xff, NULL);
    	    	    #endif
			 //BltBitMapRastPort( data->savebm, data->savefromx,data->savefromy, rp, gbox.Left+data->savefromx,gbox.Top+data->savefromy, kbox.Width,kbox.Height, 0xc0 );

			 data->savefromx = kbox.Left;
			 data->savefromy = kbox.Top;
    	    	    #ifdef __STORMGCC__
			 myBltBitMap( data->savebm, kbox.Left,kbox.Top, data->knobbm, 0,0, kbox.Width,kbox.Height, 0xc0, 0xff, NULL, GfxBase );
    	    	    #else
			 BltBitMap( data->savebm, kbox.Left,kbox.Top, data->knobbm, 0,0, kbox.Width,kbox.Height, 0xc0, 0xff, NULL );
    	    	    #endif
			 data->trp.BitMap = data->savebm;

			 DrawKnob(data, &data->trp, dri, &kbox, 0);
			 //BltBitMapRastPort( data->savebm, kbox.Left,kbox.Top, rp, gbox.Left+kbox.Left,gbox.Top+kbox.Top, kbox.Width,kbox.Height, 0xc0 );
			 BltBitMapRastPort( data->savebm, sbox.Left,sbox.Top, rp, gbox.Left+sbox.Left,gbox.Top+sbox.Top, sbox.Width,sbox.Height, 0xc0 );
		    } /* if (!data->savebm || (kbox.Left != data->savefromx) || (kbox.Top != data->savefromy)) */

		    break;
		}
		
	    } /* if ((kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height)) */

    	case GREDRAW_REDRAW:
    	    {
		struct RastPort	*trp = &data->trp;

		if( gbox.Width != data->savebmwidth || gbox.Height != data->savebmheight )
		{
		    if( data->savebm )
		    {
			WaitBlit();
			FreeBitMap( data->savebm );
			data->savebm = NULL;
		    }

		    if( data->knobbm )
		    {
			WaitBlit();
			FreeBitMap( data->knobbm );
			data->knobbm = NULL;
		    }

		    if( data->buffer )
		    {
			FreeVec( data->buffer );
			data->buffer = NULL;
		    }
		}

		if( data->savebm == NULL )
		{
		    struct TagItem fitags[] =
		    {
			{IA_Width	, gbox.Width	},
			{IA_Height	, gbox.Height	},
			{TAG_DONE			}
		    };

		    SetAttrsA(data->frame, fitags);

		    if( ! ( data->savebm = AllocBitMap(
			    gbox.Width, gbox.Height,
			    GetBitMapAttr(rp->BitMap, BMA_DEPTH),
			    BMF_MINPLANES, rp->BitMap ) ) )
		    {
			break;
		    }

		    data->savebmwidth = gbox.Width;
		    data->savebmheight = gbox.Height;

		    if( ! ( data->knobbm = AllocBitMap(
			    kbox.Width, kbox.Height,
			    GetBitMapAttr(data->savebm, BMA_DEPTH),
			    BMF_MINPLANES, data->savebm ) ) )
		    {
			FreeBitMap( data->savebm );
			data->savebm = NULL;
			break;
		    }

		    /* Draw frame */
		    trp->BitMap = data->savebm;
		    DrawImageState( trp, (struct Image *) data->frame, 0,0, IDS_NORMAL, dri );
		}

		/* Draw slider background */
		if( (sbox.Width >= 2) && (sbox.Height >= 2) )
		{
	            if (data->numpens < 2)
		    {
			WORD pen = dri->dri_Pens[BACKGROUNDPEN];

			if (data->penarray && (data->numpens == 1))
			{
		            pen = data->penarray[0];
			}

			SetDrMd(trp, JAM1);
			SetAPen(trp, pen);
			RectFill(trp, sbox.Left, sbox.Top, sbox.Left + sbox.Width - 1, sbox.Top + sbox.Height - 1);
		    } /* ff (data->numpens < 2) */
		    else
		    {
		    	DrawGradient(trp,
    		     		     sbox.Left,
				     sbox.Top,
				     sbox.Left + sbox.Width - 1,
				     sbox.Top + sbox.Height - 1,
				     data->penarray, data->numpens, data->freedom,
				     msg->gpr_GInfo->gi_Screen->ViewPort.ColorMap
			);
		    } /* data->numpens >= 2 */
		} /* if ((sbox.Width >= 2) && (sbox.Height >= 2)) */

		/* Backup area over which knob will be drawn */
		if ((kbox.Width > 0) && (kbox.Height > 0) &&
		    (kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height))
		{
    	    	#ifdef __STORMGCC__
		    myBltBitMap( data->savebm, kbox.Left,kbox.Top, data->knobbm, 0,0, kbox.Width,kbox.Height, 0xc0, 0xff, NULL, GfxBase );
    	    	#else
		    BltBitMap( data->savebm, kbox.Left,kbox.Top, data->knobbm, 0,0, kbox.Width,kbox.Height, 0xc0, 0xff, NULL );
    	    	#endif

		    data->savefromx    = kbox.Left;
		    data->savefromy    = kbox.Top;

	    	    /* Render knob */
		    DrawKnob(data, trp, dri, &kbox, 0);
		} /* if ((kbox.Width > 0) && (kbox.Height > 0) && (kbox.Width <= sbox.Width) && (kbox.Height <= sbox.Height)) */

		BltBitMapRastPort( data->savebm, 0,0, rp, gbox.Left,gbox.Top,gbox.Width,gbox.Height, 0xc0 );
	    }
	    break;
    } /* switch (redraw) */

    if( EG(o)->Flags & GFLG_DISABLED )
	    DrawDisabledPattern( rp, &gbox, dri->dri_Pens[SHADOWPEN]);
            	
    ReturnVoid("GradientSlider::Render");
}

/***************************************************************************************************/

IPTR GradientSlider__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *msg)
{
//  struct GradientSliderData 	*data = INST_DATA(cl, o);
    struct IBox			gbox, sbox;
    WORD			mousex, mousey;
    IPTR 			retval = 0UL;

    EnterFunc(bug("GradientSlider::HitTest()\n"));

    if( EG(o)->Flags & GFLG_DISABLED )
    	return 0UL;

    GetGadgetIBox(o, msg->gpht_GInfo, &gbox);
    GetSliderBox(&gbox, &sbox);

    if ((sbox.Width > 2) && (sbox.Height > 2))
    {
	/* calc mouse coords relative to slider box */

	mousex = msg->gpht_Mouse.X - (sbox.Left - gbox.Left);
	mousey = msg->gpht_Mouse.Y - (sbox.Top  - gbox.Top);

	if ((mousex >= 0) && (mousey >= 0) &&
    	    (mousex < sbox.Width) && (mousey < sbox.Height)) retval = GMR_GADGETHIT;
    }
    
    ReturnInt("GradientSlider::HitTest", IPTR, retval);
}

/***************************************************************************************************/

IPTR GradientSlider__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    struct GradientSliderData 	*data = INST_DATA(cl, o);
    struct IBox			gbox, sbox, kbox;
    WORD			mousex, mousey;
    ULONG			old_curval = data->curval;
    ULONG			new_curval;
    BOOL			knobhit = TRUE;
    IPTR 			retval = 0UL;

    EnterFunc(bug("GradientSlider::GoActive()\n"));

    if (!msg->gpi_IEvent) return GMR_NOREUSE;

    GetGadgetIBox(o, msg->gpi_GInfo, &gbox);
    GetSliderBox(&gbox, &sbox);
    GetKnobBox(data, &sbox, &kbox);

    mousex = msg->gpi_Mouse.X + gbox.Left;
    mousey = msg->gpi_Mouse.Y + gbox.Top;

    data->x = msg->gpi_Mouse.X;
    data->y = msg->gpi_Mouse.Y;
    
    data->clickoffsetx = mousex - kbox.Left + (sbox.Left - gbox.Left);
    data->clickoffsety = mousey - kbox.Top  + (sbox.Top - gbox.Top);
    
    if ( ((data->freedom == LORIENT_HORIZ) && (mousex < kbox.Left)) ||
         ((data->freedom == LORIENT_VERT)  && (mousey < kbox.Top)) )
    {
  	new_curval = old_curval - data->skipval;
	if ((LONG)new_curval < 0) new_curval = 0;
	knobhit = FALSE;	
    }
    else if ( ((data->freedom == LORIENT_HORIZ) && (mousex >= kbox.Left + kbox.Width)) ||
              ((data->freedom == LORIENT_VERT)  && (mousey >= kbox.Top + kbox.Height)) )

    {
        new_curval = old_curval + data->skipval;
	if (new_curval > data->maxval) new_curval = (LONG)data->maxval;
	knobhit = FALSE;
    }    

    data->saveval = data->curval;
    
    if (!knobhit)
    {
   	struct RastPort	*rp;
   		
   	data->curval = new_curval;   		

   	GetKnobBox(data, &sbox, &kbox);

   	notify_curval(cl, o, msg->gpi_GInfo, FALSE, TRUE);
   		
   	if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
   	{
   	    DoMethod(o, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE );
   	    ReleaseGIRPort( rp );
   	}
   /*
   	if( mousex >= kbox.Left &&
   	    mousey >= kbox.Top &&
   	    mousex < (kbox.Left+kbox.Width) &&
   	    mousey < (kbox.Top+kbox.Height) )
   	{
   	    data->clickoffsetx = mousex - kbox.Left + (sbox.Left - gbox.Left);
   	    data->clickoffsety = mousey - kbox.Top  + (sbox.Top - gbox.Top);
   	    return GMR_MEACTIVE;
   	}
   */
   
	retval = GMR_VERIFY | GMR_NOREUSE;
	/* original AmigaOS gradientslider.gadget does not seem to place any meaningful
	   value into gpi_Termination :-\ */
	*msg->gpi_Termination = data->curval;
		
    } else {
        retval = GMR_MEACTIVE;
    }
    
    ReturnInt("GradientSlider::GoActive", IPTR, retval);
}

/***************************************************************************************************/

IPTR GradientSlider__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    struct GradientSliderData	*data = INST_DATA(cl, o);
    struct IBox			gbox, sbox, kbox;
    struct InputEvent 		*ie = msg->gpi_IEvent;
    LONG			new_curval; 			/* use LONG instead of ULONG for easier checks against < 0 */
    WORD			mousex, mousey;
    IPTR 			retval = GMR_MEACTIVE;
    
    //EnterFunc(bug("GradientSlider::HandleInput\n"));
    
    GetGadgetIBox(o, msg->gpi_GInfo, &gbox);
    GetSliderBox(&gbox, &sbox);
    GetKnobBox(data, &sbox, &kbox);
    
    switch(ie->ie_Class)
    {
        case IECLASS_RAWMOUSE:
	    switch(ie->ie_Code)
	    {
	        case SELECTUP:
		    retval = GMR_VERIFY | GMR_NOREUSE;
		    /* original AmigaOS gradientslider.gadget does not seem to place any meaningful
		       value into gpi_Termination :-\ */
		    *msg->gpi_Termination = data->curval;
		    
		    notify_curval(cl, o, msg->gpi_GInfo, FALSE, TRUE);

		    break; /* SELECTUP */
		    
		case IECODE_NOBUTTON:
		    if( data->x == msg->gpi_Mouse.X && data->y == msg->gpi_Mouse.Y )
		    	break;

		    data->x = msg->gpi_Mouse.X;
		    data->y = msg->gpi_Mouse.Y;
		    
		    mousex = msg->gpi_Mouse.X - data->clickoffsetx;
		    mousey = msg->gpi_Mouse.Y - data->clickoffsety;
		    
		    if (data->freedom == LORIENT_HORIZ)
		    {
		        if (sbox.Width != data->knobpixels) /* avoid div by 0 */
			{
			    new_curval = mousex * ((LONG)data->maxval) / (sbox.Width - (LONG)data->knobpixels);
			}
		    } else {
		        if (sbox.Height != data->knobpixels) /* avoid div by 0 */
			{
			    new_curval = mousey * ((LONG)data->maxval) / (sbox.Height - (LONG)data->knobpixels);
			}
		    }
		    
		    if (new_curval < 0)
		    {
		        new_curval = 0;
		    }
		    else if (new_curval > data->maxval)
		    {
		        new_curval = (LONG)data->maxval;
		    }
		    
		    if ((ULONG)new_curval != data->curval)
		    {
		        struct RastPort *rp;
			
			data->curval = (ULONG)new_curval;
			
			if ((rp = ObtainGIRPort(msg->gpi_GInfo)))
			{
	    		    DoMethod(o, GM_RENDER, (IPTR)msg->gpi_GInfo, (IPTR)rp, GREDRAW_UPDATE);
			    ReleaseGIRPort(rp);
			}
			
			notify_curval(cl, o, msg->gpi_GInfo, TRUE, TRUE);
		    }
		    break; /* IECODE_NOBUTTON */
		    
	    } /* switch(ie->ie_Code) */
	    
	    break; /* IECLASS_RAWMOUSE */
	    
    } /* switch(ie->ie_Class) */
    
    ReturnInt("GradientSlider::HandleInput", IPTR, retval);
}

/***************************************************************************************************/

IPTR GradientSlider__GM_DOMAIN(Class *cl, Object *o, struct gpDomain *msg)
{
    struct GradientSliderData *data = INST_DATA(cl, o);
    struct DrawInfo	      *dri = (struct DrawInfo *) GetTagData( GA_DrawInfo, NULL, msg->gpd_Attrs );
    struct RastPort	      *rp = msg->gpd_RPort;
    UWORD		       width, height, x=1,y=1;

    if( dri )
    {
    	y = dri->dri_Resolution.X;
    	x = dri->dri_Resolution.Y;
    }

    switch( msg->gpd_Which )
    {
    	case GDOMAIN_MINIMUM:
    	   if( data->freedom == LORIENT_VERT )
    	   {
    	    	width  = 3 + ( ( rp->TxHeight * x ) / y ) + 3;
		height = 2 + (data->knobpixels*5) + 2;
	   }
	   else
	   {
		width = 3 + (data->knobpixels*5) + 3;
		height = 2 + rp->TxHeight + 2;
	   }
	break;

	case GDOMAIN_NOMINAL:
	   if( data->freedom == LORIENT_VERT )
    	   {
    	    	width = 3 + ( ( rp->TxHeight * x ) / y ) + 3;
		height = 2 + (10*data->knobpixels) + 2;
	   }
	   else
	   {
		width = 3 + (10*data->knobpixels) + 3;
		height = 2 + rp->TxHeight + 2;
	   }
	break;

	case GDOMAIN_MAXIMUM:
	   if( data->freedom == LORIENT_VERT )
    	   {
    	    	width = 3 + ( ( rp->TxHeight * x ) / y ) + 3;
		height = 0x7fff;
	   }
	   else
	   {
		width = 0x7fff;
		height = 2 + rp->TxHeight + 2;
	   }
	break;
    }

    msg->gpd_Domain.Left 	=
    msg->gpd_Domain.Top  	= 0;
    msg->gpd_Domain.Width	= width;
    msg->gpd_Domain.Height 	= height;

    return 1L;
}

/***************************************************************************************************/
