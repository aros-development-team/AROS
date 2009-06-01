#define MUI_OBSOLETE

#include <exec/memory.h>
#include <dos/dostags.h>
#include <libraries/mui.h>
#include <string.h>

#include <proto/muimaster.h>
#include <proto/workbench.h>

#include "interface.h"
#include "includes.h"
#include "libraries.h"
#include "protos/protos.h"

extern struct WBStartup *WBenchMsg;

Object *BlankersLvw, *PrefsBtn, *InfoBtn, *ToggleBtn, *HideBtn;
Object *SettingsBtn, *QuitBtn, *BlankWnd, *BlankApp;
struct Library *MUIMasterBase = 0L;
ULONG MUI_Sigs = 0L;
BYTE Title[128];

STRPTR SettingsFmt = "BLANKKEY=%s\nPOPKEY=%s\nTIMEOUT=%ld\nREPLACE=%s\n"
    "RANDTIMEOUT=%ld\nBLANKCORNER=%s\nDONTCORNER=%s";


AROS_UFH3(APTR, EntryConsFunc,
AROS_UFHA(struct Hook *, h, A0),
AROS_UFHA(Object *, dummy, A2),
AROS_UFHA(BlankerEntry *, Entry, A1))
{
    AROS_USERFUNC_INIT
    return Entry;
    AROS_USERFUNC_EXIT
}
struct Hook EntryConsHook = {{ 0L, 0L }, ( APTR )EntryConsFunc, 0L, 0L };


AROS_UFH3(VOID, EntryDestFunc,
AROS_UFHA(struct Hook *, h, A0),
AROS_UFHA(Object *, object, A2),
AROS_UFHA(BlankerEntry *, Entry, A1))
{
    AROS_USERFUNC_INIT
    AROS_USERFUNC_EXIT
}
struct Hook EntryDestHook = {{ 0L, 0L }, ( APTR )EntryDestFunc, 0L, 0L };

AROS_UFH3(LONG, EntryDispFunc,
AROS_UFHA(struct Hook *, h, A0),
AROS_UFHA(BYTE **, Array, A2),
AROS_UFHA(BlankerEntry *, Entry, A1))
{
    AROS_USERFUNC_INIT

    static BYTE Buffer[64];

    if( Entry->be_Disabled )
    {
        strcpy( Buffer, "(" );
        strcat( Buffer, Entry->be_Name );
        strcat( Buffer, ")" );
    }
    else
        strcpy( Buffer, Entry->be_Name );

    *Array = Buffer;

    return 0L;
    AROS_USERFUNC_EXIT
}
struct Hook EntryDispHook = {{ 0L, 0L }, ( APTR )EntryDispFunc, 0L, 0L };


ULONG ISigs( VOID )
{
    return MUI_Sigs;
}

LONG OpenInterface( VOID )
{
    if( MUIMasterBase )
        return OK;

    if(!( MUIMasterBase = OpenLibrary( MUIMASTER_NAME, MUIMASTER_VMIN )))
        return QUIT;

    strcpy( Title, "Garshneblanker ( PopKey=" );
    strcat( Title, Prefs->bp_PopKey );
    strcat( Title, ", BlankKey=" );
    strcat( Title, Prefs->bp_BlankKey );
    strcat( Title, " )" );

    BlankWnd = WindowObject,
        MUIA_Window_ID, MAKE_ID( 'M', 'A', 'I', 'N' ),
        MUIA_Window_ScreenTitle, Title,
        MUIA_Window_Title, FilePart( ProgName ),
        MUIA_Window_RootObject, VGroup,
            Child,
                BlankersLvw = ListviewObject,
                    MUIA_Listview_List, ListObject,
                        MUIA_Frame, MUIV_Frame_InputList,
                        MUIA_List_ConstructHook, &EntryConsHook,
                        MUIA_List_DestructHook, &EntryDestHook,
                        MUIA_List_DisplayHook, &EntryDispHook,
                    End,
                End,
            Child,
                HGroup,
                    Child, PrefsBtn = KeyButton( "Prefs", 'p' ),
                    Child, InfoBtn = KeyButton( "Info", 'i' ),
                    Child, ToggleBtn = KeyButton( "Toggle", 't' ),
                End,
            Child,
                HGroup,
                    Child, HideBtn = KeyButton( "Hide", 'h' ),
                    Child, SettingsBtn = KeyButton( "Settings", 's' ),
                    Child, QuitBtn = KeyButton( "Quit", 'q' ),
                End,
        End,
    End;

    BlankApp = ApplicationObject,
        MUIA_Application_Title, "Garshneblanker",
        MUIA_Application_Version, VERS,
        MUIA_Application_Copyright, "Free Software",
        MUIA_Application_Author, "Michael D. Bayne",
        MUIA_Application_Description, "Screen blanker",
        MUIA_Application_Base, "GBLANKER",
        MUIA_Application_Window, BlankWnd,
    End;

    if( BlankApp )
    {
        BlankerEntry *Tmp;
        LONG i, Selected, Rand;
        
#ifdef FUNKY_MUI
        DoMethod( BlankApp, MUIM_Application_Load,
            MUIV_Application_Load_ENVARC );
#endif
        DoMethod( BlankWnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            BlankApp, 2, MUIM_Application_ReturnID, ID_HIDE );
        DoMethod( BlankersLvw, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
            BlankApp, 2, MUIM_Application_ReturnID, ID_BLANKERS );
        DoMethod( BlankersLvw, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
            BlankApp, 2, MUIM_Application_ReturnID, ID_PREFS );
        DoMethod( PrefsBtn, MUIM_Notify, MUIA_Pressed, FALSE, BlankApp, 2,
            MUIM_Application_ReturnID, ID_PREFS );
        DoMethod( InfoBtn, MUIM_Notify, MUIA_Pressed, FALSE, BlankApp, 2,
            MUIM_Application_ReturnID, ID_INFO );
        DoMethod( ToggleBtn, MUIM_Notify, MUIA_Pressed, FALSE, BlankApp, 2,
            MUIM_Application_ReturnID, ID_TOGGLE );
        DoMethod( HideBtn, MUIM_Notify, MUIA_Pressed, FALSE, BlankApp, 2,
            MUIM_Application_ReturnID, ID_HIDE );
        DoMethod( SettingsBtn, MUIM_Notify, MUIA_Pressed, FALSE, BlankApp, 2,
            MUIM_Application_ReturnID, ID_SET );
        DoMethod( QuitBtn, MUIM_Notify, MUIA_Pressed, FALSE, BlankApp, 2,
            MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit );
        DoMethod( BlankWnd, MUIM_Window_SetCycleChain, BlankersLvw, PrefsBtn,
            InfoBtn, ToggleBtn, HideBtn, SettingsBtn, QuitBtn, 0L );
        for( i = 0, Tmp = ( BlankerEntry * )BlankerEntries->lh_Head;
            Tmp->be_Node.ln_Succ;
            i++, Tmp = ( BlankerEntry * )Tmp->be_Node.ln_Succ )
        {
            if( !Stricmp( Tmp->be_Name, Prefs->bp_Blanker ))
                Selected = i;
            DoMethod( BlankersLvw, MUIM_List_InsertSingle, Tmp,
                MUIV_List_Insert_Bottom );
        }
        SetAttrs( BlankersLvw, MUIA_List_Active, Selected, TAG_DONE );
        Rand = !Stricmp( Prefs->bp_Blanker, "Random" );
        SetAttrs( PrefsBtn, MUIA_Disabled, Rand, TAG_END );
        SetAttrs( ToggleBtn, MUIA_Disabled, Rand, TAG_END );
        SetAttrs( BlankWnd, MUIA_Window_Open, TRUE, TAG_DONE );
        DoMethod( BlankersLvw, MUIM_List_Jump, Selected );

        return HandleInterface();
    }

    if( MUIMasterBase )
        CloseLibrary( MUIMasterBase );

    return QUIT;
}

