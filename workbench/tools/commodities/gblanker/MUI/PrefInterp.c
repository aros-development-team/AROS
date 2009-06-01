#define MUI_OBSOLETE

#include <aros/debug.h>

#include <exec/memory.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/asl.h>

#include <proto/Garshnelib.h>

#include <clib/alib_protos.h>

#include <string.h>

#include "PrefInterp_rev.h"
#include "PrefInterp.h"
#include "../protos/parse.h"
#include "../defs.h"

#define SIG_REPLY ( 1L << ReplyPort->mp_SigBit )

Object *ModuleApp, *ModuleWnd, **Objects;
struct MsgPort *ReplyPort = 0L;
ULONG ModuleSigs = 0L;
LONG NumGadgets;
VOID *Memory = 0L;
PrefObject *Prefs;

STRPTR LToStr( LONG Value )
{
	static BYTE Buffer[16];
	STRPTR Ptr = &Buffer[15];

	if( !Value )
		return "0";

	*Ptr = '\0';

	while( Value )
	{
		*(--Ptr) = ( Value % 10 ) + '0';
		Value /= 10;
	}

	return Ptr;
}

STRPTR Ununderscore( STRPTR String )
{
	static BYTE Buffer[128];
	LONG i = 0;

	while( *String )
	{
		if( *String != '_' )
			Buffer[i++] = *String;
		String++;
	}
	Buffer[i] = '\0';

	return Buffer;
}

VOID FontRequest( struct Window *Wnd, struct TextAttr *Attr )
{
    struct FontRequester *fReq;

    fReq = MUI_AllocAslRequestTags( ASL_FontRequest,
							   ASL_FontName, Attr->ta_Name,
							   ASL_FontHeight, Attr->ta_YSize,
							   ASL_MaxHeight, 100, TAG_DONE );
    if( fReq )
    {
        if( MUI_AslRequestTags( fReq,
						   ASLFO_Window, Wnd,
						   ASLFO_SleepWindow, TRUE,
						   ASLFO_TitleText, ( LONG )"Please choose a font...",
						   TAG_DONE ))
        {
            CopyMem( fReq->fo_Attr.ta_Name, Attr->ta_Name, 31 );
            Attr->ta_YSize = fReq->fo_Attr.ta_YSize;
        }
        MUI_FreeAslRequest( fReq );
    }
}   

VOID SendMessageToPort( LONG Type, STRPTR PortName )
{
    struct MsgPort *ForeignPort;
    BlankMsg *ClientMsg;
    
    if( ForeignPort = FindPort( PortName ))
    {
        if( ClientMsg = AllocPooled( Memory, sizeof( BlankMsg )))
        {
            ClientMsg->bm_Mess.mn_ReplyPort = ReplyPort;
            ClientMsg->bm_Mess.mn_Length = sizeof( BlankMsg );
            ClientMsg->bm_Type = Type;
            PutMsg( ForeignPort, ( struct Message * )ClientMsg );
        }
    }
}

