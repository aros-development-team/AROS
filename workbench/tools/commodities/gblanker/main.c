/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#define DEBUG 1
#include <aros/debug.h>

#include <exec/memory.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <string.h>

#include "includes.h"
#include "libraries.h"
#include "protos/protos.h"

extern LONG RangeSeed;
STATIC const UBYTE VersTag[] = VERSTAG;

LONG Blanking = FALSE, BlankAfterInit = FALSE;
struct MsgPort *ServerPort, *TimerPort;
struct timerequest *TimeOutIO;
struct List *BlankerEntries;
struct Task *ServerTask, *PingTask = 0L;
BlankerPrefs *Prefs;
BYTE ProgName[108];
LONG RetVal;
struct WBStartup *WBenchMsg;

int main( void )
{
    LONG Sigs;

    RangeSeed = ( LONG )( ServerTask = FindTask( 0L ));
    
    if( RetVal = AllocResources())
    {
        bug("AllocResourced failed\n");
        FreeResources( RetVal );
        return RETURN_FAIL;
    }

    Prefs = LoadDefaultPrefs();
    BlankerEntries = LoadBlankerEntries( Prefs->bp_Dir );
    if(!( Prefs->bp_Flags & BF_REPLACE )||
       Stricmp( Prefs->bp_Blanker, "Random" ))
        LoadModule( Prefs->bp_Dir, Prefs->bp_Blanker );

    if( SetupCX() == QUIT )
    {
        bug("SetupCX failed\n");
        FreeResources( 0 );
        return RETURN_WARN;
    }

    if( Prefs->bp_PopUp )
        RetVal = OpenInterface();

    do
    {
        Sigs = Wait( SIG_CX | ISigs() | SIG_SERVPORT | SIGBREAKF_CTRL_C | 
					SIGBREAKF_CTRL_D );
        
        if( Sigs & SIG_SERVPORT )
            RetVal = HandleServerMsg();

        if( Sigs & SIG_CX )
            RetVal = HandleCxMess();

        if( Sigs & SIGBREAKF_CTRL_C )
            RetVal = 0L;

        if( Sigs & SIGBREAKF_CTRL_D )
            RetVal = HandleMouseCheck();
                        
        if( Sigs & ISigs())
            RetVal = HandleInterface();

        switch( RetVal )
        {
        case CLOSEWIN:
            CloseInterface();
            break;
        case RESTART:
            CloseInterface();
            ShutdownCX();
            FreeBlankerEntries( BlankerEntries );
            Prefs = LoadDefaultPrefs();
            BlankerEntries = LoadBlankerEntries( Prefs->bp_Dir );
            if(( RetVal = SetupCX()) == OK )
                RetVal = OpenInterface();
            break;
        }
    }
    while( RetVal );

    FreeResources( 0 );

    return RETURN_OK;
}

VOID SafeKill( STRPTR PortName )
{
    ULONG Sigs;

    if( !FindPort( PortName ))
        return;

    MessageModule( PortName, BM_DOQUIT );

    if( !CheckIO(( struct IORequest * )TimeOutIO ))
    {
        AbortIO(( struct IORequest * )TimeOutIO );
        WaitIO(( struct IORequest * )TimeOutIO );
        SetSignal( 0L, SIG_TIMER );
    }

    TimeOutIO->tr_node.io_Command = TR_ADDREQUEST;
    TimeOutIO->tr_time.tv_secs = 2;
    TimeOutIO->tr_time.tv_micro = 0;
    SendIO(( struct IORequest * )TimeOutIO );

    Sigs = Wait( SIG_SERVPORT | SIG_TIMER );

    if( Sigs & SIG_SERVPORT )
    {
        AbortIO(( struct IORequest * )TimeOutIO );
        HandleServerMsg();
    }

    WaitIO(( struct IORequest * )TimeOutIO );
    SetSignal( 0L, SIG_TIMER );
}

VOID FreeResources( LONG Level )
{
    switch( Level )
    {
    case 0:
		if( PingTask )
		{
			Signal( PingTask, SIGBREAKF_CTRL_C );
			SetTaskPri( PingTask, 1 );
			PingTask = 0L;
		}
        SafeKill( "GarshneClient" );
        SafeKill( "GarshnePrefs" );
    case 1:
        CloseInterface();
    case 2:
        FreeBlankerEntries( BlankerEntries );
    case 3:
        if( TimeOutIO )
        {
            if( TimeOutIO->tr_node.io_Device )
                CloseDevice(( struct IORequest * )TimeOutIO );
            DeleteExtIO(( struct IORequest * )TimeOutIO );
        }
    case 4:
        if( TimerPort )
            DeletePort( TimerPort );
        if( ServerPort )
            DeletePort( ServerPort );
    case 5:
        ShutdownCX();
    case 6:
        CloseLibraries();
    }
}

LONG AllocResources( VOID )
{
    struct Process *Proc;

    Proc = ( struct Process * )ServerTask;
    
    if( !Proc->pr_CLI )
        strcpy( ProgName, WBenchMsg->sm_ArgList->wa_Name );
    else
        strcpy( ProgName, "PROGDIR:Garshneblanker" );

    if( OpenLibraries() == 1L )
    {
        bug("OpenLibraries failed\n");
        return 6L;
    }

#if 0
    // FIXME disabled because it returns error.
    if( CheckCX() == QUIT )
    {
        bug("CheckCX failed\n");
        return 6L;
    }
#endif

    ServerPort = CreatePort( "GarshneServer", 0 );
    TimerPort = CreatePort( 0L, 0 );
    if( !ServerPort || !TimerPort )
    {
        bug("ServerPort %p or TimerPort %p wrong\n", ServerPort, TimerPort);
        return 5L;
    }

    TimeOutIO = ( struct timerequest * )
        CreateExtIO( TimerPort, sizeof( struct timerequest ));
    if( !TimeOutIO )
    {
        bug("CreateExtIO for TimerPort failed\n");
        return 4L;
    }

    if( OpenDevice( "timer.device", UNIT_VBLANK,
                   ( struct IORequest * )TimeOutIO, 0L ))
    {
        bug("OpenDevice(timer.device...) failed\n");
        return 3L;
    }

    return 0L;
}
