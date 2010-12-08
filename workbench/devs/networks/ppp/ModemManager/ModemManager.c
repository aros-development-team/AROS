/*
 * $Id: ModemManager.c 34949 2010-10-25 18:27:45Z Sami $
 */

#define DEBUG 1
#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos/dos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <utility/hooks.h>
#include <libraries/mui.h>
#include <aros/debug.h>

#include "ppp.h"
#include "misc.h"

#define SimpleText(text) TextObject, MUIA_Text_Contents, (IPTR) text, End

#define TIMERVALUE 10
#define STRSIZE 100

#define STATE_UNPLUGGED 0
#define STATE_PLUGGED 1
#define STATE_OPENDEV 2
#define STATE_NETWORK 3
#define STATE_CLOSEDEV 4

Object *application, *window, *DisConBut, *ConBut;
Object *GENERAL_Info, *IN_Info,*OUT_Info;


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

BOOL SendCtrlMsg(ULONG command,IPTR Arg,struct Conf *c){
	struct PPPcontrolMsg *CtrlMsg=0;
	struct MsgPort *CtrlPort=0;
	BOOL succes=FALSE;
	if( CtrlPort = CreatePort(0,0) ){
		if( CtrlMsg = AllocMem(sizeof(struct PPPcontrolMsg),MEMF_PUBLIC | MEMF_CLEAR)){

			CtrlMsg->Command = command;
			CtrlMsg->Arg = Arg;

			CtrlMsg->DeviceName = c->DeviceName;
			CtrlMsg->UnitNum = c->SerUnitNum;
			CtrlMsg->username = c->username;
			CtrlMsg->password = c->password;

			CtrlMsg->Msg.mn_Node.ln_Type = NT_MESSAGE;
			CtrlMsg->Msg.mn_Length = sizeof(struct PPPcontrolMsg);
			CtrlMsg->Msg.mn_ReplyPort = CtrlPort;
		//	 bug("ModemManager: SendCtrlMsg\n");
			if( SafePutToPort(CtrlMsg, "ppp-control") ){
				WaitPort(CtrlPort);
				GetMsg(CtrlPort);
				succes=TRUE;
		//	 bug("ModemManager: SendCtrlMsg OK\n");
			}
			FreeMem(CtrlMsg,sizeof(struct PPPcontrolMsg));
		}
		DeletePort(CtrlPort);
	}
	return succes;
}

void speedstr(BYTE *buf,BYTE *label,LONG s){
	float speed= (float)s;
	BYTE e = 'b';
	if( speed > 1000.0 ){
		e = 'k';
		speed /= 1000.0;
	}
	if( speed > 1000.0 ){
		 e = 'M';
		 speed /= 1000.0;
	}
	snprintf( buf, STRSIZE , speed == (ULONG)speed ? "%s %.0f %c/s" :"%s %.2f %c/s" , label , speed , e );
}

static void DisconnectFunc(struct Hook *hook, Object *app, APTR *arg)
{

	struct Conf *c = *arg;
	SendCtrlMsg( PPP_CTRL_SETPHASE , PPP_PHASE_TERMINATE , c );
	Delay(150);
	SendCtrlMsg( PPP_CTRL_CLOSE_SERIAL , 0 , c );
	c->state = STATE_PLUGGED;

}

BYTE FindModemUnit(struct Conf *c){
	struct EasySerial *Ser=0;
	BYTE result=-1;
	for(int i=0;i<100;i++){
		set( IN_Info , MUIA_Text_Contents, (IPTR)"Test Device..");
		set( OUT_Info , MUIA_Text_Contents, (IPTR)c->DeviceName );
		if( Ser = OpenSerial( c->DeviceName ,i ) ){
			set( IN_Info , MUIA_Text_Contents, (IPTR)"Modem Test...");
			//set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
			if( TestModem( Ser , c ) ){
				result = i;
				DrainSerial( Ser );
				CLOSESERIAL( Ser );
				break;
			}
			DrainSerial( Ser );
			CLOSESERIAL( Ser );
		} else break;
	}
	return result;
}

