
/* INCLUDES */

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <libraries/gadtools.h>
#include <intuition/icclass.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <gadgets/colorwheel.h>
#include <gadgets/gradientslider.h>
#include <graphics/gfxbase.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <proto/alib.h>
#include <proto/colorwheel.h>
#include <clib/macros.h>
#include <string.h>

#include <libraries/reqtools.h>
#include <proto/reqtools.h>

#include "general.h"
#include "gadstub.h"
#include "boopsigads.h"

#include "rtlocale.h"

/****************************************************************************************/

extern struct ReqToolsBase *ReqToolsBase;

/****************************************************************************************/

#define COLORWHEEL
#define GRADIENT

/****************************************************************************************/

extern ULONG ASM LoopReqHandler (ASM_REGPARAM(a1, struct rtHandlerInfo *,));
extern ULONG ASM myGetDisplayInfoData (
        OPT_REGPARAM(a1, UBYTE *,),
	OPT_REGPARAM(d0, unsigned long,),
	OPT_REGPARAM(d1, unsigned long,),
	OPT_REGPARAM(d2, unsigned long,));

/****************************************************************************************/

#define RED_ID		0		/* DO NOT CHANGE !!! */
#define GREEN_ID	1		/* DO NOT CHANGE !!! */
#define BLUE_ID		2		/* DO NOT CHANGE !!! */

#define PALETTE_ID	3
#define COPY_ID		4
#define SWAP_ID		5
#define SPREAD_ID	6
#define OK_ID		7
#define UNDO_ID		8
#define CANCEL_ID	9
#define SLIDER_ID	10
#define WHEEL_ID	11

/****************************************************************************************/

STRPTR ModeTitles[] =
{
	MSG_COPY_TITLE,
	MSG_SWAP_TITLE,
	MSG_SPREAD_TITLE
};

/****************************************************************************************/

#define MODE_TITLE_OFFSET	COPY_ID


#define LOWGRADPENS		4
#define HIGHGRADPENS		8
#define FREEFORHIGH		32

#define NO_TITLE		( ( STRPTR ) ~0 )

/****************************************************************************************/

extern struct Library 		*GadToolsBase;
extern struct IntuitionBase 	*IntuitionBase;
extern struct GfxBase 		*GfxBase;

extern struct TextAttr 		topaz80;

/****************************************************************************************/

typedef struct RealHandlerInfo			GlobData;

struct RealHandlerInfo
{
    LONG 				(*func)();     	/* private */
    ULONG 				rthi_WaitMask;
    ULONG 				rthi_DoNotWait;

    /* PRIVATE */
    ULONG 				cols[3];	/* DO NOT MOVE, see cols offset in misc.asm */
    struct ViewPort 			*vp;		/* DO NOT MOVE, see vp offset in misc.asm */
    union
    {							/* DO NOT MOVE, see offsets in misc.asm */
	struct
	{
	    ULONG 			red, green, blue;
	} 				bits;
	ULONG 				colbits[3];
    } 					col;
    struct NewWindow 			newpalwin;
    struct KeyButtonInfo 		buttoninfo;
    struct Screen 			*scr, *frontscr;
    struct Gadget 			*colgad[3], *palgad;
    struct Window 			*palwin, *win, *prwin;
    struct ColorMap 			*cm;
    struct TextAttr 			font;
    struct TextFont 			*reqfont;
    struct DrawInfo 			*drinfo;
    struct Image 			labelimages;
    struct rtReqInfo 			*reqinfo;
    struct Hook 			*imsghook, backfillhook;
    struct Catalog 			*catalog;
    struct IntuiText			itxt;
    long 				color, coldepth, colcount, reqpos, leftedge, topedge;
    int 				waitpointer, lockwindow, shareidcmp, noscreenpop;
    int 				fontwidth, fontheight, mode, os30, maxcolval[3];
    char 				key[3], palettekey;
    APTR 				colormap, undomap;
    APTR 				visinfo, winlock;
    WORD				zoom[4];

#ifdef COLORWHEEL
    /* Color wheel */
    struct Library 			*ColorWheelBase;
    struct Library 			*GradientSliderBase;
    struct ColorWheelRGB 		wheel_rgb;
    struct ColorWheelHSB 		wheel_hsb;
    struct Gadget 			*wheel;
    struct Gadget 			*wheel_slider;
    ULONG 				wheel_colortable[3];

#ifdef GRADIENT
    UWORD				wheel_pens[ HIGHGRADPENS + 1 ];
    UWORD				numgradpens;
#endif
    WORD				dowheel, fancywheel;
    Point				screenres;
#endif
};

/****************************************************************************************/

#define redbits			col.bits.red
#define greenbits		col.bits.green
#define bluebits		col.bits.blue

#define ThisProcess()		( ( struct Process * ) FindTask( NULL ) )

/****************************************************************************************/

extern ULONG ASM MakeColVal (ASM_REGPARAM(d0, ULONG,), ASM_REGPARAM(d4, ULONG,));
extern void REGARGS SpreadColors (GlobData *, int, int, ULONG *);

static int REGARGS SetupPalWindow (GlobData *, char *);
static void REGARGS SelectColor (GlobData *, int);
static void REGARGS FreeAll (GlobData *);
static void REGARGS SetColor (GlobData *, int, ULONG *);
static void REGARGS SetWheelColor (GlobData *, struct TagItem * );
static void REGARGS UpdateWheel( GlobData *, ULONG * );
static void REGARGS RestorePaletteFreeAll (GlobData *);
static void REGARGS DoColorShortcut (GlobData *, int, int, int);
static LONG ASM SAVEDS PalReqHandler (
	REGPARAM(a1, struct RealHandlerInfo *,),
	REGPARAM(d0, ULONG,),
	REGPARAM(a0, struct TagItem *,));

