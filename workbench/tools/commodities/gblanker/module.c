/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <dos/dostags.h>
#include <strings.h>

#include "includes.h"
#include "libraries.h"
#include "protos/protos.h"

struct Screen *ServerScr;
struct Window *Wnd;
LONG NumBlankEntries;

VOID ToggleModuleDisabled( BlankerPrefs *Prefs )
{
    struct FileInfoBlock *Blk;
    BPTR BlankerLock;
    BYTE Path[108];
	
    strcpy( Path, Prefs->bp_Dir );
    AddPart( Path, Prefs->bp_Blanker, 108 );
	
    if( BlankerLock = Lock( Path, ACCESS_READ ))
    {
        if( Blk = AllocDosObject( DOS_FIB, 0L ))
        {
            if( Examine( BlankerLock, Blk ))
            {
                if( Stricmp( Blk->fib_Comment, "Disabled" ))
                    SetComment( Path, "Disabled" );
                else
                    SetComment( Path, "" );
            }
            FreeDosObject( DOS_FIB, Blk );
        }
        UnLock( BlankerLock );
    }
}

VOID ExecSubProc( STRPTR Command, STRPTR Extension )
{
    BPTR in = Open( "NIL:", MODE_OLDFILE );
    BPTR out = Open( "NIL:", MODE_OLDFILE );
    BYTE Path[216], Target[108];
	
    strcpy( Path, Prefs->bp_Dir );
    AddPart( Path, Command, 216 );
    strcat( Path, " " );
	
    strcpy( Target, Prefs->bp_Dir );
    AddPart( Target, Prefs->bp_Blanker, 108 );
    strcat( Path, Target );
    strcat( Path, Extension );
	
	if( in && out &&( SystemTags( Path, SYS_Asynch, TRUE, SYS_Input, in,
								 SYS_Output, out, TAG_END ) != -1 ))
        return;
	
    if( in )
        Close( in );
    if( out )
        Close( out );
}    

#define SIG_DELAY ( 1L << DelayPort->mp_SigBit )

