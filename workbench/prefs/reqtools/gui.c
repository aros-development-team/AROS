#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <libraries/reqtools.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <string.h>
#include <math.h>

#ifdef __AROS__
#include <proto/alib.h>
#include <aros/asmcall.h>

extern struct ReqToolsBase *ReqToolsBase;
#endif

#include "misc.h"
#include "gui.h"

#include <proto/reqtools.h>


#ifndef max
#define max(a,b) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#endif

// static struct GfxBase *GfxBase;
static UWORD		*Pens;


struct Gadget *glist, *xoffgad, *yoffgad, *mixdirsgad, *dirsfirstgad, *mmbgad;
struct Gadget *sizegad, *immsortgad, *popgad, *mingad, *maxgad, *defposgad;
struct Gadget *testgad, *noledgad, *deffontgad, *fkeysgad, *dowheelgad;


UBYTE *DefaultPosLabels[] =
{
    MSG_POINTER, MSG_CENTERWIN, MSG_CENTERSCR, MSG_TOPLEFTWIN, MSG_TOPLEFTSCR,
    NULL
};

UBYTE *TypeLabels[] =
{
    MSG_FILEREQ, MSG_FONTREQ, MSG_PALETTEREQ, MSG_SCRMODEREQ, MSG_VOLUMEREQ, MSG_OTHERREQ,
    NULL
};

STRPTR WheelLabels[] =
{
    MSG_WHEEL_NONE, MSG_WHEEL_SIMPLE, MSG_WHEEL_FANCY,
    NULL
};


struct TextAttr topaz8 = { (STRPTR)"topaz.font", 8, 0x00, 0x01 };

struct IntuiText GeneralText, FileReqText, DefSizeText, NrOfEntriesText;

LONG BevelLeft, BevelTop, BevelHeight, BevelWidth;

