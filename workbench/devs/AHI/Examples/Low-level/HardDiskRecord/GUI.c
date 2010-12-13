/*********************************************/
/*                                           */
/*       Designer (C) Ian OConnor 1994       */
/*                                           */
/*      Designer Produced C include file     */
/*                                           */
/*********************************************/

#include <exec/types.h>
#include <libraries/locale.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <diskfont/diskfont.h>
#include <utility/utility.h>
#include <graphics/gfxbase.h>
#include <workbench/workbench.h>
#include <graphics/scale.h>
#include <clib/locale_protos.h>
#include <clib/exec_protos.h>
#include <clib/wb_protos.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/graphics_protos.h>
#include <clib/utility_protos.h>
#include <string.h>
#include <clib/diskfont_protos.h>

#include "GUI.extras.h"
#include "GUI.h"


ULONG BevelTags[] = 
	{
	(GTBB_Recessed), TRUE,
	(GT_VisualInfo), 0,
	(TAG_DONE)
	};
UBYTE Win0FirstScaleTexts = 0;

UBYTE Win0TextsLocalized = 0;

struct IntuiText Win0Texts[] =
	{
	1, 3, JAM1, 230, 101, NULL, (UBYTE *)Win0Texts1, NULL
	};

struct Window *Win0 = NULL;
APTR Win0VisualInfo;
APTR Win0DrawInfo;
struct Gadget *Win0GList;
struct Gadget *Win0Gadgets[15];
UBYTE Win0FirstRun = 0;

STRPTR Win0_formatLabels[] =
{
	(STRPTR)Win0_formatString0,
	(STRPTR)Win0_formatString1,
	NULL
};

ULONG Win0GadgetTags[] =
	{
	(GTLV_ShowSelected), 0,
	(GT_Underscore), '_',
	(GTLV_Selected), 0,
	(TAG_END),
	(GTLV_ShowSelected), 0,
	(GT_Underscore), '_',
	(GTLV_Selected), 0,
	(TAG_END),
	(GTSL_LevelFormat), 0,
	(GTSL_MaxLevelLen), 64,
	(GTSL_LevelPlace), 2,
	(GA_Immediate), TRUE,
	(GA_RelVerify), TRUE,
	(GT_Underscore), '_',
	(TAG_END),
	(GT_Underscore), '_',
	(TAG_END),
	(GTTX_CopyText), TRUE,
	(GT_TagBase+74), 0,  /* Justification in V39 */
	(TAG_END),
	(GTTX_CopyText), TRUE,
	(GT_TagBase+74), 0,  /* Justification in V39 */
	(TAG_END),
	(GTSL_Min), 100,
	(GTSL_Max), 100,
	(GTSL_Level), 100,
	(GTSL_LevelFormat), 0,
	(GTSL_MaxLevelLen), 64,
	(GTSL_LevelPlace), 2,
	(GA_Immediate), TRUE,
	(GA_RelVerify), TRUE,
	(GT_Underscore), '_',
	(TAG_END),
	(GTIN_MaxChars), 4,
	(GT_Underscore), '_',
	(TAG_END),
	(GTNM_Border), TRUE,
	(GT_TagBase+74), 0,  /* Justification in V39 */
	(GT_TagBase+75), 0,  /* String Formatting */
	(TAG_END),
	(GTST_String), (ULONG)"NewSample.AIFC",
	(GTST_MaxChars), 1024,
	(GT_Underscore), '_',
	(TAG_END),
	(GT_Underscore), '_',
	(TAG_END),
	(GT_Underscore), '_',
	(TAG_END),
	(GT_Underscore), '_',
	(TAG_END),
	(GTNM_Border), TRUE,
	(GT_TagBase+74), 0,  /* Justification in V39 */
	(TAG_END),
	(GT_Underscore), '_',
	(GTCY_Active), 1,
	(GTCY_Labels), (ULONG)&Win0_formatLabels[0],
	(TAG_END),
	};

UWORD Win0GadgetTypes[] =
	{
	LISTVIEW_KIND,
	LISTVIEW_KIND,
	SLIDER_KIND,
	BUTTON_KIND,
	TEXT_KIND,
	TEXT_KIND,
	SLIDER_KIND,
	INTEGER_KIND,
	NUMBER_KIND,
	STRING_KIND,
	BUTTON_KIND,
	BUTTON_KIND,
	BUTTON_KIND,
	NUMBER_KIND,
	CYCLE_KIND,
	};