/****************************************************************************************/

#ifdef COLORWHEEL
#define ColorWheelBase		glob->ColorWheelBase
#define GradientSliderBase	glob->GradientSliderBase
#endif


/****************************************************************************************/

#if !USE_ASM_FUNCS

/****************************************************************************************/

ULONG MakeColVal(ULONG val, ULONG bits)                          
{
    ULONG val2;
    
    val2 = val << (32 - bits);
    val = val2;
    do
    {
        val2 >>= bits;
	val |= bits;
    } while(val2);
    
    return val;
}

/****************************************************************************************/

void REGARGS SpreadColors (GlobData *glob, int from, int to, ULONG *rgb2)
{
    LONG colstep;
    LONG step[3];
    LONG rgb[3];
    WORD actcol, steps, gun;
    
    colstep = 1;

    steps = to - from;
    if (!steps) return;

    if (steps < 0)
    {
        steps = -steps;
	colstep = -1;
    }
    
    for(gun = 0; gun < 3; gun++)
    {
        LONG diff = rgb2[gun] - glob->cols[gun];
	step[gun] = (diff << 16L) / steps;
	rgb[gun] = glob->cols[gun] << 16;
    }
    
    actcol = from;
           
    for(actcol = from;
        actcol != to;
        actcol += colstep, rgb[0] += step[0], rgb[1] += step[1], rgb[2] += step[2])
    {
        ULONG red   = (((ULONG)rgb[0]) + 0x8000) >> 16;
	ULONG green = (((ULONG)rgb[1]) + 0x8000) >> 16;
	ULONG blue  = (((ULONG)rgb[2]) + 0x8000) >> 16;
	
	if (GfxBase->LibNode.lib_Version >= 39)
	{
	    SetRGB32(glob->vp, actcol, MakeColVal(red, glob->col.colbits[0]),
	    			       MakeColVal(green, glob->col.colbits[1]),
				       MakeColVal(blue, glob->col.colbits[2]));
	}
	else
	{
	    SetRGB4(glob->vp, actcol, red, green, blue);
	}
        
    }
    
}

/****************************************************************************************/

#endif /* !USE_ASM_FUNCS */

/****************************************************************************************/

#ifdef DO_CM_DEPTH
static int
ColBits( int num )
{
    int	i, j;

    if( num < 2 )
    {
	return( 1 );
    }

    for( i = 1, j = 4; i <= 8; ++i, j <<= 1 )
    {
	if( j > num )
	{
	    return( i );
	}
    }

    return( 8 );
}
#endif

/****************************************************************************************/

