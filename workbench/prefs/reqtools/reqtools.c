/********************************************
*                                           *
*  (C) Copyright 92-94 by Nico François     *
*                95-96 by Magnus Holmgren   *
*                                           *
********************************************/

#include <exec/execbase.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/utility.h>


#include <proto/reqtools.h>
#include <proto/wb.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>

#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <dos/stdio.h>
#include <dos/dosextens.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfxbase.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <libraries/reqtools.h>
#include <libraries/locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "misc.h"

#include "reqtools_rev.h"

#ifdef __AROS__
#include <aros/macros.h>
#include <proto/alib.h>
#define __exit exit
#define __main was__main
#define __stdargs
#define _WBenchMsg WBenchMsg
#endif

const char VersTag[] = VERSTAG;

#define CONFIGFILE        	"Env:ReqTools.prefs"
#define CONFIGFILE_ARC    	"EnvArc:ReqTools.prefs"

#ifndef __AROS__
#pragma libcall ReqToolsBase rtLockPrefs   a8 00
#pragma libcall ReqToolsBase rtUnlockPrefs ae 00
#endif

#define STRGADBUFF(x)		((struct StringInfo *)(x->SpecialInfo))->Buffer
#define INTGADVALUE(x)		((struct StringInfo *)(x->SpecialInfo))->LongInt


struct Library *ConsoleDevice;

struct MenuItem *iconitem;

/* Global stuff */

struct Library        	*GadToolsBase;
struct Library        	*IconBase;
struct GfxBase       	*GfxBase;
struct IntuitionBase    *IntuitionBase;
struct ReqToolsBase    	*ReqToolsBase;
#ifdef __AROS__
struct UtilityBase    	*UtilityBase;
#else	
struct Library        	*UtilityBase;
#endif
struct Window       	*WindowPtr;
struct Screen        	*Screen;
struct DrawInfo        	*DrawInfo;
struct Menu        	*Menus;
APTR            	VisualInfo;
UWORD    		Zoom[ 4 ];

/* Local stuff */

TEXT    		File[ 256 ];

struct ReqToolsPrefs    RTPrefs;
struct ReqToolsPrefs    DefaultPrefs;
struct ReqDefaults    	*ReqDefs;

#define PREFSLEN    	RTPREFS_SIZE

WORD    		CurrentReq, WheelType;
BOOL    		CreateIcons, UseScreenFont = TRUE;

struct rtFileRequester  *FileReq;

struct RDArgs        	*RDArgs;
struct DiskObject    	*DiskObject;

struct Hook        	IntuiHook;


struct NewMenu NewMenu[] =
{
    { NM_TITLE, MSG_PROJECT_MENU,        NULL, 0, 0, 0 },
    { NM_ITEM,  MSG_PROJECT_OPEN,        NULL, 0, 0, 0 },
    { NM_ITEM,  MSG_PROJECT_SAVEAS,      NULL, 0, 0, 0 },
    { NM_ITEM,  NM_BARLABEL,             NULL, 0, 0, 0 },
    { NM_ITEM,  MSG_PROJECT_ABOUT,       NULL, 0, 0, 0 },
    { NM_ITEM,  NM_BARLABEL,             NULL, 0, 0, 0 },
    { NM_ITEM,  MSG_PROJECT_QUIT,        NULL, 0, 0, 0 },
    { NM_TITLE, MSG_EDIT_MENU,           NULL, 0, 0, 0 },
    { NM_ITEM,  MSG_EDIT_RESET,          NULL, 0, 0, 0 },
    { NM_ITEM,  MSG_EDIT_LASTSAVED,      NULL, 0, 0, 0 },
    { NM_ITEM,  MSG_EDIT_RESTORE,        NULL, 0, 0, 0 },
    { NM_TITLE, MSG_OPTIONS_MENU,        NULL, 0, 0, 0 },
    { NM_ITEM,  MSG_OPTIONS_CREATEICONS, NULL, CHECKED|CHECKIT|MENUTOGGLE, 0, 0 },
    { NM_END,   NULL,                    NULL, 0, 0, 0 }
};

#define MENU_PROJECT    	0
#define MENU_EDIT    		1
#define MENU_OPTIONS    	2