#ifdef __AROS__
AROS_UFH3(void, IntuiMsgFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, req, A2),
    AROS_UFHA(struct IntuiMessage *, imsg, A1))
{
    AROS_USERFUNC_INIT
#else
void __asm __saveds IntuiMsgFunc( register __a0 struct Hook *hook,
				  register __a2 APTR req,
				  register __a1 struct IntuiMessage *imsg)
{
#endif
    if (imsg->Class == IDCMP_REFRESHWINDOW)
    {
	GT_BeginRefresh( WindowPtr );
	RenderPrefsWindow();
	GT_EndRefresh ( WindowPtr, TRUE);
    }
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif    
}


VOID
GadgetOff( struct Gadget *gad )
{
    if (gad->Flags & GFLG_DISABLED) return;
    GT_SetGadgetAttrs (gad, WindowPtr, NULL, GA_Disabled, TRUE, TAG_END);
}

VOID
GadgetOn( struct Gadget *gad )
{
    if (!(gad->Flags & GFLG_DISABLED)) return;
    GT_SetGadgetAttrs (gad, WindowPtr, NULL, GA_Disabled, FALSE, TAG_END);
}


VOID
SetCheckState( struct Gadget *gad, BOOL state )
{
    GT_SetGadgetAttrs( gad, WindowPtr, NULL, GTCB_Checked, state,
    					     TAG_END );
}


VOID
SetIntGad( struct Gadget *gad, LONG val )
{
    GT_SetGadgetAttrs( gad, WindowPtr, NULL, GA_Disabled, FALSE	,
	    				     GTIN_Number, val	,
    					     TAG_END );
}


LONG
IntGadValue( struct Gadget *gad )
{
    return( ( ( struct StringInfo * )( gad->SpecialInfo ) )->LongInt );
}


VOID
RenderPrefsWindow( VOID )
{
    struct RastPort	*rp = WindowPtr->RPort;

    PrintIText( rp, &GeneralText,     0, 0 );
    PrintIText( rp, &FileReqText,     0, 0 );
    PrintIText( rp, &DefSizeText,     0, 0 );
    PrintIText( rp, &NrOfEntriesText, 0, 0 );

    DrawBevelBox( rp, BevelLeft, BevelTop, BevelWidth, BevelHeight, GT_VisualInfo, (IPTR) VisualInfo	,
	    							    GTBB_Recessed, TRUE		,
    								    TAG_DONE );
}


static struct NewGadget NewGadget;
static struct IntuiText IntuiText;


STRPTR Column1Text[] =
{
    MSG_POPSCREEN, MSG_DEFAULTFONT, MSG_FKEYS,
    MSG_IMMEDIATESORT, MSG_DRAWERSFIRST, MSG_DRAWERSMIXED, MSG_LED, MSG_MMB_PARENT,
    NULL
};

STRPTR SaveUseCancel[] =
{
    MSG_SAVE, MSG_USE, MSG_CANCEL,
    NULL
};

STRPTR PositionOffset[] =
{
    MSG_POSITION, MSG_OFFSET,
    NULL
};


static VOID
InitNewGadget( WORD x, LONG y, WORD w, WORD h, STRPTR str, UWORD id)
{
    NewGadget.ng_LeftEdge   = x;
    NewGadget.ng_TopEdge    = y;
    NewGadget.ng_Width      = w;
    NewGadget.ng_Height     = h;
    NewGadget.ng_GadgetText = str;
    NewGadget.ng_GadgetID   = id;
}


LONG
ArrayWidth_Localize( STRPTR *array )
{
    LONG width, len;

    width = 0;

    while( *array )
    {
	IntuiText.IText = GetString( *array );
	len = IntuiTextLength( &IntuiText );

	if( len > width )
	{
	    width = len;
	}

	++array;
    }

    return( width );
}


LONG
ArrayWidth( STRPTR *array)
{
    LONG width, len;

    width = 0;

    while( *array )
    {
	IntuiText.IText = *array;
	len = IntuiTextLength( &IntuiText );

	if( len > width )
	{
	    width = len;
	}

	++array;
    }

    return( width );
}


static LONG
ObjectWidth( struct NewGadget *ng, LONG normalw, LONG normalh )
{
    if( ( GadToolsBase->lib_Version >= 39 ) && ( ng->ng_TextAttr->ta_YSize > normalh ) )
    {
	return( normalw + ( ng->ng_TextAttr->ta_YSize - normalh ) * 2 );
    }

    return( normalw );
}


static LONG
ObjectHeight( struct NewGadget *ng, int normalh )
{
    if( ( GadToolsBase->lib_Version >= 39 ) && ( ng->ng_TextAttr->ta_YSize > normalh ) )
    {
	return( ng->ng_TextAttr->ta_YSize );
    }

    return( normalh );
}


LONG
CheckBoxWidth( struct NewGadget *ng )
{
    return( ObjectWidth( ng, CHECKBOX_WIDTH, CHECKBOX_HEIGHT ) );
}


LONG
CheckBoxHeight( struct NewGadget *ng )
{
    return( ObjectHeight( ng, CHECKBOX_HEIGHT ) );
}


const static struct TextAttr Topaz80 =
{
    "topaz.font", 8, FS_NORMAL, FPF_ROMFONT | FPF_DESIGNED
};


long OpenPrefsWindow (void)
{
    struct Gadget 	*gad;
    struct TextAttr 	*font;
    struct ReqDefaults 	*filereqdefs = &RTPrefs.ReqDefaults[RTPREF_FILEREQ];
    ULONG 		flags = RTPrefs.Flags;
    char 		*str;
    int 		offy, col1, fontht, top, scrwidth, scrheight, spacing;
    int 		len, left, intgadwidth, col1ht, col2ht, proptop, len2;
    int 		width, winwidth, dis;
    int 		checkw, checkh, checkskip;

    /* First localize some stuff */
    LocalizeLabels( DefaultPosLabels );
    LocalizeLabels( TypeLabels );
    LocalizeLabels( WheelLabels );

    GeneralText.IText = GetString( MSG_GENERAL );
    FileReqText.IText = GetString( MSG_FILEREQUESTER );
    DefSizeText.IText = GetString( MSG_SIZE );
    NrOfEntriesText.IText = GetString( MSG_NR_OF_ENTRIES );

    /* Layout vars */
    spacing = rtGetVScreenSize( Screen, ( ULONG * ) &scrwidth, ( ULONG * ) &scrheight );
    if (UseScreenFont)
        font = Screen->Font;
    else
        font = &Topaz80;

retryopenwin:
    IntuiText.ITextFont = font;
    fontht = font->ta_YSize;
    offy = Screen->WBorTop + Screen->Font->ta_YSize + 1;

    NewGadget.ng_TextAttr = font;
    NewGadget.ng_VisualInfo = VisualInfo;

    checkw = CheckBoxWidth (&NewGadget);
    checkh = checkskip = CheckBoxHeight (&NewGadget);
    if (fontht > checkskip) checkskip = fontht;

    /* Calculate width of first column */
    col1 = ArrayWidth_Localize ( Column1Text ) + checkw + 8;
    top = ArrayWidth( WheelLabels ) + 36;
    col1 = max( col1, top );

    if (!(gad = CreateContext (&glist))) return (1);

    /* First column */

    top = offy + spacing + 3;

    GeneralText.FrontPen = Pens[HIGHLIGHTTEXTPEN];
    GeneralText.ITextFont = font;
    GeneralText.TopEdge = top;
    GeneralText.LeftEdge = 12 + ((col1 - IntuiTextLength (&GeneralText)) / 2);
    top += fontht + spacing / 2;

    InitNewGadget (12, top, checkw, checkh,
			    GetString (MSG_POPSCREEN), NOSCRTOFRONT_GADID);
    NewGadget.ng_Flags = PLACETEXT_RIGHT;
    popgad = gad = CreateGadget (CHECKBOX_KIND, gad, &NewGadget, GTCB_Scaled, TRUE,
				 				 GTCB_Checked, !(flags & RTPRF_NOSCRTOFRONT),
								 TAG_END);
    top += checkskip + spacing;

    NewGadget.ng_TopEdge = top;
    NewGadget.ng_GadgetText = GetString (MSG_DEFAULTFONT);
    NewGadget.ng_GadgetID = DEFAULTFONT_GADID;
    deffontgad = gad = CreateGadget (CHECKBOX_KIND, gad, &NewGadget, GTCB_Scaled, TRUE,
	    			     				     GTCB_Checked, flags & RTPRF_DEFAULTFONT,
								     TAG_END);

    top += checkskip + spacing;

    NewGadget.ng_TopEdge = top;
    NewGadget.ng_GadgetText = GetString( MSG_FKEYS );
    NewGadget.ng_GadgetID = FKEYS_GADID;
    fkeysgad = gad = CreateGadget (CHECKBOX_KIND, gad, &NewGadget, GTCB_Scaled, TRUE,
		      		   				   GTCB_Checked, flags & RTPRF_FKEYS,
								   TAG_END);

    top += checkskip + spacing;

    NewGadget.ng_TopEdge = top;
    NewGadget.ng_Width = ArrayWidth( WheelLabels ) + 36;
    NewGadget.ng_Height = fontht + 6;
    NewGadget.ng_GadgetText = NULL;
    NewGadget.ng_GadgetID = DOWHEEL_GADID;
    dowheelgad = gad = CreateGadget (CYCLE_KIND, gad, &NewGadget, GTCY_Labels,	(IPTR) WheelLabels,
	    							  GTCY_Active,	WheelType,
    								  TAG_END);

    top += checkskip + 6 + spacing;

    top += spacing;
    FileReqText.FrontPen = Pens[HIGHLIGHTTEXTPEN];
    FileReqText.ITextFont = font;
    FileReqText.TopEdge = top;
    FileReqText.LeftEdge	= 12 + ((col1 - IntuiTextLength (&FileReqText)) / 2);
    top += fontht + spacing / 2;

    NewGadget.ng_TopEdge = top;
    NewGadget.ng_Width = checkw;
    NewGadget.ng_Height = checkh;
    NewGadget.ng_GadgetText = (UBYTE *)GetString (MSG_IMMEDIATESORT);
    NewGadget.ng_GadgetID = IMMSORT_GADID;
    immsortgad = gad = CreateGadget (CHECKBOX_KIND, gad, &NewGadget, GTCB_Scaled, TRUE,
				       				     GTCB_Checked, flags & RTPRF_IMMSORT,
								     TAG_END);
    top += checkskip + spacing;

    NewGadget.ng_TopEdge = top;
    NewGadget.ng_GadgetText = GetString (MSG_DRAWERSFIRST);
    NewGadget.ng_GadgetID = DIRSFIRST_GADID;
    dirsfirstgad = gad = CreateGadget (CHECKBOX_KIND, gad, &NewGadget, GTCB_Scaled, TRUE,
					      			       GTCB_Checked, flags & RTPRF_DIRSFIRST,
					      			       TAG_END);
    top += checkskip + spacing;

    NewGadget.ng_TopEdge = top;
    NewGadget.ng_GadgetText = GetString (MSG_DRAWERSMIXED);
    NewGadget.ng_GadgetID = DIRSMIXED_GADID;
    mixdirsgad = gad = CreateGadget (CHECKBOX_KIND, gad, &NewGadget, GTCB_Scaled, TRUE,
					    			     GTCB_Checked, flags & RTPRF_DIRSMIXED,
					    			     TAG_END);
    top += checkskip + spacing;

    NewGadget.ng_TopEdge = top;
    NewGadget.ng_GadgetText = GetString (MSG_LED);
    NewGadget.ng_GadgetID = NOLED_GADID;
    noledgad = gad = CreateGadget (CHECKBOX_KIND, gad, &NewGadget, GTCB_Scaled, TRUE,
						      		   GTCB_Checked, !(flags & RTPRF_NOLED),
						      		   TAG_END);
    top += checkskip + spacing;

    NewGadget.ng_TopEdge = top;
    NewGadget.ng_GadgetText = GetString (MSG_MMB_PARENT);
    NewGadget.ng_GadgetID = MMB_GADID;
    mmbgad = gad = CreateGadget (CHECKBOX_KIND, gad, &NewGadget, GTCB_Scaled, TRUE,
						      		 GTCB_Checked, flags & RTPRF_MMBPARENT,
						      		 TAG_END);
    top += checkskip + spacing;

    col1ht = top;

    /* Second column */

    top = offy + spacing;
    left = col1 + 24;
    BevelLeft = left;

    str = GetString (MSG_REQUESTER);
    IntuiText.IText = str;
    len = IntuiTextLength (&IntuiText);
    InitNewGadget (left + len + 8, top, ArrayWidth (TypeLabels) + 36, fontht + 6, str, REQTYPE_GADID);
    NewGadget.ng_Flags = PLACETEXT_LEFT|NG_HIGHLABEL;
    gad = CreateGadget (CYCLE_KIND, gad, &NewGadget, GTCY_Labels, (IPTR) TypeLabels, TAG_DONE);
    top += fontht + 6 + spacing;
    winwidth = NewGadget.ng_LeftEdge + NewGadget.ng_Width;

    left += 10;

    BevelTop = top;
    top += 1 + spacing;

    DefSizeText.FrontPen = Pens[HIGHLIGHTTEXTPEN];
    DefSizeText.ITextFont = font;
    DefSizeText.TopEdge = top;
    DefSizeText.LeftEdge = left;
    top += fontht + spacing / 2;
    width = IntuiTextLength (&DefSizeText) + 20;
    if (width > winwidth) winwidth = width;

    proptop = top;
    top += fontht + 6 + spacing;

    IntuiText.IText = "000";
    intgadwidth = IntuiTextLength (&IntuiText) + 12;

    NrOfEntriesText.FrontPen = Pens[HIGHLIGHTTEXTPEN];
    NrOfEntriesText.ITextFont = font;
    NrOfEntriesText.TopEdge = top;
    NrOfEntriesText.LeftEdge = left;
    top += fontht + spacing / 2;
    width = IntuiTextLength (&NrOfEntriesText) + 20;
    if (width > winwidth) winwidth = width;

    str = GetString (MSG_MIN);
    IntuiText.IText = str;
    len = IntuiTextLength (&IntuiText);
    InitNewGadget (left + len + 8, top, intgadwidth, fontht + 6, str, MINENTRIES_GADID);
#ifdef __AROS__	
    NewGadget.ng_Flags = NG_HIGHLABEL | PLACETEXT_LEFT;
#else
    NewGadget.ng_Flags = NG_HIGHLABEL;
#endif
    mingad = gad = CreateGadget (INTEGER_KIND, gad, &NewGadget, GTIN_Number, filereqdefs->MinEntries,
						      		GTIN_MaxChars, 2,
								TAG_END);

    str = GetString (MSG_MAX);
    IntuiText.IText = str;
    len = IntuiTextLength (&IntuiText);
    NewGadget.ng_LeftEdge += intgadwidth + 16 + len + 8;
    NewGadget.ng_GadgetText = str;
    NewGadget.ng_GadgetID = MAXENTRIES_GADID;
    maxgad = gad = CreateGadget (INTEGER_KIND, gad, &NewGadget, GTIN_Number, filereqdefs->MaxEntries,
					      			GTIN_MaxChars, 2,
								TAG_END);

    top += fontht + 6 + spacing;
    width = NewGadget.ng_LeftEdge + NewGadget.ng_Width + 10;
    if (width > winwidth) winwidth = width;

    len = ArrayWidth_Localize (PositionOffset) + 8;

    InitNewGadget (left + len, top, ArrayWidth (DefaultPosLabels) + 36,
					    fontht + 6, GetString (MSG_POSITION), POSITION_GADID);
    defposgad = gad = CreateGadget (CYCLE_KIND, gad, &NewGadget, GTCY_Labels, (IPTR) DefaultPosLabels,
				      				 GTCY_Active, filereqdefs->ReqPos,
				      				 TAG_END);
    top += fontht + 6 + spacing;
    width = NewGadget.ng_LeftEdge + NewGadget.ng_Width + 10;
    if (width > winwidth) winwidth = width;

    IntuiText.IText = "0000";
    intgadwidth = IntuiTextLength (&IntuiText) + 12;

    dis = (filereqdefs->ReqPos <= REQPOS_CENTERSCR);

    InitNewGadget (left + len, top, intgadwidth, fontht + 6, GetString (MSG_OFFSET), OFFSETX_GADID);
    xoffgad = gad = CreateGadget (INTEGER_KIND, gad, &NewGadget, GA_Disabled, dis,
						    		 GTIN_Number, filereqdefs->LeftOffset,
						    		 GTIN_MaxChars, 3,
								 TAG_END);
    NewGadget.ng_LeftEdge += intgadwidth;
    NewGadget.ng_GadgetText = NULL;
    NewGadget.ng_GadgetID = OFFSETY_GADID;
    yoffgad = gad = CreateGadget (INTEGER_KIND, gad, &NewGadget, GA_Disabled, dis,
						    		 GTIN_Number, filereqdefs->TopOffset,
						    		 GTIN_MaxChars, 3,
						    		 TAG_END);
    top += fontht + 6 + spacing;

#if 0
    str = GetString (MSG_TEST);
    IntuiText.IText = str;
    len = IntuiTextLength (&IntuiText);
    NewGadget.ng_LeftEdge += NewGadget.ng_Width + 32;
    NewGadget.ng_Width = len + 16;
    NewGadget.ng_GadgetText = str;
    NewGadget.ng_Flags = 0;
    NewGadget.ng_GadgetID = TEST_GADID;
    testgad = gad = CreateGadget (BUTTON_KIND, gad, &NewGadget, TAG_END);
    ng.ng_Flags = NG_HIGHLABEL;
#endif

    width = NewGadget.ng_LeftEdge + NewGadget.ng_Width + 10;
    if (width > winwidth) winwidth = width;

    BevelWidth = winwidth - BevelLeft;

    /* Prop gadget + Set gadget */

    IntuiText.IText = "100%";
    len = IntuiTextLength (&IntuiText) + 8;
#if 0
    str = GetString (MSG_SET);
    IntuiText.IText = str;
    len2 = IntuiTextLength (&IntuiText) + 16;
#else
len2 = 0;
#endif
    InitNewGadget (left + len, proptop, winwidth - left - 10 - len - len2,
					    fontht + 6, NULL, DEFSIZE_GADID);
    sizegad = gad = CreateGadget (SLIDER_KIND, gad, &NewGadget, GTSL_Min, 25, GTSL_Max, 100,
				    				GTSL_Level, filereqdefs->Size,
			       					GTSL_MaxLevelLen, 6, GTSL_LevelFormat, (IPTR) "%3ld%%",
				    				PGA_Freedom, LORIENT_HORIZ, GA_RelVerify, TRUE,
				    				TAG_DONE);
#if 0
    InitNewGadget (NewGadget.ng_LeftEdge + NewGadget.ng_Width, proptop, len2, fontht + 6, str, SETSIZE_GADID);
    NewGadget.ng_Flags = 0;
    setgad = gad = CreateGadget (BUTTON_KIND, gad, &NewGadget, TAG_END);
#endif

    winwidth += 12;

    BevelHeight = top - BevelTop + 1;
    top += 1 + spacing;

    col2ht = top;

    /* Save, Use, Cancel */

    top = max (col1ht, col2ht) + spacing;
    len = ArrayWidth_Localize (SaveUseCancel) + 16;
    if (len < 87) len = 87;
    InitNewGadget (12, top, len, fontht + 6, GetString (MSG_SAVE), SAVE_GADID);
    gad = CreateGadget (BUTTON_KIND, gad, &NewGadget, TAG_END);

    NewGadget.ng_LeftEdge = (winwidth - len) / 2;
    NewGadget.ng_GadgetText = GetString (MSG_USE);
    NewGadget.ng_GadgetID = USE_GADID;
    gad = CreateGadget (BUTTON_KIND, gad, &NewGadget, TAG_END);

    NewGadget.ng_LeftEdge = winwidth - 12 - len;
    NewGadget.ng_GadgetText = GetString (MSG_CANCEL);
    NewGadget.ng_GadgetID = CANCEL_GADID;
    gad = CreateGadget (BUTTON_KIND, gad, &NewGadget, TAG_END);

    top += fontht + 6 + spacing;

    if (!gad) return (2);

    if (winwidth > scrwidth || (top + Screen->WBorBottom) > scrheight)
    {
	ClosePrefsWindow();
	if (font->ta_YSize > 8)
	{
	    font = &topaz8;
	    goto retryopenwin;
	}
	return (3);
    }

    if (!( WindowPtr = OpenWindowTags (NULL,
		    WA_Left, 1,
		    WA_Top, Screen->BarHeight + 1,
		    WA_Width, winwidth,
		    WA_Height, top + Screen->WBorBottom,
		    WA_DepthGadget, FALSE,
		    WA_Zoom, (IPTR) Zoom,
		    WA_IDCMP, CHECKBOXIDCMP|BUTTONIDCMP|SLIDERIDCMP|INTEGERIDCMP|CYCLEIDCMP|IDCMP_CLOSEWINDOW
					    |IDCMP_VANILLAKEY|IDCMP_MOUSEMOVE|IDCMP_REFRESHWINDOW|IDCMP_MENUPICK,
		    WA_Flags, WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH|WFLG_CLOSEGADGET,
		    WA_Gadgets, (IPTR) glist,
		    WA_Title, (IPTR) GetString (MSG_WINDOW_TITLE),
		    WA_PubScreen, (IPTR) Screen,
		    WA_NewLookMenus, TRUE, TAG_DONE)))
    {
	ClosePrefsWindow();
	if (font->ta_YSize > 8)
	{
	    font = &topaz8;
	    goto retryopenwin;
	}
	return (4);
    }

    SetMenuStrip ( WindowPtr, Menus);
    GT_RefreshWindow ( WindowPtr, NULL);
    RenderPrefsWindow();
    
    return (0);
}

void UpdatePrefsWindow (int onlydefs)
{
   struct ReqDefaults 	*reqdefs = &RTPrefs.ReqDefaults[ CurrentReq ];
   int 			dis;

   if( CurrentReq == RTPREF_OTHERREQ || CurrentReq == RTPREF_PALETTEREQ)
   {
       GT_SetGadgetAttrs (sizegad, WindowPtr, NULL, GA_Disabled, TRUE, TAG_END);
//		GT_SetGadgetAttrs (setgad, WindowPtr, NULL, GA_Disabled, TRUE, TAG_END);
       GT_SetGadgetAttrs (mingad, WindowPtr, NULL, GA_Disabled, TRUE, TAG_END);
       GT_SetGadgetAttrs (maxgad, WindowPtr, NULL, GA_Disabled, TRUE, TAG_END);
   }
   else
   {
       GT_SetGadgetAttrs (sizegad, WindowPtr, NULL, GA_Disabled, FALSE,
						    GTSL_Level, reqdefs->Size,
						    TAG_END);
//		GT_SetGadgetAttrs (setgad, WindowPtr, NULL, GA_Disabled, FALSE,
//							    TAG_END);
       GT_SetGadgetAttrs (mingad, WindowPtr, NULL, GA_Disabled, FALSE,
						   GTIN_Number, reqdefs->MinEntries,
						   TAG_END);
       GT_SetGadgetAttrs (maxgad, WindowPtr, NULL, GA_Disabled, FALSE,
						   GTIN_Number, reqdefs->MaxEntries,
						   TAG_END);
   }
   GT_SetGadgetAttrs (defposgad, WindowPtr, NULL, GTCY_Active, reqdefs->ReqPos,
   						 TAG_END);
   dis = (reqdefs->ReqPos <= REQPOS_CENTERSCR);
   GT_SetGadgetAttrs (xoffgad, WindowPtr, NULL, GA_Disabled, dis,
						GTIN_Number, reqdefs->LeftOffset,
						TAG_END);
   GT_SetGadgetAttrs (yoffgad, WindowPtr, NULL, GA_Disabled, dis,
						GTIN_Number, reqdefs->TopOffset,
						TAG_END);

   if (!onlydefs)
   {
       GT_SetGadgetAttrs (popgad, WindowPtr, NULL, GTCB_Checked, !(RTPrefs.Flags & RTPRF_NOSCRTOFRONT),
						   TAG_END);
       GT_SetGadgetAttrs (deffontgad, WindowPtr, NULL, GTCB_Checked, RTPrefs.Flags & RTPRF_DEFAULTFONT,
						      TAG_END);
       GT_SetGadgetAttrs (dowheelgad, WindowPtr, NULL, GTCB_Checked, RTPrefs.Flags & RTPRF_DOWHEEL,
						       TAG_END);
       GT_SetGadgetAttrs (fkeysgad, WindowPtr, NULL, GTCB_Checked, RTPrefs.Flags & RTPRF_FKEYS,
						     TAG_END);
       GT_SetGadgetAttrs (immsortgad, WindowPtr, NULL, GTCB_Checked, RTPrefs.Flags & RTPRF_IMMSORT,
						       TAG_END);
       GT_SetGadgetAttrs (dirsfirstgad, WindowPtr, NULL, GTCB_Checked, RTPrefs.Flags & RTPRF_DIRSFIRST,
							 TAG_END);
       GT_SetGadgetAttrs (mixdirsgad, WindowPtr, NULL, GTCB_Checked, RTPrefs.Flags & RTPRF_DIRSMIXED,
						       TAG_END);
       GT_SetGadgetAttrs (noledgad, WindowPtr, NULL, GTCB_Checked, !( RTPrefs.Flags & RTPRF_NOLED ),
						     TAG_END);
#if 1
       /* AROS/AmigaOS bug fix: this "!" seems to be wrong */
       GT_SetGadgetAttrs (mmbgad, WindowPtr, NULL, GTCB_Checked, ( RTPrefs.Flags & RTPRF_MMBPARENT ),
						   TAG_END);
#else
       GT_SetGadgetAttrs (mmbgad, WindowPtr, NULL, GTCB_Checked, !( RTPrefs.Flags & RTPRF_MMBPARENT ),
						   TAG_END);
#endif
    }
    
}

void ClosePrefsWindow (void)
{
    if (WindowPtr)
    {
	CloseWindow (WindowPtr);
	WindowPtr = NULL;
    }
    if (glist)
    {
	FreeGadgets (glist);
	glist = NULL;
    }
}




BOOL
OpenGUI( VOID )
{
//  GfxBase = (struct GfxBase *)OpenLibrary ("graphics.library", 37);

    Pens = DrawInfo->dri_Pens;

    if( !OpenPrefsWindow() )
    {
	return( TRUE );
    }

    ClosePrefsWindow();
    
    return( FALSE );
}


VOID
CloseGUI( VOID )
{
    ClosePrefsWindow();
//  CloseLibrary( GfxBase );
}


VOID
LoopGUI( VOID )
{
    BOOL	run = TRUE;
    ULONG	signals;

    while( run )
    {
	struct IntuiMessage im, *imsg;

	signals = Wait(1L << WindowPtr->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);

	if( signals & (1L << WindowPtr->UserPort->mp_SigBit ) )
	{

	    while( ( imsg = ( struct IntuiMessage * ) GT_GetIMsg( WindowPtr->UserPort ) ) )
	    {
	    	struct Gadget	*gad;
	    	UWORD		code;

	    	im = *imsg;
	    	GT_ReplyIMsg( imsg );
	    	gad = ( struct Gadget * ) im.IAddress;

	    	switch( im.Class )
	    	{
			case IDCMP_REFRESHWINDOW:
		    	GT_BeginRefresh( WindowPtr );
		    	RenderPrefsWindow();
		    	GT_EndRefresh( WindowPtr, TRUE );
		    	break;

			case IDCMP_GADGETUP:
		        run = ProcessGadget( gad->GadgetID, im.Code );
		        break;

			case IDCMP_MENUPICK:
		        code = im.Code;

		        while( run && ( code != MENUNULL ) )
		        {
				run = ProcessMenuItem( code );
				code = ItemAddress( Menus, code )->NextSelect;
		        }

		        break;

			case IDCMP_VANILLAKEY:
			switch( imsg->Code )
			{
				case 27: /* User has pressed the escape key */
				run = FALSE;
				break;
			}
			break;

			case IDCMP_CLOSEWINDOW:
			run = FALSE;
			break;
	    	}
	    }
    	}
	if( signals & SIGBREAKF_CTRL_C )
	    run = FALSE;
    }
}