LONG ASM SAVEDS PaletteRequestA (
	REGPARAM(a2, char *, title),
	REGPARAM(a3, struct rtReqInfo *, reqinfo),
	REGPARAM(a0, struct TagItem *, taglist))
{
    GlobData 		*glob;
    struct DisplayInfo 	displayinfo;
    struct TagItem 	*tag;
    const struct TagItem *tstate;
    struct TextFont 	*deffont = NULL;
    struct TextAttr 	*fontattr = NULL;
    struct Locale 	*locale = NULL;
    char 		*pubname = NULL;
    ULONG 		tagdata, reqhandler = FALSE;

    if (!(glob = AllocVec (sizeof(GlobData), MEMF_PUBLIC|MEMF_CLEAR)))
	return (-1);

    glob->os30 = (GfxBase->LibNode.lib_Version >= 39);
    glob->color = 1;
    glob->reqpos = REQPOS_DEFAULT;
    
    if ((glob->reqinfo = reqinfo))
    {
	glob->reqpos = reqinfo->ReqPos;
	glob->leftedge = reqinfo->LeftOffset;
	glob->topedge = reqinfo->TopOffset;
	deffont = reqinfo->DefaultFont;
	glob->waitpointer = reqinfo->WaitPointer;
	glob->lockwindow = reqinfo->LockWindow;
	glob->shareidcmp = reqinfo->ShareIDCMP;
	glob->imsghook = reqinfo->IntuiMsgFunc;
    }

    /* parse tags */
    while ((tag = NextTagItem (&tstate)))
    {
	tagdata = tag->ti_Data;
	if (tag->ti_Tag > RT_TagBase)
	{
	    switch (tag->ti_Tag)
	    {
		case RT_Window:		glob->prwin = (struct Window *)tagdata; break;
		case RT_ReqPos:		glob->reqpos = tagdata; break;
		case RT_LeftOffset:	glob->leftedge = tagdata; break;
		case RT_TopOffset:	glob->topedge = tagdata; break;
		case RT_PubScrName:	pubname = (char *)tagdata; break;
		case RT_Screen:		glob->scr = (struct Screen *)tagdata; break;
		case RT_ReqHandler:	*(APTR *)tagdata = glob;
					reqhandler = TRUE;
					break;
		case RT_DefaultFont:	deffont = (struct TextFont *)tagdata; break;
		case RT_WaitPointer:	glob->waitpointer = tagdata; break;
		case RT_ShareIDCMP:	glob->shareidcmp = tagdata; break;
		case RT_LockWindow:	glob->lockwindow = tagdata; break;
		case RT_ScreenToFront:	glob->noscreenpop = !tagdata; break;
		case RT_TextAttr:	fontattr = (struct TextAttr *)tagdata; break;
		case RT_IntuiMsgFunc:	glob->imsghook = (struct Hook *)tagdata; break;
		case RT_Locale:		locale = (struct Locale *)tagdata; break;
		case RTPA_Color:	glob->color = tagdata; break;
	    }
	}
    }

    glob->catalog = RT_OpenCatalog (locale);

    if (!glob->prwin || !glob->prwin->UserPort ||
        (glob->prwin->UserPort->mp_SigTask != ThisProcess()))
	glob->shareidcmp = FALSE;

    if (!(glob->scr = GetReqScreen (&glob->newpalwin, &glob->prwin, glob->scr, pubname)))
    {
	FreeAll (glob);
    	return (-1);
    }
    glob->vp = &glob->scr->ViewPort;
    glob->cm = glob->vp->ColorMap;

    if (!(glob->coldepth = GetVpCM (glob->vp, &glob->colormap)) ||
        !GetVpCM (glob->vp, &glob->undomap))
    {
	FreeAll (glob);
	return (-1);
    }

    glob->colcount = (1 << glob->coldepth);
    glob->newpalwin.Screen = glob->scr;

    if (fontattr)
        glob->font = *fontattr;
    else
        glob->font = *glob->scr->Font;

    glob->redbits = glob->greenbits = glob->bluebits = 4;
    if (glob->os30)
    {
	if (myGetDisplayInfoData ((UBYTE *)&displayinfo, sizeof (struct DisplayInfo),
				  DTAG_DISP, GetVPModeID (glob->vp)) > 0)
	{
	    glob->redbits = displayinfo.RedBits;
	    glob->greenbits = displayinfo.GreenBits;
	    glob->bluebits = displayinfo.BlueBits;
#ifdef COLORWHEEL

#ifdef __AROS__
#warning DisplayInfo.Resolution does not seem to have correct/any values yet in AROS
	    glob->screenres.x = glob->screenres.y = 22;
#else
	    glob->screenres = displayinfo.Resolution;
#endif

#endif
	}
    }
    
    glob->maxcolval[RED_ID] = (1 << glob->redbits) - 1;
    glob->maxcolval[GREEN_ID] = (1 << glob->greenbits) - 1;
    glob->maxcolval[BLUE_ID] = (1 << glob->bluebits) - 1;

    if (!(glob->visinfo = GetVisualInfoA (glob->scr, NULL)) ||
        !(glob->drinfo = GetScreenDrawInfo (glob->scr)))
    {
	FreeAll (glob);
	return (-1);
    }

#ifdef COLORWHEEL
    {
	struct ReqToolsPrefs *prefs;

	prefs = rtLockPrefs();
	glob->dowheel = ( prefs->Flags & RTPRF_DOWHEEL ) && glob->os30;
	glob->fancywheel = glob->dowheel && ( prefs->Flags & RTPRF_FANCYWHEEL );
	rtUnlockPrefs();
    }

    if( glob->dowheel )
    {
	if ((ColorWheelBase = OpenLibrary ("gadgets/colorwheel.gadget", 39)))
	{
	    GradientSliderBase = OpenLibrary ("gadgets/gradientslider.gadget", 39);
	}
    }
#endif

    SelectColor (glob, glob->color);

retryopenwin:
    if (!(glob->reqfont = GetReqFont (&glob->font, deffont, &glob->fontheight,
		                      &glob->fontwidth, glob->os30)))
    {
	FreeAll (glob);
	return (-1);
    }

    if (!SetupPalWindow (glob, title))
    {
	if( glob->dowheel )
	{
		glob->dowheel = FALSE;
#ifdef COLORWHEEL
		DisposeObject( glob->wheel );
		DisposeObject( glob->wheel_slider );
		glob->wheel = NULL;
		glob->wheel_slider = NULL;
		CloseLibrary( ColorWheelBase );
		CloseLibrary( GradientSliderBase );
		ColorWheelBase = NULL;
		GradientSliderBase = NULL;
#endif
		goto retryopenwin;
	}

	if (glob->font.ta_YSize > 8)
	{
	    glob->font = topaz80;
	    CloseFont (glob->reqfont);
	    goto retryopenwin;
	}
	
	FreeAll (glob);
	return (-1);
    }

#ifdef COLORWHEEL
    if( glob->fancywheel )
    {
	/* Try to get a fresh undo color map (in case the wheel allocates some) */
	RefreshVpCM( glob->vp, glob->undomap );

	/* And make sure selected color still is correct */
	SelectColor( glob, glob->color );
    }
#endif

    glob->winlock = DoLockWindow (glob->prwin, glob->lockwindow, NULL, TRUE);
    DoWaitPointer (glob->prwin, glob->waitpointer, TRUE);

    glob->frontscr = IntuitionBase->FirstScreen;
    DoScreenToFront (glob->scr, glob->noscreenpop, TRUE);

    /* fill in RealHandlerInfo */
    glob->func = (LONG (*)())PalReqHandler;
    glob->rthi_WaitMask = (1 << glob->palwin->UserPort->mp_SigBit);

    if (reqhandler) return (CALL_HANDLER);
    
    return ((LONG)LoopReqHandler ((struct rtHandlerInfo *)glob));
}

/****************************************************************************************/

