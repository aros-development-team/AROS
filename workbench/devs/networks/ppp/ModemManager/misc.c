/*
 * $Id$
 */

#define DEBUG 1

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <exec/lists.h>

#ifdef __AROS__
#include <aros/io.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#endif

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
#ifdef __AROS__
#include <proto/oop.h>
#endif
#include <proto/timer.h>
#include <proto/utility.h>

#include <stdlib.h>
#include <stdio.h>
#include <aros/debug.h>
	 
#ifdef __AROS__
#include <oop/oop.h>
#include <hidd/pci.h>
#endif

#include <ctype.h>
#include <ctype.h>
#include <devices/serial.h>
#include <devices/timer.h>

#include "ppp.h"
#include "misc.h"

#define STRSIZE 1000

#define PREFSFILE "ENV:MobileBroadband.prefs"
#ifdef __MORPHOS__
#define INTERFACEFILE "ENV:sys/net/interfaces"
#else
#define INTERFACEFILE "ENV:AROSTCP/db/interfaces"
#endif

BOOL StartStack()
{
	ULONG trycount = 0;
	TEXT arostcppath[256];

	struct TagItem tags[] =
	{
		{ SYS_Input,  (IPTR)NULL },
		{ SYS_Output, (IPTR)NULL },
		{ SYS_Error,  (IPTR)NULL },
		{ SYS_Asynch, (IPTR)TRUE },
		{ TAG_DONE,   0          }
	};

	arostcppath[0] = 0;
	GetVar( "SYS/Packages/AROSTCP" , arostcppath , 256 , LV_VAR);
	AddPart(arostcppath, "C", 256);
	AddPart(arostcppath, "AROSTCP", 256);

	bug("Start AROSTCP: %s\n",arostcppath);
	SystemTagList(arostcppath, tags);

	/* Check if startup successful */
	trycount = 0;
	while (!FindTask("bsdsocket.library"))
	{
		if (trycount > 9) return FALSE;
		Delay(50);
		trycount++;
	}
	return TRUE;
}


BOOL ReadConfig(struct Conf *c){
	TEXT linebuff[1024],*tok;
	BPTR ConfigFile;
	struct at_command  *atc;
	BOOL result = TRUE;
	
	strcpy( c->modemmodel , "Unknow");
	strcpy( c->username ,   "DummyName");
	strcpy( c->password ,   "DummyName");
	strcpy( c->DeviceName , "usbmodem.device");
	c->InterfaceName[0] = 0;

	c->SerUnitNum = -1;  // default: test all units
	c->CommandTimeOut = 10; // default timeout 10 sec.

	while( atc = (struct at_command *)RemHead( &c->atcl ) ){
		FreeMem( atc , sizeof(struct at_command) );
	}

	bug("Read config file:%s\n",PREFSFILE);
	if(ConfigFile = Open( PREFSFILE ,MODE_OLDFILE)){
		while(FGets(ConfigFile, linebuff, STRSIZE )){
			if( ( linebuff[0] == '#' ) | ( linebuff[0] == ';' ) ) /* Skip comment lines */
				continue;

			//  bug("line:%s:\n",linebuff);
			
			if( tok = strtok( &linebuff[0]," \t\n") ){

				if( strcasecmp("DEVICE",tok) == 0 ){
					if( tok = strtok( NULL , " \n" ) ) strcpy( c->DeviceName , tok );
				}else if( strcasecmp("UNIT",tok) == 0 ){
					if( tok = strtok( NULL , " \n" ) ) c->SerUnitNum = atoi( tok );
				}else if( strcasecmp("TIMEOUT",tok) == 0 ){
					if( tok = strtok( NULL , " \n" ) ) c->CommandTimeOut = atoi( tok );	
				}else if( strcasecmp("USERNAME",tok) == 0 ){
					if( tok = strtok( NULL , " \n" ) ) strcpy( c->username , tok );
				}else if( strcasecmp("PASSWORD",tok) == 0 ){
					if( tok = strtok( NULL , " \n" ) ) strcpy( c->password , tok );
				}

				else if( strcasecmp("SEND",tok) == 0 ){
					if( tok = strtok( NULL , "\n" ) ){
						if(atc = AllocMem(sizeof(struct at_command), MEMF_CLEAR | MEMF_PUBLIC )){
							strcpy( atc->str , tok );
							AddTail( &c->atcl , (struct Node*)atc );
						}
					}
				}
			}
		}
		Close(ConfigFile);		
	}else{
		bug("ModemManager:Config file missing !!!!\n");
		result = FALSE;
	}
		
	bug("Read config file:%s\n",INTERFACEFILE);
	if(ConfigFile = Open( INTERFACEFILE ,MODE_OLDFILE)){
		while(FGets(ConfigFile, linebuff, STRSIZE )){
			if( ( linebuff[0] == '#' ) | ( linebuff[0] == ';' ) ) /* Skip comment lines */
				continue;
			if( strcasestr( linebuff , "ppp.device" ) != NULL ){
				if( tok = strtok( linebuff , " " ) ){ strcpy( c->InterfaceName , tok );
					break;
				}
			}
		}
		Close(ConfigFile);		
	}else{
		bug("ModemManager:Config file missing !!!!\n");
		result = FALSE;
	}

	if( c->InterfaceName[0] == 0 ){
		bug("ModemManager:No PPP interface defined !!!!\n");
		result = FALSE;
	}
	
	bug("Config:\n");
	bug("    Interface %s\n",c->InterfaceName);
	bug("    Device    %s\n",c->DeviceName);
	bug("    Unit      %d\n",c->SerUnitNum);
	bug("    username  %s\n",c->username);
	bug("    password  %s\n",c->password);
	bug("    TimeOut   %d\n",c->CommandTimeOut);
	ULONG i=0;
	ForeachNode(&c->atcl,atc){
		bug("    Init%d \"%s\"\n",++i,atc->str);
	}
	
	return result;
}