#define ITEM_OPEN    		FULLMENUNUM( MENU_PROJECT, 0, NOSUB )
#define ITEM_SAVEAS    		FULLMENUNUM( MENU_PROJECT, 1, NOSUB )
#define ITEM_ABOUT    		FULLMENUNUM( MENU_PROJECT, 3, NOSUB )
#define ITEM_QUIT		FULLMENUNUM( MENU_PROJECT, 5, NOSUB )

#define ITEM_RESET    		FULLMENUNUM( MENU_EDIT, 0, NOSUB )
#define ITEM_LASTSAVED  	FULLMENUNUM( MENU_EDIT, 1, NOSUB )
#define ITEM_RESTORE    	FULLMENUNUM( MENU_EDIT, 2, NOSUB )

#define ITEM_CREATEICONS    	FULLMENUNUM( MENU_OPTIONS, 0, NOSUB )


/*******
* MISC *
*******/


#define CloseLib(lib)    	CloseLibrary( ( struct Library * ) lib )

VOID
FreeExit( LONG rc )
{
    if( Menus )
    {
        if( WindowPtr )
        {
            ClearMenuStrip( WindowPtr );
        }

        FreeMenus( Menus );
    }

    CloseGUI();

    if( Screen )
    {
        FreeScreenDrawInfo( Screen, DrawInfo );
        UnlockPubScreen (NULL, Screen );
    }

    if( GadToolsBase )
    {
        FreeVisualInfo( VisualInfo );
    }

    if( FileReq )
    {
        rtFreeRequest( FileReq );
    }

    if( DiskObject )
    {
        FreeDiskObject( DiskObject );
    }

    FreeArgs( RDArgs );
    FreeLocale();

    CloseLib( GadToolsBase );
    CloseLib( IconBase );
    CloseLib( IntuitionBase );
    CloseLib( ReqToolsBase );
    CloseLib( UtilityBase );

    CloseLibrary( ( struct Library * ) GfxBase );
    __exit( rc );
}

/*******
* MAIN *
*******/

VOID    WriteErr( VOID );
VOID    OpenFile( VOID );
VOID    SaveAs( VOID );
VOID    CreateIcon( STRPTR );
LONG    GetFilename( STRPTR, STRPTR, ULONG );
LONG    LoadConfig( STRPTR );
LONG    SaveConfig( STRPTR );


struct Args
{
    STRPTR    From;
    STRPTR    ScreenFont;
    STRPTR    PubScreen;
};


#define TEMPLATE    "FROM,SCREENFONT/K,PUBSCREEN/K"

extern struct WBStartup *_WBenchMsg;


WORD
GetWheelType( ULONG flags )
{
    WORD    type = 0;

    if( flags & RTPRF_DOWHEEL )
    {
        type = 1;

        if( flags & RTPRF_FANCYWHEEL )
        {
            type = 2;
        }
    }

    return( type );
}


ULONG
SetWheelType( WORD type )
{
    switch( type )
    {
        case 1:
            return( RTPRF_DOWHEEL );

        case 2:
            return( RTPRF_DOWHEEL | RTPRF_FANCYWHEEL );
    }

    return( 0 );
}


APTR
OpenLib( STRPTR name, LONG ver )
{
    APTR    lib;

    if( !( lib = OpenLibrary( name, ver ) ) )
    {
        LocEZReq( MSG_ERROR_LIBRARY, MSG_ABORT, name, ver );
        FreeExit( RETURN_FAIL );
    }

    return( lib );
}


