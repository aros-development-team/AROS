/***************************************************************
**** JanoPrefs.c: Configuration window for JanoEditor       ****
**** Free software under GNU license, started on 11/11/2000 ****
**** © T.Pierron, C.Guillaume.                              ****
***************************************************************/

#define	ASL_V38_NAMES_ONLY
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <libraries/dos.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Jed.h"
#include "JanoPrefs.h"
#include "IPC_Prefs.h"
#include "Sample.h"

#define  CATCOMP_NUMBERS
#include "strings.h"
#define	LocaleInfo				LocaleInfoTmp		/* Avoid redefinition :-( */
#include "../../tools/Edit/strings.h"

/* Shared libraires we'll need to open */
struct IntuitionBase	*IntuitionBase = NULL;
struct GfxBase			*GfxBase       = NULL;
struct Library			*GadToolsBase  = NULL;
struct Library			*AslBase       = NULL;
struct Library			*LocaleBase    = NULL;
struct Library			*DiskfontBase  = NULL;
struct Library       *LayersBase    = NULL;

struct IntuiMessage *msg, msgbuf;
struct NewWindow NewPrefWin =
{
	0,0,0,0, 1,1,					/* LE,TE,Wid,Hei, Pens */
	IDCMP_MENUPICK   | IDCMP_CLOSEWINDOW | IDCMP_GADGETUP   | IDCMP_NEWSIZE  | IDCMP_VANILLAKEY,
	WFLG_CLOSEGADGET | WFLG_NEWLOOKMENUS | WFLG_DEPTHGADGET | WFLG_ACTIVATE  | WFLG_SMART_REFRESH |
	WFLG_DRAGBAR,
	NULL,								/* FirstGadget */
	NULL,								/* CheckMark */
	NULL,								/* Title */
	NULL,								/* Screen */
	NULL,								/* BitMap */
	100, 50, 0xffff, 0xffff,	/* Min & Max size */
	CUSTOMSCREEN
};

/** Standard GUI ressource **/
struct Screen    *Scr  = NULL;		/* Screen where window opens */
struct Window    *Wnd  = NULL;		/* Pointer to the opened window */
void             *Vi   = NULL;		/* Visual Info for gadget */
struct Menu      *Menu = NULL;		/* Menu of interface */
struct TextFont  *font;					/* Font used for gadgets */
struct RastPort  RPT;					/* Font measurement */

UBYTE *FSCycTxt[5],CB_state[CBS];

/* Tag list for various gadget */
struct TagItem IntTags[]      = { /*{GTIN_Number, 0},*/ {GTST_MaxChars,2}, {TAG_DONE}};
struct TagItem SepTags[]      = {{GTST_MaxChars, MAX_SPLIT}, {TAG_DONE}};
struct TagItem TextFontTags[] = {{GTCY_Labels, (IPTR)FTCycTxt},  {GTCY_Active,0}, {TAG_DONE}};
struct TagItem ScrFontTags[]  = {{GTCY_Labels, (IPTR)FSCycTxt},  {GTCY_Active,0}, {TAG_DONE}};
struct TagItem ScrMdTags[]    = {{GTCY_Labels, (IPTR)ScrCycTxt}, {GTCY_Active,0}, {TAG_DONE}};
struct TagItem ColorTags[]    = {{GTCY_Labels, (IPTR)ColCycTxt}, {GTCY_Active,0}, {TAG_DONE}};
struct TagItem MenuTags[]     = {{GTMN_FrontPen, 1L}, {TAG_DONE}};

struct TagItem *GadTags[]     = {IntTags, SepTags, TextFontTags, ScrFontTags, ScrMdTags};

/* Tag list for opening the main window */
WORD  PrefZoom[4];
struct TagItem PrefTags[] = {
	{WA_Zoom,(ULONG)PrefZoom},
	{WA_NewLookMenus,TRUE},
	{TAG_DONE}
};

