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
#include <ctype.h>
#include <devices/serial.h>
#include <devices/timer.h>

#include "ppp.h"
#include "device_protos.h"
#include LC_LIBDEFS_FILE


void SetTimer(struct EasyTimer* t,const ULONG s){
	if( t ){
		//bug("setTimer\n");
		if( t->TimeReq ){
			AbortIO((struct IORequest *)t->TimeReq);
			WaitIO((struct IORequest *)t->TimeReq);
			while(GetMsg(t->TimeMsg));

			if( s <= 0 ) return;

			t->TimeReq->tr_time.tv_secs = s;
			t->TimeReq->tr_time.tv_micro = 0;
			((struct IORequest *)t->TimeReq)->io_Command = TR_ADDREQUEST;
			SendIO( (struct IORequest *)t->TimeReq );
			//bug("setTimer ok\n");
		}
	}
}


void CloseTimer(struct EasyTimer* t){
	if( t ){
		//bug("closeTimer\n");
		if( t->TimeReq ){
			AbortIO((struct IORequest *)t->TimeReq);
			WaitIO((struct IORequest *)t->TimeReq);
			while(GetMsg(t->TimeMsg));
			CloseDevice((struct IORequest *)t->TimeReq);
			DeleteIORequest(t->TimeReq);
		}
		if(t->TimeMsg) DeleteMsgPort(t->TimeMsg);
		FreeMem( t , sizeof(struct EasyTimer) );
		//bug("closeTimer ok\n");
	}
}

struct EasyTimer* OpenTimer(){
	struct EasyTimer* t;
	//bug("OpenTimer\n");
	if( t = AllocMem( sizeof(struct EasyTimer),MEMF_CLEAR|MEMF_PUBLIC) ){
		if( t->TimeMsg = CreateMsgPort() ){
			if( t->TimeReq = CreateIORequest((struct MsgPort *)t->TimeMsg, sizeof(struct timerequest))){
				if(!OpenDevice("timer.device", UNIT_VBLANK,(struct IORequest *)t->TimeReq, 0)){
					t->TimeReq->tr_time.tv_secs = 0;
					t->TimeReq->tr_time.tv_micro = 1;
					((struct IORequest *)t->TimeReq)->io_Command = TR_ADDREQUEST;
					DoIO( (struct IORequest *)t->TimeReq );
					SetTimer(t,0);
					//bug("OpenTimer ok!\n");
					return t;
				}
			}
		}
	}
	//bug("OpenTimer FAIL!\n");
	CloseTimer(t);
	return NULL;
}


VOID DoStr(struct EasySerial *s,const STRPTR str){

	if( ! s ) return;
	if( ! s->Ok ) return;

	s->SerTx->IOSer.io_Length = strlen((char*) str);
	s->SerTx->IOSer.io_Data = str;
	s->SerTx->IOSer.io_Command =  CMD_WRITE;
	DoIO((struct IORequest*)s->SerTx);

}

void DoBYTES(struct EasySerial *s, BYTE *p,ULONG len){

	if( ! s ) return;
	if( ! s->Ok ) return;

	s->SerTx->IOSer.io_Length = len;
	s->SerTx->IOSer.io_Data = p;
	s->SerTx->IOSer.io_Command =  CMD_WRITE;
	DoIO((struct IORequest*)s->SerTx);
}

void SendBYTES(struct EasySerial *s, BYTE *p, ULONG len){

	if( ! s ) return;
	if( ! s->Ok ) return;

	s->SerTx->IOSer.io_Length = len;
	s->SerTx->IOSer.io_Data = p;
	s->SerTx->IOSer.io_Command =  CMD_WRITE;
	SendIO((struct IORequest*)s->SerTx);
}


struct EasySerial *OpenSerial(BYTE *name,ULONG unit){
	struct EasySerial *s = NULL;
	D(bug("OpenSerial\n"));
	
