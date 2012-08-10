/*
 * $Id$
 */

#include <clib/alib_protos.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/lists.h>

#include <aros/io.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <stdio.h>

#include "ppp.h"
#include "device_protos.h"
#include LC_LIBDEFS_FILE


BOOL SafePutToPort(struct PPPcontrolMsg *message, STRPTR portname)
{
	struct MsgPort *port;
	Forbid();
		port = FindPort(portname);
		if (port){
			struct Message *M;
			ForeachNode(&port->mp_MsgList,M){
				if( (APTR)message == (APTR)M ){
					//	bug("SafePutToPort: message is already here !\n");
					Permit();
					return FALSE;		
				};
			}
			PutMsg(port,(struct Message *)message);
		}
	Permit();
	return(port ? TRUE : FALSE);
}

#define TIMERVALUE 5
#define MAXINFOFAIL 10

VOID PPP_Process(VOID){

	struct Process *proc;
	struct IOExtSer *ioser;
	struct IOSana2Req *ios2;
	struct EasyTimer *timer=0;
	ULONG waitmask,signals;
	BYTE signalbit;
	LIBBASETYPEPTR LIBBASE;
	struct MsgPort *CtrlPort=0;
	struct PPPcontrolMsg *CtrlMsg=0;
	struct PPPcontrolMsg *InfoMsg=0;
	ULONG oldin=0,oldout=0;
	ULONG InfoFail=0;
	
	UBYTE GUIPortName[PPP_MAXARGLEN];
	BOOL UpdateInfo;

	bug("PPP process  hello!\n");

	do{
		LIBBASE = FindTask(NULL)->tc_UserData;

		GUIPortName[0] = 0;

		NEWLIST((struct List *)&LIBBASE->Rx_List);
		NEWLIST((struct List *)&LIBBASE->Tx_List);
		InitSemaphore(&LIBBASE->sdu_ListLock);

		proc = (struct Process *)FindTask(0L);
		signalbit = AllocSignal(-1L);
		if(signalbit == -1) break;
		LIBBASE->sd_Unit->unit_MsgPort.mp_SigBit = signalbit;
		LIBBASE->sd_Unit->unit_MsgPort.mp_SigTask = (struct Task *)proc;
		LIBBASE->sd_Unit->unit_MsgPort.mp_Flags = PA_SIGNAL;

		if( !( CtrlPort = CreatePort("ppp-control",0) ) ) break;
		if( !(InfoMsg = AllocMem(sizeof(struct PPPcontrolMsg),MEMF_PUBLIC | MEMF_CLEAR))) break;

		if( ! (timer=OpenTimer())) break;

		bug("PPP process: for ever loop...\n");

		SetTimer(timer,TIMERVALUE);
		LIBBASE->sdu_Proc_run = TRUE;

		for(;;){

			// if Serial device is not ok, close it.
			if(LIBBASE->ser ){
				if( ! LIBBASE->ser->Ok){
					CloseSerial(LIBBASE->ser);
				}
			}

			// device is down, close serial
			if( Phase() == PPP_PHASE_DEAD ){
				CloseSerial(LIBBASE->ser);
			}
			
			waitmask = (1L<< signalbit ) |
					 ( LIBBASE->ser ? (1L<< LIBBASE->ser->RxPort->mp_SigBit ) : 0 ) |
					 ( LIBBASE->ser ? (1L<< LIBBASE->ser->TxPort->mp_SigBit ) : 0 ) |
					   (1L<< timer->TimeMsg->mp_SigBit ) |
					   (1L<< CtrlPort->mp_SigBit ) |
					   SIGBREAKF_CTRL_C  ;

			signals = Wait(waitmask);

			UpdateInfo = FALSE;

			// Time Out
			if(GetMsg(timer->TimeMsg)){
				// Calculate speed
				LIBBASE->SpeedIn =  ( LIBBASE->BytesIn - oldin ) / TIMERVALUE;
				LIBBASE->SpeedOut =  ( LIBBASE->BytesOut - oldout ) / TIMERVALUE;
				LIBBASE->UpTime += TIMERVALUE;
				oldin = LIBBASE->BytesIn;
				oldout = LIBBASE->BytesOut;
				
				if( GUIPortName[0] && (
					InfoMsg->BytesIn != LIBBASE->BytesIn ||
					InfoMsg->BytesOut != LIBBASE->BytesOut ||
					InfoMsg->SpeedIn != LIBBASE->SpeedIn ||
					InfoMsg->SpeedOut != LIBBASE->SpeedOut
				)) UpdateInfo = TRUE;

				if(LIBBASE->device_up && LIBBASE->ser ){
					ppp_timer(TIMERVALUE);
				}

				SetTimer(timer,TIMERVALUE);

			}


			// Have we been signaled to shut down?
			if(signals & SIGBREAKF_CTRL_C){
				bug("PPP process: received SIGBREAKF_CTRL_C\n");
				break;
			}

			// SANA2
			while(ios2 = (struct IOSana2Req *)GetMsg((struct MsgPort *)LIBBASE->sd_Unit)){
				PerformIO(LIBBASE,ios2);
			}

			// Serial handling
			if( LIBBASE->ser && Phase() != PPP_PHASE_DEAD ){
				if(ioser = (struct IOExtSer *)GetMsg(LIBBASE->ser->RxPort))	{
					CMD_READ_Ready(LIBBASE,ioser);
				}
				if(ioser = (struct IOExtSer *)GetMsg(LIBBASE->ser->TxPort)){
					CMD_WRITE_Ready(LIBBASE);
				}
			}


			// Control Port handling
			while( CtrlMsg = (struct PPPcontrolMsg*)GetMsg(CtrlPort) ){
				bug("PPP process: received Control message\n");
				if( CtrlMsg->Msg.mn_Length == sizeof(struct PPPcontrolMsg) ){

					switch(CtrlMsg->Command){

						case PPP_CTRL_SETPHASE:
							Set_phase( (ULONG)CtrlMsg->Arg );
						break;

						case PPP_CTRL_OPEN_SERIAL:
							strlcpy( LIBBASE->DeviceName , CtrlMsg->DeviceName , PPP_MAXARGLEN );
							strlcpy( LIBBASE->username , CtrlMsg->username , PPP_MAXARGLEN );
							strlcpy( LIBBASE->password , CtrlMsg->password , PPP_MAXARGLEN );
							LIBBASE->SerUnitNum = CtrlMsg->UnitNum;								
							if( LIBBASE->ser ) CloseSerial( LIBBASE->ser );
							if( LIBBASE->ser = OpenSerial( LIBBASE->DeviceName , LIBBASE->SerUnitNum ) ){
								init_ppp( LIBBASE );
								Set_phase( PPP_PHASE_CONFIGURATION );
								QueueSerRequest(LIBBASE->ser, PPP_MAXBUFF );
								if( ! LIBBASE->ser->Ok ){
									CloseSerial(LIBBASE->ser);
								} 
							}
						break;

						case PPP_CTRL_CLOSE_SERIAL:
							CloseSerial( LIBBASE->ser );
							Set_phase( PPP_PHASE_DEAD );
						break;

						case PPP_CTRL_INFO_REQUEST:
							bug("PPP:  INFO requester received\n");
							if( strlen( (APTR)CtrlMsg->Arg ) < PPP_MAXARGLEN ){
								strcpy( GUIPortName , (APTR)CtrlMsg->Arg );
								bug("PPP:portname is %s\n",GUIPortName);
							}
						break;

						default:
							bug("ERROR unknow PPP_CTRL\n");
						break;

					}
					UpdateInfo = TRUE;
				}
				ReplyMsg((struct Message *)CtrlMsg);
			}


			if( GUIPortName[0] && (
				InfoMsg->Ser != (LIBBASE->ser ? TRUE:FALSE) ||
				InfoMsg->Up !=  LIBBASE->device_up ||
				InfoMsg->Phase != Phase()
			)) UpdateInfo = TRUE;

			if( GUIPortName[0] && UpdateInfo ){
				//bug("PPP:  INFO reply\n");
				InfoMsg->Ser = LIBBASE->ser ? TRUE:FALSE;
				InfoMsg->Up =  LIBBASE->device_up;
				InfoMsg->Phase = Phase();
				InfoMsg->BytesIn = LIBBASE->BytesIn;
				InfoMsg->BytesOut = LIBBASE->BytesOut;
				InfoMsg->SpeedIn = LIBBASE->SpeedIn;
				InfoMsg->SpeedOut = LIBBASE->SpeedOut;
				InfoMsg->UpTime = LIBBASE->UpTime;
				
				memcpy( InfoMsg->LocalIP , LIBBASE->LocalIP , 4 );
				memcpy( InfoMsg->RemoteIP , LIBBASE->RemoteIP , 4 );
				memcpy( InfoMsg->PrimaryDNS , LIBBASE->PrimaryDNS , 4 );
				memcpy( InfoMsg->SecondaryDNS , LIBBASE->SecondaryDNS , 4 );
				
				InfoMsg->num++;
				InfoMsg->Command = PPP_CTRL_INFO;
				InfoMsg->Msg.mn_Node.ln_Type = NT_MESSAGE;
				InfoMsg->Msg.mn_Length = sizeof(struct PPPcontrolMsg);
				InfoMsg->Msg.mn_ReplyPort = 0;
			//	 bug("PPP: SendInfoMsg num %d -> %s\n",InfoMsg->num,GUIPortName);
				if( SafePutToPort( InfoMsg , GUIPortName ) ){
					InfoFail = 0;
				}else{
					 bug("PPP: SendInfoMsg FAIL\n");
					 if( ++InfoFail > MAXINFOFAIL )GUIPortName[0]=0;
				}
			}

		}

	}while(0);

	bug("PPP process: shut everything down..\n");

	CloseSerial(LIBBASE->ser);
	CloseTimer(timer);

	if(CtrlPort){
		Forbid();
			while( CtrlMsg = (struct PPPcontrolMsg*)GetMsg(CtrlPort) ) ReplyMsg((struct Message *)CtrlMsg);
			DeletePort(CtrlPort);
		Permit();
	}

   	if( InfoMsg ) FreeMem(InfoMsg,sizeof(struct PPPcontrolMsg));
	if(signalbit)  FreeSignal(signalbit);

	bug("PPP process: shut down OK\n");
	Forbid();
	LIBBASE->sdu_Proc_run = FALSE;
}