struct NewGadget NG;							/* To create the gadgets */
struct Gadget *pgads=NULL;					/* Gadtool's gadgets */
struct Gadget *gads[CBS+CGS];
extern PREFS prefs;							/* Preference buffer */
extern UBYTE Modif[];						/* Buffer for string's gadget */
ULONG  prefsigbit, msgport;				/* Signal bits for asynchronous messages collect */
ULONG  extended;								/* Extended cycle gadgets */
UBYTE  ConfigFile = TRUE;					/* TRUE is a config file is edited */
UBYTE  ColorIndex = 0;						/* Color index to change */
UBYTE  StrInfo[60];							/* Screen/font description */
WORD   wid[5];									/* Width of various strings */

/*** Display an error message in the window's title ***/
static UBYTE *Title = NULL;
static ULONG  err_time;
void ThrowError(struct Window *W, UBYTE *Msg)
{
	if( W ) {
		/* Save old title */
		if(Title == 0)
			Title = (W->Flags & WFLG_BACKDROP ? W->WScreen->Title : W->Title);
		/* If window is backdrop'ed, change screen's title instead of window */
		if(W->Flags & WFLG_BACKDROP) SetWindowTitles(W,(STRPTR)-1,Msg);
		else SetWindowTitles(W,Msg,(STRPTR)-1);

		DisplayBeep(W->WScreen);
		err_time = 0;
		/* To be sure that mesage will be disappear one day */
		ModifyIDCMP(W,W->IDCMPFlags | IDCMP_INTUITICKS);
	}	else puts(Msg);
}

/*** Show messages associated with IoErr() number ***/
void ThrowDOSError(struct Window *W, STRPTR Prefix, UBYTE err)
{
	static UBYTE Message[100];

	/* Get standard DOS error message */
	Fault(err, Prefix, Message, sizeof(Message));

	ThrowError(W, Message);
}

/*** Reset the old title ***/
void StopError(struct Window *W)
{
	if(W->Flags & WFLG_BACKDROP) SetWindowTitles(W,(STRPTR)-1,Title);
	else SetWindowTitles(W,Title,(STRPTR)-1);
	Title=0;
	/* INTUITICKS aren't required anymore */
	ModifyIDCMP(W,W->IDCMPFlags & ~IDCMP_INTUITICKS);
}