VOID __stdargs
__main( char *argstring )
{
    static struct Args     args;
    LONG    rev;

    /* Get arguments if started from CLI */
    if( !_WBenchMsg )
    {
        if( !( RDArgs = ReadArgs( TEMPLATE, ( LONG * ) &args, NULL ) ) )
        {
            PrintFault( IoErr(), GetString( MSG_ERROR_ARGS ) );
            __exit( 0 );
        }
    }

    IntuiHook.h_Entry = ( HOOKFUNC ) IntuiMsgFunc;

//    DefaultPrefs.Flags = 0;
    DefaultPrefs.ReqDefaults[ RTPREF_OTHERREQ      ].ReqPos = REQPOS_POINTER;
    DefaultPrefs.ReqDefaults[ RTPREF_FILEREQ       ].Size = 75;
    DefaultPrefs.ReqDefaults[ RTPREF_FONTREQ       ].Size =
    DefaultPrefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].Size =
    DefaultPrefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].Size = 65;
    DefaultPrefs.ReqDefaults[ RTPREF_FILEREQ       ].ReqPos =
    DefaultPrefs.ReqDefaults[ RTPREF_FONTREQ       ].ReqPos =
    DefaultPrefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].ReqPos =
    DefaultPrefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].ReqPos =
    DefaultPrefs.ReqDefaults[ RTPREF_PALETTEREQ    ].ReqPos = REQPOS_TOPLEFTSCR;
    DefaultPrefs.ReqDefaults[ RTPREF_FILEREQ       ].LeftOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_FONTREQ       ].LeftOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].LeftOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].LeftOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_PALETTEREQ    ].LeftOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_OTHERREQ      ].LeftOffset = 25;
    DefaultPrefs.ReqDefaults[ RTPREF_FILEREQ       ].TopOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_FONTREQ       ].TopOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].TopOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].TopOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_PALETTEREQ    ].TopOffset =
    DefaultPrefs.ReqDefaults[ RTPREF_OTHERREQ      ].TopOffset = 18;
    DefaultPrefs.ReqDefaults[ RTPREF_FILEREQ       ].MinEntries = 10;
    DefaultPrefs.ReqDefaults[ RTPREF_FONTREQ       ].MinEntries =
    DefaultPrefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].MinEntries =
    DefaultPrefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].MinEntries = 6;
    DefaultPrefs.ReqDefaults[ RTPREF_FILEREQ       ].MaxEntries = 50;
    DefaultPrefs.ReqDefaults[ RTPREF_FONTREQ       ].MaxEntries =
    DefaultPrefs.ReqDefaults[ RTPREF_SCREENMODEREQ ].MaxEntries =
    DefaultPrefs.ReqDefaults[ RTPREF_VOLUMEREQ     ].MaxEntries = 10;

    if( !( IntuitionBase = ( struct IntuitionBase * ) OpenLibrary( "intuition.library", 37 ) ) )
    {
        BPTR    con;

        if( ( con = Open( "CON:40/20/320/40/ReqTools 2.8", MODE_NEWFILE ) ) )
        {
            Write( con, "\nNeed OS 2.04 or better!\n", 25 );
            Delay( 120L );
            Close( con );
        }

        FreeExit( 0 );
    }

    InitLocale();
    GfxBase      = OpenLib( "graphics.library", 37 );
    UtilityBase  = OpenLib( "utility.library", 36 );
    IconBase     = OpenLib( "icon.library", 0 );
    GadToolsBase = OpenLib( "gadtools.library", 37 );
    ReqToolsBase = OpenLib( "reqtools.library", 38 );

    rev = ReqToolsBase->LibNode.lib_Revision;

    if( ( rev >= 693 && rev <= 811 ) || ( rev >= 347 && rev <= 363 ) )
    {
        LocEZReq( MSG_WRONG_REQTOOLS_VERSION, MSG_ABORT );
        FreeExit( RETURN_FAIL );
    }

    if( rtLockPrefs()->PrefsSize != PREFSLEN )
    {
        LocEZReq( MSG_ALL_PREFS_NOT_SUPPORTED, MSG_OK );
    }

    rtUnlockPrefs();

    if( _WBenchMsg )
    {
        struct WBArg    *wbarg;
        BPTR    oldcd;

        CreateIcons = TRUE;
        wbarg = &_WBenchMsg->sm_ArgList[ 0 ];
        oldcd = CurrentDir( wbarg->wa_Lock );

        if( ( DiskObject = GetDiskObject( wbarg->wa_Name ) ) )
        {
            STRPTR    str;

            if( ( str = FindToolType( (UBYTE **)DiskObject->do_ToolTypes, "CREATEICONS" ) ) )
            {
                CreateIcons = Stricmp( str, "NO" );
            }

            if( ( str = FindToolType( (UBYTE **)DiskObject->do_ToolTypes, "SCREENFONT" ) ) )
            {
                UseScreenFont = Stricmp( str, "NO" );
            }

            if( ( str = FindToolType( (UBYTE **)DiskObject->do_ToolTypes, "PUBSCREEN" ) ) )
            {
                args.PubScreen = str;
            }
        }

        CurrentDir( oldcd );
    }
    else
    {
        if( args.ScreenFont )
        {
            UseScreenFont = Stricmp( args.ScreenFont, "NO" );
        }
    }

    if( !( FileReq = rtAllocRequestA( RT_FILEREQ, NULL ) ) )
    {
        FreeExit( RETURN_FAIL );
    }

        {
        struct TagItem tags[] =
        {
            {RTFI_Dir    , (IPTR)"Presets"    },
            {TAG_DONE                }
        };
        
        rtChangeReqAttrA( FileReq, tags );
    }
    
    /* Get current prefs from ReqTools */
    CopyMem( rtLockPrefs(), &RTPrefs, sizeof( struct ReqToolsPrefs ) );
    rtUnlockPrefs();

    /* If FROM was used load prefs from disk */
    if( args.From )
    {
        if( !LoadConfig( args.From ) )
        {
            FreeExit( RETURN_ERROR );
        }
    }

    WheelType = GetWheelType( RTPrefs.Flags );

    if( !( Screen = LockPubScreen( args.PubScreen ) ) )
    {
        LocEZReq( MSG_COULDNT_LOCK_PUBSCREEN, MSG_ABORT );
        FreeExit( RETURN_ERROR );
    }

    if( !( DrawInfo = GetScreenDrawInfo( Screen ) ) )
    {
        LocEZReq( MSG_ERROR_GETSCREENDRAWINFO, MSG_ABORT );
        FreeExit( RETURN_ERROR );
    }

    if( !( VisualInfo = GetVisualInfoA( Screen, NULL ) ) )
    {
        LocEZReq( MSG_ERROR_GETVISUALINFO, MSG_ABORT );
        FreeExit( RETURN_FAIL );
    }

    if( IntuitionBase->LibNode.lib_Version >= 39 )
    {
        Zoom[ 0 ] = Zoom[ 1 ] = 65535;
    }
    else
    {
        Zoom[ 1 ] = Screen->BarHeight + 1;
    }

    Zoom[ 2 ] = 250;
    Zoom[ 3 ] = Screen->WBorTop + Screen->Font->ta_YSize + 1;
    LocalizeMenus( NewMenu );

    if( !( Menus = CreateMenusA( NewMenu, NULL ) ) )
    {
        LocEZReq( MSG_ERROR_MENUS, MSG_ABORT );
        FreeExit( RETURN_FAIL );
    }

    LayoutMenus( Menus, VisualInfo,
        GTMN_NewLookMenus,    TRUE,
    TAG_END );

    if( !OpenGUI() )
    {
        LocEZReq( MSG_COULDNT_OPEN_WINDOW, MSG_ABORT );
        FreeExit( RETURN_FAIL );
    }


    {
        struct MenuItem    *iconItem;

        iconItem = ItemAddress( Menus, FULLMENUNUM( OPTIONS_MENU, SAVEICONS_ITEM, NOSUB ) );

        if( !CreateIcons )
        {
            iconItem->Flags &= ~CHECKED;
        }
    }

    CurrentReq = RTPREF_FILEREQ;
    ReqDefs = &RTPrefs.ReqDefaults[ CurrentReq ];
    LoopGUI();
    FreeExit( 0 );
}


