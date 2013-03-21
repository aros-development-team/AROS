/*
**	$VER: MonED  Release V3 (29.01.94)
**
**	Monitor Specs Editor , for changing monitor spec details such as syncs!
**
**  Programmed by : Raul A. Sobon.
**
**	(C) Copyright 1993 PRoJeCT-23, Inc.
**		All Rights Reserved
**
**
**
**
*/


#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/stdio.h>				// dos stdio

#include <graphics/gfxbase.h>
#include <graphics/monitor.h>
#include <graphics/modeid.h>
#include <intuition/classes.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuitionbase.h>

#include "moned_globals.h"			// my private data

#include <proto/exec.h>				// use amiga library stuff
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h> 
#include <proto/diskfont.h> 
#include <proto/asl.h>
#include <proto/icon.h>

#include <stdio.h>					// and the thing we all use!
#include <stdlib.h>
#include <string.h>

#include "compiler.h"

#define D(x)
#define DIDCMP(x)
#define DMSG(x)

#define TOOLS_IDCMP			ARROWIDCMP | BUTTONIDCMP | CHECKBOXIDCMP |\
							 INTEGERIDCMP | LISTVIEWIDCMP | MXIDCMP |\
							 CYCLEIDCMP | PALETTEIDCMP | SCROLLERIDCMP |\
							 SLIDERIDCMP | STRINGIDCMP

BOOL	Monitor_Editable = TRUE;


ULONG			IClass;
UWORD			Qualifier, Code, Prev_Code;
struct Gadget	*IObject;
APTR			MainVisualInfo;
struct TextAttr thinpaz8 = {
	( STRPTR )"thinpaz.font", 8, 0x00, 0x00 };
struct TextAttr	Topaz80 = {
	( STRPTR )"topaz.font", 8, 0x00, 0x00 };

struct Screen	*MainScreen;
struct TextFont	*thinpazfont8;
struct TextAttr *default_font;

struct	MonitorSpec	*currmonitor;
struct	MonitorSpec	oldmonitor;
struct	SpecialMonitor	oldspecial;

#include	"moned_rev.h"

UBYTE verstr[]={ VERSTAG };



/*
 * --- All my requesters use these tags
 */
struct TagItem				ReqTags[] = {
	{WA_Left,				50l},
	{WA_Top,					50l},
	{WA_Width,				330l},
	{WA_Height,				186l},
	{WA_IDCMP,				IDCMP_CLOSEWINDOW | TOOLS_IDCMP | IDCMP_VANILLAKEY | IDCMP_REFRESHWINDOW},
	{WA_Flags,				WFLG_DRAGBAR | WFLG_DEPTHGADGET| WFLG_CLOSEGADGET | WFLG_ACTIVATE | WFLG_RMBTRAP | WFLG_SMART_REFRESH | WFLG_GIMMEZEROZERO},
	{WA_Gadgets,				0l},
	{WA_Title,				0l},
	{WA_AutoAdjust,			TRUE},
	{WA_CustomScreen,		0l},
	{TAG_DONE} };

/*
 * --- window gadget ID's
 */
#define GD_HBSTRT	1
#define GD_HSSTRT	2
#define GD_HSSTOP	3
#define GD_HBSTOP	4
#define GD_VBSTRT	5
#define GD_VSSTRT	6
#define GD_VSSTOP	7
#define GD_VBSTOP	8
#define GD_TOTROWS	9
#define GD_TOTCLKS	10
#define GD_SAVE		50
#define GD_USE		51
#define GD_CANCEL	52
#define GD_JUMP		53






/*
 * --- Program gadget pointers that needs to be changed.
 */
struct Window			*sp_Wnd	= NULL;
struct Gadget
						*sp_HBSTRT,
						*sp_HSSTRT,
						*sp_HSSTOP,
						*sp_HBSTOP,
						*sp_VBSTRT,
						*sp_VSSTRT,
						*sp_VSSTOP,
						*sp_VBSTOP,
						*sp_TOTROWS,
						*sp_TOTCLKS;

struct Gadget			*sp_GList = NULL;
struct Gadget			*g;
struct NewGadget		ng;
UBYTE					*sp_Title = "MonitorSpec Ed V3a (Raul. A. Sobon)";

/*
 * --- TagItems for the slider gadgets.
 */
struct TagItem  sp_HsyncTags[] = {
	{GTSL_LevelFormat,		(IPTR)"%03lx"},
	{GTSL_MaxLevelLen,		3L},
	{GTSL_Min,				0L},
	{GTSL_Max,				64L},
	{GA_Disabled,			FALSE},
	{TAG_DONE} };

struct TagItem  sp_VsyncTags[] = {
	{GTSL_LevelFormat,		(IPTR)"%04lx"},
	{GTSL_MaxLevelLen,		4L},
	{GTSL_Min,				0L},
	{GTSL_Max,				8000L},
	{GA_Disabled,			FALSE},
	{TAG_DONE} };

