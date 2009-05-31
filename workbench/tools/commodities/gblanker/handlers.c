/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include "includes.h"
#include "libraries.h"
#include "protos/protos.h"
#include <exec/execbase.h>

LONG BlankingDisabled = FALSE;

ULONG UsagePercent( VOID )
{
    ULONG OldIdle, OldDisp, Idle, Disp;
	
    OldIdle = (( struct ExecBase * )SysBase )->IdleCount;
    OldDisp = (( struct ExecBase * )SysBase )->DispCount;
	
    Delay( 10 );
	
    Idle = (( struct ExecBase * )SysBase )->IdleCount - OldIdle;
    Disp = (( struct ExecBase * )SysBase )->DispCount - OldDisp;
	
    return Disp * 100 / ( Idle + Disp );
}

LONG HandleServerMsg( VOID )
{
	ULONG PubScreenModes;
	BlankMsg *CurMsg;
	
	while( CurMsg = ( BlankMsg * )GetMsg( ServerPort ))
	{
		LONG Type = CurMsg->bm_Type;
		LONG Flags = CurMsg->bm_Flags;

		if( Flags & BF_REPLY )
		{
			if(!( Flags & BF_INTERNAL ))
				FreeVec( CurMsg );
		}
		else
		{
			CurMsg->bm_Flags |= BF_REPLY;
			ReplyMsg(( struct Message * )CurMsg );
		}
		
		switch( Type )
		{
		case BM_DOBLANK:
			if( Flags & BF_REPLY )
			{
				if( !CheckIO(( struct IORequest * )TimeOutIO ))
				{
					AbortIO(( struct IORequest * )TimeOutIO );
					WaitIO(( struct IORequest * )TimeOutIO );
					SetSignal( 0L, SIG_TIMER );
				}
				if( ServerScr )
				{
					UnblankMousePointer( Wnd );
					CloseScreen( ServerScr );
					ServerScr = 0L;
				}
				if( !PingTask )
					PingTask = CreateTask( "GarshnePing", -5, PingFunc, 4096 );
				Blanking = TRUE;
			}
			break;
		case BM_DOTESTBLANK:
			if( Flags & BF_REPLY )
			{
				if( !PingTask )
					PingTask = CreateTask( "GarshnePing", -5, PingFunc, 4096 );
				Blanking = TRUE;
			}
			break;
		case BM_INITMSG:
			if( BlankAfterInit )
			{
				BlankAfterInit = FALSE;
				MessageModule( "GarshneClient", BM_DOBLANK );
			}
			break;
		case BM_FAILED:
			if( PingTask )
			{
				Signal( PingTask, SIGBREAKF_CTRL_C );
				PingTask = 0L;
			}
			InternalBlank();
			break;
		case BM_SENDBLANK:
			if(( Flags & BF_REPLY )|| BlankingDisabled )
				break;
			if( ServerScr &&( UsagePercent() > 40 ))
				break;
			PubScreenModes = SetPubScreenModes( 0L );
			if( Stricmp( Prefs->bp_Blanker, "Random" ))
			{
				if( !Blanking || ServerScr )
					MessageModule( "GarshneClient", BM_DOBLANK );
			}
			else
			{
				if( !Blanking || Prefs->bp_Flags & BF_REPLACE )
				{
					MessageModule( "GarshneClient", BM_DELAYEDQUIT );
					BlankAfterInit = TRUE;
					LoadModule( Prefs->bp_Dir, Prefs->bp_Blanker );
				}
				else if( ServerScr )
					MessageModule( "GarshneClient", BM_DOBLANK );
			}
			SetPubScreenModes( PubScreenModes );
			break;
		case BM_SENDTEST:
			if(( Flags & BF_REPLY )|| BlankingDisabled )
				break;
			PubScreenModes = SetPubScreenModes( 0L );
			MessageModule( "GarshneClient", BM_DOTESTBLANK );
			SetPubScreenModes( PubScreenModes );
			break;
		case BM_SENDUNBLANK:
			if( Flags & BF_REPLY )
				break;
			if( ServerScr )
			{
				UnblankMousePointer( Wnd );
				CloseScreen( ServerScr );
				ServerScr = 0L;
			}
			Blanking = FALSE;
			MessageModule( "GarshneClient", BM_UNBLANK );
			if( PingTask )
			{
				Signal( PingTask, SIGBREAKF_CTRL_C );
				PingTask = 0L;
			}
			break;
		default:
			break;
		}
	}

	return OK;
}

LONG HandleMouseCheck( VOID )
{
	LONG MouseX, MouseY, Lock, Width, Height;

	BlankingDisabled = FALSE;

	Lock = LockIBase( 0L );

	MouseX = IntuitionBase->MouseX;
	MouseY = IntuitionBase->MouseY;
	if( IntuitionBase->FirstScreen )
	{
		Width = IntuitionBase->FirstScreen->Width - 1;
		Height = IntuitionBase->FirstScreen->Height - 1;
	}
	else
	{
		Width = ~0L;
		Height = ~0L;
	}
	
	UnlockIBase( Lock );

	switch( Prefs->bp_BlankCorner )
	{
	case BC_UPPERLEFT:
		if( !MouseX && !MouseY )
			timeCount = Prefs->bp_Timeout - 5;
		break;
	case BC_UPPERRIGHT:
		if( !MouseY && MouseX == Width )
			timeCount = Prefs->bp_Timeout - 5;
		break;
	case BC_LOWERRIGHT:	
		if( MouseX == Width && MouseY == Height )
			timeCount = Prefs->bp_Timeout - 5;
	   	break;
	case BC_LOWERLEFT:
		if( !MouseX && MouseY == Height )
			timeCount = Prefs->bp_Timeout - 5;
		break;
	}

	switch( Prefs->bp_DontCorner )
	{
	case BC_UPPERLEFT:
		if( !MouseX && !MouseY )
			BlankingDisabled = TRUE;
		break;
	case BC_UPPERRIGHT:
		if( !MouseY && MouseX == Width )
			BlankingDisabled = TRUE;
		break;
	case BC_LOWERRIGHT:	
		if( MouseX == Width && MouseY == Height )
			BlankingDisabled = TRUE;
	   	break;
	case BC_LOWERLEFT:
		if( !MouseX && MouseY == Height )
			BlankingDisabled = TRUE;
		break;
	}

	return OK;
}