BOOL TestModem(struct EasySerial *s,struct Conf *c){

	UBYTE *buf,*tok;
	BOOL result=FALSE;
	bug("ModemTest\n");
    UBYTE delim[] = ": ,\n\r";
	
	if( buf = AllocMem(STRSIZE,MEMF_CLEAR|MEMF_PUBLIC)){
		
		result=TRUE;
		c->signal=-1;
		c->AccessType=-1;
		strcpy( c->modemmodel , "Unknow" );
		
		do{
			
			DoStr( s ,  "\r\r\r" );

			// atz 
			DrainSerial(s);
			DoStr( s,  "ATZ\r" );
			if( ! GetResponse(s,buf,STRSIZE,5)){
				bug("ATZ FAIL\n");	
				result = FALSE; break;
			} 		
				
			// echo off 
			DrainSerial(s);
			DoStr( s,  "ATE 0\r" );
			if( ! GetResponse(s,buf,STRSIZE,5)){
				bug("ATE 0 FAIL\n");	
				result = FALSE; break;
			}
			
			// Get modem model 
			DrainSerial(s);
			DoStr( s,  "AT+GMM\r" );
			if( ! GetResponse(s,buf,STRSIZE,5)){
				bug("AT+GMM FAIL\n");	
			}
			else if( tok = strtok( buf , "\n\r" ) ){
				strcpy( c->modemmodel , tok );		
			}

			// ask signal strength 
			DrainSerial(s);
			DoStr( s,  "AT+CSQ\r" );
			if( ! GetResponse(s,buf,STRSIZE,5)){
				bug("AT+CSQ FAIL\n");	
			}
			else if( tok = strtok( buf , delim ) ){	
				if( tok = strtok( NULL , delim ) ){
					if( isdigit( tok[0] ) ) c->signal = atoi( tok );
				}			
			}
			
			//  Network Registration Status
			DrainSerial(s);
			DoStr( s,  "AT+COPS?\r" );
			if( ! GetResponse(s,buf,STRSIZE,5)){
				bug("AT+COPS? FAIL\n");	
			}else{
				if( tok = strtok( buf , delim ) ){
				if( tok = strtok( NULL , delim ) ){
				if( tok = strtok( NULL , delim ) ){
				if( tok = strtok( NULL , delim ) ){
				if( tok = strtok( NULL , delim ) ){
					if( isdigit( tok[0] ) ) c->AccessType = atoi( tok );
				}}}}}
			}	
		
			bug("model=%s\n",c->modemmodel);
			bug("AccessType=%d\n",c->AccessType);
			bug("signal=%d\n",c->signal);
				
		}while(0);
			
		FreeMem( buf , STRSIZE );
	}	
	return result;
}


BOOL DialUp(struct EasySerial *s,struct Conf *c){
	
	struct at_command *atc=NULL;
	UBYTE buf[PPP_MAXARGLEN];
	
	// Init commands
	ForeachNode(&c->atcl,atc){
		DrainSerial(s);
		bug("SEND \"%s\"\n",atc->str);
		DoStr( s, atc->str);
		DoStr( s,  "\r" );
		if( GetResponse( s , buf , PPP_MAXARGLEN , c->CommandTimeOut )){
			if( strcasestr(buf,"CONNECT") ) return TRUE;
		}else{	
			bug("FAIL!\n");
			break;
		}	
	}

	return FALSE;

}



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
	DoBYTES(s,str,strlen((char*) str));
}

void DoBYTES(struct EasySerial *s, BYTE *p,ULONG len){

	if( ! s ) return;
	if( ! s->Ok ) return;

	WaitIO((struct IORequest*)s->SerTx);

	s->SerTx->IOSer.io_Length = len;
	s->SerTx->IOSer.io_Data = p;
	s->SerTx->IOSer.io_Command =  CMD_WRITE;
	DoIO((struct IORequest*)s->SerTx);
}

void SendBYTES(struct EasySerial *s, BYTE *p, ULONG len){

	if( ! s ) return;
	if( ! s->Ok ) return;

	WaitIO((struct IORequest*)s->SerTx);

	s->SerTx->IOSer.io_Length = len;
	s->SerTx->IOSer.io_Data = p;
	s->SerTx->IOSer.io_Command =  CMD_WRITE;
	SendIO((struct IORequest*)s->SerTx);
}