struct PPP_DevUnit *InitPPPUnit(LIBBASETYPEPTR LIBBASE,ULONG s2unit){

	struct PPP_DevUnit *sdu;
	D(bug("InitPPPUnit\n"));

	if(!LIBBASE->sd_Unit){

		/* Allocate a new Unit structure */
		if(sdu = AllocMem(sizeof(struct Unit), MEMF_CLEAR|MEMF_PUBLIC)){

			LIBBASE->sd_Unit = (struct Unit *)sdu;

			/* Do some initialization on the Unit structure */

			NEWLIST(&LIBBASE->sd_Unit->unit_MsgPort.mp_MsgList);
			LIBBASE->sd_Unit->unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
			LIBBASE->sd_Unit->unit_MsgPort.mp_Flags = PA_IGNORE;
			LIBBASE->sd_Unit->unit_MsgPort.mp_Node.ln_Name = "PPP";

			LIBBASE->sdu_Proc_run = FALSE;

			D(bug("New ppp process:\n"));
			if(LIBBASE->sdu_Proc = CreateNewProcTags(
										NP_Entry, PPP_Process,
										NP_Name, "PPP process",
										NP_Synchronous , FALSE,
										NP_Priority, 1,
										NP_UserData, LIBBASE,
										NP_StackSize, 30000,
										TAG_DONE))

			{
				while( ! LIBBASE->sdu_Proc_run ) Delay(5);
				D(bug("...ok\n"));
			}else{
				D(bug("New process:FAILL !!!\n"));
			}

		}

		if(!LIBBASE->sdu_Proc){
			/* The Unit process couldn't start for some reason, so free the Unit structure. */
			FreeMem(sdu,sizeof(struct Unit));
			LIBBASE->sd_Unit = NULL;
		}

	}
	return((struct PPP_DevUnit *)LIBBASE->sd_Unit);
}