/*** Try to open configuration window, return 1 if all's ok ***/
BYTE setup_guipref( void )
{
	extern UBYTE Path[];
	WORD width,i;

	/** Look if window isn't already open, thus just activate it **/
	if( Wnd ) {
		ActivateWindow(Wnd);
		WindowToFront(Wnd);
		/* Uniconified it, if it is */
		if(Wnd->Height<=Wnd->BorderTop) ZipWindow(Wnd);
		ScreenToFront(Scr);
		return TRUE;
	}

	/** Visual info pointer **/
	if(Vi == NULL && !(Vi = (void *) GetVisualInfoA(Scr,NULL))) return 0;

	/** Init missing strings **/
	CopyMem(FTCycTxt,FSCycTxt,sizeof(FSCycTxt));
	CopyMem(FTCycTxt+1,ScrCycTxt+2,sizeof(UBYTE *)*2);
	FTCycTxt[0]=StrInfo;
	FSCycTxt[0]=StrInfo+20;
	ScrCycTxt[0]=StrInfo+40;
	/** Init temporary rastport, for font measurement **/
	InitRastPort(&RPT);
	SetFont(&RPT, font = Scr->RastPort.Font);

	/** Compute window width and height **/
	wid[0] = meas_table(MiscTxt);
	wid[1] = meas_table(ChkTxt);
	wid[2] = meas_table(OkCanSav)+20;
	wid[3] = meas_table(FSCycTxt);
	wid[4] = meas_table(ColCycTxt);
	width  = meas_table(ScrCycTxt);
	if(wid[3] < width) wid[3] = width;

	NewPrefWin.Width    = wid[0] + wid[3] + wid[1] + 130;
	NewPrefWin.Height   = Scr->BarHeight + font->tf_YSize * 8 + (54 + SAMPLE_HEI);
	NewPrefWin.LeftEdge = 50;
	NewPrefWin.TopEdge  = 50;
	NewPrefWin.Screen   = Scr;
	NewPrefWin.Title    = (ConfigFile ? Path : PrefMsg[0]);

	/** Does it fit in the screen ? **/
	fit_in_screen(&NewPrefWin, Scr);

	/** Compute the iconified window dimensions: **/
	PrefZoom[2] = TextLength(&RPT,NewPrefWin.Title,strlen(NewPrefWin.Title))+100;
	if(prefs.use_pub) PrefZoom[0] = Scr->Width-PrefZoom[2]-15, PrefZoom[1] = 0;
	else PrefZoom[0] = PrefZoom[1] = 50;
	PrefZoom[3] = Scr->WBorTop + Scr->Font->ta_YSize + 1;

	/** Setup inital tags: **/
	ScrFontTags[1].ti_Data = 0;
	ScrMdTags[1].ti_Data = 0;
	TextFontTags[1].ti_Data = 0;
	extended=0;
	
	if( prefs.use_txtfont )
		font_info(StrInfo, prefs.txtfont),
		TextFontTags[0].ti_Data = (IPTR) FTCycTxt, extended |= 1;
	else
		TextFontTags[0].ti_Data = (IPTR) (FTCycTxt+1);

	if( prefs.use_scrfont )
		font_info(StrInfo+20, prefs.scrfont),
		ScrFontTags[0].ti_Data = (IPTR) FSCycTxt, extended |= 2;
	else
		ScrFontTags[0].ti_Data = (IPTR) (FSCycTxt+1);

	if( prefs.use_pub==1 )
		scr_info(StrInfo+40,Scr->Width, Scr->Height, Scr->RastPort.BitMap->Depth),
		ScrMdTags[0].ti_Data = (IPTR) ScrCycTxt, extended |= 4;
	else {
		ScrMdTags[0].ti_Data = (IPTR) (ScrCycTxt+1);
		if(prefs.use_pub!=2) ScrMdTags[1].ti_Data = 1; /* Clone */
	}
	/* Checkbox states */
	CopyMem(&prefs.backdrop, CB_state, CBS);

	/* If it isn't the frontmost screen */
	if(Scr != IntuitionBase->ActiveScreen) ScreenToFront(Scr);

	/** Init Menus **/
	if(NULL == (Menu = (void *) CreateMenusA(newmenu, MenuTags)) ||
	   0 == LayoutMenus(Menu, Vi, GTMN_TextAttr, Scr->Font, TAG_DONE)) return FALSE;

	/** Try to open the window **/
	if(( Wnd = (struct Window *) OpenWindowTagList(&NewPrefWin,PrefTags) ))
	{
		struct Gadget **pg = gads, *gad;
		pgads = NULL;
		gad = (struct Gadget *) CreateContext(&pgads);
		NG.ng_TopEdge    = Wnd->BorderTop+5;
		NG.ng_Width      = wid[3]+40;
		NG.ng_LeftEdge   = wid[0]+20;
		NG.ng_Height     = font->tf_YSize+6;
		NG.ng_Flags      = PLACETEXT_LEFT | NG_HIGHLABEL;
		NG.ng_TextAttr   = Scr->Font;
		NG.ng_VisualInfo = Vi;

		/* Create left column of gadgets */
		for(i=0; i<CGS; i++, NG.ng_TopEdge+=NG.ng_Height, *pg++ = gad)
		{
		    	WORD kind;
			
			NG.ng_GadgetText = MiscTxt[i];
			NG.ng_GadgetID   = i;
			if (i == 0) 
			{
			    kind = INTEGER_KIND;
			}
			else if (i == 1)
			{
			    kind = STRING_KIND;
			}
			else
			{
			    kind = CYCLE_KIND;
			}
			
			gad = (void *) CreateGadgetA(kind, gad, &NG, GadTags[i]);
			if(i==0) AddNum (prefs.tabsize,  GetSI(gad)->Buffer);
			if(i==1) CopyMem(prefs.wordssep, GetSI(gad)->Buffer, MAX_SPLIT);
		}

		/* Create right column of gads */
		NG.ng_TopEdge = Wnd->BorderTop+5;
		NG.ng_LeftEdge += NG.ng_Width+20;
		NG.ng_Width = (NG.ng_Height -= 2);
		NG.ng_Flags = PLACETEXT_RIGHT;
		for(; i<CGS+CBS; i++, NG.ng_TopEdge+=NG.ng_Height, *pg++ = gad)
		{
			NG.ng_GadgetText = ChkTxt[i-CGS];
			NG.ng_GadgetID   = i;
			gad = (void *) CreateGadget(CHECKBOX_KIND, gad, &NG, GTCB_Checked, CB_state[i-CGS], TAG_DONE);
			if(i == CGS && prefs.use_pub == 0)
				gad->Flags |= GFLG_DISABLED;
		}

		/* Create bottom row of gadgets */
		NG.ng_TopEdge  = Wnd->Height-NG.ng_Height-5;
		NG.ng_LeftEdge = 10;
		NG.ng_Width    = wid[2];
		NG.ng_Flags    = PLACETEXT_IN;
		width = (Wnd->Width-20-wid[2]) >> 1;
		for(; i<UCS+3; i++, NG.ng_LeftEdge+=width)
		{
			NG.ng_GadgetText = OkCanSav[i-UCS];
			NG.ng_GadgetID   = i;
			gad = (void *) CreateGadgetA(BUTTON_KIND, gad, &NG, NULL);
		}

		/* Initiate sample editor to see colors adjustement */
		init_sample(Wnd, &prefs, NG.ng_TopEdge -= 5 + SAMPLE_HEI);

		/* Cycle color chooser */
		NG.ng_TopEdge   -= NG.ng_Height+3;
		NG.ng_Width      = wid[4]+40;
		NG.ng_GadgetText = NULL;
		NG.ng_GadgetID   = i;
		NG.ng_LeftEdge   = 10;
		gad = (void *) CreateGadgetA(CYCLE_KIND, gad, &NG, ColorTags);

		/* Palette chooser */
		NG.ng_Width      = Wnd->Width - 10 - (
		NG.ng_LeftEdge  += NG.ng_Width+10);
		NG.ng_GadgetID   = i+1;
		CreateGadget(PALETTE_KIND, gad, &NG,
						 GTPA_Depth, ((Scr->RastPort.BitMap->Depth > 5) ? 5 : Scr->RastPort.BitMap->Depth),
						 TAG_DONE);

		/* Add gadgets to window: */
		CopyMem(Wnd->RPort, &RPT, sizeof(RPT));
		SetFont(&RPT, prefs.scrfont);
		render_sample(Wnd, EDIT_ALL);
		AddGList( Wnd, pgads, -1, -1, NULL);
		RefreshGList(pgads, Wnd, NULL, -1);
		SetMenuStrip( Wnd, Menu );
		GT_RefreshWindow(Wnd, NULL);

		/* Add window's signal bit: */
		prefsigbit = 1 << Wnd->UserPort->mp_SigBit;
		return TRUE;
	}
	return FALSE;
}