struct NewGadget Win0NewGadgets[] =
	{
	16, 24, 140, 28, (UBYTE*)Win0_srclistString, NULL, Win0_srclist, 4, NULL,  (APTR)&Win0GadgetTags[0],
	163, 24, 140, 28, (UBYTE*)Win0_dstlistString, NULL, Win0_dstlist, 4, NULL,  (APTR)&Win0GadgetTags[7],
	163, 54, 93, 14, (UBYTE*)Win0_volsliderString, NULL, Win0_volslider, 1, NULL,  (APTR)&Win0GadgetTags[14],
	312, 54, 267, 14, (UBYTE*)Win0_ambuttonString, NULL, Win0_ambutton, 16, NULL,  (APTR)&Win0GadgetTags[27],
	317, 26, 258, 12, (UBYTE*)Win0_amtext1String, NULL, Win0_amtext1, 4, NULL,  (APTR)&Win0GadgetTags[30],
	317, 38, 258, 12, (UBYTE*)Win0_amtext2String, NULL, Win0_amtext2, 1, NULL,  (APTR)&Win0GadgetTags[35],
	163, 70, 93, 14, (UBYTE*)Win0_gainsliderString, NULL, Win0_gainslider, 1, NULL,  (APTR)&Win0GadgetTags[40],
	163, 98, 61, 14, (UBYTE*)Win0_durationString, NULL, Win0_duration, 1, NULL,  (APTR)&Win0GadgetTags[59],
	163, 114, 141, 14, (UBYTE*)Win0_lengthString, NULL, Win0_length, 1, NULL,  (APTR)&Win0GadgetTags[64],
	163, 130, 416, 14, (UBYTE*)Win0_filenameString, NULL, Win0_filename, 1, NULL,  (APTR)&Win0GadgetTags[71],
	16, 130, 141, 14, (UBYTE*)Win0_fnbuttonString, NULL, Win0_fnbutton, 16, NULL,  (APTR)&Win0GadgetTags[78],
	312, 98, 267, 14, (UBYTE*)Win0_createString, NULL, Win0_create, 16, NULL,  (APTR)&Win0GadgetTags[81],
	312, 114, 267, 14, (UBYTE*)Win0_beginString, NULL, Win0_begin, 16, NULL,  (APTR)&Win0GadgetTags[84],
	401, 146, 70, 14, (UBYTE*)Win0_secleftString, NULL, Win0_secleft, 2, NULL,  (APTR)&Win0GadgetTags[87],
	163, 146, 230, 14, (UBYTE*)Win0_formatString, NULL, Win0_format, 1, NULL,  (APTR)&Win0GadgetTags[92],
	};

struct Library *AslBase = NULL;
struct Library *DiskfontBase = NULL;
struct Library *GadToolsBase = NULL;
struct GfxBase *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library *UtilityBase = NULL;
struct LocaleBase *LocaleBase = NULL;
struct Catalog *HardDiskRecord_Catalog = NULL;
STRPTR HardDiskRecord_BuiltInLanguage = (STRPTR)"english";
LONG HardDiskRecord_Version = 0;

STRPTR HardDiskRecord_Strings[] =
{
  (STRPTR)"Please select an audio mode!",
  (STRPTR)"File has not been prepared yet!",
  (STRPTR)"OK",
  (STRPTR)"Can't allocate audio hardware.",
  (STRPTR)"Unknown error.",
  (STRPTR)"Finished!",
  (STRPTR)"(seconds)",
  (STRPTR)"IFF-AIFF",
  (STRPTR)"IFF-AIFC (Not compr.)",
  (STRPTR)"%ld %%",
  (STRPTR)"%ld %%",
  (STRPTR)"%ld kB",
  (STRPTR)"Source",
  (STRPTR)"Loopback dest.",
  (STRPTR)"Loopback volume",
  (STRPTR)"Select audio mode...",
  (STRPTR)"",
  (STRPTR)"",
  (STRPTR)"Input gain",
  (STRPTR)"Sample duration",
  (STRPTR)"Sample length",
  (STRPTR)"",
  (STRPTR)"File name...",
  (STRPTR)"Prepare sample file",
  (STRPTR)"Begin recording",
  (STRPTR)"seconds left",
  (STRPTR)"File format",
  (STRPTR)"AHI Hard disk record example",
  (STRPTR)"HardDiskRecord",
};