VOID __saveds PingFunc( VOID )
{
    struct MsgPort *ClientPort, *ReplyPort;
	struct Library *SysBase;
    BlankMsg PingMsg;

	SysBase = *( struct Library ** )4L;

	if( ReplyPort = CreateMsgPort())
	{
		PingMsg.bm_Mess.mn_ReplyPort = ReplyPort;
		PingMsg.bm_Mess.mn_Length = sizeof( BlankMsg );
		PingMsg.bm_Type = BM_PING;
		/* Stealthimania, to alleviate processing at the blanker */
		PingMsg.bm_Mess.mn_Node.ln_Name = ( UBYTE * )( &Blanking );

		for( ;; )
		{
			if( SetSignal( 0L, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
				break;

			if( ClientPort = FindPort( "GarshneClient" ))
			{
				PingMsg.bm_Flags = 0L;
				PutMsg( ClientPort, ( struct Message * )( &PingMsg ));
				WaitPort( ReplyPort );
				GetMsg( ReplyPort );
			}
		}
		DeletePort( ReplyPort );
	}
}

VOID InternalBlank( VOID )
{
    if( !ServerScr )
    {
        ServerScr = OpenScreenTags( 0L, SA_DisplayID, getTopScreenMode(),
                                   SA_Depth, 1, SA_Behind, 1, SA_Quiet, TRUE,
								   TAG_END );
        if( ServerScr )
        {
            Wnd = BlankMousePointer( ServerScr );
            SetRGB4(&( ServerScr->ViewPort ), 0, 0, 0, 0 );
            SetRGB4(&( ServerScr->ViewPort ), 1, 0, 0, 0 );
            ScreenToFront( ServerScr );
            Blanking = TRUE;
        }
    }
}

VOID MessageModule( STRPTR PortName, LONG Type )
{
    struct MsgPort *ClientPort;
    BlankMsg *ClientMsg;
    
    if( ClientPort = FindPort( PortName ))
    {
        if( ClientMsg = AllocVec( sizeof( BlankMsg ), MEMF_CLEAR|MEMF_PUBLIC ))
        {
            ClientMsg->bm_Mess.mn_ReplyPort = ServerPort;
            ClientMsg->bm_Mess.mn_Length = sizeof( BlankMsg );
            /* Stealthimania */
            ClientMsg->bm_Mess.mn_Node.ln_Name = ( UBYTE * )( &Blanking );
            ClientMsg->bm_Type = Type;
            PutMsg( ClientPort, ( struct Message * )ClientMsg );
            return;
        }
    }
    
    if( Type == BM_DOBLANK )
        InternalBlank();
}

VOID LoadModule( STRPTR Dir, STRPTR Module )
{
    BPTR in = Open( "NIL:", MODE_OLDFILE );
    BPTR out = Open( "NIL:", MODE_OLDFILE );
    BYTE Path[256];
    
    if( !Stricmp( Module, "Random" ))
        Module = RandomModule();
	
    if( !Module )
        return;
    
    strcpy( Path, Dir );
    AddPart( Path, Module, 256 );
    
    if( in && out )
        if( SystemTags( Path, SYS_Asynch, TRUE, SYS_Input, in, SYS_Output, out,
					   TAG_END ) != -1 )
            return;
    
    if( in )
        Close( in );
    if( out )
        Close( out );
}

BlankerEntry *NewBlankerEntry( STRPTR Path, STRPTR Name, STRPTR Comment )
{
    BlankerEntry *New = AllocVec( sizeof( BlankerEntry ), MEMF_CLEAR );
	
    if( New )
    {
        strcpy( New->be_Path, Path );
        AddPart( New->be_Path, Name, 128 );
        New->be_Name = FilePart( New->be_Path );
        New->be_Disabled = !Stricmp( Comment, "Disabled" );
        New->be_Node.ln_Name = New->be_Name;
    }
	
    return New;
}

LONG FileIsModule( STRPTR Path, STRPTR Name )
{
    if( strstr( Name, ".ifc" ) || strstr( Name, ".txt" ) ||
	   strstr( Name, ".prefs" ))
        return FALSE;
	
    if( !Stricmp( Name, "PrefInterp" ))
        return FALSE;
	
    if( !Stricmp( Name, "ShowInfo" ))
        return FALSE;
	
    return TRUE;
}

VOID FreeBlankerEntries( struct List *Entries )
{
    BlankerEntry *FreeMe;
	
    if( !Entries )
        return;
	
    while( FreeMe = ( BlankerEntry * )RemHead( Entries ))
        FreeVec( FreeMe );
	
    FreeVec( Entries );
}

struct List *LoadBlankerEntries( STRPTR Path )
{
    struct FileInfoBlock *Blk;
    struct List *Entries = 0L;
    BlankerEntry *New;
    BPTR DirLock;
	
    if(!( Entries = AllocVec( sizeof( struct List ), MEMF_CLEAR )))
        return 0L;
    else
        NewList( Entries );
	
    NumBlankEntries = 0;
    
    if( DirLock = Lock( Path, ACCESS_READ ))
    {
        if(( Blk = AllocDosObject( DOS_FIB, 0L ))&&( Examine( DirLock, Blk )))
        {
            while( ExNext( DirLock, Blk ))
            {
                if(( Blk->fib_FileName )&&
                   ( FileIsModule( Path, Blk->fib_FileName )))
                {
                    if( New = NewBlankerEntry( Path, Blk->fib_FileName,
											  Blk->fib_Comment ))
                    {
                        New->be_Node.ln_Pri = 128 - ( New->be_Name[0] - 'a' );
                        Enqueue( Entries, ( struct Node * )New );
                        NumBlankEntries++;
                    }
                }
            }
            FreeDosObject( DOS_FIB, Blk );
        }
        UnLock( DirLock );
    }
    
    if( New = NewBlankerEntry( ":", "Random", "" ))
        AddTail( Entries, ( struct Node * )New );
	
    return Entries;
}