BOOL
ProcessGadget( UWORD id, UWORD code )
{
    BOOL    run = TRUE;
    LONG    val;

    switch( id )
    {
        case NOSCRTOFRONT_GADID:
            RTPrefs.Flags ^= RTPRF_NOSCRTOFRONT;
            break;

        case IMMSORT_GADID:
            RTPrefs.Flags ^= RTPRF_IMMSORT;
            break;

        case DIRSFIRST_GADID:
            RTPrefs.Flags ^= RTPRF_DIRSFIRST;

            if( RTPrefs.Flags & RTPRF_DIRSMIXED )
            {
                SetCheckState( mixdirsgad, FALSE );
                RTPrefs.Flags &= ~RTPRF_DIRSMIXED;
            }

            break;

        case DIRSMIXED_GADID:
            RTPrefs.Flags ^= RTPRF_DIRSMIXED;

            if( RTPrefs.Flags & RTPRF_DIRSFIRST )
            {
                SetCheckState( dirsfirstgad, FALSE );
                RTPrefs.Flags &= ~RTPRF_DIRSFIRST;
            }

            break;

        case NOLED_GADID:
            RTPrefs.Flags ^= RTPRF_NOLED;
            break;

        case MMB_GADID:
            RTPrefs.Flags ^= RTPRF_MMBPARENT;
            break;

        case DEFAULTFONT_GADID:
            RTPrefs.Flags ^= RTPRF_DEFAULTFONT;
            break;

        case DOWHEEL_GADID:
            /* First clear all bits */
            RTPrefs.Flags &= ~( RTPRF_DOWHEEL | RTPRF_FANCYWHEEL );
            /* Then set appropriate ones */
            RTPrefs.Flags |= SetWheelType( WheelType = code );
            break;

        case FKEYS_GADID:
            RTPrefs.Flags ^= RTPRF_FKEYS;
            break;

        case REQTYPE_GADID:
            if( CurrentReq != code )
            {
                CurrentReq = code;
                UpdatePrefsWindow( TRUE );
                ReqDefs = &RTPrefs.ReqDefaults[ CurrentReq ];
            }

            break;

        case DEFSIZE_GADID:
            ReqDefs->Size = code;
            break;

        case MINENTRIES_GADID:
            val = IntGadValue( mingad );

            if( val < 3 )
            {
                val = 3;
            }

            if( val > ReqDefs->MaxEntries )
            {
                val = ReqDefs->MaxEntries;
            }

            if( val != IntGadValue( mingad ) )
            {
                SetIntGad( mingad, val );
            }

            ReqDefs->MinEntries = val;
            break;

        case MAXENTRIES_GADID:
            val = IntGadValue( maxgad );

            if( val > 50 )
            {
                val = 50;
            }

            if( val < ReqDefs->MinEntries )
            {
                val = ReqDefs->MinEntries;
            }

            if( val != IntGadValue( maxgad ) )
            {
                SetIntGad( maxgad, val );
            }

            ReqDefs->MaxEntries = val;
            break;

        case POSITION_GADID:
            if( ReqDefs->ReqPos != code )
            {
                ReqDefs->ReqPos = code;

                if( code <= REQPOS_CENTERSCR )
                {
                    GadgetOff( xoffgad );
                    GadgetOff( yoffgad );
                }
                else
                {
                    GadgetOn( xoffgad );
                    GadgetOn( yoffgad );
                }
            }

            break;

        case OFFSETX_GADID:
            ReqDefs->LeftOffset = IntGadValue( xoffgad );
            break;

        case OFFSETY_GADID:
            ReqDefs->TopOffset = IntGadValue( yoffgad );
            break;

        case SAVE_GADID:
            SaveConfig( CONFIGFILE_ARC );
            /* FALLTHROUGH! */

        case USE_GADID:
            {
                struct ReqToolsPrefs    *prefs;

                SaveConfig( CONFIGFILE );
                prefs = rtLockPrefs();
                CopyMem( &RTPrefs.Flags, &prefs->Flags, PREFSLEN );
                rtUnlockPrefs();
            }

            /* FALLTHROUGH! */

        case CANCEL_GADID:
            run = FALSE;
            break;
    }

    return( run );
}