static void ConnectFunc(struct Hook *hook, Object *app, APTR *arg)
{
	struct Conf *c = *arg;
	struct EasySerial *Ser=0;
	BOOL tested=FALSE;

	if( c->state == STATE_PLUGGED ){
		ReadConfig(c);

		// if unitnumber < 0 Test them one by one.
		if( c->SerUnitNum < 0 ){
			c->SerUnitNum = FindModemUnit(c);
			tested = TRUE;
		}
		
		if( c->SerUnitNum >=0 ){
			set( IN_Info , MUIA_Text_Contents, (IPTR)"Open Serial Device...");
			set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
			if( Ser = OpenSerial( c->DeviceName ,c->SerUnitNum ) ){
				if( tested ) goto skiptest;
				set( IN_Info , MUIA_Text_Contents, (IPTR)"Modem Test...");
				//set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
				if( TestModem( Ser , c ) ){
					skiptest:
					set( IN_Info , MUIA_Text_Contents, (IPTR)"DialUp...");
					//set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
					if( DialUp(Ser,c) ){
						CLOSESERIAL(Ser);
						if( SendCtrlMsg( PPP_CTRL_OPEN_SERIAL , 0 , c )){
							c->state = STATE_OPENDEV;
							set( OUT_Info , MUIA_Text_Contents, (IPTR)"OK,Starting PPP...");
						}
					}
				}
				CLOSESERIAL(Ser);
			}else c->state = STATE_UNPLUGGED;	
		}else c->state = STATE_UNPLUGGED;
		
		if( c->state != STATE_OPENDEV ){
			set( OUT_Info , MUIA_Text_Contents, (IPTR)"ERROR");
		}
	}
}


#define FILEBUFFSIZE 4000