static LONG ASM SAVEDS PalReqHandler (
    REGPARAM(a1, struct RealHandlerInfo *, glob),
    REGPARAM(d0, ULONG, sigs),
    REGPARAM(a0, struct TagItem *, taglist))
{
    struct IntuiMessage *palmsg;
    struct Gadget 	*gad;
    struct TagItem 	*tag;
    const struct TagItem *tstate;
    ULONG 		rgb[3], rgbcol;
    ULONG 		tagdata, class;
    UWORD 		code, qual;
    int 		i, gadid, shifted, alt;
    char 		key;

    /* uncomment if sigs is no longer ignored */
    //if (glob->rthi_DoNotWait) sigs = 0;

    /* parse tags */
    while ((tag = NextTagItem (&tstate)))
    {
	tagdata = tag->ti_Data;
	if (tag->ti_Tag > RT_TagBase)
	{
	    switch (tag->ti_Tag)
	    {
		case RTRH_EndRequest:
		    if (tagdata == REQ_OK)
		    {
			FreeAll (glob);
			return (0);
		    }
		    RestorePaletteFreeAll (glob);
		    return (-1);
	    }
	}
    }

    while ((palmsg = GetWin_GT_Msg (glob->palwin, glob->imsghook, glob->reqinfo)))
    {
	class = palmsg->Class;
	code = palmsg->Code;
	qual = palmsg->Qualifier;
	gad = (struct Gadget *)palmsg->IAddress;
	Reply_GT_Msg (palmsg);
	
	switch (class)
	{
	    case IDCMP_REFRESHWINDOW:
		GT_BeginRefresh (glob->palwin);
		GT_EndRefresh (glob->palwin, TRUE);
		break;
		
	    case IDCMP_CLOSEWINDOW:
		RestorePaletteFreeAll (glob);
		return (-1);
		
	    case IDCMP_MOUSEMOVE:
	    case IDCMP_GADGETDOWN:
		if (gad->GadgetID <= BLUE_ID)
		{
		    glob->cols[gad->GadgetID] = code;
		    SetColor (glob, glob->color, glob->cols);
#ifdef COLORWHEEL
		    UpdateWheel( glob, glob->cols );
#endif
		}
		break;
		
	    case IDCMP_RAWKEY:
	    case IDCMP_GADGETUP:
		if (class == IDCMP_RAWKEY)
		{
		    if (!(gadid = CheckGadgetKey (code, qual, &key, &glob->buttoninfo)))
		    {
			/* key press was not for gadget, so check other cases */
			shifted = qual & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT);
			alt = qual & (IEQUALIFIER_LALT|IEQUALIFIER_RALT);
			if (key == glob->key[RED_ID])
				DoColorShortcut (glob, RED_ID, shifted, alt);
			else if (key == glob->key[GREEN_ID])
				DoColorShortcut (glob, GREEN_ID, shifted, alt);
			else if (key == glob->key[BLUE_ID])
				DoColorShortcut (glob, BLUE_ID, shifted, alt);
			else if (key == glob->palettekey)
			{
			    code = glob->color;
			    if (!shifted) 
			    {
				code++;
				if (code >= glob->colcount) code = 0;
			    }
			    else
			    {
				if (code <= 0) code = glob->colcount;
				code--;
			    }
			    myGT_SetGadgetAttrs (glob->palgad, glob->palwin, NULL,
						 GTPA_Color, code, TAG_END);
			    gadid = PALETTE_ID;
			    glob->mode = 0;
			}
			
		    }
		    
		} /* if (class == IDCMP_RAWKEY) */
		else gadid = gad->GadgetID;
		
		if (gadid) switch (gadid)
		{
		    case CANCEL_ID:
			RestorePaletteFreeAll (glob);
			return (-1);
			
		    case OK_ID:
			code = glob->color;
			FreeAll (glob);
			return ((LONG)code);
			
		    case PALETTE_ID:
			RefreshVpCM (glob->vp, glob->undomap);
			
			if (!glob->os30)
			{
			    rgbcol = GetRGB4 (glob->cm, code);
			    for (i = 2; i >= 0; i--)
			    {
				rgb[i] = rgbcol & 0xf;
				rgbcol >>= 4;
			    }
			}
			else
			{
			    GetRGB32 (glob->cm, code, 1, rgb);
			    rgb[0] >>= (32 - glob->redbits);
			    rgb[1] >>= (32 - glob->greenbits);
			    rgb[2] >>= (32 - glob->bluebits);
			}
			
			switch (glob->mode)
			{
			    case SWAP_ID:
				    SetColor (glob, glob->color, rgb);
			    case COPY_ID:
				    SetColor (glob, code, glob->cols);
				    break;
			    case SPREAD_ID:
				    SpreadColors (glob, glob->color, code, rgb);
				    break;
			}
			
			SelectColor (glob, code);
			glob->mode = 0;
			SetWindowTitles( glob->palwin, glob->newpalwin.Title, NO_TITLE );
#ifdef COLORWHEEL
			UpdateWheel( glob, glob->cols );
#endif
			break;
			    
		    case UNDO_ID:
			LoadCMap (glob->vp, glob->undomap);
			SelectColor (glob, glob->color);
#ifdef COLORWHEEL
			UpdateWheel( glob, glob->cols );
#endif
			break;

		    default:
			if( ( gadid == SWAP_ID ) || ( gadid == COPY_ID ) || ( gadid == SPREAD_ID ) )
			{
			    glob->mode = gadid;
			    SetWindowTitles( glob->palwin, GetStr( glob->catalog, ModeTitles[ gadid - MODE_TITLE_OFFSET ] ), NO_TITLE );
			}

			break;
			    
		} /* if (gadid) switch (gadid) */
		break;

#ifdef COLORWHEEL
	    case IDCMP_IDCMPUPDATE:
		SetWheelColor( glob, ( struct TagItem * ) gad );
		break;
#endif
	}

    } /* while ((palmsg = GetWin_GT_Msg (glob->palwin, glob->imsghook, glob->reqinfo))) */
	
    return (CALL_HANDLER);
}

/****************************************************************************************/

static void REGARGS DoColorShortcut (GlobData *glob, int id, int shifted, int alt)
{
    int t = glob->cols[id];

    if (shifted)
    {
	if (alt) t -= 8; else t--;
	if (t < 0) t = 0;
    }
    else
    {
	if (alt) t += 8; else t++;
	if (t >= glob->maxcolval[id]) t = glob->maxcolval[id];
    }
    
    glob->cols[id] = t;
    SetColor (glob, glob->color, glob->cols);
    SelectColor (glob, glob->color);
}

/****************************************************************************************/

