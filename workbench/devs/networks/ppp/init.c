/*
 * $Id$
 */

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/lists.h>

#include <aros/io.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>
#include <devices/timer.h>
#include <devices/serial.h>
#include <devices/timer.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/timer.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <stdio.h>

#include <oop/oop.h>
#include <hidd/pci.h>

#include <ctype.h>

#include "ppp.h"
#include "device_protos.h"
#include LC_LIBDEFS_FILE


BOOL GetToken(BYTE *b,BYTE *t){
	BYTE *tok;
	ULONG tlen;
	for(tok=b;*tok==' '|*tok=='\n'|*tok=='\r';tok++ );
	for(tlen=0 ;tok[tlen]!=' '&&tok[tlen]!=0&&tok[tlen]!='\n'&&tok[tlen]!='\r';tlen++); // I am sorry
	tok[tlen] = 0;
	if( tlen < 1 || tlen >= PPP_MAXARGLEN ) return FALSE;
	strcpy(t,tok);
	for( ; b < tok+tlen+1 ; b++ )*b=' ';
	//bug("t:%s:",tok);
	return TRUE;
}

BOOL GetLineEnd(BYTE *b,BYTE *t){
	BYTE *tok;
	ULONG tlen;
	for(tok=b;*tok==' '|*tok=='\n'|*tok=='\r';tok++ );
	for(tlen=0;tok[tlen]!=0&&tok[tlen]!='\n'&&tok[tlen]!='\r';tlen++);
	tok[tlen] = 0;
	if( tlen < 1 || tlen >= PPP_MAXARGLEN ) return FALSE;
	strcpy(t,tok);
	while( tok[ strlen( tok ) ] == ' ' ) tok[ strlen( tok ) ] = 0;
	//bug("t:%s:",tok);
	return TRUE;
}