static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE){
	D(bug("[PPP] Init()\n"));

	InitSemaphore(&LIBBASE->sd_Lock);
	LIBBASE->ser  = NULL;
	LIBBASE->device_up  = TRUE;  // hmmm... why this is needed?

	return TRUE;
}


static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE){

	D(bug("[PPP] Expunge()\n"));

	if(LIBBASE->sd_OpenCnt){  // Sorry, we're busy.  We'll expunge later on if we can.
		LIBBASE->sd_Flags |= LIBF_DELEXP;
		D(bug("[PPP] Expunge,busy\n"));
		return FALSE;
	}
	return TRUE;

}


static int GM_UNIQUENAME(Open)
(
	LIBBASETYPEPTR LIBBASE,
	struct IOSana2Req* req,
	ULONG unitnum,
	ULONG flags
){
	struct PPP_DevUnit *sdu;
	struct TagItem *bufftag;

	BOOL status = FALSE;

	D(bug("[PPP] Open unit %d\n",unitnum));

	if(req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req)){
		bug("[PPP] ERROR wrong ios2_Req lenght\n");
		return FALSE;
	}


	ObtainSemaphore(&LIBBASE->sd_Lock);

	LIBBASE->sd_OpenCnt++;

	if(unitnum == 0 ){
		if(sdu = InitPPPUnit(LIBBASE,unitnum)){
			if(bufftag = FindTagItem(S2_CopyToBuff, (struct TagItem *)req->ios2_BufferManagement)){
				LIBBASE->CopyToBuffer =  (APTR)bufftag->ti_Data;
				if(bufftag = FindTagItem(S2_CopyFromBuff, (struct TagItem *)req->ios2_BufferManagement)){
					LIBBASE->CopyFromBuffer =  (APTR)bufftag->ti_Data;

					status = TRUE;

					LIBBASE->sd_Flags &=~LIBF_DELEXP;
					LIBBASE->sd_Unit->unit_OpenCnt++;

					//req->ios2_BufferManagement = (VOID *)bm;
					req->ios2_Req.io_Error = 0;
					req->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
					req->ios2_Req.io_Unit =(APTR)sdu;
					req->ios2_Req.io_Device =(APTR)LIBBASE;
				}
			}
		}
	}

	/* See if something went wrong. */
	if(!status){
		req->ios2_Req.io_Error = IOERR_OPENFAIL;
		req->ios2_Req.io_Unit = (struct Unit *) -1;
		req->ios2_Req.io_Device = (struct Device *) -1;
		LIBBASE->sd_OpenCnt--;
	}

	ReleaseSemaphore(&LIBBASE->sd_Lock);

	return( status );
}