static void REGARGS SetColor (GlobData *glob, int col, ULONG *rgb)
{
    if( !glob->os30 )
    {
	SetRGB4( glob->vp, col, rgb[ 0 ], rgb[ 1 ], rgb[ 2 ] );
    }
    else
    {
	SetRGB32( glob->vp, col,
		MakeColVal( rgb[ 0 ], glob->redbits ),
		MakeColVal( rgb[ 1 ], glob->greenbits ),
		MakeColVal( rgb[ 2 ], glob->bluebits) );
    }
}

/****************************************************************************************/

#ifdef COLORWHEEL

/****************************************************************************************/

#ifdef GRADIENT

/****************************************************************************************/

static void REGARGS
UpdateGrad( GlobData *glob )
{
    if( glob->numgradpens )
    {
	LONG	i;

	GetAttr( WHEEL_HSB, (Object *)glob->wheel, ( IPTR * ) &glob->wheel_hsb );

	for( i = 0; i < glob->numgradpens; ++i )
	{
	    glob->wheel_hsb.cw_Brightness = 0xffffffff - ( ( 0xffffffff / ( glob->numgradpens - 1 ) ) * i );
	    ConvertHSBToRGB( &glob->wheel_hsb, &glob->wheel_rgb );
	    SetRGB32( &glob->scr->ViewPort, glob->wheel_pens[ i ],
		    glob->wheel_rgb.cw_Red, glob->wheel_rgb.cw_Green,
		    glob->wheel_rgb.cw_Blue );
	}
    }
}

/****************************************************************************************/

#endif

/****************************************************************************************/

static void REGARGS
UpdateWheel( GlobData *glob, ULONG *cols )
{
    if( glob->wheel )
    {
	glob->wheel_rgb.cw_Red   = MakeColVal( cols[ 0 ], glob->redbits );
	glob->wheel_rgb.cw_Green = MakeColVal( cols[ 1 ], glob->greenbits );
	glob->wheel_rgb.cw_Blue  = MakeColVal( cols[ 2 ], glob->bluebits );

	SetGadgetAttrs( glob->wheel, glob->palwin, NULL, WHEEL_RGB, (IPTR) &glob->wheel_rgb,
							 TAG_DONE );

#ifdef GRADIENT
	UpdateGrad( glob );
#endif
    }
}

/****************************************************************************************/

static void REGARGS
SetWheelColor( GlobData *glob, struct TagItem *tag )
{
    if( glob->wheel )
    {
	ULONG i;

	GetAttr( WHEEL_HSB, (Object *)glob->wheel, ( IPTR * ) &glob->wheel_hsb );
	ConvertHSBToRGB( &glob->wheel_hsb, &glob->wheel_rgb );
	glob->cols[ RED_ID ]	= glob->wheel_rgb.cw_Red >> ( 32 - glob->redbits );
	glob->cols[ GREEN_ID ]	= glob->wheel_rgb.cw_Green >> ( 32 - glob->greenbits );
	glob->cols[ BLUE_ID ]	= glob->wheel_rgb.cw_Blue >> ( 32 - glob->bluebits );
	SetColor( glob, glob->color, glob->cols );

	for( i = 0; i < 3; ++i )
	{
	    if( glob->colgad[ i ] )
	    {
		myGT_SetGadgetAttrs( glob->colgad[ i ], glob->palwin, NULL, GTSL_Level,	glob->cols[ i ],
									    TAG_END );
	    }
	}

#ifdef GRADIENT
	UpdateGrad( glob );
#endif
    }
}

/****************************************************************************************/

#endif

/****************************************************************************************/

static void REGARGS SelectColor (GlobData *glob, int col)
{
    ULONG rgb[3], rgbcol = 0;
    int i;

    if (!glob->os30) rgbcol = GetRGB4 (glob->cm, col);
    else GetRGB32 (glob->cm, col, 1, rgb);
    
    for (i = 2; i >= 0; i--)
    {
	if (!glob->os30)
	{
	    glob->cols[i] = rgbcol & 0xF;
	    rgbcol >>= 4;
	}
	else glob->cols[i] = rgb[i] >> (32 - glob->col.colbits[i]);
	
	if (glob->colgad[i])
	    myGT_SetGadgetAttrs (glob->colgad[i], glob->palwin, NULL,
			         GTSL_Level, glob->cols[i], TAG_END);
    }
    
    glob->color = col;
}

/****************************************************************************************/

static void REGARGS RestorePaletteFreeAll (GlobData *glob)
{
    LoadCMap (glob->vp, glob->colormap);
    FreeAll (glob);
}

/****************************************************************************************/

static void REGARGS FreeAll (GlobData *glob)
{
#ifdef COLORWHEEL
#ifdef GRADIENT
    LONG	i;
#endif
#endif
    if (glob->newpalwin.Type == PUBLICSCREEN) UnlockPubScreen (NULL, glob->scr);
    DoScreenToFront (glob->frontscr, glob->noscreenpop, FALSE);
    
    if (glob->palwin)
    {
	DoLockWindow (glob->prwin, glob->lockwindow, glob->winlock, FALSE);
	DoWaitPointer (glob->prwin, glob->waitpointer, FALSE);
	DoCloseWindow (glob->palwin, glob->shareidcmp);
    }
    
    RT_CloseCatalog (glob->catalog);
    my_FreeGadgets (glob->buttoninfo.glist);
    my_FreeLabelImages (&glob->labelimages);

#ifdef COLORWHEEL
    DisposeObject (glob->wheel);
    DisposeObject (glob->wheel_slider);
    
#ifdef GRADIENT
    if( glob->numgradpens )
    {
	for( i = 0; glob->wheel_pens[ i ] != (UWORD)~0; i++ )
	{
	    ReleasePen( glob->scr->ViewPort.ColorMap, glob->wheel_pens[ i ] );
	}
    }
#endif

    CloseLibrary (GradientSliderBase);
    CloseLibrary (ColorWheelBase);
#endif

    FreeVisualInfo (glob->visinfo);
    
    if (glob->drinfo) FreeScreenDrawInfo (glob->scr, glob->drinfo);
    if (glob->reqfont) CloseFont (glob->reqfont);
    
    FreeVpCM (glob->vp, glob->colormap, FALSE);
    FreeVpCM (glob->vp, glob->undomap, FALSE);
    FreeVec (glob);
}