BOOL ReadConfig(LIBBASETYPEPTR LIBBASE){
	UBYTE *linebuff,tok[PPP_MAXARGLEN];
	BPTR ConfigFile;
	struct at_command  *atc;

	D(bug("ReadConfig:  ENV:AROSTCP/db/ppp.config\n"));
	strcpy( LIBBASE->modemmodel , "PPP");
	strcpy( LIBBASE->username ,   "DummyName");
	strcpy( LIBBASE->password ,   "DummyName");
	strcpy( LIBBASE->DeviceName , "DummyName");
	LIBBASE->SerUnitNum = 0;
	LIBBASE->enable_dns = FALSE;

	while( atc = (struct at_command *)RemHead( &LIBBASE->atcl ) ){
		FreeMem( atc , sizeof(struct at_command) );
	}

	if(ConfigFile = Open("ENV:AROSTCP/db/ppp.config",MODE_OLDFILE)){

		if(linebuff = AllocMem(256,MEMF_CLEAR|MEMF_PUBLIC)){

			while(FGets(ConfigFile, linebuff, 255)){

				if( ( linebuff[0] == '#' ) | ( linebuff[0] == ';' ) ) /* Skip comment lines */
					continue;

				//  bug("line:%s:\n",linebuff);

				if( GetToken(linebuff,tok) ){

					if( strcasecmp("DEVICE",tok) == 0 ){
						if( GetToken(linebuff,tok) ) strcpy( LIBBASE->DeviceName , tok );
					}else if( strcasecmp("UNIT",tok) == 0 ){
						if( GetToken(linebuff,tok) ) LIBBASE->SerUnitNum = atoi( tok );
					}else if( strcasecmp("USERNAME",tok) == 0 ){
						if( GetToken(linebuff,tok) ) strcpy( LIBBASE->username , tok );
					}else if( strcasecmp("PASSWORD",tok) == 0 ){
						if( GetToken(linebuff,tok) ) strcpy( LIBBASE->password , tok );
					}else if( strcasecmp("ENABLE",tok) == 0 ){
						if( GetToken(linebuff,tok) ){
							if( strcasecmp("DNS",tok) == 0 ){
								LIBBASE->enable_dns = TRUE;
							}
						}
					}

					if( strcasecmp("SEND",tok) == 0 ){
						if( GetLineEnd(linebuff,tok) ){
							if(atc = AllocMem(sizeof(struct at_command), MEMF_CLEAR | MEMF_PUBLIC )){
								strcpy( atc->str , tok );
								atc->command = COM_SEND;
								AddTail( &LIBBASE->atcl , (struct Node*)atc );
							}
						}
					}

					else if( strcasecmp("WAIT",tok) == 0 ){
						if( GetToken(linebuff,tok) ){
							if(atc = AllocMem(sizeof(struct at_command), MEMF_CLEAR | MEMF_PUBLIC )){
								strcpy( atc->str , tok );
								atc->command = COM_WAIT;
								if( GetToken(linebuff,tok) ){
									atc->arg = atoi( tok );
								}else{
									atc->arg = 5;
								}
								AddTail( &LIBBASE->atcl , (struct Node*)atc );
							}
						}
					}

					else if( strcasecmp("DELAY",tok) == 0 ){
						if(atc = AllocMem(sizeof(struct at_command), MEMF_CLEAR | MEMF_PUBLIC )){
							atc->command = COM_DELAY;
							if( GetToken(linebuff,tok) ){
								atc->arg = atoi( tok );
							}else{
								atc->arg = 5;
							}
							AddTail( &LIBBASE->atcl , (struct Node*)atc );
						}
					}


				}

			}
			FreeMem(linebuff, 256);
		}
		Close(ConfigFile);
	}else{
		bug("ppp.config missing !!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
	return TRUE;
}



BOOL TestModem(LIBBASETYPEPTR LIBBASE){

	UBYTE buf[PPP_MAXARGLEN];
	UBYTE buf2[PPP_MAXARGLEN];

	bug("ModemTest\n",buf);
	DoStr( LIBBASE->ser ,  "\r\r\r" );

	DrainSerial(LIBBASE->ser);
	DoStr( LIBBASE->ser,  "ATZ\r" );
	if( GetResponse(LIBBASE->ser,buf,PPP_MAXARGLEN,5)){
		if( strcasestr(buf,"OK") == NULL ){
			bug("ATZ FAIL\n");
			return FALSE;
		}
	}

	// echo off
	DrainSerial(LIBBASE->ser);
	DoStr( LIBBASE->ser,  "ATE 0\r" );
	if( GetResponse(LIBBASE->ser,buf,PPP_MAXARGLEN,5)){
		if( strcasestr(buf,"OK") == NULL ){
			bug("ATE 0 FAIL\n");
			return FALSE;
		}
	}

	// Get modem model
	DrainSerial(LIBBASE->ser);
	DoStr( LIBBASE->ser,  "AT+GMM\r" );
	if( GetResponse(LIBBASE->ser,buf,PPP_MAXARGLEN,5)){
		if( strcasestr(buf,"OK") == NULL ){
			bug("AT+GMM FAIL\n");
			return FALSE;
		}
		if( GetLineEnd(buf,buf2)){
			 strcpy( LIBBASE->modemmodel , buf2 );
		}
	}

	bug("ModemTest OK\n",buf);
	return TRUE;
}

#define MAXBUF 256
BOOL DialUp(LIBBASETYPEPTR LIBBASE){

	struct at_command *atc=NULL;
	UBYTE buf[MAXBUF];
	TestModem(LIBBASE);

	ForeachNode(&LIBBASE->atcl,atc){

		if( atc->command == COM_DELAY ){
			bug("DELAY %d sec. RESPONSE IS:\n",atc->arg);
			Delay(50*atc->arg);
			DrainSerial(LIBBASE->ser);
			bug("\n");
		}

		else if( atc->command == COM_WAIT ){
			bug("WAIT \"%s\" %d sec. RESPONSE IS:\n",atc->str , atc->arg);

			if( ! GetResponse(LIBBASE->ser,buf,MAXBUF,atc->arg)){
				bug("...FAIL:TIMEOUT ?:\n%s\n",buf);
				return FALSE;
			}else{
				if( strcasestr(buf,atc->str) == NULL ){
					bug("...FAIL:WRONG RESPONSE:\n%s\n",buf);
					return FALSE;
				}
				bug("%s\n",buf);
			}
		}

		else if( atc->command == COM_SEND ){
			DrainSerial(LIBBASE->ser);
			bug("SEND \"%s\"\n",atc->str);
			DoStr( LIBBASE->ser, atc->str);
			DoStr( LIBBASE->ser,  "\r" );
		}
	}

	return TRUE;

}


VOID PPP_Process(VOID){
	struct Process *proc;
	struct IOExtSer *ioser;
	struct IOSana2Req *ios2;
	struct EasyTimer *timer;

	ULONG waitmask,signals;
	UBYTE signalbit;

	LIBBASETYPEPTR LIBBASE;
	LIBBASE = FindTask(NULL)->tc_UserData;

	bug("PPP process  hello!\n");

	proc = (struct Process *)FindTask(0L);

	signalbit = AllocSignal(-1L);

	if(signalbit != -1){

		LIBBASE->sd_Unit->unit_MsgPort.mp_SigBit = signalbit;
		LIBBASE->sd_Unit->unit_MsgPort.mp_SigTask = (struct Task *)proc;
		LIBBASE->sd_Unit->unit_MsgPort.mp_Flags = PA_SIGNAL;

		InitSemaphore(&LIBBASE->sdu_ListLock);

		NEWLIST((struct List *)&LIBBASE->Rx_List);
		NEWLIST((struct List *)&LIBBASE->Tx_List);
		NEWLIST(&LIBBASE->atcl);

		if(timer=OpenTimer()){

			bug("PPP process: forewer loop...\n");

			SetTimer(timer,5);

			LIBBASE->sdu_Proc_run = TRUE;

			for(;;){

				waitmask = (1L<< signalbit ) |
						   (1L<< LIBBASE->ser->RxPort->mp_SigBit ) |
						   (1L<< LIBBASE->ser->TxPort->mp_SigBit ) |
						   (1L<< timer->TimeMsg->mp_SigBit ) |
						   SIGBREAKF_CTRL_F |
						   SIGBREAKF_CTRL_C  ;

				signals = Wait(waitmask);

				if(GetMsg(timer->TimeMsg)){

					 bug("PPP process: Timer\n");
					// bug(" ser %d,ppp %d,dev %d\n",LIBBASE->serial_ok,( Phase() == PPP_PHASE_NETWORK ),LIBBASE->device_up );

					if( LIBBASE->device_up && ( ! LIBBASE->ser ) ){

						ReadConfig(LIBBASE);

						D(bug("[PPP] ModemDemonProcess: trying OpenSerial..!\n"));
						if( LIBBASE->ser = OpenSerial(LIBBASE->DeviceName,LIBBASE->SerUnitNum) ){
							D(bug("[PPP] ModemDemonProcess: Serial OK !\n"));
							open_gui(LIBBASE);
							init_ppp( LIBBASE );
						}

					}

					if( LIBBASE->device_up && LIBBASE->ser && Phase() == PPP_PHASE_DEAD ){
						if( DialUp(LIBBASE) ){
							QueueSerRequest(LIBBASE->ser, PPP_MAXBUFF );
							Set_phase( PPP_PHASE_CONFIGURATION );
						}
					}

					if(LIBBASE->device_up && LIBBASE->ser ){
						ppp_timer(5);
					}
					SetTimer(timer,5);

				}

				/* signal from gui process? */
				if(signals & SIGBREAKF_CTRL_F){
					bug("PPP process: received SIGBREAKF_CTRL_F\n");
					switch(LIBBASE->gui_message){

						case GUIM_DISCONNECT:
							LIBBASE->device_up = FALSE;
							Set_phase( PPP_PHASE_DEAD );
							if( LIBBASE->ser ){
								SendTerminateReq();
								Delay(300);
								CloseSerial(LIBBASE->ser);
								LIBBASE->ser = NULL;
							}
							LIBBASE->device_up = FALSE;
						break;

						case GUIM_CONNECT:
							LIBBASE->device_up = TRUE;
						break;

					}
				}

				/* Have we been signaled to shut down? */
				if(signals & SIGBREAKF_CTRL_C){
					bug("PPP process: received SIGBREAKF_CTRL_C\n");
					break;
				}

				BOOL More = TRUE;
				while( More ){

					More = FALSE;

					if(ios2 = (struct IOSana2Req *)GetMsg((struct MsgPort *)LIBBASE->sd_Unit)){
						More = TRUE;
						PerformIO(LIBBASE,ios2);
					}

					if( LIBBASE->ser ){
						if(ioser = (struct IOExtSer *)GetMsg(LIBBASE->ser->RxPort))	{
							More = TRUE;
							CMD_READ_Ready(LIBBASE,ioser);
						}

						if(ioser = (struct IOExtSer *)GetMsg(LIBBASE->ser->TxPort)){
							More = TRUE;
							CMD_WRITE_Ready(LIBBASE);
						}
					}
				}
			}
		}
	}
	bug("PPP process: shut everything down..\n");

	CloseSerial(LIBBASE->ser);
	LIBBASE->ser = NULL;
	
	CloseTimer(timer);

	struct at_command  *atc;
	while( atc = (struct at_command *)RemHead( &LIBBASE->atcl ) ){
		FreeMem( atc , sizeof(struct at_command) );
	}

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
			LIBBASE->gui_run = FALSE;

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

	close_gui(LIBBASE);

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
	LIBBASE->ser = NULL;

	/* Trash the io_Device and io_Unit fields so that any attempt to use this
	   request will die immediatly. */

	req->ios2_Req.io_Device = (struct Device *) -1;
	req->ios2_Req.io_Unit = (struct Unit *) -1;

	/* I always shut the unit process down if the open count drops to zero.
	   That way, if I need to expunge, I never have to Wait(). */

	LIBBASE->sd_Unit->unit_OpenCnt--;

	if(!LIBBASE->sd_Unit->unit_OpenCnt){
		ExpungeUnit(LIBBASE);
	}

	LIBBASE->sd_OpenCnt--;
	ReleaseSemaphore(&LIBBASE->sd_Lock);

	D(bug("[PPP] Close OK\n"));
	return TRUE;
}


/*
** This funcion is used to locate an IO request in a linked
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

	if(  ( ! LIBBASE->ser->RxPort ) &&  LIBBASE->ser->TxPort ){ // PPP_process is busy because openserial wait and wait...
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
	return 0;
	AROS_LIBFUNC_EXIT
}


ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)











