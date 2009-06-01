/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <aros/debug.h>

#include <exec/memory.h>
#include <libraries/commodities.h>
#include <devices/inputevent.h>

#include "includes.h"
#include "libraries.h"
#include "protos/protos.h"

BlankMsg *InterruptMsg;
struct MsgPort *CxPort;
CxObj *ServerBroker;
LONG timeCount = 0;

// FIXME __interrupt
VOID CxBFunc( CxMsg *CxMessage, CxObj *CxObject )
{
    struct InputEvent *Event = ( struct InputEvent * )CxMsgData( CxMessage );
    
    switch( Event->ie_Class )
    {
    case IECLASS_TIMER:
        if( ++timeCount >=
           ( Blanking ? Prefs->bp_RandTimeout : Prefs->bp_Timeout ))
        {
            InterruptMsg->bm_Flags = BF_INTERNAL;
            InterruptMsg->bm_Type = BM_SENDBLANK;
            PutMsg( ServerPort, ( struct Message * )InterruptMsg );
            timeCount = 0;
        }
        return;
    case IECLASS_RAWMOUSE:
        if(( Event->ie_Code == IECODE_NOBUTTON )&&
           ( Event->ie_Qualifier & IEQUALIFIER_RELATIVEMOUSE )&&
           ( Prefs->bp_BlankCorner || Prefs->bp_DontCorner ))
        {
            Signal( ServerTask, SIGBREAKF_CTRL_D );
        }
        break;
    case IECLASS_RAWKEY:
        if( Event->ie_Code & IECODE_UP_PREFIX )
            return;
    }

    if( Blanking )
    {
        Blanking = FALSE;
        InterruptMsg->bm_Flags = BF_INTERNAL;
        InterruptMsg->bm_Type = BM_SENDUNBLANK;
        PutMsg( ServerPort, ( struct Message * )InterruptMsg );
    }
    timeCount = 0;
}

LONG HandleCxMess( VOID )
{
    LONG msgid, msgtype;
    CxMsg *msg;
    
    while( msg = ( CxMsg * )GetMsg( CxPort ))
    {
        msgid = CxMsgID( msg );
        msgtype = CxMsgType( msg );
        ReplyMsg(( struct Message * )msg );
        
        switch( msgtype )
        {
        case CXM_IEVENT:
            switch( msgid )
            {
            case EVT_CX_POPUP:
                OpenInterface();
                break;
            case EVT_CX_BLANK:
                InterruptMsg->bm_Flags = BF_INTERNAL;
                InterruptMsg->bm_Type = BM_SENDBLANK;
                PutMsg( ServerPort, ( struct Message * )InterruptMsg );
                break;
            }
            break;
        case CXM_COMMAND:
            switch( msgid )
            {
            case CXCMD_DISABLE:
                ActivateCxObj( ServerBroker, 0l );
                break;
            case CXCMD_ENABLE:
                ActivateCxObj( ServerBroker, 1l );
                break;
            case CXCMD_KILL:
                return QUIT;
            case CXCMD_APPEAR:
            case CXCMD_UNIQUE:
                OpenInterface();
                break;
            case CXCMD_DISAPPEAR:
                CloseInterface();
                break;
            default:
                break;
            }
        default:
            break;
        }
    }

    return OK;
}

VOID ShutdownCX( VOID )
{
    CxMsg *msg;
    
    if( InterruptMsg )
        FreeVec( InterruptMsg );

    if( CxPort )
    {
        if( ServerBroker )
            DeleteCxObjAll( ServerBroker );
        ServerBroker = 0L;
        
        while( msg = ( CxMsg * )GetMsg( CxPort ))
            ReplyMsg(( struct Message * )msg );
        DeletePort( CxPort );
        CxPort = 0L;
    }
}

CxObj *GarshneBroker( LONG Pri, struct MsgPort *Port, LONG *cxError )
{
    struct NewBroker Broker;

    Broker.nb_Version = NB_VERSION;
    Broker.nb_Name = "Garshneblanker";
    Broker.nb_Title = VERS;
    Broker.nb_Descr = "Groovy modular screen blanker";
    Broker.nb_Unique = NBU_UNIQUE|NBU_NOTIFY;
    Broker.nb_Flags = COF_SHOW_HIDE;
    Broker.nb_Pri = Pri;
    Broker.nb_Port = Port;
    
    return CxBroker( &Broker, cxError );
}
    
LONG SetupCX( VOID )
{
    InterruptMsg = AllocVec( sizeof( BlankMsg ), MEMF_CLEAR|MEMF_PUBLIC );
    if( !InterruptMsg )
        return QUIT;
    
    InterruptMsg->bm_Mess.mn_ReplyPort = ServerPort;
    InterruptMsg->bm_Mess.mn_Length = sizeof( BlankMsg );
        
    if( CxPort = CreatePort( 0L, 0L ))
    {
        LONG cxError = 0L;
    
        ServerBroker = GarshneBroker( Prefs->bp_Priority, CxPort, &cxError );

        if( cxError == CBERR_OK )
        {
            CxObj *pHotKey, *bHotKey;
            
            ActivateCxObj( ServerBroker, 0l );

            pHotKey = CxFilter( Prefs->bp_PopKey );
            AttachCxObj( pHotKey, CxSender( CxPort, EVT_CX_POPUP ));
            AttachCxObj( pHotKey, CxTranslate( 0L ));
            AttachCxObj( ServerBroker, pHotKey );
            
            bHotKey = CxFilter( Prefs->bp_BlankKey );
            AttachCxObj( bHotKey, CxSender( CxPort, EVT_CX_BLANK ));
            AttachCxObj( bHotKey, CxTranslate( 0L ));
            AttachCxObj( ServerBroker, bHotKey );
            
            AttachCxObj( ServerBroker, CxCustom( CxBFunc, 0L ));

            if( !CxObjError( ServerBroker ))
            {
                ActivateCxObj( ServerBroker, 1l );
                return OK;
            }
        }
    }
    ShutdownCX();

    return QUIT;
}

LONG CheckCX( VOID )
{
    LONG cxError = 0L;
    
    ServerBroker = GarshneBroker( 0L, 0L, &cxError );
    DeleteCxObj( ServerBroker );

    bug("CheckCX error %d\n", cxError);
    return ( cxError == CBERR_OK ) ? OK : QUIT;
}