BOOL
ProcessMenuItem( UWORD id )
{
    BOOL    run = TRUE;

    switch( id )
    {
        case ITEM_LASTSAVED:
        case ITEM_OPEN:
            if( id == ITEM_LASTSAVED )
            {
                if( !LoadConfig( CONFIGFILE_ARC ) )
                {
                    run = FALSE;
                }
            }
            else
            {
                OpenFile();
            }

            UpdatePrefsWindow( FALSE );
            break;

        case ITEM_SAVEAS:
            SaveAs();
            break;

        case ITEM_ABOUT:
            {
                struct TagItem tags[] =
                {
                    {RT_LockWindow    , TRUE            },
                {RT_Window    , (IPTR)WindowPtr    },
                {RT_ShareIDCMP    , TRUE            },
                {RT_IntuiMsgFunc, (IPTR)&IntuiHook    },
                {RTEZ_Flags    , EZREQF_CENTERTEXT    },
                {TAG_DONE                }
                };
                
                rtEZRequestA(
                "ReqTools Preferences 2.8\n"
                "\n"
                "Copyright © 1992-1994 Nico François\n"
                "            1995-1996 Magnus Holmgren\n"
                "(Compilation Date: " DATE ")",
                GetString( MSG_OK ), NULL, NULL, tags);
            }    
            break;

        case ITEM_QUIT:
            run = FALSE;
            break;

        case ITEM_RESET:
            CopyMem( &DefaultPrefs, &RTPrefs, sizeof( struct ReqToolsPrefs ) );
            UpdatePrefsWindow( FALSE );
            break;

        case ITEM_RESTORE:
            CopyMem( rtLockPrefs(), &RTPrefs, sizeof( struct ReqToolsPrefs ) );
            rtUnlockPrefs();
            UpdatePrefsWindow( FALSE );
            break;

        case ITEM_CREATEICONS:
            CreateIcons = !CreateIcons;
            break;
    }

    return( run );
}