struct TagItem  sp_RowTags[] = {
	{GTSL_LevelFormat,		(IPTR)"%03lx"},
	{GTSL_MaxLevelLen,		3L},
	{GTSL_Min,				0L},
	{GTSL_Max,				768L},
	{GA_Disabled,			FALSE},
	{TAG_DONE} };

struct TagItem  sp_TotClkTags[] = {
	{GTSL_LevelFormat,		(IPTR)"%03lx"},
	{GTSL_MaxLevelLen,		3L},
	{GTSL_Min,				1L},
	{GTSL_Max,				768L},
	{GA_Disabled,			FALSE},
	{TAG_DONE} };


ULONG		version=30;







VOID __stdargs ShowFault(LONG code,char *header)
{
	struct EasyStruct EasyStruct;

	char ErrorString[80];

	EasyStruct.es_StructSize = sizeof(struct EasyStruct);
	EasyStruct.es_Flags = 0;
	EasyStruct.es_Title = "MonitorSpec Ed";
	EasyStruct.es_GadgetFormat = "Ok";

	if(!code)
		EasyStruct.es_TextFormat = header;
	else
	{
		Fault(code,header,ErrorString,80);

		EasyStruct.es_TextFormat = ErrorString;
	}

	EasyRequest(NULL,&EasyStruct,NULL,NULL);
}



void SetAllProps( void ){
	if( currmonitor ){
		if( currmonitor->ms_Special ){
			GT_SetGadgetAttrs( sp_HBSTRT, sp_Wnd, NULL, GTSL_Level, currmonitor->ms_Special->hblank.asi_Start , TAG_DONE );
			GT_SetGadgetAttrs( sp_HSSTRT, sp_Wnd, NULL, GTSL_Level, currmonitor->ms_Special->hsync.asi_Start , TAG_DONE );
			GT_SetGadgetAttrs( sp_HSSTOP, sp_Wnd, NULL, GTSL_Level, currmonitor->ms_Special->hsync.asi_Stop , TAG_DONE );
			GT_SetGadgetAttrs( sp_HBSTOP, sp_Wnd, NULL, GTSL_Level, currmonitor->ms_Special->hblank.asi_Stop , TAG_DONE );
			GT_SetGadgetAttrs( sp_VBSTRT, sp_Wnd, NULL, GTSL_Level, currmonitor->ms_Special->vblank.asi_Start , TAG_DONE );
			GT_SetGadgetAttrs( sp_VSSTRT, sp_Wnd, NULL, GTSL_Level, currmonitor->ms_Special->vsync.asi_Start , TAG_DONE );
			GT_SetGadgetAttrs( sp_VSSTOP, sp_Wnd, NULL, GTSL_Level, currmonitor->ms_Special->vsync.asi_Stop , TAG_DONE );
			GT_SetGadgetAttrs( sp_VBSTOP, sp_Wnd, NULL, GTSL_Level, currmonitor->ms_Special->vblank.asi_Stop , TAG_DONE );
		}
		GT_SetGadgetAttrs( sp_TOTROWS, sp_Wnd, NULL, GTSL_Level,currmonitor->total_rows , TAG_DONE );
		GT_SetGadgetAttrs( sp_TOTCLKS, sp_Wnd, NULL, GTSL_Level,currmonitor->total_colorclocks , TAG_DONE );
	}
}


/*
 * --- Reads a message from the window message port.
 * --- Returns TRUE if a message was read and puts the
 * --- message data in the globals. Return FALSE if there
 * --- was no message at the port.
 */
long ReadIMsg( struct Window *iwnd )
{
	struct IntuiMessage *imsg;

	if ((imsg = GT_GetIMsg( iwnd->UserPort ))) {
		IClass	  =	imsg->Class;
		Qualifier =	imsg->Qualifier;
		Code	  =	imsg->Code;
		IObject	  =	imsg->IAddress;
		DMSG(printf("Got message, IClass 0x%08X, Qualifier 0x%04X, Code %u, IObject %p\n", IClass, Qualifier, Code, IObject));

		GT_ReplyIMsg( imsg );

		return TRUE;
	}
	return FALSE;
}



/*
 * --- Clears all message from a message port.
 */
void ClearMsgPort( struct MsgPort *mport )
{
	struct IntuiMessage  *msg;

	while ((msg = GT_GetIMsg( mport ))) GT_ReplyIMsg( msg );
}





/*
 * --- Open lots of libraries that I need.
 */