VOID ExpungeUnit(LIBBASETYPEPTR LIBBASE){

	D(bug("[PPP] ExpungeUnit \n"));

	D(bug("[PPP] ExpungeUnit Signal\n"));
	Signal( (struct Task*)LIBBASE->sdu_Proc , SIGBREAKF_CTRL_C );
	D(bug("[PPP] ExpungeUnit Wait\n"));
	while(  LIBBASE->sdu_Proc_run ) Delay(5);

	D(bug("[PPP] ExpungeUnit FreeMem\n"));
	LIBBASE->sd_Unit = NULL;
	FreeMem(LIBBASE->sd_Unit, sizeof(struct Unit));
	D(bug("[PPP] ExpungeUnit ok\n"));
}


static int GM_UNIQUENAME(Close)
(
	LIBBASETYPEPTR LIBBASE,
	struct IOSana2Req* req
){

	D(bug("[PPP] Close\n"));
	ObtainSemaphore(&LIBBASE->sd_Lock);

	CloseSerial(LIBBASE->ser);

	req->ios2_Req.io_Device = (struct Device *) -1;
	req->ios2_Req.io_Unit = (struct Unit *) -1;

	LIBBASE->sd_Unit->unit_OpenCnt--;
	if(!LIBBASE->sd_Unit->unit_OpenCnt){
	//	ExpungeUnit(LIBBASE);
	}

	LIBBASE->sd_OpenCnt--;
	ReleaseSemaphore(&LIBBASE->sd_Lock);

	D(bug("[PPP] Close OK\n"));
	return TRUE;
}


/*
** This function is used to locate an IO request in a linked
** list and abort it if found.
*/
ULONG AbortReq(LIBBASETYPEPTR LIBBASE,struct MinList *minlist, struct IOSana2Req *ios2){
	struct Node *node, *next;
	ULONG result=IOERR_NOCMD;

	node = (struct Node *)minlist->mlh_Head;

	while(node->ln_Succ){
		next = node->ln_Succ;
		if(node == (struct Node *)ios2){
			Remove((struct Node *)ios2);
			ios2->ios2_Req.io_Error = IOERR_ABORTED;
			TermIO(LIBBASE,ios2);
			result = 0;
		}
		node = next;
	}
	return(result);
}



AROS_LH1(void, beginio,
		 AROS_LHA(struct IOSana2Req *, req, A1),
		 LIBBASETYPEPTR, LIBBASE, 5, PPPDev){
	AROS_LIBFUNC_INIT
	if( 0 ){
	//if(  ( ! LIBBASE->ser->RxPort ) &&  LIBBASE->ser->TxPort ){ // PPP_process is busy because openserial wait and wait...
		req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		req->ios2_WireError = S2WERR_UNIT_OFFLINE;
		TermIO(LIBBASE,req);
	}else{
		req->ios2_Req.io_Flags &= ~IOF_QUICK;
		PutMsg(  (struct MsgPort *) req->ios2_Req.io_Unit , (struct Message *)req );
	}

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
		 AROS_LHA(struct IOSana2Req *, req, A1),
		 LIBBASETYPEPTR, LIBBASE, 6, PPPDev){
	AROS_LIBFUNC_INIT
	ULONG result = 0L;

	D(bug("[PPP] AbortIO()\n"));

	Disable();
	if(req->ios2_Req.io_Message.mn_Node.ln_Type != NT_REPLYMSG){
		switch (req->ios2_Req.io_Command){
		case CMD_READ:
			result=AbortReq(LIBBASE,&LIBBASE->Rx_List,req);
			break;

		case CMD_WRITE:
			result=AbortReq(LIBBASE,&LIBBASE->Tx_List,req);
			break;

		default:
			result=IOERR_NOCMD;
			break;
		}
	}
	Enable();
	return result;
	AROS_LIBFUNC_EXIT
}


ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)