	do{
		
		if( ! (s = AllocMem( sizeof(struct EasySerial),MEMF_CLEAR|MEMF_PUBLIC))) break;
		if( ! (s->TxBuff = AllocMem( SERIAL_BUFSIZE ,MEMF_CLEAR|MEMF_PUBLIC))) break;
		if( ! (s->RxBuff = AllocMem( SERIAL_BUFSIZE ,MEMF_CLEAR|MEMF_PUBLIC))) break;
		if( ! (s->TxPort = CreateMsgPort())) break;
		if( ! (s->SerTx = CreateIORequest(s->TxPort,sizeof(struct IOExtSer)))) break;
		

		D(bug("OpenDevice: \"%s\" unit %d\n",name,unit));
		if(  OpenDevice( name , unit , (struct IORequest *)s->SerTx,0)){
			DeleteIORequest(s->SerTx);
			s->SerTx = NULL;
			break;
		}

		bug("Test CMD_WRITE\n");
		s->SerTx->IOSer.io_Length = 0;
		s->SerTx->IOSer.io_Data = " ";
		s->SerTx->IOSer.io_Command =  CMD_WRITE;
		if( DoIO((struct IORequest *)s->SerTx)){
			break;
		}
		
		if( ! (s->RxPort = CreateMsgPort()) ) break;
		if( ! (s->SerRx = CreateIORequest(s->RxPort,sizeof(struct IOExtSer)))) break;
		
		bug("Test SDCMD_QUERY\n");
		s->SerRx->IOSer.io_Device = s->SerTx->IOSer.io_Device;
		s->SerRx->IOSer.io_Unit   = s->SerTx->IOSer.io_Unit;
		s->SerRx->IOSer.io_Command = SDCMD_QUERY;
		if( DoIO((struct IORequest *)s->SerRx)){
			break;
		}
		
		D(bug("OpenSerial OK\n"));
		s->Ok = TRUE;
		return s;
	
	}while(0);
	
	D(bug("OpenSerial FAIL !!\n"));
	CloseSerial(s);
	return NULL;
}


VOID CloseSerial(struct EasySerial *s){

	if( ! s ) return ;

	D(bug("CloseSerial\n"));
	
	s->Ok = FALSE;
	
	if( s->SerRx ){
		AbortIO((struct IORequest *)s->SerRx);
		WaitIO((struct IORequest *)s->SerRx);
		while(GetMsg(s->RxPort));
	}

	if( s->SerTx ){
		AbortIO((struct IORequest *)s->SerTx);
		WaitIO((struct IORequest *)s->SerTx);
		while(GetMsg(s->TxPort));
	}

	if(s->SerTx) CloseDevice((struct IORequest *)s->SerTx);

	if(s->SerTx) DeleteIORequest(s->SerTx);
	if(s->TxPort) DeleteMsgPort(s->TxPort);

	if(s->SerRx) DeleteIORequest(s->SerRx);
	if(s->RxPort) DeleteMsgPort(s->RxPort);

	if( s->TxBuff ) FreeMem( s->TxBuff , SERIAL_BUFSIZE );
	if( s->RxBuff ) FreeMem( s->RxBuff , SERIAL_BUFSIZE );
	FreeMem( s , sizeof(struct EasySerial) );

	D(bug("CloseSerial OK!\n"));
}


VOID QueueSerRequest(struct EasySerial *s , LONG maxlength){

	if( ! s ) return;
	if( ! s->Ok ) return;
	if( maxlength < 1 ) return;
	
	if( maxlength > SERIAL_BUFSIZE ) maxlength = SERIAL_BUFSIZE;
	s->SerRx->IOSer.io_Command = SDCMD_QUERY;
	DoIO((struct IORequest *)s->SerRx);

	if( s->SerRx->IOSer.io_Error ){
		bug("QueueSerRequest() OOOOPS lost device!\n");
		s->Ok = FALSE;
		return;
	}

	s->SerRx->IOSer.io_Command = CMD_READ;
	s->SerRx->IOSer.io_Data = s->RxBuff;
	s->SerRx->IOSer.io_Length = s->SerRx->IOSer.io_Actual ;

	if( s->SerRx->IOSer.io_Length > maxlength )
		s->SerRx->IOSer.io_Length = maxlength;
	/* If the number of bytes available is zero, queue a request
	   for one byte. */
	if(!s->SerRx->IOSer.io_Length)
		s->SerRx->IOSer.io_Length = 1;

	SendIO((struct IORequest *)s->SerRx);
	if( s->SerRx->IOSer.io_Error ){
		bug("QueueSerRequest()  SendIO -> CMD_READ error!");
	}

}