long OpenLibraries( void ){

	if ( !(IconBase = (struct Library *) OpenLibrary((UBYTE *) "icon.library" , 36l ))) {
		WriteStr("\ticon.library\n");
		return FALSE;
		}

	if ( !(GfxBase = (struct GfxBase *) OpenLibrary((UBYTE *) "graphics.library" , 36l ))) {
		WriteStr("\tgraphics.library\n");
		return FALSE;
		}

	if ( !(DosBase = (struct DosBase *) OpenLibrary((UBYTE *) "dos.library", 36l ))) {
		WriteStr("\tdos.library\n");
		return FALSE;
		}

	if ( !(IntuitionBase = (struct IntuitionBase *) OpenLibrary((UBYTE *) "intuition.library", 36l ))) {
		WriteStr("\tintuition.library\n");
		return FALSE;
		}

	if ( !(AslBase = (struct Library *) OpenLibrary((UBYTE *) "asl.library", 37l ))) {
		WriteStr("\tasl.library\n");
		return FALSE;
		}

	if ( !(GadToolsBase = (struct Library *) OpenLibrary((UBYTE *) "gadtools.library", 36l ))) {
		WriteStr("\tgadtools.library\n");
		return FALSE;
		}

	if ( !(DiskFontBase = (struct Library *) OpenLibrary((UBYTE *) "diskfont.library", 36l ))) {
		WriteStr("\tdiskfont.library\n");
		return FALSE;
		}

	return TRUE;
}




/*
 * --- Close the libraries which are opened by me.
 */
void CloseLibraries( void ){
	if (DiskFontBase)	CloseLibrary( (struct Library *) DiskFontBase );
	if (GadToolsBase)	CloseLibrary( (struct Library *) GadToolsBase );
	if (AslBase)		CloseLibrary( (struct Library *) AslBase );
	if (IntuitionBase)	CloseLibrary( (struct Library *) IntuitionBase );
	if (DosBase)		CloseLibrary( (struct Library *) DosBase );
	if (GfxBase)		CloseLibrary( (struct Library *) GfxBase );
	if (IconBase)		CloseLibrary( (struct Library *) IconBase );
}


void SaveMonitor( void ){
	if( currmonitor ){
		CopyMem(  currmonitor, &oldmonitor, sizeof( struct MonitorSpec) );
		if( currmonitor->ms_Special )
			CopyMem(  currmonitor->ms_Special, &oldspecial, sizeof( struct SpecialMonitor ) );
	}
}


void RestoreMonitor( void ){
	if( currmonitor ){
		CopyMem(  &oldmonitor, currmonitor,  sizeof( struct MonitorSpec) );
		if( currmonitor->ms_Special )
			CopyMem(  &oldspecial, currmonitor->ms_Special, sizeof( struct SpecialMonitor ) );
	}
}






static	UBYTE	mon[16];


void SaveDetails( void ){
	struct	DiskObject *diskobj;
	UBYTE	tt[6][16]={
						"TOTROWS=0x00",
						"TOTCLKS=0x00",
					 	"HBSTRT=0x00",
						"HBSTOP=0x00",
						"VBSTRT=0x00",
						"VBSTOP=0x00"  };
	UBYTE	*txtptrs[8];
	UBYTE	**oldtxtptrs;
	UBYTE	monname[35];

	if( currmonitor ){
		sprintf( monname,"DEVS:Monitors/%s",mon );
		if ((diskobj=GetDiskObject( monname ))){
			oldtxtptrs = diskobj->do_ToolTypes;

			sprintf( &tt[0][10],"%02x",currmonitor->total_rows);
			sprintf( &tt[1][10],"%02x",currmonitor->total_colorclocks);
			txtptrs[0] = &tt[0][0];
			txtptrs[1] = &tt[1][0];

			if( currmonitor->ms_Special ){
				sprintf( &tt[2][9],"%02x", currmonitor->ms_Special->hblank.asi_Start );
				sprintf( &tt[3][9],"%02x", currmonitor->ms_Special->hblank.asi_Stop );
				sprintf( &tt[4][9],"%02x", currmonitor->ms_Special->vblank.asi_Start );
				sprintf( &tt[5][9],"%02x", currmonitor->ms_Special->vblank.asi_Stop );
				txtptrs[2] = &tt[2][0];
				txtptrs[3] = &tt[3][0];
				txtptrs[4] = &tt[4][0];
				txtptrs[5] = &tt[5][0];
				txtptrs[6] = NULL;
			}
			else
				txtptrs[2] = NULL;

			diskobj->do_ToolTypes = txtptrs;
			if( !PutDiskObject( monname, diskobj ) )
				ShowFault(IoErr(),"icon.library/PutDiskObject()");

			diskobj->do_ToolTypes = oldtxtptrs;
			FreeDiskObject( diskobj );
		}
		else
			ShowFault(IoErr(),"icon.library/GetDiskObject()");
	}
	else
		ShowFault(0, "Invalid monitor in use");
}