/*** Close the preference window ***/
void close_prefwnd( BYTE cmd )
{
	if(Wnd != NULL)
	{
		if( Menu ) ClearMenuStrip(Wnd);
		CloseWindow(Wnd);
		prefsigbit=0; Wnd=NULL;
	}
	if(pgads) {
		/* Flush content of some gadget that may not be confirmed */
		prefs.tabsize = atoi(GetSI(gads[0])->Buffer);
		CopyMem(GetSI(gads[1])->Buffer, prefs.wordssep, MAX_SPLIT);

		FreeGadgets(pgads); pgads=NULL;
	}

	if(Menu) FreeMenus(Menu),Menu=NULL;

	/* Do we need to send modifications to JANO? */
	if( cmd && !ConfigFile ) send_jano(&prefs, cmd);
}

/*** Handle messages coming from gadgets ***/
BYTE handle_pref_gadget(struct Gadget *G)
{
	struct TextFont *newfont;
	static UBYTE useit[]={1,2,0,1};
	LONG  Code;

	switch( G->GadgetID )
	{
		case 0:	/* Check tabulation: */
			check_tab( G );
			break;
		case 2:	/* Changing default text font: */

			Code = msgbuf.Code; if(extended & 1) Code--;
			if(Code <= 0)
			{
				if( !(prefs.use_txtfont = (Code==-1)) )
					/* User want to use the default text font: */
					text_to_attr(prefs.txtfont=GfxBase->DefaultFont, &prefs.attrtxt);
				else
					if((newfont = get_old_font(FTCycTxt[0])))
						text_to_attr(prefs.txtfont=newfont, &prefs.attrtxt);
					else { ThrowError(Wnd, ErrMsg(ERR_LOADFONT)); break; }
				render_sample(Wnd, EDIT_AREA);
				break;
			} else
				/* User want to change the text font: */
				if((newfont = change_fonts(&prefs.attrtxt, Wnd, TRUE)))
				{
					if( prefs.txtfont ) CloseFont(prefs.txtfont);
					font_info(StrInfo, prefs.txtfont = newfont );
					TextFontTags[0].ti_Data = (IPTR) FTCycTxt; extended |= 1;
					prefs.use_txtfont = TRUE;
					TextFontTags[1].ti_Data = 0;
				} /* else we must reset the original cycle gadget entry */
			GT_SetGadgetAttrsA(G, Wnd, NULL, TextFontTags);
			render_sample(Wnd, EDIT_AREA);
			break;
		case 3:	/* Changing default screen font */

			Code = msgbuf.Code; if(extended & 2) Code--;
			if(Code <= 0)
			{
				if( !(prefs.use_scrfont = (Code==-1)) )
					/* User want to use default screen font */
					text_to_attr(prefs.scrfont=prefs.parent->RastPort.Font, &prefs.attrscr);
				else
					if((newfont = get_old_font(FSCycTxt[0])))
						text_to_attr(prefs.scrfont=newfont, &prefs.attrscr);
					else { ThrowError(Wnd, ErrMsg(ERR_LOADFONT)); break; }

				render_sample(Wnd, EDIT_ALL);
				break;
			} else
				/* User want to change the screen font */
				if((newfont = change_fonts(&prefs.attrscr, Wnd, FALSE)))
				{
					if( prefs.scrfont ) CloseFont(prefs.scrfont);
					font_info(StrInfo+20, prefs.scrfont = newfont );
					ScrFontTags[0].ti_Data = (IPTR) FSCycTxt; extended |= 2;
					prefs.use_scrfont = TRUE;
					ScrFontTags[1].ti_Data = 0;
				}
			GT_SetGadgetAttrsA(G, Wnd, NULL, ScrFontTags);
			render_sample(Wnd, EDIT_ALL);
			break;
		case 4:	/* Change screen mode/type */

			Code = msgbuf.Code; if(extended & 4) Code--;
			if(Code <= 1)
				prefs.use_pub = useit[Code+1];
			else
			{
				/* User want to use a pubscreen instead of workbench screen: */
				if((Code = change_screen_mode(wid, prefs.modeid)) != INVALID_ID)
				{
					prefs.depth  = wid[2];
					prefs.modeid = Code;
					ScrMdTags[0].ti_Data = (IPTR) ScrCycTxt;
					ScrMdTags[1].ti_Data = 0;
					scr_info(StrInfo+40, wid[0], wid[1], wid[2]);
					prefs.use_pub = TRUE; extended |= 4;
				} else ScrMdTags[1].ti_Data = useit[ prefs.use_pub+1 ];
				GT_SetGadgetAttrsA(G, Wnd, NULL, ScrMdTags);
			}
			/* If user want to use a pubscreen, then disable backdrop checkbox **
			** It is not recommended to use a backdrop'ed window on a such scr */
			if( prefs.use_pub )
				OnGadget  (gads[CGS], Wnd, NULL);
			else
				OffGadget (gads[CGS], Wnd, NULL);
			break;
		case CGS:   case CGS+1:	/* Checkbuttons */
		case CGS+2: case CGS+3:
			(&prefs.backdrop)[G->GadgetID-CGS] = ((G->Flags & GFLG_SELECTED) == GFLG_SELECTED);
			break;
		case UCS:	/* Save */
			close_prefwnd(CMD_SAVPREF);
			if(ConfigFile) save_prefs(&prefs);
			 return TRUE;
		case UCS+1:	/* Use */
			close_prefwnd(CMD_NEWPREF); return TRUE;
		case UCS+2:	/* Cancel */
			close_prefwnd(0); return TRUE;
		case UCS+3: /* Cycle color */
			ColorIndex = msgbuf.Code; break;
		case UCS+4: /* Palette */
			if((&pen.bg)[ ColorIndex ] != msgbuf.Code)
				(&pen.bg)[ ColorIndex ]  = msgbuf.Code,
				(&prefs.pen.bg)[ ColorIndex ] = -msgbuf.Code-1,
				render_sample(Wnd, Modif[ ColorIndex ] );
	}
	/* Returns TRUE if window is closed */
	return FALSE;
}