/************
* LOAD/SAVE *
************/

TEXT    FileName[ 108 ];

ULONG RTTags[] =
{
    RTFI_Flags,        0,
    RT_Window,        0,
    RT_LockWindow,        TRUE,
    RT_ShareIDCMP,        TRUE,
    RT_IntuiMsgFunc,     ( ULONG ) &IntuiHook,
    TAG_END
};

#define TAG_FLAGS    1
#define TAG_WINDOW    3


LONG
GetFilename( STRPTR file, STRPTR hail, ULONG flags )
{
    RTTags[ TAG_FLAGS  ] = flags;
    RTTags[ TAG_WINDOW ] = ( ULONG ) WindowPtr;

    if( rtFileRequestA( FileReq, FileName, hail, ( struct TagItem * ) RTTags ) )
    {
        strcpy( file, FileReq->Dir );
        return( ( LONG ) AddPart( file, FileName, 256 ) );
    }

    return( 0 );
}


VOID
OpenFile( VOID )
{
    if( GetFilename( File, GetString( MSG_PROJECT_OPEN ) + 2, 0 ) )
    {
        if( !LoadConfig( File ) )
        {
            FreeExit( RETURN_ERROR );
        }
    }
}


VOID
SaveAs( VOID )
{
    if( GetFilename( File, GetString( MSG_PROJECT_SAVEAS ) + 2, FREQF_SAVE ) )
    {
        if( SaveConfig( File ) )
        {
            if( CreateIcons )
            {
                CreateIcon( File );
            }
        }
    }
}

TEXT    FaultBuff[ 100 ];

#ifdef __AROS__
#define	LOADSAVETO configbuffer
UBYTE	configbuffer[PREFSLEN];
#else
#define LOADSAVETO &RTPrefs.Flags
#endif

LONG
LoadConfig( STRPTR fname )
{
    BPTR    file;

    if( !( file = Open( fname, MODE_OLDFILE ) ) )
    {
        Fault( IoErr(), "", FaultBuff, sizeof( FaultBuff ) );
        LocEZReq( MSG_ERROR_ACCESSING_FILE, MSG_OK, fname, FaultBuff + 2 );
        return( TRUE );
    }

    if( Read( file, LOADSAVETO, PREFSLEN ) != PREFSLEN )
    {
        LocEZReq( MSG_READ_ERROR, MSG_ABORT );
        Close( file );
        return( FALSE );
    }
#ifdef __AROS__
#define READ_ULONG *((ULONG *)configptr); configptr += sizeof(ULONG)
#define READ_UWORD *((UWORD *)configptr); configptr += sizeof(UWORD)

    {
        UBYTE *configptr = configbuffer;
	ULONG val;
	WORD  i;
	
	val = READ_ULONG;
	RTPrefs.Flags = AROS_LONG2BE(val);
	
	for(i = 0;i < RTPREF_NR_OF_REQ; i++)
	{
	    val = READ_ULONG;
	    RTPrefs.ReqDefaults[i].Size = AROS_LONG2BE(val);
	    
	    val = READ_ULONG;
	    RTPrefs.ReqDefaults[i].ReqPos = AROS_LONG2BE(val);
	    
	    val = READ_UWORD;
	    RTPrefs.ReqDefaults[i].LeftOffset = AROS_WORD2BE(val);

	    val = READ_UWORD;
	    RTPrefs.ReqDefaults[i].TopOffset = AROS_WORD2BE(val);

	    val = READ_UWORD;
	    RTPrefs.ReqDefaults[i].MinEntries = AROS_WORD2BE(val);

	    val = READ_UWORD;
	    RTPrefs.ReqDefaults[i].MaxEntries = AROS_WORD2BE(val);	    
	}
	
    }
#endif

    Close( file );
    WheelType = GetWheelType( RTPrefs.Flags );
    return( TRUE );
}