void RendWindowWin0( struct Window *Win, void *vi )
{
int loop;
UWORD offx,offy;
ULONG scalex,scaley;
scalex = 65535*Win->WScreen->RastPort.Font->tf_XSize/8;
scaley = 65535*Win->WScreen->RastPort.Font->tf_YSize/8;
offx = Win->BorderLeft;
offy = Win->BorderTop;
if (Win != NULL) 
	{
	BevelTags[3] = (ULONG)vi;
	DrawBevelBoxA( Win->RPort, 8*scalex/65535+offx,4*scaley/65535+offy,579*scalex/65535,85*scaley/65535, (struct TagItem *)(&BevelTags[0]));
	DrawBevelBoxA( Win->RPort, 8*scalex/65535+offx,93*scaley/65535+offy,579*scalex/65535,72*scaley/65535, (struct TagItem *)(&BevelTags[0]));
	DrawBevelBoxA( Win->RPort, 312*scalex/65535+offx,24*scaley/65535+offy,267*scalex/65535,28*scaley/65535, (struct TagItem *)(&BevelTags[0]));
	for( loop=0; loop<1; loop++)
		{
		if (Win0Texts[loop].ITextFont==NULL)
			Win0Texts[loop].ITextFont=Win->WScreen->Font;
		if (Win0FirstScaleTexts==0)
			{
			Win0Texts[loop].LeftEdge = Win0Texts[loop].LeftEdge*scalex/65535;
			Win0Texts[loop].TopEdge = Win0Texts[loop].TopEdge*scaley/65535;
			}
		}
	Win0FirstScaleTexts = 1;
	if (Win0TextsLocalized == 0)
		{
		Win0TextsLocalized = 1;
		for( loop=0; loop<1; loop++)
			Win0Texts[loop].IText = GetString((LONG)Win0Texts[loop].IText);
		}
	PrintIText( Win->RPort, Win0Texts, offx, offy);
	}
}

int OpenWindowWin0( STRPTR ScrName)
{
struct Screen *Scr;
UWORD offx, offy;
UWORD loop;
struct NewGadget newgad;
struct Gadget *Gad;
struct Gadget *Gad2;
APTR Cla;
ULONG scalex,scaley;
if (Win0FirstRun == 0)
	{
	Win0FirstRun = 1;
	for ( loop=0; loop<2; loop++)
		Win0_formatLabels[loop] = GetString((LONG)Win0_formatLabels[loop]);
	Win0GadgetTags[15] = (ULONG)GetString(Win0_volsliderLevelFormat);
	Win0GadgetTags[47] = (ULONG)GetString(Win0_gainsliderLevelFormat);
	Win0GadgetTags[69] = (ULONG)GetString(Win0_lengthStringFormat);
	}
if (Win0 == NULL)
	{
	Scr = LockPubScreen(ScrName);
	if (NULL == Scr)
	Scr = LockPubScreen(NULL);
	if (NULL != Scr)
		{
		offx = Scr->WBorLeft;
		offy = Scr->WBorTop + Scr->Font->ta_YSize+1;
		scalex = 65535*Scr->RastPort.Font->tf_XSize/8;
		scaley = 65535*Scr->RastPort.Font->tf_YSize/8;
		if (NULL != ( Win0VisualInfo = GetVisualInfoA( Scr, NULL)))
			{
			if (NULL != ( Win0DrawInfo = GetScreenDrawInfo( Scr)))
				{
				Win0GList = NULL;
				Gad = CreateContext( &Win0GList);
				for ( loop=0 ; loop<15 ; loop++ )
					if (Win0GadgetTypes[loop] != 198)
						{
						CopyMem((char * )&Win0NewGadgets[loop], ( char * )&newgad, (long)sizeof( struct NewGadget ));
						newgad.ng_VisualInfo = Win0VisualInfo;
						newgad.ng_LeftEdge = newgad.ng_LeftEdge*scalex/65535;
						newgad.ng_TopEdge = newgad.ng_TopEdge*scaley/65535;
						if (Win0GadgetTypes[loop] != GENERIC_KIND)
							{
							newgad.ng_Width = newgad.ng_Width*scalex/65535;
							newgad.ng_Height = newgad.ng_Height*scaley/65535;
							};
						newgad.ng_TextAttr = Scr->Font;
						newgad.ng_LeftEdge += offx;
						newgad.ng_TopEdge += offy;
						if ( newgad.ng_GadgetText != (UBYTE *)~0)
							newgad.ng_GadgetText = GetString((LONG)newgad.ng_GadgetText);
						else
							newgad.ng_GadgetText = (UBYTE *)0;
						Win0Gadgets[ loop ] = NULL;
						Win0Gadgets[ newgad.ng_GadgetID - Win0FirstID ] = Gad = CreateGadgetA( Win0GadgetTypes[loop], Gad, &newgad, (struct TagItem *) newgad.ng_UserData );
						}
				for ( loop=0 ; loop<15 ; loop++ )
					if (Win0GadgetTypes[loop] == 198)
						{
						Win0Gadgets[ loop ] = NULL;
						Cla = NULL;
						if (Gad)
							Win0Gadgets[ loop ] = Gad2 = (struct Gadget *) NewObjectA( (struct IClass *)Cla, Win0NewGadgets[ loop ].ng_GadgetText, (struct TagItem *)Win0NewGadgets[ loop ].ng_UserData );
						}
				if (Gad != NULL)
					{
					if (NULL != (Win0 = OpenWindowTags( NULL, (WA_Left), 7,
									(WA_Top), 18,
									(WA_InnerWidth), 595*scalex/65535,
									(WA_InnerHeight), 169*scaley/65535,
									(WA_Title), (LONG)GetString(Win0Title),
									(WA_ScreenTitle), (LONG)GetString(Win0ScreenTitle),
									(WA_MinWidth), 10,
									(WA_MinHeight), 2,
									(WA_MaxWidth), 1200,
									(WA_MaxHeight), 1200,
									(WA_DragBar), TRUE,
									(WA_DepthGadget), TRUE,
									(WA_CloseGadget), TRUE,
									(WA_Activate), TRUE,
									(WA_Dummy+0x30), TRUE,
									(WA_SmartRefresh), TRUE,
									(WA_AutoAdjust), TRUE,
									(WA_Gadgets), Win0GList,
									(WA_PubScreen) , (LONG)Scr,
									(WA_IDCMP),6292092,
									(TAG_END))))
						{
						RendWindowWin0(Win0, Win0VisualInfo );
						GT_RefreshWindow( Win0, NULL);
						RefreshGList( Win0GList, Win0, NULL, ~0);
						UnlockPubScreen( NULL, Scr);
						return( 0L );
						}
					}
				FreeGadgets( Win0GList);
				FreeScreenDrawInfo( Scr, (struct DrawInfo *) Win0DrawInfo );
				}
			FreeVisualInfo( Win0VisualInfo );
			}
		UnlockPubScreen( NULL, Scr);
		}
	}
else
	{
	WindowToFront(Win0);
	ActivateWindow(Win0);
	return( 0L );
	}
return( 1L );
}