/*** Keyboard shorcut ***/
BYTE handle_pref_kbd( UBYTE Key )
{
	switch( Key )
	{
		case '\t':
		case 't':
		case 'T':
			GetSI(gads[0])->BufferPos = GetSI(gads[0])->MaxChars;
			ActivateGadget(gads[0], Wnd, NULL); break;
		case 27: close_prefwnd(0); return TRUE;
		case 13:
		case 'u':
		case 'U': close_prefwnd(CMD_NEWPREF); return TRUE;
		case 's':
		case 'S':
			close_prefwnd(CMD_SAVPREF);
			if(ConfigFile) save_prefs(&prefs);
			return TRUE;
		case 127:
			if(Wnd->Height > Wnd->BorderTop)
				ZipWindow(Wnd);
	}
	return FALSE;
}

/*** Handle newmenu events ***/
BYTE handle_pref_menu( ULONG MenuID )
{
	switch( MenuID )
	{
		case 101: load_pref(&prefs);      break; /* Load */
		case 102: save_pref_as(&prefs);   break; /* Save as */
		case 103: close_prefwnd(0);       return TRUE;
		case 201: default_prefs(&prefs);  break; /* Default prefs */
		case 202: restore_config(&prefs); break; /* Last saved */
	}
	return FALSE;
}