void UpDateMonitor( void ){
	if( currmonitor ){
		if( currmonitor->ms_Special ){
			if( currmonitor->ms_Special->do_monitor ) {
				currmonitor->ms_Special->do_monitor(currmonitor);
				MakeScreen( (struct Screen *)MainScreen );
				RethinkDisplay();
			}
		}
	}
}



#define	PICASSO_MONITOR_ID	0x000f1000
#define	EGS24_MONITOR_ID	0x04001000

void UpDateDisplay( void ){
	UBYTE	tstr[40];
	ULONG	rows=0,clks=0,fps,scan,mode;

	if( currmonitor ){
		rows = currmonitor->total_rows;
		clks = currmonitor->total_colorclocks;
	}
	if( rows && clks )
		fps = abs(1000000000/(rows*clks*280));
	else
		fps = 0;
	scan = fps*rows;

	mode = GetVPModeID(&IntuitionBase->ActiveScreen->ViewPort)&MONITOR_ID_MASK;

	switch( (ULONG)(mode) ) {
		case NTSC_MONITOR_ID:		sprintf(mon,"NTSC");break;
		case PAL_MONITOR_ID:		sprintf(mon,"PAL");break;
		case VGA_MONITOR_ID:		sprintf(mon,"Multiscan");break;
		case A2024_MONITOR_ID:		sprintf(mon,"A2024");break;
		case PROTO_MONITOR_ID:		sprintf(mon,"PROTO");break;
		case EURO72_MONITOR_ID:		sprintf(mon,"EURO72");break;
		case EURO36_MONITOR_ID:		sprintf(mon,"EURO36");break;
		case SUPER72_MONITOR_ID:	sprintf(mon,"SUPER72");break;
		case DBLNTSC_MONITOR_ID:
			if( version >= 39 )	sprintf(mon,"DBLNTSC"); else 	sprintf(mon,"DoubleNTSC");
			break;
		case DBLPAL_MONITOR_ID:
			if( version >= 39 )	sprintf(mon,"DBLPAL"); else 	sprintf(mon,"DoublePAL");
			break;
		case PICASSO_MONITOR_ID:	sprintf(mon,"PICASSO");break;
		case EGS24_MONITOR_ID:		sprintf(mon,"EGS-SPECTRUM");break;
		case DEFAULT_MONITOR_ID:	sprintf(mon,"PAL");break;
		default:					sprintf(mon,"CUSTOM(%4x)",mode);break;
	}

	sprintf(tstr,"%s:  %02d.%02d Khz, %02d Hz       ",mon,scan/1000,(scan%1000)*10,fps );
	if( sp_Wnd ){
		SetAPen( sp_Wnd->RPort, 2 );
		Move( sp_Wnd->RPort, 9, 136 );
		Text( sp_Wnd->RPort, tstr, strlen(tstr) );
//		printf( "*** %s",tstr );
	}
	else
		printf( "%s.\n", tstr );

	UpDateMonitor();
}






/**********************************************************************
 *
 *						Draw3DOutBox
 *	Description : just like normal bevelbox except inside color is selectable
 *				  plus it looks MONUMENTAL!
 *								¯¯¯¯¯¯¯¯¯¯¯
 *  Returns		: NULL
 *	Globals		: usual
 *
 */
void Draw3DBox(
	struct	RastPort *rp,		// window rastport to draw into
	UWORD	xpos,				// X coordinate to place MEGA-BEVEL
	UWORD	ypos,				// Y coordinate to place MEGA-BEVEL
	UWORD	xsize,				// X size of MEGA-BEVEL
	UWORD	ysize,				// X size of MEGA-BEVEL
	UWORD	shine_pen,			// top-left pen
	UWORD	body_pen,			// central pen
	UWORD	shadow_pen			// bot-right pen
){

#define	LineDraw(x,y,x2,y2)	Move( rp, x , y); \
							Draw( rp, x2, y2 )

	xsize--;ysize--;

	if( body_pen == 0xffff )
		SetAPen( rp, shine_pen );
	else {
		SetAPen( rp, body_pen );
		RectFill( rp, xpos, ypos, xpos+xsize, ypos+ysize);
	}

	LineDraw( xpos, ypos, xpos+1, ypos+1 );
	LineDraw( xpos+xsize-1, ypos+ysize-1, xpos+xsize, ypos+ysize );

	SetAPen( rp, shine_pen );
	LineDraw( xpos+0, ypos+0, xpos+xsize-1, ypos );
	LineDraw( xpos+0, ypos+1, xpos+xsize-2, ypos+1 );
	LineDraw( xpos+0, ypos+0, xpos, ypos+ysize-1 );
	LineDraw( xpos+1, ypos+0, xpos+1, ypos+ysize-2 );

	SetAPen( rp, shadow_pen );
	LineDraw( xpos+1, ypos+ysize-0, xpos+xsize-1, ypos+ysize-0 );
	LineDraw( xpos+2, ypos+ysize-1, xpos+xsize-2, ypos+ysize-1 );
	LineDraw( xpos+xsize-0, ypos+1, xpos+xsize-0, ypos+ysize-1 );
	LineDraw( xpos+xsize-1, ypos+2, xpos+xsize-1, ypos+ysize-2 );

	SetAPen( rp, 2 );
	if( shine_pen == shadow_pen ){
		SetAPen( rp, shine_pen );
		LineDraw( xpos, ypos+ysize, xpos+1, ypos+ysize-1 );
		LineDraw( xpos+xsize-1, ypos+1, xpos+xsize, ypos );
	}
	else
	if(shine_pen > shadow_pen ){
		LineDraw( xpos+0, ypos+0, xpos+2, ypos+0 );
		LineDraw( xpos+0, ypos+0, xpos+0, ypos+2 );
	}
	else {
		LineDraw( xpos+xsize, ypos+ysize, xpos+xsize-2, ypos+ysize );
		LineDraw( xpos+xsize, ypos+ysize, xpos+xsize-0, ypos+ysize-2 );
	}
}






