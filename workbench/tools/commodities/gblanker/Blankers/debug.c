/*
 *	Copyright (c) 1994 Michael D. Bayne.
 *	All rights reserved.
 *
 *	Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfxbase.h>
#include <string.h>

#include "includes.h"

LONG PortRemoved = FALSE, Blanking = FALSE, *ServerBlanking = 0L;
ULONG InitSecs, InitMicros, CheckCPU = FALSE, StopIt = FALSE;
struct Library *IntuitionBase, *GarshnelibBase, *GfxBase;
BYTE PortName[] = "GarshneClient";
struct MsgPort *ClientPort, *TimerPort;
struct timerequest *TimeOutIO;
PrefObject *CurPrefs;
BYTE PrefsPath[128];

extern __far LONG RangeSeed;

PrefObject *LoadPrefs( STRPTR Path )
{
	PrefObject *Prefs;
	BPTR PrefFile;
	LONG Objects;

	if( Prefs = AllocVec( sizeof( PrefObject ) * 25, MEMF_CLEAR ))
	{
		if( PrefFile = Open( Path, MODE_OLDFILE ))
		{
			Read( PrefFile, &Objects, sizeof( LONG ));
			Read( PrefFile, Prefs, sizeof( PrefObject ) * Objects );
			Close( PrefFile );
		}
		else
			Defaults( Prefs );
	}

	return Prefs;
}

int main( void )
{
	struct Process *Proc = ( struct Process * )FindTask( 0L );
	BlankMsg *FreeMsg;
	LONG ReturnVal = 0;
	
	if( Proc->pr_CLI )
	{
		STRPTR Str = BADDR((( struct CommandLineInterface * )
							BADDR( Proc->pr_CLI ))->cli_CommandName );
		CopyMem( Str + 1, PrefsPath, *Str );
		PrefsPath[*Str] = '\0';
		strcat( PrefsPath, ".prefs" );
	}
	
	IntuitionBase = OpenLibrary( "intuition.library", 37L );
	GfxBase = OpenLibrary( GRAPHICSNAME, 37L );
	GarshnelibBase = OpenLibrary( "Garshnelib.library", 37L );
	
	if( IntuitionBase && GfxBase && GarshnelibBase )
	{
		ClientPort = CreateMsgPort();
		TimerPort = CreateMsgPort();
		
		if( ClientPort && TimerPort )
		{
			TimeOutIO = ( struct timerequest * )
				CreateExtIO( TimerPort, sizeof( struct timerequest ));
			
			if( TimeOutIO && !OpenDevice( "timer.device", UNIT_VBLANK,
										 ( struct IORequest * )TimeOutIO, 0L ))
			{
				ClientPort->mp_Node.ln_Name = PortName;
				ClientPort->mp_Node.ln_Pri = 0L;
				AddPort( ClientPort );
				CurPrefs = LoadPrefs( PrefsPath );
				
				CurrentTime( &InitSecs, &InitMicros );
				RangeSeed = InitSecs + InitMicros;
				
				Blank( CurPrefs );

				while( FreeMsg = ( BlankMsg * )GetMsg( ClientPort ))
				{
					if( FreeMsg->bm_Type & BF_REPLY )
						FreeVec( FreeMsg );
					else
					{
						FreeMsg->bm_Type |= BF_REPLY;
						ReplyMsg(( struct Message * )FreeMsg );
					}
				}
				
				if( CurPrefs )
					FreeVec( CurPrefs );
			
				if( !PortRemoved )
					RemPort( ClientPort );
			}
			
			if( TimeOutIO )
			{
				if( TimeOutIO->tr_node.io_Device )
					CloseDevice(( struct IORequest * )TimeOutIO );
				DeleteExtIO(( struct IORequest * )TimeOutIO );
			}
		}
		else
		{
			Complain( "ClientPort or TimerPort failed to open." );
			ReturnVal = 2;
		}

		if( ClientPort )
		{
			BlankMsg *TmpMsg;
			while( TmpMsg = ( BlankMsg * )GetMsg( ClientPort ))
			{
				TmpMsg->bm_Flags |= BF_REPLY;
				ReplyMsg(( struct Message * )TmpMsg );
			}
			DeleteMsgPort( ClientPort );
		}

		if( TimerPort )
			DeleteMsgPort( TimerPort );
	}
	else
	{
		Complain( "A library failed to open." );
		ReturnVal = 1;
	}
	
	if( GarshnelibBase )
		CloseLibrary( GarshnelibBase );
	
	if( GfxBase )
		CloseLibrary( GfxBase );
	
	if( IntuitionBase )
		CloseLibrary( IntuitionBase );
	
	return ReturnVal;
}

LONG MessageServer( LONG Type )
{
	struct MsgPort *ServerPort;
	BlankMsg *ClientMsg;
	
	if( ServerPort = FindPort( "GarshneServer" ))
	{
		if( ClientMsg = AllocVec( sizeof( BlankMsg ), MEMF_PUBLIC|MEMF_CLEAR ))
		{
			ClientMsg->bm_Mess.mn_ReplyPort = ClientPort;
			ClientMsg->bm_Mess.mn_Length = sizeof( BlankMsg );
			ClientMsg->bm_Type = Type;
			PutMsg( ServerPort, ( struct Message * )ClientMsg );
			
			return OK;
		}
	}
	
	return QUIT;
}

LONG HandleSignal( LONG Signal )
{
	BlankMsg *CurMsg;
	LONG RetVal = OK;
	
	if( Signal & SIG_TIMER )
	{
		MessageServer( BM_FAILED );
		RetVal = UNBLANK;
	}
	
	if( Signal & SIG_PORT )
	{
		while( CurMsg = ( BlankMsg * )GetMsg( ClientPort ))
		{
			LONG Type = CurMsg->bm_Type;
			LONG Flags = CurMsg->bm_Flags;
			
			if( Type != BM_PING )
				ServerBlanking = ( LONG * )CurMsg->bm_Mess.mn_Node.ln_Name;
			
			if( Flags & BF_REPLY )
				FreeVec( CurMsg );
			else
			{
				CurMsg->bm_Flags |= BF_REPLY;
				ReplyMsg(( struct Message * )CurMsg );
			}
			
			switch( Type )
			{
			case BM_DOBLANK:
			case BM_DOTESTBLANK:
				if( !Blanking )
				{
					PrefObject *TmpPrefs;

					Blanking = TRUE;
					CurrentTime( &InitSecs, &InitMicros );
					if( Type == BM_DOTESTBLANK )
						TmpPrefs = LoadPrefs( "T:GBlankerTmpPrefs" );
					switch( Blank( Type == BM_DOBLANK ? CurPrefs : TmpPrefs ))
					{
					case FAILED:
						/* In this case the Blank() function failed to init */
						/* (ie. no memory or something). So we yell. */
						MessageServer( BM_FAILED );
						break;
					case DELAYEDQUIT:
					case QUIT:
						RetVal = QUIT;
						break;
					}
					Blanking = FALSE;
					if( Type == BM_DOTESTBLANK )
						FreeVec( TmpPrefs );
				}
				break;
			case BM_RELOADPREFS:
				if( CurPrefs )
					FreeVec( CurPrefs );
				CurPrefs = LoadPrefs( PrefsPath );
				break;
			case BM_DELAYEDQUIT:
				RemPort( ClientPort );
				PortRemoved = TRUE;
				RetVal = DELAYEDQUIT;
				break;
			case BM_DOQUIT:
				RetVal = QUIT;
				break;
			case BM_UNBLANK:
				break;
			}
		}
	}
	
	if( Signal & SIGBREAKF_CTRL_C )
		RetVal = QUIT;

	return RetVal;
}

LONG ContinueBlanking( VOID )
{
	if( SetSignal( 0L, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
		return QUIT;

	if( StopIt )
		return QUIT;

	return OK;
}

VOID Complain( STRPTR Bitch )
{
	if( IntuitionBase )
	{
		struct EasyStruct ErrorReq = {
			sizeof( struct EasyStruct ), 0, "Error", 0L, "Ok" };
		ErrorReq.es_TextFormat = Bitch;
		EasyRequestArgs( 0L, &ErrorReq, 0L, 0L );
	}
}