/*** Handle events coming from pref window ***/
void handle_pref( void )
{
	/* Collect messages posted to the window port: */
	while(( msg=GT_GetIMsg(Wnd->UserPort) ))
	{
		/* Copy the entire message into the buffer: */
		CopyMem(msg, &msgbuf, sizeof(msgbuf));
		GT_ReplyIMsg( msg );

		switch( msgbuf.Class )
		{
			case IDCMP_CLOSEWINDOW:	close_prefwnd(CMD_NEWPREF); return;
			case IDCMP_INTUITICKS:
				/* An error message which needs to be removed? */
				if(err_time == 0) err_time = msgbuf.Seconds;
				if(err_time && msgbuf.Seconds-err_time>4) StopError(Wnd);
				break;
			case IDCMP_VANILLAKEY:
				if( handle_pref_kbd( msgbuf.Code ) ) return;
				else break;
			case IDCMP_MENUPICK:
			{
			    	struct MenuItem *item = ItemAddress(Menu,msgbuf.Code);
				
				if (item)
				{
				    if( handle_pref_menu((ULONG)GTMENUITEM_USERDATA(item))) return;
				}
				break;				
			}
			case IDCMP_GADGETUP:
				if( handle_pref_gadget( (struct Gadget *) msgbuf.IAddress) ) return;
				else break;
			case IDCMP_NEWSIZE:
				if( Wnd->Height > SAMPLE_HEI ) render_sample(Wnd, EDIT_ALL);
		}
	}
}