LONG main( LONG argc, char **argv )
{
    Object *VertGroup, *SaveBtn, *TestBtn, *CancelBtn, *DisplayBtn, *CtrlGrp;
    Object *NameInf, *SizeInf;
    LONG i, j, LastNonGrp = 0, DispID = -1, Min, Max, *Types, rc, ID, Sigs;
    BYTE DescripName[108], PrefsName[108], BogusBuf[128], ValidPrefs = FALSE;
    STRPTR *Labels, *KeyStrs, AppName;
    ULONG Args[] = { 0L, 0L }, GroupObjects = 0, *Tags, Exit = FALSE;
    BPTR Descrip, Tmp;
    Object **Indics;

    if( argc != 2 )
    {
        bug("Argument missing\n");
        return RETURN_WARN;
    }
    
    if( FindPort( "GarshnePrefs" ))
    {
        bug("Port GarshnePrefs already exists\n");
        return RETURN_WARN;
    }

    if(!( Memory = CreatePool( MEMF_CLEAR, 1024, 512 )))
    {
        bug("CreatePool failed\n");
        goto JAIL;
    }

    if(!( ReplyPort = CreatePort( "GarshnePrefs", 0L )))
    {
        bug("CreatePort for ReplyPort failed\n");
        goto JAIL;
    }

    strcpy( DescripName, argv[1] );
    strcat( DescripName, ".ifc" );

    strcpy( PrefsName, argv[1] );
    strcat( PrefsName, ".prefs" );
    
    if(!( Descrip = Open( DescripName, MODE_OLDFILE )))
    {
        bug("Open failed\n");
        goto JAIL;
    }

    NumGadgets = ScanDigit( Descrip );

    Objects = AllocPooled( Memory, sizeof( Object * ) * ( NumGadgets + 1 ));
    Indics = AllocPooled( Memory, sizeof( Object * ) * ( NumGadgets + 1 ));
    Prefs = AllocPooled( Memory, sizeof( PrefObject ) * ( NumGadgets + 1 ));
    Types = AllocPooled( Memory, sizeof( LONG ) * ( NumGadgets + 1 ));
    KeyStrs = AllocPooled( Memory, sizeof( STRPTR ) * ( NumGadgets + 1 ));
    
    if( !Objects || !Indics || !Prefs || !Types || !KeyStrs )
    {
        bug("AllocPooled failed\n");
        goto PREJAIL;
    }

    AppName = FilePart( argv[1] );

    if( Tmp = Open( PrefsName, MODE_OLDFILE ))
    {
        Read( Tmp, Prefs, sizeof( LONG )); /* Ignore entry count */
        if( Read( Tmp, Prefs, sizeof( PrefObject ) * NumGadgets ) ==
           sizeof( PrefObject ) * NumGadgets )
            ValidPrefs = TRUE;
        Close( Tmp );
    }

    for( i = 0; i < NumGadgets; i++ )
    {
        STRPTR LabelStr;
        
        switch( Prefs[i].po_Type = Types[i] = ScanType( Descrip ))
        {
        case GAD_CYCLE:
            LabelStr = Ununderscore( ScanToken( Descrip ));
            KeyStrs[i] = ScanToken( Descrip );
            Labels = ScanTokenArray( Descrip );
            if( ValidPrefs )
                ScanDigit( Descrip );
            else
                Prefs[i].po_Active = ScanDigit( Descrip );
            Indics[i] = KeyLabel( LabelStr, *KeyStrs[i] );
            Objects[i] = CycleObject,
                MUIA_Cycle_Active, Prefs[i].po_Active,
                MUIA_Cycle_Entries, Labels,
                MUIA_ControlChar, *KeyStrs[i],
            End;
            break;
        case GAD_SLIDER:
            LabelStr = Ununderscore( ScanToken( Descrip ));
            KeyStrs[i] = ScanToken( Descrip );
            Min = ScanDigit( Descrip );
            Max = ScanDigit( Descrip );
            if( ValidPrefs )
                ScanDigit( Descrip );
            else
                Prefs[i].po_Level = ScanDigit( Descrip );
            Indics[i] = KeyLabel( LabelStr, *KeyStrs[i] );
            Objects[i] = KeySlider( Min, Max, Prefs[i].po_Level, *KeyStrs[i] );
            break;
        case GAD_FONT:
            if( ValidPrefs )
            {
                ScanToken( Descrip );
                ScanDigit( Descrip );
            }
            else
            {
                strcpy( Prefs[i].po_Name, ScanToken( Descrip ));
                Prefs[i].po_Attr.ta_YSize = ScanDigit( Descrip );
            }
            Prefs[i].po_Attr.ta_Name = Prefs[i].po_Name;
            Objects[i] = KeyButton( "Font", 'f' );
			Objects[i] = TextObject,
                ButtonFrame,
                MUIA_Text_Contents, "Font",
                MUIA_Text_PreParse, "\33c",
                MUIA_Text_HiChar, 'f',
                MUIA_ControlChar, 'f',
                MUIA_InputMode, MUIV_InputMode_RelVerify,
                MUIA_Background, MUII_ButtonBack,
				MUIA_HorizWeight, 20,
			End;
            Args[0] = Prefs[i].po_Attr.ta_YSize,
            Indics[i] = HGroup,
                Child, NameInf = TextObject,
                    ReadListFrame,
                    MUIA_Text_Contents, Prefs[i].po_Name,
					MUIA_HorizWeight, 70,
					MUIA_Background, MUII_BACKGROUND,
                End,
                Child, SizeInf = TextObject,
                    ReadListFrame,
                    MUIA_Text_Contents, LToStr( Prefs[i].po_Attr.ta_YSize ),
					MUIA_HorizWeight, 10,
					MUIA_Background, MUII_BACKGROUND,
                End,
            End;
            break;
        case GAD_STRING:
            LabelStr = Ununderscore( ScanToken( Descrip ));
            KeyStrs[i] = ScanToken( Descrip );
            if( ValidPrefs )
                FGets( Descrip, BogusBuf, 128 );
            else
            {
                FGets( Descrip, Prefs[i].po_Value, 128 );
                Prefs[i].po_Value[strlen( Prefs[i].po_Value )-1] = '\0';
            }
            Indics[i] = KeyLabel( LabelStr, *KeyStrs[i] );
            Objects[i] = KeyString( Prefs[i].po_Value, 127, *KeyStrs[i] );
            break;
        case GAD_DISPLAY:
            DispID = i + 10;
            if( ValidPrefs )
                ScanDigit( Descrip );
            else
            {
                Prefs[i].po_ModeID = getTopScreenMode();
                Prefs[i].po_Depth = ScanDigit( Descrip );
            }
            break;
        case GAD_DELIM:
			Tags = AllocPooled( Memory,
							   4 * sizeof( LONG * ) * ( i - LastNonGrp ) + 3 );
			if( Tags )
			{
				LONG n = 0;

				Tags[n++] = MUIA_Group_Horiz;
				Tags[n++] = TRUE;
				for( j = LastNonGrp; j < i; j++ )
				{
					Tags[n++] = MUIA_Group_Child;
					Tags[n++] = ( Types[j] == GAD_FONT ) ?
						( ULONG )Objects[j] : ( ULONG )Indics[j];
					Tags[n++] = MUIA_Group_Child;
					Tags[n++] = ( Types[j] == GAD_FONT ) ?
						( ULONG )Indics[j] : ( ULONG )Objects[j];
				}
				Tags[n] = TAG_END;
				Objects[i] = MUI_NewObjectA( MUIC_Group,
										   ( struct TagItem * )Tags );
				FreePooled( Memory, Tags,
						   4 * sizeof( LONG * ) * ( i - LastNonGrp ) + 3 );
				LastNonGrp = i+1;
				GroupObjects += 1;
			}
            break;
        default:
            break;
        }
    }
    
    if( Descrip )
    {
        Close( Descrip );
        Descrip = 0L;
    }

    if( DispID != -1 )
    {
        CtrlGrp = HGroup,
            Child, SaveBtn = KeyButton( "Save", 's' ),
            Child, TestBtn = KeyButton( "Test", 't' ),
            Child, DisplayBtn = KeyButton( "Display", 'd' ),
            Child, CancelBtn = KeyButton( "Cancel", 'c' ),
        End;
    }
    else
    {
        CtrlGrp = HGroup,
            Child, SaveBtn = KeyButton( "Save", 's' ),
            Child, TestBtn = KeyButton( "Test", 't' ),
            Child, CancelBtn = KeyButton( "Cancel", 'c' ),
        End;
    }

	if( Tags = AllocPooled( Memory, sizeof( LONG * ) * GroupObjects * 2 + 3 ))
	{
		LONG n = 0;

		for( i = 0; i < NumGadgets; i++ )
		{
			if( Types[i] == GAD_DELIM )
			{
				Tags[n++] = MUIA_Group_Child;
				Tags[n++] = ( ULONG )Objects[i];
			}
		}
		Tags[n++] = MUIA_Group_Child;
		Tags[n++] = ( ULONG )CtrlGrp;
		Tags[n] = TAG_END;
		VertGroup = MUI_NewObjectA( MUIC_Group, ( struct TagItem * )Tags );
		FreePooled( Memory, Tags, sizeof( LONG * ) * GroupObjects * 2 + 3 );
	}

    ModuleApp = ApplicationObject,
        MUIA_Application_Title, "Garshneprefs",
        MUIA_Application_Version, VERS,
        MUIA_Application_Copyright, "Free Software",
        MUIA_Application_Author, "Michael D. Bayne",
        MUIA_Application_Description, "Module preferences interpreter",
        MUIA_Application_Base, "GPREFS",
        MUIA_Application_Window, ModuleWnd = WindowObject,
            MUIA_Window_ID, MAKE_ID( AppName[0], AppName[1], AppName[2],
                AppName[3]  ),
			MUIA_Window_ScreenTitle, AppName,
            MUIA_Window_Title, AppName,
            MUIA_Window_RootObject, VertGroup,
		End,
    End;
    
    if( !ModuleApp )
        goto PREJAIL;

#ifdef FUNKY_MUI
    DoMethod( ModuleApp, MUIM_Application_Load,
        MUIV_Application_Load_ENVARC );
#endif
    DoMethod( ModuleWnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        ModuleApp, 2, MUIM_Application_ReturnID,
        MUIV_Application_ReturnID_Quit );
    DoMethod( SaveBtn, MUIM_Notify, MUIA_Pressed, FALSE, ModuleApp, 2,
        MUIM_Application_ReturnID, ID_SAVE );
    DoMethod( TestBtn, MUIM_Notify, MUIA_Pressed, FALSE, ModuleApp, 2,
        MUIM_Application_ReturnID, ID_TEST );
    if( DispID != -1 )
        DoMethod( DisplayBtn, MUIM_Notify, MUIA_Pressed, FALSE, ModuleApp, 2,
            MUIM_Application_ReturnID, DispID );
    DoMethod( CancelBtn, MUIM_Notify, MUIA_Pressed, FALSE, ModuleApp, 2,
        MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit );
    
    for( i = 0; i < NumGadgets; i++ )
    {
        switch( Types[i] )
        {
        case GAD_SLIDER:
            DoMethod( Objects[i], MUIM_Notify, MUIA_Slider_Level,
					 MUIV_EveryTime, ModuleApp, 2, MUIM_Application_ReturnID,
					 i + 10 );
            break;
        case GAD_CYCLE:
            DoMethod( Objects[i], MUIM_Notify, MUIA_Cycle_Active,
					 MUIV_EveryTime, ModuleApp, 2, MUIM_Application_ReturnID,
					 i + 10 );
            break;
        case GAD_STRING:
            DoMethod( Objects[i], MUIM_Notify, MUIA_String_Contents,
					 MUIV_EveryTime, ModuleApp, 2, MUIM_Application_ReturnID,
					 i + 10 );
            break;
        case GAD_FONT:
            DoMethod( Objects[i], MUIM_Notify, MUIA_Pressed, FALSE, ModuleApp,
                2, MUIM_Application_ReturnID, i + 10 );
            break;
        default:
            break;
        }
    }
    SetAttrs( ModuleWnd, MUIA_Window_Open, TRUE, TAG_DONE );
    while( ModuleApp && !Exit )
    {
        rc = DoMethod( ModuleApp, MUIM_Application_Input, &ModuleSigs );
		switch( rc )
        {
        case ID_SAVE:
            if( Tmp = Open( PrefsName, MODE_NEWFILE ))
            {
                Write( Tmp, &NumGadgets, sizeof( LONG ));
                Write( Tmp, Prefs, sizeof( PrefObject ) * NumGadgets );
                Close( Tmp );
            }
            SendMessageToPort( BM_RELOADPREFS, "GarshneClient" );
        case MUIV_Application_ReturnID_Quit:
			Exit = TRUE;
            break;
        case ID_TEST:
            if( Tmp = Open( "T:GBlankerTmpPrefs", MODE_NEWFILE ))
            {
                Write( Tmp, &NumGadgets, sizeof( LONG ));
                Write( Tmp, Prefs, sizeof( PrefObject ) * NumGadgets );
                Close( Tmp );
                SendMessageToPort( BM_SENDTEST, "GarshneServer" );
            }
            break;
        default:
            ID = rc - 10;
            if( rc > 9 && ID < NumGadgets )
            {
                switch( Types[ID] )
                {
                case GAD_CYCLE:
                    GetAttr( MUIA_Cycle_Active, Objects[ID],
                        ( IPTR * )&Prefs[ID].po_Active );
                    break;
                case GAD_SLIDER:
                    GetAttr( MUIA_Slider_Level, Objects[ID],
                        ( IPTR * )&Prefs[ID].po_Level );
                    break;
                case GAD_STRING:
                {
                    STRPTR TmpPtr;

                    GetAttr( MUIA_String_Contents, Objects[ID],
                            ( IPTR * )&TmpPtr );
                    strcpy( Prefs[ID].po_Value, TmpPtr );
                    break;
                }
                case GAD_FONT:
				{
                    struct Window *Wnd;

                    GetAttr( MUIA_Window_Window, ModuleWnd, ( IPTR * )&Wnd );
                    if( Wnd )
						FontRequest( Wnd, &Prefs[ID].po_Attr );
                    SetAttrs( NameInf, MUIA_Text_Contents, Prefs[ID].po_Name,
							 TAG_DONE );
                    SetAttrs( SizeInf, MUIA_Text_Contents,
							 LToStr( Prefs[ID].po_Attr.ta_YSize ), TAG_DONE );
                    break;
				}
                case GAD_DISPLAY:
                {
                    struct Window *Wnd;

                    GetAttr( MUIA_Window_Window, ModuleWnd, ( IPTR * )&Wnd );
                    if( Wnd )
                        ScreenModeRequest( Wnd, &Prefs[ID].po_ModeID,
                            Prefs[ID].po_Depth ? &Prefs[ID].po_Depth : 0L );
                    break;
                }
                default:
                    break;
                }
            }
            break;
        }

		if( ModuleSigs )
			Sigs = Wait( ModuleSigs | SIG_REPLY | SIGBREAKF_CTRL_C );
        else
			continue;
		
        if( Sigs & SIG_REPLY )
        {
            BlankMsg *FreeMe;

            while( FreeMe = ( BlankMsg * )GetMsg( ReplyPort ))
            {
                switch( FreeMe->bm_Type )
                {
                case BM_DOQUIT:
                    FreeMe->bm_Flags |= BF_REPLY;
                    ReplyMsg(( struct Message * )FreeMe );
					Exit = TRUE;
                    break;
                case BM_RELOADPREFS:
                case BM_SENDTEST:
                    FreePooled( Memory, FreeMe, sizeof( BlankMsg ));
                    break;
                }
            }
        }

        if( Sigs & SIGBREAKF_CTRL_C )
			Exit = TRUE;
    }        

    DisposeObject( ModuleApp );
    
 PREJAIL:
    if( Descrip )
        Close( Descrip );

 JAIL:
    if( ReplyPort )
        DeletePort( ReplyPort );
    if( Memory )
        DeletePool( Memory );

    return RETURN_OK;
}