void CloseWindowWin0( void )
{
if (Win0 != NULL)
	{
	FreeScreenDrawInfo( Win0->WScreen, (struct DrawInfo *) Win0DrawInfo );
	Win0DrawInfo = NULL;
	CloseWindow( Win0);
	Win0 = NULL;
	FreeVisualInfo( Win0VisualInfo);
	FreeGadgets( Win0GList);
	}
}

int OpenLibs( void )
{
LocaleBase = (struct LocaleBase * )OpenLibrary((UBYTE *)"locale.library", 38);
if ( NULL != (AslBase = OpenLibrary((UBYTE *)"asl.library" , 37)))
	if ( NULL != (DiskfontBase = OpenLibrary((UBYTE *)"diskfont.library" , 36)))
		if ( NULL != (GadToolsBase = OpenLibrary((UBYTE *)"gadtools.library" , 37)))
			if ( NULL != (GfxBase = (struct GfxBase * )OpenLibrary((UBYTE *)"graphics.library" , 37)))
				if ( NULL != (IntuitionBase = (struct IntuitionBase * )OpenLibrary((UBYTE *)"intuition.library" , 37)))
					if ( NULL != (UtilityBase = OpenLibrary((UBYTE *)"utility.library" , 37)))
						return( 0L );
CloseLibs();
return( 1L );
}

void CloseLibs( void )
{
if (NULL != AslBase )
	CloseLibrary( AslBase );
if (NULL != DiskfontBase )
	CloseLibrary( DiskfontBase );
if (NULL != GadToolsBase )
	CloseLibrary( GadToolsBase );
if (NULL != GfxBase )
	CloseLibrary( ( struct Library * )GfxBase );
if (NULL != IntuitionBase )
	CloseLibrary( ( struct Library * )IntuitionBase );
if (NULL != UtilityBase )
	CloseLibrary( UtilityBase );
if (NULL != LocaleBase )
	CloseLibrary( ( struct Library * )LocaleBase );
}

STRPTR GetString(LONG strnum)
{
	if (HardDiskRecord_Catalog == NULL)
		return(HardDiskRecord_Strings[strnum]);
	return(GetCatalogStr(HardDiskRecord_Catalog, strnum, HardDiskRecord_Strings[strnum]));
}

void CloseHardDiskRecordCatalog(void)
{
	if (LocaleBase != NULL)
		CloseCatalog(HardDiskRecord_Catalog);
	HardDiskRecord_Catalog = NULL;
}

void OpenHardDiskRecordCatalog(struct Locale *loc, STRPTR language)
{
	LONG tag, tagarg;
	if (language == NULL)
		tag=TAG_IGNORE;
	else
		{
		tag = OC_Language;
		tagarg = (LONG)language;
		}
	if (LocaleBase != NULL  &&  HardDiskRecord_Catalog == NULL)
		HardDiskRecord_Catalog = OpenCatalog(loc, (STRPTR) "HardDiskRecord.catalog",
											OC_BuiltInLanguage, HardDiskRecord_BuiltInLanguage,
											tag, tagarg,
											OC_Version, HardDiskRecord_Version,
											TAG_DONE);
}