VOID CloseInterface( VOID )
{
#ifdef FUNKY_MUI
    DoMethod( BlankApp, MUIM_Application_Save,
        MUIV_Application_Save_ENVARC );
#endif
    MUI_Sigs = 0L;
    DisposeObject( BlankApp );
    BlankApp = 0L;
    CloseLibrary( MUIMasterBase );
    MUIMasterBase = 0L;
}

LONG HandleInterface( VOID )
{
    BlankerEntry *Entry;
    LONG Rand, RetVal = OK;

    if( !MUIMasterBase )
        return OK;

    do
	{
		switch( DoMethod( BlankApp, MUIM_Application_Input, &MUI_Sigs ))
		{
		case MUIV_Application_ReturnID_Quit:
			RetVal = QUIT;
			break;
		case ID_HIDE:
			RetVal = CLOSEWIN;
			break;
		case ID_SET:
			if( WBenchMsg )
			{
				struct Library *WorkbenchBase;
				
				if( WorkbenchBase = OpenLibrary( "workbench.library", 39L ))
				{
					struct Screen *PubScr;
					
					if( PubScr = LockPubScreen( 0L ))
					{
						WBInfo( WBenchMsg->sm_ArgList->wa_Lock,
							   WBenchMsg->sm_ArgList->wa_Name, PubScr );
						UnlockPubScreen( 0L, PubScr );
						RetVal = RESTART;
					}
					CloseLibrary( WorkbenchBase );
				}
			}
			break;
		case ID_TOGGLE:
			DoMethod( BlankersLvw, MUIM_List_GetEntry,
					 MUIV_List_GetEntry_Active, &Entry );
			ToggleModuleDisabled( Prefs );
			Entry->be_Disabled = !Entry->be_Disabled;
			DoMethod( BlankersLvw, MUIM_List_Redraw, MUIV_List_Redraw_Active );
			break;
		case ID_PREFS:
			if( Stricmp( Prefs->bp_Blanker, "Random" ))
				ExecSubProc( "PrefInterp", "" );
			break;
		case ID_INFO:
			ExecSubProc( "ShowInfo", ".txt" );
			break;
		case ID_BLANKERS:
			MessageModule( "GarshneClient", BM_DOQUIT );
			MessageModule( "GarshnePrefs", BM_DOQUIT );
			DoMethod( BlankersLvw, MUIM_List_GetEntry,
					 MUIV_List_GetEntry_Active, &Entry );
			strcpy( Prefs->bp_Blanker, Entry->be_Name );
			if( Stricmp( Prefs->bp_Blanker, "Random" ))
				LoadModule( Prefs->bp_Dir, Prefs->bp_Blanker );
			BlankerToEnv( Prefs );
			Rand = !Stricmp( Prefs->bp_Blanker, "Random" );
			SetAttrs( PrefsBtn, MUIA_Disabled, Rand, TAG_END );
			SetAttrs( ToggleBtn, MUIA_Disabled, Rand, TAG_END );
			break;
		default:
			break;
		}
	}
	while(( RetVal == OK )&& !MUI_Sigs );

    return RetVal;
}