void ConfNetWork(struct PPPcontrolMsg *msg){

	BPTR InFile, OutFile;
	UBYTE *buff;
	UBYTE *linebuff;
	BOOL putline;
	BYTE myhostname[] = "SYS:Prefs/Env-Archive/AROSTCP/db/netdb-myhost";

	bug("\n###########################################################\n");
	bug("PPP is ONLINE !\n");
	bug("Local IP address %d.%d.%d.%d\n",msg->LocalIP[0],msg->LocalIP[1],msg->LocalIP[2],msg->LocalIP[3]);
	bug("Remote IP address %d.%d.%d.%d\n",msg->RemoteIP[0],msg->RemoteIP[1],msg->RemoteIP[2],msg->RemoteIP[3]);

	if( buff = AllocMem( FILEBUFFSIZE , MEMF_CLEAR|MEMF_PUBLIC ) ){


		bug("Primary DNS address %d.%d.%d.%d\n", msg->PrimaryDNS[0],msg->PrimaryDNS[1],
			msg->PrimaryDNS[2],msg->PrimaryDNS[3] );
		bug("Secondary DNS address %d.%d.%d.%d\n", msg->SecondaryDNS[0],msg->SecondaryDNS[1],
			msg->SecondaryDNS[2],msg->SecondaryDNS[3] );

		if(linebuff = AllocMem( FILEBUFFSIZE , MEMF_CLEAR|MEMF_PUBLIC ) ){
			bug( "Open File \"%s\"\n" , myhostname );
			if( InFile  =  Open( myhostname , MODE_OLDFILE ) ){
				buff[0]=0;
				while(FGets( InFile , linebuff, FILEBUFFSIZE-1 )){

					putline = TRUE;

					if( strcasestr( linebuff , "NAMESERVER" ) ){ // remove existing nameservers
						putline = FALSE;
					}

					if( strcasestr( linebuff , "autogenerated by PPP.device" ) ){ // remove existing comment
						putline = FALSE;
					}

					if( putline ) strcat( buff, linebuff );

				}

				strcat( buff, ";Name Servers autogenerated by PPP.device\n" );

				sprintf( linebuff ,"NAMESERVER %d.%d.%d.%d\n", msg->PrimaryDNS[0],msg->PrimaryDNS[1],
						 msg->PrimaryDNS[2],msg->PrimaryDNS[3] );
				strcat( buff, linebuff );
				sprintf( linebuff ,"NAMESERVER %d.%d.%d.%d\n", msg->SecondaryDNS[0],msg->SecondaryDNS[1],
						 msg->SecondaryDNS[2],msg->SecondaryDNS[3] );
				strcat( buff, linebuff );

				Close(InFile);
				if( OutFile  =  Open( myhostname , MODE_NEWFILE ) ){
					FPuts( OutFile , buff );
					Close( OutFile );
				}
				bug( "File \"%s\"  is modified !\n" , myhostname );
			}else{
				bug("FAIL!\n");
			}
			FreeMem( linebuff, FILEBUFFSIZE );
		}


		sprintf(buff,"SYS:System/Network/AROSTCP/c/ifconfig ppp0 %d.%d.%d.%d %d.%d.%d.%d" ,
				msg->LocalIP[0],msg->LocalIP[1],
				msg->LocalIP[2],msg->LocalIP[3],
				msg->RemoteIP[0],msg->RemoteIP[1],
				msg->RemoteIP[2],msg->RemoteIP[3] );
		bug("Executing command:\"%s\"\n",buff);
		InFile  = Open( "NIL:" , MODE_OLDFILE );
		OutFile = Open( "NIL:" , MODE_OLDFILE );
		if( SystemTags( buff, NP_Name, "ppp-ifconfig",
						 SYS_Asynch, FALSE,
						 SYS_Input, InFile,
						 SYS_Output, OutFile ,
						 TAG_END )
				!= 0 ) bug("command FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

	sprintf(buff,"SYS:System/Network/AROSTCP/c/route add default %d.%d.%d.%d",
				msg->RemoteIP[0],msg->RemoteIP[1],
				msg->RemoteIP[2],msg->RemoteIP[3] );
		bug("Executing command:\"%s\"\n",buff);
		InFile  = Open( "NIL:" , MODE_OLDFILE );
		OutFile = Open( "NIL:" , MODE_NEWFILE );
		if(SystemTags( buff, NP_Name, "ppp-route",
						SYS_Asynch, TRUE,
						SYS_Input, InFile,
						SYS_Output, OutFile ,
						TAG_END )
				!= 0 ) bug("command FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

		bug("\n############################################################*\n");

		FreeMem( buff , FILEBUFFSIZE );
	}
}


int main(void)
{

   	struct Hook DisconnectHook,ConnectHook;
	struct EasyTimer *timer=0;
	BYTE buf[STRSIZE];
	buf[0]=0;

	struct PPPcontrolMsg *CtrlMsg=0;
	struct PPPcontrolMsg *InfoMsg=0;
	struct MsgPort *CtrlPort=0;

	struct EasySerial *Ser=0;

	UBYTE *PortName = "GUI.PORT";

	struct Conf *c=0;

	if( timer=OpenTimer() ){
	if( CtrlPort = CreatePort(PortName,0) ){
	if( CtrlMsg = AllocMem(sizeof(struct PPPcontrolMsg),MEMF_PUBLIC | MEMF_CLEAR)){
	if( c = AllocMem(sizeof(struct Conf),MEMF_PUBLIC | MEMF_CLEAR)){

		NEWLIST(&c->atcl);
		ReadConfig(c);
		c->state = STATE_UNPLUGGED;

		DisconnectHook.h_Entry = HookEntry;
		DisconnectHook.h_SubEntry = (HOOKFUNC) DisconnectFunc;
		ConnectHook.h_Entry = HookEntry;
		ConnectHook.h_SubEntry = (HOOKFUNC) ConnectFunc;

		application = ApplicationObject,
		SubWindow, window = WindowObject,
			MUIA_Window_Title,	(IPTR) "ModemManager",
			MUIA_Window_Activate,TRUE,
				WindowContents, (IPTR) VGroup,
					Child, (IPTR) VGroup,
						Child, GENERAL_Info = SimpleText(""),
						Child, IN_Info = SimpleText(""),
						Child, OUT_Info = SimpleText(""),
					End,
					Child, (IPTR) HGroup,
						Child, (IPTR) (   ConBut = SimpleButton("   Connect    ")),
						Child, (IPTR) (DisConBut = SimpleButton("  Disconnect  ")),
					End,
				End,
			End,
		End;

		if (application)
		{
			ULONG sigs = 0;

			DoMethod(
					window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
					(IPTR) application, 2, MUIM_Application_ReturnID,
					MUIV_Application_ReturnID_Quit
			);

			DoMethod(
				ConBut,MUIM_Notify,MUIA_Pressed,FALSE,
				application, (IPTR) 3,
				MUIM_CallHook, &ConnectHook,c
			);

			DoMethod(
				DisConBut,MUIM_Notify,MUIA_Pressed,FALSE,
				application, (IPTR) 3,
				MUIM_CallHook, &DisconnectHook,c
			);

			set(window,MUIA_Window_Open,TRUE);
			set(DisConBut,MUIA_Disabled,TRUE);
			set(   ConBut,MUIA_Disabled,TRUE);

			SetTimer( timer , 1 );

			while(
					DoMethod(
						application, MUIM_Application_NewInput, (IPTR) &sigs
					) != MUIV_Application_ReturnID_Quit
				)
			{
				if (sigs){
					 sigs = Wait(	sigs |
									SIGBREAKF_CTRL_C |
									SIGBREAKF_CTRL_F |
									(1L<< CtrlPort->mp_SigBit ) |
									(1L<< timer->TimeMsg->mp_SigBit )
									);

					if (sigs & SIGBREAKF_CTRL_C) break;

					while( InfoMsg = (struct PPPcontrolMsg*)GetMsg(CtrlPort) ){
						if( InfoMsg->Msg.mn_Length == sizeof(struct PPPcontrolMsg)
							&&  InfoMsg->Command == PPP_CTRL_INFO
							 ){
							bug("ModemManager: received info message num %d\n",InfoMsg->num);

							UBYTE genbuf[STRSIZE];
							snprintf( genbuf, STRSIZE , "%s" , c->modemmodel );
							set( GENERAL_Info , MUIA_Text_Contents, genbuf);

							// PPP initializing is ready
							if(  c->state == STATE_OPENDEV && InfoMsg->Phase == PPP_PHASE_NETWORK ){
								 ConfNetWork( InfoMsg );
								 c->state = STATE_NETWORK;
							}

							// serial connection is lost (device unplugged)
							if( c->state == STATE_NETWORK && (! InfoMsg->Ser) ){
								  c->state = STATE_UNPLUGGED;
							}

							// Connection is ok
							if( InfoMsg->Phase == PPP_PHASE_NETWORK &&  InfoMsg->Ser ){
								  c->state = STATE_NETWORK;
							}

							// net connection is lost
							if( c->state == STATE_NETWORK &&  InfoMsg->Phase != PPP_PHASE_NETWORK ){
								SendCtrlMsg( PPP_CTRL_CLOSE_SERIAL , 0 , c );
								c->state = STATE_UNPLUGGED;
							}



							if( c->state == STATE_UNPLUGGED ){
								set( IN_Info , MUIA_Text_Contents, (IPTR)"Unplugged");
								set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
								set(DisConBut, MUIA_Disabled , TRUE );
								set(   ConBut, MUIA_Disabled , TRUE );
							}
							else if( c->state == STATE_PLUGGED ){
								set( IN_Info , MUIA_Text_Contents, (IPTR)"Disconnected");
								set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
								set(DisConBut, MUIA_Disabled , TRUE );
								set(   ConBut, MUIA_Disabled , FALSE );
							}
							else if( c->state == STATE_OPENDEV &&  ! InfoMsg->Ser){
								set( IN_Info , MUIA_Text_Contents, (IPTR)"OpenDevice:");
								snprintf( buf, STRSIZE , "%s unit %d",c->DeviceName,c->SerUnitNum);
								set( OUT_Info , MUIA_Text_Contents, (IPTR)buf);
								set(DisConBut, MUIA_Disabled , FALSE );
								set(   ConBut, MUIA_Disabled , TRUE );
							}

							else if( c->state == STATE_OPENDEV ){
								set( IN_Info , MUIA_Text_Contents, (IPTR)"Connection in progress:");
								UBYTE phase = InfoMsg->Phase;

								set( OUT_Info , MUIA_Text_Contents,
									(IPTR)( phase == PPP_PHASE_CONFIGURATION  ? "LCP configuration" :
											phase == PPP_PHASE_AUTHENTICATION ? "Authentication" :
											phase == PPP_PHASE_PROTOCOL_CONF  ? "Protocol configuration" :
																				"Unknow"
									));

								set(DisConBut, MUIA_Disabled , FALSE );
								set(   ConBut, MUIA_Disabled , TRUE );
								//set(window,MUIA_Window_Title,(IPTR)c->modemmodel);
							}
							else if( c->state == STATE_NETWORK ){

								set(DisConBut, MUIA_Disabled , FALSE );
								set(   ConBut, MUIA_Disabled , TRUE );
								speedstr(buf," In",InfoMsg->SpeedIn);
								set( IN_Info , MUIA_Text_Contents, (IPTR)buf);
								speedstr(buf,"Out",InfoMsg->SpeedOut);
								set( OUT_Info , MUIA_Text_Contents, (IPTR)buf);
							}

						}

						ReplyMsg((struct Message *)InfoMsg);
						SetTimer( timer , TIMERVALUE );
					//bug("ModemManager: ReplyMsg OK\n");
					}


					if(GetMsg(timer->TimeMsg)){

						if( c->state == STATE_UNPLUGGED ){
							
							if( c->SerUnitNum < 0 ){
								c->SerUnitNum = FindModemUnit(c);
							}
							
							if( c->SerUnitNum >= 0 ){			
								if( Ser = OpenSerial( c->DeviceName ,c->SerUnitNum ) ){
									if( TestModem( Ser , c ) ){
										UBYTE genbuf[STRSIZE];
										snprintf( genbuf, STRSIZE , "%s" , c->modemmodel );
										set( GENERAL_Info , MUIA_Text_Contents, genbuf);
										set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
										set(DisConBut, MUIA_Disabled , TRUE );
										set(   ConBut, MUIA_Disabled , FALSE );
										 c->state = STATE_PLUGGED;
									}
									CLOSESERIAL(Ser);
								}
							}
							
							if( c->state == STATE_PLUGGED ){
								UBYTE genbuf[STRSIZE];
								snprintf( genbuf, STRSIZE , "%s" , c->modemmodel );
								set( GENERAL_Info , MUIA_Text_Contents, genbuf);
								set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
								set(DisConBut, MUIA_Disabled , TRUE );
								set(   ConBut, MUIA_Disabled , FALSE );
							}else{
								set( GENERAL_Info , MUIA_Text_Contents, "Unplugged");
								set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
								set( IN_Info , MUIA_Text_Contents, (IPTR)"");
								set(DisConBut, MUIA_Disabled , TRUE );
								set(   ConBut, MUIA_Disabled , TRUE );
							}

						}

						CtrlMsg->Command = PPP_CTRL_INFO_REQUEST;
						CtrlMsg->Arg = (IPTR)PortName;
						CtrlMsg->Msg.mn_Node.ln_Type = NT_MESSAGE;
						CtrlMsg->Msg.mn_Length = sizeof(struct PPPcontrolMsg);
						CtrlMsg->Msg.mn_ReplyPort = 0;
						if( SafePutToPort(CtrlMsg, "ppp-control") ){
							bug("ModemManager: Send PPP_CTRL_INFO_REQUEST message\n");
						}

						SetTimer( timer , TIMERVALUE );
					}

				}
			}
		}

		MUI_DisposeObject(application);

	}}}}

	bug("ModemManager shutdown\n");
	struct at_command  *atc;
	while( atc = (struct at_command *)RemHead( &c->atcl ) ){
		FreeMem( atc , sizeof(struct at_command) );
	}

	if(CtrlPort){
		while( GetMsg(CtrlPort) ) ReplyMsg((struct Message *)CtrlMsg);
		DeletePort(CtrlPort);
	}

	if( c ) FreeMem( c , sizeof(struct Conf));
	if( CtrlMsg ) FreeMem(CtrlMsg , sizeof(struct PPPcontrolMsg));

	CloseTimer(timer);

  return 0;
}