LONG
SaveConfig( STRPTR fname )
{
    BPTR    file;

    if( !( file = Open( fname, MODE_NEWFILE ) ) )
    {
        Fault( IoErr(), "", FaultBuff, sizeof( FaultBuff ) );
        LocEZReq( MSG_ERROR_ACCESSING_FILE, MSG_OK, fname, FaultBuff + 2 );
        return( FALSE );
    }

#ifdef __AROS__
#define WRITE_ULONG *((ULONG *)configptr) = AROS_LONG2BE(val); configptr += sizeof(ULONG)
#define WRITE_UWORD *((UWORD *)configptr) = AROS_WORD2BE(val); configptr += sizeof(UWORD)

    {
        UBYTE *configptr = configbuffer;
	ULONG val;
	WORD  i;
	
	val = RTPrefs.Flags;
	WRITE_ULONG;
	
	for(i = 0;i < RTPREF_NR_OF_REQ; i++)
	{
	    val = RTPrefs.ReqDefaults[i].Size;
	    WRITE_ULONG;
	    
	    val = RTPrefs.ReqDefaults[i].ReqPos;
	    WRITE_ULONG;
	    
	    val = RTPrefs.ReqDefaults[i].LeftOffset;
	    WRITE_UWORD;
	    
	    val = RTPrefs.ReqDefaults[i].TopOffset;
	    WRITE_UWORD;
	    
	    val = RTPrefs.ReqDefaults[i].MinEntries;
	    WRITE_UWORD;
	    
	    val = RTPrefs.ReqDefaults[i].MaxEntries;
	    WRITE_UWORD;

	}
	
    }
#endif
    if( Write( file, LOADSAVETO, PREFSLEN ) != PREFSLEN )
    {
        LocEZReq( MSG_ERROR_SAVING_PREFS, MSG_OK );
        Close( file );
        return( FALSE );
    }

    Close( file );
    return( TRUE );
}


/*******
* ICON *
*******/


UWORD IconImageData[ 176 ] =
{
    0,0,4,1023,0,0,1,1023,0,2047,32768,17407,0,6144,24576,5119,0,8444,4096,3071,
    0,16642,2048,4095,0,16514,2048,4095,0,16514,2048,4095,0,8452,2048,4095,
    0,7704,4096,4095,0,96,8192,4095,0,128,49152,4095,0,259,0,4095,0,540,0,4095,
    0,264,0,4095,0,240,0,4095,0,264,0,4095,0,264,0,4095,16384,240,0,4095,
    4096,0,0,4095,1024,0,0,4095,511,65535,65535,65535,65535,65535,65528,1023,
    54613,21845,21846,1023,54613,20480,21845,33791,54613,18431,38229,25599,54613,
    24323,58709,21503,54613,15957,62805,21503,54613,16213,62805,21503,54613,
    16213,62805,21503,54613,24147,62805,21503,54613,16711,58709,21503,54613,
    21791,54613,21503,54613,21887,5461,21503,54613,21756,21845,21503,54613,
    21985,21845,21503,54613,21749,21845,21503,54613,21765,21845,21503,54613,
    21749,21845,21503,54613,21749,21845,21503,13653,21765,21845,21503,3413,
    21845,21845,21503,853,21845,21845,21503,0,0,0,1023
};

struct Image IconImage =
{
    0, 0, 54, 22, 2, IconImageData, 3, 0, NULL
};


STRPTR ToolTypes[] =
{
    "ACTION=USE",
    NULL
};


VOID
CreateIcon( STRPTR fname )
{
    struct DiskObject    *dob;
    struct Gadget        *gad;

    if( ( dob = GetDiskObjectNew( NULL ) ) )
    {
        gad = &dob->do_Gadget;
        gad->Width        = 54;
        gad->Height        = 22;
        gad->Flags        = GFLG_GADGIMAGE | GFLG_GADGBACKFILL;
        gad->GadgetType        = GTYP_BOOLGADGET;
        gad->GadgetRender    = ( APTR ) &IconImage;
        gad->UserData        = ( APTR ) WB_DISKREVISION;
        dob->do_Magic        = WB_DISKMAGIC;
        dob->do_Version        = WB_DISKVERSION;
        dob->do_Type        = WBPROJECT;
        dob->do_DefaultTool    = "ReqTools";
        dob->do_ToolTypes    = ToolTypes;
        dob->do_CurrentX    = dob->do_CurrentY = NO_ICON_POSITION;
        PutDiskObject( fname, dob );
        FreeDiskObject( dob );
    }
}

#ifdef __AROS__
int main(int argc, char **argv)
{
    __main(NULL);
    return 0;
}
#endif