/****************************************************************************************/

char *colstr[] = { MSG_RED, MSG_GREEN, MSG_BLUE };
char *gadtxt[] = { MSG_COPY, MSG_SWAP, MSG_SPREAD, MSG_OK, MSG_UNDO, MSG_CANCEL };

/****************************************************************************************/

static int REGARGS SetupPalWindow (GlobData *glob, char *title)
{
    struct NewGadget 	ng;
    struct Gadget 	*gad;
    struct Image 	*img;
    int 		val, i, top, buttonheight, winheight;
    int 		spacing, scrwidth, scrheight, maxwidth;
    int 		winwidth, width1, width2, reqpos, levelwidth;
#ifdef COLORWHEEL
    int 		wheelwidth = 0, wheelheight = 0, wheeltopoff;
#endif
    int 		wheeloff = 0;
    int 		leftoff, rightoff;
    ULONG 		gadlen[6], gadpos[6];
    char 		*str, *string[6];


    spacing = rtGetVScreenSize (glob->scr, (ULONG *)&scrwidth, (ULONG *)&scrheight);
    leftoff = glob->scr->WBorLeft + 5;
    rightoff = glob->scr->WBorRight + 5;

    top = (glob->scr->WBorTop + glob->scr->Font->ta_YSize + 1) + spacing / 2 + 1;
    glob->itxt.ITextFont = &glob->font;

#ifdef COLORWHEEL
    wheeltopoff = top;
#endif

    width1 = width2 = 0;
    
    for (i = 0; i < 6; i++)
    {
	string[i] = GetStr (glob->catalog, gadtxt[i]);
	val = StrWidth_noloc (&glob->itxt, string[i]) + 16;
	if (i < 3)
	{
	    if (val > width1) width1 = val;
	}
	else
	{
	    if (val > width2) width2 = val;
	}
    }

    for (i = 0; i < 3; i++) gadlen[i] = width1;
    for (i = 3; i < 6; i++) gadlen[i] = width2;

    width1 *= 3;
    width2 *= 3;

    winwidth = (leftoff + rightoff) + 25 + width1 + 2 * 8;
    val = (leftoff + rightoff) + width2 + 2 * 8;
    if (val > winwidth) winwidth = val;
    if (winwidth < 256) winwidth = 256;

    val = glob->fontheight * 2 + 4;
#ifdef __AROS__
    val += 5;
#endif

    if (glob->colcount >= 64) val *= 2;
    if (glob->colcount >= 128) val *= 2;


#ifdef COLORWHEEL
    if( glob->dowheel && glob->screenres.x && glob->screenres.y && ColorWheelBase && GradientSliderBase )
    {
	LONG	maxheight = 120;

	if( scrheight > 600 )
	{
	    maxheight = 160;
	}
	else if( scrheight < 300 )
	{
	    maxheight = 75;
	}

	wheelheight = val + glob->fontheight * 4 + 6 + 3 + spacing * 3;

	if( wheelheight > maxheight )
	{
	    wheelheight = maxheight;
	}

    #ifndef __AROS__
    #warning Changed, because gcc produced wrong code! gcc 2.95.1 compiled under UAE JIT for Linux!?
    	wheelwidth = glob->screenres.y;
	wheelwidth *= wheelheight;
	wheelwidth /= glob->screenres.x;
    #else
	wheelwidth = wheelheight * glob->screenres.y / glob->screenres.x;
    #endif

	if( ( scrwidth - winwidth > wheelwidth + 8 ) &&
	      ( scrheight > 200 ) )
	{
	    wheeloff = wheelwidth + 8;
	    winwidth += wheeloff;
	}
	else
	{
	    glob->dowheel = FALSE;
	}

    } /* if( glob->dowheel && glob->screenres.x && glob->screenres.y && ColorWheelBase && GradientSliderBase ) */
    else
    {
	glob->dowheel = FALSE;
    }
#endif


    rtSpread (gadpos, gadlen, width1, leftoff + wheeloff + 25, winwidth - rightoff, 3);
    rtSpread (gadpos + 3, gadlen + 3, width2, leftoff, winwidth - rightoff, 3);


    gad = (struct Gadget *)CreateContext (&glob->buttoninfo.glist);
    img = &glob->labelimages;
    ng.ng_Flags = 0;
    ng.ng_VisualInfo = glob->visinfo;
    ng.ng_TextAttr = &glob->font;

    str = GetStr (glob->catalog, MSG_PALETTE_COLORS);
    glob->palettekey = KeyFromStr (str, '_');

    i = leftoff + 25 + ( winwidth - ( leftoff + rightoff + 25 ) -
	    StrWidth_noloc( &glob->itxt, str ) ) / 2;

#ifdef COLORWHEEL
    if( i < wheeloff )
    {
	i = wheeloff;
    }
#endif

    img = my_CreateGadgetLabelImage (img, &ng, str, i, top, HIGHLIGHTTEXTPEN);
    top += glob->fontheight + 1 + spacing / 2;

    InitNewGadget (&ng, leftoff + wheeloff + 25, top, winwidth - ( wheeloff + leftoff + rightoff + 25),
					    val, NULL, PALETTE_ID);

    gad = glob->palgad = myCreateGadget (PALETTE_KIND, gad, &ng,
	    GTPA_Depth, (IPTR)glob->coldepth,
	    GTPA_IndicatorWidth, 38,
	    GTPA_Color, (IPTR)glob->color, TAG_END);
	    
    if (glob->os30) top += gad->Height + spacing;
    else top += val + spacing;

    buttonheight = glob->fontheight + 6;

    for (i = 0; i < 3; i++)
    {
	InitNewGadget (&ng, gadpos[i], top, gadlen[i],
			buttonheight, string[i], COPY_ID + i);
			
	gad = my_CreateButtonGadget (gad, '_', &ng);
    }
    
    top += buttonheight + spacing;

    ng.ng_Flags |= NG_HIGHLABEL;

    maxwidth = 0;
    
    for (i = 0; i < 3; i++)
    {
	string[i] = GetStr (glob->catalog, colstr[i]);
	glob->key[i] = KeyFromStr (string[i], '_');
	val = StrWidth_noloc (&glob->itxt, string[i]);
	if (val > maxwidth) maxwidth = val;
    }
    
    levelwidth = StrWidth_noloc (&glob->itxt, "000 ");
    maxwidth += levelwidth;


    for (i = 0; i < 3; i++)
    {

	val = leftoff + wheeloff + 2 + maxwidth + 8;
	InitNewGadget (&ng, val, top, winwidth - val - rightoff,
				glob->fontheight + 6, NULL, RED_ID + i);

	glob->colgad[i] = gad = myCreateGadget (SLIDER_KIND, gad, &ng,
		GTSL_LevelFormat, (IPTR) "%3ld",
		GTSL_LevelPlace, PLACETEXT_LEFT,
		GTSL_MaxLevelLen, 3, GA_RelVerify, TRUE, GA_Immediate, TRUE,
		GTSL_Level, glob->cols[i],
		GTSL_Max, glob->maxcolval[i],
		((GfxBase->LibNode.lib_Version >= 40) ? GTSL_Justification : TAG_IGNORE), GTJ_RIGHT,
		GTSL_MaxPixelLen, levelwidth,
		TAG_END);

	img = my_CreateGadgetLabelImage (img, &ng, string[i],
			leftoff + wheeloff + 2, top + 2, HIGHLIGHTTEXTPEN);
	top += glob->fontheight + 6 + spacing / 2;

    }

    top += spacing / 2;
    ng.ng_Flags &= ~NG_HIGHLABEL;


#ifdef COLORWHEEL
    if( glob->dowheel )
    {
	int wheeltop = top - ( wheelheight + glob->fontheight + 3 + spacing * 2 );

	if( wheeltop < wheeltopoff )
	{
	    top += wheeltopoff - wheeltop;
	    wheeltop = wheeltopoff;
	}

#ifdef GRADIENT

	if( glob->fancywheel )
	{

	    glob->numgradpens = LOWGRADPENS;

	    if( glob->scr->ViewPort.ColorMap
		&& glob->scr->ViewPort.ColorMap->PalExtra
		&& ( glob->scr->ViewPort.ColorMap->PalExtra->pe_NFree > FREEFORHIGH ) )
	    {
		glob->numgradpens = HIGHGRADPENS;
	    }

	    /* get the RGB components of active color */
	    glob->wheel_rgb.cw_Red   = glob->cols[ RED_ID   ];
	    glob->wheel_rgb.cw_Green = glob->cols[ GREEN_ID ];
	    glob->wheel_rgb.cw_Blue  = glob->cols[ BLUE_ID  ];

	    /* now convert the RGB values to HSB, and max out B component */
	    ConvertRGBToHSB( &glob->wheel_rgb, &glob->wheel_hsb );
	    glob->wheel_hsb.cw_Brightness = 0xffffffff;


	    /* Set up colors for gradient slider */
	    for( i = 0; i < glob->numgradpens; i++ )
	    {
		glob->wheel_hsb.cw_Brightness = 0xffffffff
			- ( ( 0xffffffff / ( glob->numgradpens - 1 ) ) * ( ULONG ) i );
		ConvertHSBToRGB( &glob->wheel_hsb, &glob->wheel_rgb );
		glob->wheel_pens[ i ] = ObtainPen( glob->scr->ViewPort.ColorMap,
			-1, glob->wheel_rgb.cw_Red, glob->wheel_rgb.cw_Green,
			glob->wheel_rgb.cw_Blue, PEN_EXCLUSIVE );

		if( glob->wheel_pens[ i ] == (UWORD)~0 )
		{
		    break;
		}
	    }


	    glob->wheel_pens[ i ] = ( UWORD ) ~0;

	    if( i != glob->numgradpens )
	    {
		for( i = 0; glob->wheel_pens[ i ] != (UWORD)~0; i++ )
		{
		    ReleasePen( glob->scr->ViewPort.ColorMap, glob->wheel_pens[ i ] );
		}

		glob->numgradpens = 0;
	    }

	} /* if( glob->fancywheel ) */
#endif

    	{
	    struct TagItem slider_tags[] =
	    {
		{GA_ID	    	    , SLIDER_ID     	    	    	},
		{GA_Top     	    , wheeltop + wheelheight + spacing	},
		{GA_Left    	    , leftoff	    	    	    	},
		{GA_Width   	    , wheelwidth    	    	    	},
		{GA_Height  	    , glob->fontheight + 3  	    	},
#ifdef GRADIENT
		{glob->numgradpens ?
		 GRAD_PenArray :
		 TAG_IGNORE 	    , (IPTR)glob->wheel_pens	     	},
#endif
		{GRAD_KnobPixels    , 8     	    	    	    	},
		{PGA_Freedom	    , LORIENT_HORIZ 	    	    	},
		{ICA_TARGET 	    , ICTARGET_IDCMP	    	    	},
		{TAG_END    	    	    	    	    	    	}
	    };
	    	
	    glob->wheel_slider = (struct Gadget *)NewObjectA(NULL, "gradientslider.gadget", slider_tags);
	
	}
	
	if( glob->wheel_slider )
	{
    	    struct TagItem wheel_tags[] =
	    {
		 {GA_Top    	    	, wheeltop  	    	    },
		 {GA_Left   	    	, leftoff   	    	    },
		 {GA_Width  	    	, wheelwidth	    	    },
		 {GA_Height 	    	, wheelheight	    	    },
		 {GA_ID     	    	, WHEEL_ID  	    	    },
		 {WHEEL_Screen	    	, (IPTR)glob->scr     	    },
		 {glob->fancywheel ?
		  TAG_IGNORE :
		  WHEEL_MaxPens     	, 0 	    	    	    },
		 {WHEEL_GradientSlider	, (IPTR)glob->wheel_slider  },
#ifdef __AROS__
/* Need this, because without BevelBox AROS colorwheel gadget renders through mask
which because of bugs in gfx library functions (!?) does not work yet and instead
causes mem trashes/crashes/etc (in AmigaOS the AROS colorwheel gadget works fine
even without BevelBox, that is: with mask rendering) */
		 {WHEEL_BevelBox    	, TRUE	    	    	    },
#endif
		 {GA_Previous	    	, (IPTR)glob->wheel_slider  },
		 {ICA_TARGET	    	, ICTARGET_IDCMP    	    },
		 {TAG_END   	    	    	    	    	    }
	    };
	    
	    glob->wheel = (struct Gadget *)NewObjectA(NULL, "colorwheel.gadget", wheel_tags);

	}
	    
    } /* if( glob->dowheel )*/
#endif


    for (i = 3; i < 6; i++)
    {
	InitNewGadget (&ng, gadpos[i], top, gadlen[i],
			buttonheight, string[i], COPY_ID + i);
			
	gad = my_CreateButtonGadget (gad, '_', &ng);
    }
    
    top += buttonheight + spacing;

    winheight = top + glob->scr->WBorBottom;
    glob->newpalwin.Height = winheight;
    glob->newpalwin.Width = winwidth;
    glob->newpalwin.IDCMPFlags = glob->shareidcmp ? 0 :
	    SLIDERIDCMP|PALETTEIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_RAWKEY
	    |IDCMP_REFRESHWINDOW|IDCMP_IDCMPUPDATE;
    glob->newpalwin.Flags = WFLG_DEPTHGADGET|WFLG_DRAGBAR|WFLG_ACTIVATE
				    |WFLG_SIMPLE_REFRESH|WFLG_RMBTRAP|WFLG_CLOSEGADGET;
    glob->newpalwin.DetailPen = glob->drinfo->dri_Pens[BACKGROUNDPEN];
    glob->newpalwin.BlockPen = glob->drinfo->dri_Pens[SHADOWPEN];
    glob->newpalwin.Title = title;
    glob->newpalwin.LeftEdge = glob->leftedge;
    glob->newpalwin.TopEdge = glob->topedge;

    reqpos = CheckReqPos (glob->reqpos, RTPREF_PALETTEREQ, &glob->newpalwin);

    if (reqpos == REQPOS_POINTER)
    {
	glob->newpalwin.LeftEdge = -winwidth / 2;
	glob->newpalwin.TopEdge = -winheight / 2;
    }
    
    rtSetReqPosition (reqpos, &glob->newpalwin, glob->scr, glob->prwin);

    ng.ng_LeftEdge = ng.ng_TopEdge = ng.ng_Width = ng.ng_Height = 0;
    ng.ng_GadgetText = NULL;
    gad = myCreateGadget (GENERIC_KIND, gad, &ng, TAG_END);

    if( gad )
    {
	gad->GadgetType |= GTYP_BOOLGADGET;
	gad->Flags |= GFLG_GADGIMAGE|GFLG_GADGHNONE;
	gad->GadgetRender = ( APTR ) glob->labelimages.NextImage;
    }


    glob->zoom[ 2 ] = glob->newpalwin.Width;
    glob->zoom[ 3 ] = glob->scr->WBorTop + glob->scr->Font->ta_YSize + 1;

    if( ( glob->dowheel && !glob->wheel ) || !img || !gad ||
	  !(glob->palwin = OpenWindowBF( &glob->newpalwin,
	  &glob->backfillhook, glob->drinfo->dri_Pens, NULL, glob->zoom, FALSE ) ) )
    {
	my_FreeGadgets( glob->buttoninfo.glist );
	glob->buttoninfo.glist = NULL;
	my_FreeLabelImages( &glob->labelimages );
	glob->labelimages.NextImage = NULL;
	DisposeObject( glob->wheel );
	glob->wheel = NULL;
	DisposeObject( glob->wheel_slider );
	glob->wheel_slider = NULL;
	return( 0 );
    }

    glob->buttoninfo.win = glob->palwin;

    if( glob->shareidcmp )
    {
	glob->palwin->UserPort = glob->prwin->UserPort;
	ModifyIDCMP( glob->palwin,
		     SLIDERIDCMP | PALETTEIDCMP | BUTTONIDCMP | IDCMP_CLOSEWINDOW |
		     IDCMP_RAWKEY | IDCMP_REFRESHWINDOW | IDCMP_IDCMPUPDATE );
    }

#ifdef COLORWHEEL
    if( glob->wheel )
    {
	AddGList( glob->palwin, glob->wheel_slider, -1, -1, NULL );
	RefreshGList( glob->wheel_slider, glob->palwin, NULL, -1 );
    }
#endif

    AddGList( glob->palwin, glob->buttoninfo.glist, -1, -1, NULL );
    RefreshGList( glob->buttoninfo.glist, glob->palwin, NULL, -1 );
    GT_RefreshWindow( glob->palwin, NULL );
    UpdateWheel( glob, glob->cols );

    return( 1 );
}

/****************************************************************************************/