void DrainSerial(struct EasySerial *s){

	if( ! s ) return;
	if( ! s->Ok ) return;
	
	if( s->SerRx ){
		Delay(25);

		AbortIO((struct IORequest *)s->SerRx);
		WaitIO((struct IORequest *)s->SerRx);
		while(GetMsg(s->RxPort));
		s->RxBuff[1]=0;

		bug("Drain:\n");
		for(;;){  // Read crap out of serial device.

			s->SerRx->IOSer.io_Command = SDCMD_QUERY;
			DoIO((struct IORequest *)s->SerRx);

			if( s->SerRx->IOSer.io_Error ){
				bug("DrainSerial(): OOOOPS  lost device!\n");
				s->Ok=FALSE;
				return;
			}

		    if( s->SerRx->IOSer.io_Actual == 0 ) break;

			s->SerRx->IOSer.io_Command = CMD_READ;
			s->SerRx->IOSer.io_Data = s->RxBuff;
			s->SerRx->IOSer.io_Length = 1;
			DoIO((struct IORequest *)s->SerRx);
			if( s->SerRx->IOSer.io_Error ){
				bug("DrainSerial(): CMD_READ error!");
			}else{
			//	bug("%s",s->RxBuff);
			}
		}
		//bug("Drain end\n");

	}

	if( s->SerTx ){
		AbortIO((struct IORequest *)s->SerTx);
		WaitIO((struct IORequest *)s->SerTx);
		while(GetMsg(s->TxPort));
	}

}


BOOL GetResponse(struct EasySerial *s,UBYTE *Buffer,ULONG maxbuffer,LONG timeout){

	struct EasyTimer *t;

	ULONG sigset;
	BOOL result = FALSE;
	char c[2]={0,0};
	ULONG len = 0;
	Buffer[0]=0;

	if( ! s ) return FALSE;
	if( ! s->Ok ) return FALSE;
	
	bug("GetResponse:\n");

	if( t = OpenTimer() ){
		
		SetTimer(t,timeout);
		
		QueueSerRequest( s , 1 );
		if( ! s->Ok ){
			CloseTimer(t);
			return FALSE ;
		}
		
		for(;;){

			sigset = (1L<< s->RxPort->mp_SigBit ) |
					 (1L<< t->TimeMsg->mp_SigBit );

			Wait(sigset);

			if( GetMsg( s->RxPort ) ){

				if( len <= ( maxbuffer-1 ) ){
					Buffer[len] = s->RxBuff[0];
					c[0] = s->RxBuff[0];
					bug( "%s" , c[0] == '\r' ? "<\\r>" :
								c[0] == '\n' ? "<\\n>" :
												c 
						);
					if( Buffer[len] == '\r' )  Buffer[len] = '\n';
					Buffer[++len] = 0;
					if( strcasestr(Buffer,"OK\n") != NULL |
						strcasestr(Buffer,"CONNECT") != NULL
					){
						result = TRUE;
						break;
					}
					else if( strcasestr(Buffer,"NO CARRIER\n") != NULL |
						strcasestr(Buffer,"ERROR\n") != NULL |
						strcasestr(Buffer,"BUSY\n") != NULL
					){
						result = FALSE;
						break;
					}

				}

				QueueSerRequest( s , 1 );
				if( ! s->Ok ){
					CloseTimer(t);
					return FALSE ;
				}
			}

			if( GetMsg( t->TimeMsg ) ){
				bug( "GetResponse TimeOut ERROR\n");
				result = FALSE;
				break;
			}

		}
		CloseTimer(t);
	}

	if( s->SerRx ){
		AbortIO((struct IORequest *)s->SerRx);
		WaitIO((struct IORequest *)s->SerRx);
		while(GetMsg(s->RxPort));
	}
bug("GetResponse end\n");
	return result;
}



struct EasySerial *OpenSerial(BYTE *name,ULONG unit){
	struct EasySerial *s = NULL;

	do{
		
		if( ! (s = AllocMem( sizeof(struct EasySerial),MEMF_CLEAR|MEMF_PUBLIC))) break;
		if( ! (s->TxBuff = AllocMem( SERIAL_BUFSIZE ,MEMF_CLEAR|MEMF_PUBLIC))) break;
		if( ! (s->RxBuff = AllocMem( SERIAL_BUFSIZE ,MEMF_CLEAR|MEMF_PUBLIC))) break;
		if( ! (s->TxPort = CreateMsgPort())) break;
		if( ! (s->SerTx = CreateIORequest(s->TxPort,sizeof(struct IOExtSer)))) break;

		if(  OpenDevice( name , unit , (struct IORequest *)s->SerTx,0)){
			DeleteIORequest(s->SerTx);
			s->SerTx = NULL;
			break;
		}
		
		bug("OpenSerial:OpenDevice: \"%s\" unit %d\n",name,unit);
		
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
	
	// FAIL:
	_CloseSerial(s);
	return NULL;
}


VOID _CloseSerial(struct EasySerial *s){

	if( ! s ) return ;
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