#ifdef	DEBUG
ULONG amem, bmem;
#endif

void free_locale(void);

/*** Handle events coming from main window: ***/
void cleanup(UBYTE *msg, int errcode)
{
	free_locale();
	close_port();
	free_sample();
	free_asl();

	if(Vi)				FreeVisualInfo(Vi);
	if(DiskfontBase)	CloseLibrary(DiskfontBase);
	if(LocaleBase)		CloseLibrary(LocaleBase);
	if(AslBase)			CloseLibrary(AslBase);
	if(GadToolsBase)	CloseLibrary(GadToolsBase);
	if(GfxBase)			CloseLibrary((struct Library *)GfxBase);
	if(IntuitionBase)	CloseLibrary((struct Library *)IntuitionBase);
	if(msg)				puts(msg);

#ifdef	DEBUG
	/* Here can be compared good programs with the others :-) */
	amem = AvailMem( MEMF_PUBLIC );
	if(amem < bmem)	printf("Possible memory lost of %d bytes\n", bmem-amem);
#endif
	exit(errcode);
}

void prefs_local(void);

/*** MAIN LOOP ***/
int main(int argc, char *argv[])
{
	ULONG sigwait, sigrcvd;
#ifdef	DEBUG
	bmem = AvailMem( MEMF_PUBLIC );
#endif
	memset(&prefs,0,sizeof(prefs));

	/* If preference tool is already running, just quits */
	if( find_prefs() ) cleanup(0,0);

	/* Optionnal libraries */
	AslBase      = (struct Library *) OpenLibrary("asl.library",     36);
	LocaleBase   = (struct Library *) OpenLibrary("locale.library",  38);
	DiskfontBase = (struct Library *) OpenLibrary("diskfont.library", 0);

	if(LocaleBase) prefs_local();		/* Localize the prog */

	/* Open the required ROM libraries: */
	if( (IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 36)) &&
	    (GfxBase       = (struct GfxBase *)       OpenLibrary("graphics.library",  36)) &&
	    (GadToolsBase  = (struct Library *)       OpenLibrary("gadtools.library",  36)) &&
	    (LayersBase    = (struct Library *)       OpenLibrary("layers.library",    36)) )
	{
		if( !(msgport = create_port()) ) cleanup(0, RETURN_FAIL);

		/* User may want to "edit" a particular file */
		if(argc > 1) {
			if( load_prefs(&prefs, argv[1]) ) cleanup(ErrMsg(ERR_LOADFILE), RETURN_FAIL);
		} else
			if( find_jano(&prefs) ) ConfigFile = FALSE;
			else load_prefs(&prefs, NULL);

		save_config( ConfigFile );
		if(ConfigFile) Scr = prefs.parent;

		/* Init interface */
		if( setup_guipref() )
		{
			sigwait = msgport | prefsigbit | SIGBREAKF_CTRL_C;
			for( ;; )
			{
				sigrcvd = Wait( sigwait );
				/* From where does the signal comes from? */
				if(sigrcvd & SIGBREAKF_CTRL_C) break;

				else if(sigrcvd & msgport) handle_port();

				else handle_pref();
				if(Wnd == NULL) break;
			}
		} else ThrowError(Wnd, ErrMsg(ERR_NOGUI));
	} else ThrowError(Wnd, ErrMsg(ERR_BADOS));
	cleanup(0,0);
	
	return 0;
}