/**********************************************************************
 *
 *						Draw3DLine
 *	Description : just like normal line except its 3D (sorta!) (horizontal)
 *				  plus it looks MONUMENTAL!
 *								¯¯¯¯¯¯¯¯¯¯¯
 *  Returns		: NULL
 *	Globals		: usual
 *
 */
void Draw3DLine(
	struct	RastPort *rp,		// window rastport to draw into
	UWORD	xpos,				// X coordinate to place MEGA-BEVEL
	UWORD	ypos,				// Y coordinate to place MEGA-BEVEL
	UWORD	xsize,				// X size of MEGA-BEVEL
	UWORD	shine_pen,			// top-left pen
	UWORD	shadow_pen			// bot-right pen
){

	xsize--;

	SetAPen( rp, shine_pen );
	LineDraw( xpos+0, ypos+0, xpos+xsize, ypos+0 );
	SetAPen( rp, shadow_pen );
	LineDraw( xpos+0, ypos+1, xpos+xsize, ypos+1 );
}



long OpenDisplay( void ){
	WORD	offy,x1,y1,w,h;

	ReqTags[7].ti_Data = (ULONG)sp_Title;
	ReqTags[9].ti_Data = (Tag)MainScreen;

	if ( ! (thinpazfont8  = OpenDiskFont( &thinpaz8 )) ) {
			thinpazfont8  = OpenDiskFont( &Topaz80 );
			default_font = &Topaz80;
	} else
	    default_font = &thinpaz8;
	D(printf("Font: %p\n", thinpazfont8));

	currmonitor = GfxBase->current_monitor;
	D(printf("Current monitor: %p\n", currmonitor));

	SaveMonitor();

	if ( currmonitor ){
		sp_RowTags[4].ti_Data = FALSE;
		sp_TotClkTags[4].ti_Data = FALSE;
		Monitor_Editable = TRUE;
	}
	else {
		sp_RowTags[4].ti_Data = TRUE;
		sp_TotClkTags[4].ti_Data = TRUE;
		Monitor_Editable = FALSE;
	}

	if ( currmonitor && currmonitor->ms_Special ){
		sp_HsyncTags[4].ti_Data = FALSE;
		sp_VsyncTags[4].ti_Data = FALSE;
		Monitor_Editable = TRUE;
	}
	else {
		sp_HsyncTags[4].ti_Data = TRUE;
		sp_VsyncTags[4].ti_Data = TRUE;
		Monitor_Editable = FALSE;
	}

	D(printf("Monitor is editable: %d\n", Monitor_Editable));
		

	if ( ! ( MainVisualInfo = GetVisualInfo( MainScreen, TAG_DONE )))
		return( 2L );
	D(printf("VisualInfo %p\n", MainVisualInfo));

#define YSTEP 12

	if ((g = CreateContext( &sp_GList ))) {
		D(printf("GadTools context: %p\n", g));

		ng.ng_LeftEdge		=	89;
		ng.ng_TopEdge		=	5;
		ng.ng_Width		=	216;
		ng.ng_Height		=	10;
		ng.ng_GadgetText	=	"HBSTRT:    ";
		ng.ng_TextAttr		=	default_font;
		ng.ng_GadgetID		=	GD_HBSTRT;
		ng.ng_Flags		=	PLACETEXT_LEFT | NG_HIGHLABEL;
		ng.ng_VisualInfo	=	MainVisualInfo;
		ng.ng_UserData		=	NULL;
		sp_HBSTRT = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_HsyncTags );
		D(printf("HBSTRT gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"HSSTRT:    ";
		ng.ng_GadgetID	  =	GD_HSSTRT;
		sp_HSSTRT = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_HsyncTags );
		D(printf("HSSTRT gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"HSSTOP:    ";
		ng.ng_GadgetID	  =	GD_HSSTOP;
		sp_HSSTOP = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_HsyncTags );
		D(printf("HSSTOP gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"HBSTOP:    ";
		ng.ng_GadgetID	  =	GD_HBSTOP;
		sp_HBSTOP = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_HsyncTags );
		D(printf("HBSTOP gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"TOTCLKS:    ";
		ng.ng_GadgetID	  =	GD_TOTCLKS;
		sp_TOTCLKS = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_TotClkTags );
		D(printf("TOTCLKS gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"VBSTRT:    ";
		ng.ng_GadgetID	  =	GD_VBSTRT;
		sp_VBSTRT = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_VsyncTags );
		D(printf("VBSTRT gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"VSSTRT:    ";
		ng.ng_GadgetID	  =	GD_VSSTRT;
		sp_VSSTRT = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_VsyncTags );
		D(printf("VSSTRT gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"VSSTOP:    ";
		ng.ng_GadgetID	  =	GD_VSSTOP;
		sp_VSSTOP = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_VsyncTags );
		D(printf("VSSTOP gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"VBSTOP:    ";
		ng.ng_GadgetID	  =	GD_VBSTOP;
		sp_VBSTOP = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_VsyncTags );
		D(printf("VBSTOP gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_TopEdge		+=	YSTEP;
		ng.ng_GadgetText	=	"TOTROWS:    ";
		ng.ng_GadgetID	  =	GD_TOTROWS;
		sp_TOTROWS = g = CreateGadgetA( SLIDER_KIND, g, &ng, sp_RowTags );
		D(printf("TOTROWS gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));


	y1=	ng.ng_TopEdge		=	142;
	x1=	ng.ng_LeftEdge		=	89;
	w=	ng.ng_Width		=	50;
	h=	ng.ng_Height		=	14;
		ng.ng_Flags		=	PLACETEXT_IN;
		ng.ng_GadgetText	=	"_USE";
		ng.ng_GadgetID	  =	GD_USE;
		g = CreateGadget( BUTTON_KIND, g, &ng, GT_Underscore, (Tag)'_', TAG_DONE );
		D(printf("USE gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_LeftEdge	  	=	144;
		ng.ng_GadgetText	=	"_SAVE";
		ng.ng_GadgetID	  	=	GD_SAVE;
		g = CreateGadget( BUTTON_KIND, g, &ng, GT_Underscore, (Tag)'_', TAG_DONE );
		D(printf("SAVE gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_LeftEdge	  	=	199;
		ng.ng_GadgetText	=	"_CANCEL";
		ng.ng_GadgetID	  	=	GD_CANCEL;
		g = CreateGadget( BUTTON_KIND, g, &ng, GT_Underscore, (Tag)'_', TAG_DONE );
		D(printf("CANCEL gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));

		ng.ng_LeftEdge	  	=	254;
		ng.ng_GadgetText	=	"_JUMP";
		ng.ng_GadgetID		=	GD_JUMP;
		g = CreateGadget( BUTTON_KIND, g, &ng, GT_Underscore, (Tag)'_', TAG_DONE );
		D(printf("JUMP gadget: %p, at (%d, %d)\n", g, ng.ng_LeftEdge, ng.ng_TopEdge));


		if ( g ) {
			//ReqTags[6].ti_Data	=	(Tag)sp_GList;
			offy = MainScreen->WBorTop + MainScreen->RastPort.TxHeight + MainScreen->WBorBottom;
			ReqTags[3].ti_Data = 164+offy;
			D(printf("Opening window, size %ld x %ld...\n", ReqTags[2].ti_Data, ReqTags[3].ti_Data));
			if ((sp_Wnd = OpenWindowTagList( NULL, ReqTags ))) {
				D(printf("Window %p\n", sp_Wnd));
				SetFont( sp_Wnd->RPort, thinpazfont8 );
				if( MainScreen->BitMap.Depth > 2 ) {
					Draw3DBox( sp_Wnd->RPort, 0, 0,	sp_Wnd->Width-sp_Wnd->BorderLeft-sp_Wnd->BorderRight,
						sp_Wnd->Height-sp_Wnd->BorderTop-sp_Wnd->BorderBottom, 2, 3, 1 );

					Draw3DLine( sp_Wnd->RPort, 1, 125,sp_Wnd->Width-sp_Wnd->BorderLeft-sp_Wnd->BorderRight-3, 1, 2 );

					SetAPen( sp_Wnd->RPort, 2 );
					SetBPen( sp_Wnd->RPort, 3 );
				}
				AddGList( sp_Wnd, sp_GList, 0, -1, NULL);
				RefreshGList( sp_GList, sp_Wnd, 0, 0);
				GT_RefreshWindow( sp_Wnd, NULL );
				SetAllProps();
				UpDateDisplay();

				Draw3DBox( sp_Wnd->RPort, x1-2, y1-2, w+4, h+4, 1, 0xffff, 1 );
			}
			else
				return 0;
		}
	}
	return 1;
}





void CloseDisplay( void ) {

	if ( thinpazfont8 )
		CloseFont( thinpazfont8 );

	if ( MainVisualInfo ) {
		FreeVisualInfo( MainVisualInfo );
		MainVisualInfo = NULL;
	}
	if ( sp_Wnd )		CloseWindow( sp_Wnd );
	if ( sp_GList )		FreeGadgets( sp_GList );
}




LONG xtoi( UBYTE *txt ){
	LONG n;

	sscanf( txt,"%x",&n );

	return n;
}




// ----------------------------------------------------------------------------------------------
int main( int argc,char *argv[] )
{
	BOOL				 running  = TRUE;


	if( !OpenLibraries() ) {
		ShowFault(0, "Libraries cant be opened!");
		CloseLibraries();
		exit(5);
	}

	version=IntuitionBase->LibNode.lib_Version;

	MainScreen = (struct Screen *)(IntuitionBase->ActiveScreen);

	if( argc>1 ) {
		BOOL update = TRUE;
		currmonitor = GfxBase->current_monitor;
		SaveMonitor();

		if( *argv[1] == '?' ){
			puts( "Moned3a, © 1994 Raul Sobon / PRoJeCT-23");
			puts( " moned HBSTRT=0xnnn HBSTOP=0xnnn HBSTOP=0xnnn");
			puts( "       HSSTRT=0xnnn HSSTOP=0xnnn VBSTRT=0xnnn");
			puts( "       VBSTOP=0xnnn VSSTRT=0xnnn VSSTOP=0xnnn");
		//	puts( "       MAXOSC=0xnnn VIDOSC=0xnnn xxxxxx=0xnnn");
			puts( "NOTE: each item is optional and can be in any order and nnn is a hex number.");
		}
		else
		if( currmonitor ){
			WORD count = argc,lp=1;
			UBYTE *cmd,*num;

			while( count >1 ){
				cmd = argv[ lp ];
				num = cmd+9;
//printf("str=<%s>  n=<%s>=%d\n",cmd,num,xtoi( num ) );
				if( currmonitor->ms_Special ){
					if( !strnicmp( "HBSTRT=0x", cmd , 9 ) )	currmonitor->ms_Special->hblank.asi_Start=xtoi( num );
					else if( !strnicmp( "HBSTOP=0x", cmd , 9 ) )	currmonitor->ms_Special->hblank.asi_Stop=xtoi( num );
					else if( !strnicmp( "HSSTRT=0x", cmd , 9 ) )	currmonitor->ms_Special->hsync.asi_Start=xtoi( num );
					else if( !strnicmp( "HSSTOP=0x", cmd , 9 ) )	currmonitor->ms_Special->hsync.asi_Stop=xtoi( num );
					else if( !strnicmp( "VBSTRT=0x", cmd , 9 ) )	currmonitor->ms_Special->vblank.asi_Start=xtoi( num );
					else if( !strnicmp( "VBSTOP=0x", cmd , 9 ) )	currmonitor->ms_Special->vblank.asi_Stop=xtoi( num );
					else if( !strnicmp( "VSSTRT=0x", cmd , 9 ) )	currmonitor->ms_Special->vsync.asi_Start=xtoi( num );
					else if( !strnicmp( "VSSTOP=0x", cmd , 9 ) )	currmonitor->ms_Special->vsync.asi_Stop=xtoi( num );
					else update = FALSE;
				}
				if( !strnicmp( "TOTROW=0x", cmd , 9 ) )	{
							currmonitor->total_rows=xtoi( num ); update = TRUE; }
				else	if( !strnicmp( "TOTCLK=0x", cmd , 9 ) ) {
							currmonitor->total_colorclocks=xtoi( num ); update = TRUE; }
		//		else	if( !strnicmp( "MAXOSC=0x", cmd , 9 ) ) {
		//					currmonitor->ms_maxoscan=xtoi( num ); update = TRUE; }
		//		else	if( !strnicmp( "VIDOSC=0x", cmd , 9 ) ) {
		//					currmonitor->ms_videoscan=xtoi( num ); update = TRUE; }

				lp++; count--;
			}
			if( update ) {
				UpDateDisplay();
		//		printf("Display updated.\n");
			}
			else
				printf("argument <%s> invalid?\n",cmd);
		}
	}
	else
	if( OpenDisplay() ) {
		do {
			WaitPort( sp_Wnd->UserPort );

			while ( ReadIMsg( sp_Wnd )) {

				switch ( IClass ) {

					case	IDCMP_ACTIVEWINDOW:
						DIDCMP(printf("Window activate\n"));
						ClearMsgPort( sp_Wnd->UserPort );
						break;

					case	IDCMP_REFRESHWINDOW:
						DIDCMP(printf("Window refresh\n"));
						GT_BeginRefresh( sp_Wnd );
						GT_EndRefresh( sp_Wnd, TRUE );
						break;

					case	IDCMP_CLOSEWINDOW:
						DIDCMP(printf("Window close\n"));
						goto Cancel;
						break;

					case	IDCMP_VANILLAKEY:
						DIDCMP(printf("Keypress\n"));
						switch ( Code ) {
							case	'[':
								if( Monitor_Editable ){
									GT_SetGadgetAttrs(sp_TOTCLKS,sp_Wnd,0,GTSL_Level, --currmonitor->total_colorclocks,TAG_DONE);
									UpDateDisplay();
								}
								break;
							case	']':
								if( Monitor_Editable ){
									GT_SetGadgetAttrs(sp_TOTCLKS,sp_Wnd,0,GTSL_Level, ++currmonitor->total_colorclocks,TAG_DONE);
									UpDateDisplay();
								}
								break;

							case	'{':
								if( Monitor_Editable ){
									GT_SetGadgetAttrs(sp_TOTROWS,sp_Wnd,0,GTSL_Level, --currmonitor->total_rows,TAG_DONE);
									UpDateDisplay();
								}
								break;
							case	'}':
								if( Monitor_Editable ){
									GT_SetGadgetAttrs(sp_TOTROWS,sp_Wnd,0,GTSL_Level, ++currmonitor->total_rows,TAG_DONE);
									UpDateDisplay();
								}
								break;

							case	's':
								goto Save;

							case	13 :
							case	'u':
								goto Use;

							case	'c':
								goto Cancel;

							case	'j':
								goto Jump;
						}
						break;

					case	IDCMP_MOUSEMOVE:
						DIDCMP(printf("Mouse move\n"));
						switch ( IObject->GadgetID ) {
							case	GD_HBSTRT:
								if( currmonitor->ms_Special ){
									currmonitor->ms_Special->hblank.asi_Start = Code;
									UpDateMonitor();
								}
								break;
							case	GD_HSSTRT:
								if( currmonitor->ms_Special ){
									currmonitor->ms_Special->hsync.asi_Start = Code;
									UpDateMonitor();
								}
								break;
							case	GD_HSSTOP:
								if( currmonitor->ms_Special ){
									currmonitor->ms_Special->hsync.asi_Stop = Code;
									UpDateMonitor();
								}
								break;
							case	GD_HBSTOP:
								if( currmonitor->ms_Special ){
									currmonitor->ms_Special->hblank.asi_Stop = Code;
									UpDateMonitor();
								}
								break;

							case	GD_VBSTRT:
								if( currmonitor->ms_Special ){
									currmonitor->ms_Special->vblank.asi_Start = Code;
									UpDateMonitor();
								}
								break;
							case	GD_VSSTRT:
								if( currmonitor->ms_Special ){
									currmonitor->ms_Special->vsync.asi_Start = Code;
									UpDateMonitor();
								}
								break;
							case	GD_VSSTOP:
								if( currmonitor->ms_Special ){
									currmonitor->ms_Special->vsync.asi_Stop = Code;
									UpDateMonitor();
								}
								break;
							case	GD_VBSTOP:
								if( currmonitor->ms_Special ){
									currmonitor->ms_Special->vblank.asi_Stop = Code;
									UpDateMonitor();
								}
								break;

							case	GD_TOTROWS:
								currmonitor->total_rows = Code;
								UpDateDisplay();
								break;
							case	GD_TOTCLKS:
								currmonitor->total_colorclocks = Code;
								UpDateDisplay();
								break;
						}
						break;

					case	IDCMP_GADGETUP:
						DIDCMP(printf("Gadget up\n"));
							switch ( IObject->GadgetID ) {
								case	GD_SAVE:
									Save:
									if( Monitor_Editable ){
										SaveDetails();
										running = FALSE;
									}
									break;

								case	GD_USE :
									Use:
									running = FALSE;
									break;

								case	GD_CANCEL:
									Cancel:
									RestoreMonitor();
									UpDateDisplay();
									running = FALSE;
									break;

								case	GD_JUMP:
									Jump:
									RestoreMonitor();
									UpDateDisplay();
									MainScreen = (struct Screen *)(MainScreen->NextScreen);
									if ( !MainScreen )
										MainScreen = (struct Screen *)(IntuitionBase->FirstScreen);
									CloseDisplay();
									ScreenToFront( MainScreen );
									OpenDisplay();
									break;					  
							}
					break;
				}
			}
		}
		while ( running );
		CloseDisplay();
	}

	CloseLibraries();

	return 0;
}

