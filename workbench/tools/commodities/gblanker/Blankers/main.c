/*
 *	Copyright (c) 1994 Michael D. Bayne.
 *	All rights reserved.
 *
 *	Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <aros/debug.h>

#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfxbase.h>
#include <string.h>

#include "includes.h"

LONG PortRemoved = FALSE, Blanking = FALSE, *ServerBlanking = 0L;
ULONG InitSecs, InitMicros, CheckCPU = FALSE, CheckCPUDisabled = FALSE;
BYTE PortName[] = "GarshneClient";
struct MsgPort *ClientPort, *TimerPort;
struct timerequest *TimeOutIO;
PrefObject *CurPrefs;
BYTE PrefsPath[128];

extern LONG RangeSeed;

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
	
	if( FindPort( PortName ))
		return 1;
	
	if( Proc->pr_CLI )
	{
		STRPTR Str = AROS_BSTR_ADDR((( struct CommandLineInterface * )
							BADDR( Proc->pr_CLI ))->cli_CommandName );
		CopyMem( Str, PrefsPath, *Str );
		PrefsPath[*Str] = '\0';
		strcat( PrefsPath, ".prefs" );
	}
	
        ClientPort = CreateMsgPort();
        TimerPort = CreateMsgPort();
        bug("PrefsPath %s\n", PrefsPath);
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
                        bug("PrefsPath %s\n", PrefsPath);
                        CurPrefs = LoadPrefs( PrefsPath );
                        
                        CurrentTime( &InitSecs, &InitMicros );
                        RangeSeed = InitSecs + InitMicros;
                        
                        if( MessageServer( BM_INITMSG ) == OK )
                        {
                                LONG Sigs, Ret;
                                
                                do
                                {
                                        Sigs = Wait( SIG_PORT | SIGBREAKF_CTRL_C );
                                        Ret = HandleSignal( Sigs );
                                }
                                while( Ret == OK );
                        }
                        else
                        {
                                Complain( "Init message to server failed." );
                                ReturnVal = 3;
                        }
                        
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
                bug("HandleSignal Timeout\n");
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
                                bug("DOBLANK, DOTESTBLANK\n");
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
                                bug("RELOADPREFS\n");
				if( CurPrefs )
					FreeVec( CurPrefs );
				CurPrefs = LoadPrefs( PrefsPath );
				break;
			case BM_DELAYEDQUIT:
                                bug("DELAYEDQUIT\n");
				RemPort( ClientPort );
				PortRemoved = TRUE;
				RetVal = DELAYEDQUIT;
				break;
			case BM_DOQUIT:
                                bug("DOQUIT\n");
				RetVal = QUIT;
				break;
			case BM_UNBLANK:
                                bug("UNBLANK\n");
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
	LONG RetVal;

	if( !CheckCPUDisabled )
	{
		if( !CheckCPU )
		{
			ULONG Secs, Micros;
			
			CurrentTime( &Secs, &Micros );
			if( Secs - InitSecs > 3 )
				CheckCPU = TRUE;
		}
		
		TimeOutIO->tr_node.io_Command = TR_ADDREQUEST;
		TimeOutIO->tr_time.tv_secs = ( CheckCPU ? 1 : 5 );
		SendIO(( struct IORequest * )TimeOutIO );
	}
	
	RetVal = HandleSignal( Wait( SIG_TIMER | SIG_PORT | SIGBREAKF_CTRL_C ));

	if( !CheckCPUDisabled )
	{
		AbortIO(( struct IORequest * )TimeOutIO );
		WaitIO(( struct IORequest * )TimeOutIO );
		SetSignal( 0L, SIG_TIMER );
	}

	switch( RetVal )
	{
	case OK:
		if( !*ServerBlanking )
			RetVal = UNBLANK;
		break;
	case DELAYEDQUIT:
		/* Lower our priority so our quitting doesn't interfere with the new 
		   blanker. */
		Delay( 60 );
		break;
	}

	if( !CheckCPUDisabled )
	{
		if( RetVal == UNBLANK )
			CheckCPU = FALSE;
	}

	return RetVal;
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
