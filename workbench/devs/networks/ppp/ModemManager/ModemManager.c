/*
 * $Id$
 */

#define DEBUG 1
#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dos/dos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/miami.h>
#include <utility/hooks.h>
#include <libraries/mui.h>
#include <aros/debug.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ppp.h"
#include "misc.h"

#define SimpleText(text) TextObject, MUIA_Text_Contents, (IPTR) text, End

#define TIMERVALUE 5
#define STRSIZE 100

#define STATE_UNPLUGGED 0
#define STATE_PLUGGED 1
#define STATE_OPENDEV 2
#define STATE_NETWORK 3
#define STATE_CLOSEDEV 4

Object *application=0,*window,*DisConBut,*ConBut;
Object *ModemName,*AccessType;
Object *IN_Info,*OUT_Info;

struct EasyBitmap *SignalBM=0;

UBYTE *PortName = "ModemManager";
const TEXT version_string[] = "$VER: ModemManager 1.2 (1.3.2012)";

ULONG exPhase,exstate;
BOOL exSer;

struct EasyBitmap{
	struct BitMap *bm;
	struct RastPort *rp;
	Object *MUIbitmap;
};

struct EasyBitmap *MakeBitmap(ULONG x,ULONG y,Object *MUIwindow,Object *MUIbitmap){
	struct EasyBitmap *ebm=NULL;
	struct Window *tw=NULL;

	if( MUIwindow && MUIbitmap ){
		if( ebm = AllocMem( sizeof(struct EasyBitmap),MEMF_CLEAR|MEMF_PUBLIC)){

			tw=(struct Window *)XGET( MUIwindow , MUIA_Window_Window );

			ebm->bm = AllocBitMap(x,y,
					 GetBitMapAttr(tw->RPort->BitMap, BMA_DEPTH),
					 BMF_CLEAR,
					 tw->RPort->BitMap);
			ebm->rp = CreateRastPort();
			ebm->rp->BitMap = ebm->bm;
			ebm->MUIbitmap = MUIbitmap;
			set( MUIbitmap , MUIA_FixWidth, x );
			set( MUIbitmap , MUIA_FixHeight, y );
			set( MUIbitmap , MUIA_Bitmap_Width, x );
			set( MUIbitmap , MUIA_Bitmap_Height, y );
			set( MUIbitmap , MUIA_Bitmap_Transparent, 0 );
			set( MUIbitmap , MUIA_Bitmap_Bitmap, ebm->bm );
			DoMethod( MUIbitmap , MUIM_Draw);
		}
	}
	return ebm;
}

void _CloseBitmap(struct EasyBitmap *ebm){
	if(ebm){
		FreeRastPort(ebm->rp);
		FreeBitMap(ebm->bm);
		FreeMem(ebm,sizeof(struct EasyBitmap));
	}
}
#define CloseBitmap( ebm ) do{_CloseBitmap(ebm);ebm=NULL;}while(0)


struct EasyGraph{
	struct EasyBitmap *ebm;
	ULONG Xsize,Ysize;
	FLOAT Max;
	FLOAT *value;
};

void _CloseGraph(struct EasyGraph *egr){
	if(egr){
		CloseBitmap( egr->ebm );
		if( egr->value ) FreeMem( egr->value , sizeof(FLOAT) * egr->Xsize );
		FreeMem(egr,sizeof(struct EasyGraph));
	}
}
#define CloseGraph( x ) do{_CloseGraph(x);x=NULL;}while(0)

struct EasyGraph *MakeGraph(ULONG x,ULONG y,Object *MUIwindow,Object *MUIbitmap){
	struct EasyGraph *egr=NULL;
	if( MUIwindow && MUIbitmap ){
		if( egr = AllocMem( sizeof(struct EasyGraph),MEMF_CLEAR|MEMF_PUBLIC)){
			egr->Xsize = x;
			egr->Ysize = y;
			egr->Max = 0.0;
			if( egr->value = AllocMem( sizeof(FLOAT) * x ,MEMF_CLEAR|MEMF_PUBLIC)){
				if( egr->ebm = MakeBitmap(x,y,MUIwindow,MUIbitmap) ){
					SetRast(egr->ebm->rp,0);
					DoMethod( egr->ebm->MUIbitmap , MUIM_Draw );
				}else CloseGraph(egr);
			}
		}
	}
	return egr;
}

void UpdateGraph(struct EasyGraph *egr,FLOAT value){
	LONG i;
	LONG h;
	if(egr){

		for( i= egr->Xsize-1 ; i > 0 ; i-- ){
			egr->value[i] = egr->value[i-1];
		}

		egr->value[0] = value;
		if( value > egr->Max ) egr->Max = value;

		if( egr->ebm ){

			SetRast(egr->ebm->rp,0);
			SetAPen(egr->ebm->rp,1);
			Move(egr->ebm->rp , 0 , egr->Ysize-1 );
			Draw(egr->ebm->rp , egr->Xsize-1 , egr->Ysize-1 );
			SetAPen(egr->ebm->rp,2);

			if( egr->Max != 0.0 ){
				for( i=0 ; i < egr->Xsize ; i++ ){
					h = (LONG)( egr->value[i] / egr->Max * (FLOAT)( egr->Ysize - 2 ) );
					if(i==0) Move(egr->ebm->rp , i , (egr->Ysize-2) - h );
						else Draw(egr->ebm->rp , i , (egr->Ysize-2) - h );
				}
			}else{
				Move(egr->ebm->rp , 0 , egr->Ysize-2 );
				Draw(egr->ebm->rp , egr->Xsize-1 , egr->Ysize-2 );
			}

			DoMethod( egr->ebm->MUIbitmap , MUIM_Draw );

		}
	}
}


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
			if( SafePutToPort(CtrlMsg, "ppp-control") ){
				WaitPort(CtrlPort);
				GetMsg(CtrlPort);
				succes=TRUE;
			}
			FreeMem(CtrlMsg,sizeof(struct PPPcontrolMsg));
		}
		DeletePort(CtrlPort);
	}
	return succes;
}

BOOL SendRequest(void){
	struct PPPcontrolMsg *CtrlMsg;
	BOOL result=FALSE;

	if( CtrlMsg = AllocMem(sizeof(struct PPPcontrolMsg),MEMF_PUBLIC | MEMF_CLEAR)){
		bug("ModemManager:send info request\n");
		CtrlMsg->Command = PPP_CTRL_INFO_REQUEST;
		CtrlMsg->Arg = (IPTR)PortName;
		CtrlMsg->Msg.mn_Node.ln_Type = NT_MESSAGE;
		CtrlMsg->Msg.mn_Length = sizeof(struct PPPcontrolMsg);
		CtrlMsg->Msg.mn_ReplyPort = 0;
		if( SafePutToPort(CtrlMsg, "ppp-control") ) result=TRUE;
		FreeMem(CtrlMsg , sizeof(struct PPPcontrolMsg));
	}
	return result;
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

// draw triangular signal meter and show modem name etc..
void UpdateModemInfo(struct EasyBitmap *ebm,struct Conf *c)
{
	ULONG i;
	ULONG sig;
	if( ebm ){

		SetRast(ebm->rp,0);
		if( c->signal >= 0 && c->signal != 99 ){

			sig = c->signal;
			//sig = (ULONG)( log( (double)c->signal ) / log( 31.0 )*31.0 );

			SetAPen(ebm->rp,2);
			for(i=0;i<32;i++){
				if( sig >= i){
					Move(ebm->rp, i , 15 );
					Draw(ebm->rp, i , 15-i/2 );
				}
			}
			SetAPen(ebm->rp,1);
			Move(ebm->rp,0,15);
			Draw(ebm->rp,0,14);
			Draw(ebm->rp,29,0);
			Draw(ebm->rp,31,0);
			Draw(ebm->rp,31,15);
			Draw(ebm->rp,0,15);
		}
		DoMethod( ebm->MUIbitmap , MUIM_Draw );

		/*
		<AcT> Network access type
		0 GSM
		1 Compact GSM
		2 UTRAN
		3 GSM with EGPRS
		4 UTRAN with HSDPA
		5 UTRAN with HSUPA
		6 UTRAN with HSDPA and HSUPA ???
		*/

		set( AccessType , MUIA_Text_Contents,
				c->AccessType == -1 ? "" :
				c->AccessType == 0 ? "GSM" :
				c->AccessType == 1 ? "GPRS" :
				c->AccessType == 2 ? "3G" :
				c->AccessType == 3 ? "EDGE" :
				c->AccessType == 4 ? "3.5G" :
				c->AccessType == 5 ? "3.75G" :
				c->AccessType == 6 ? "3.8G" :
									 "?G"
		);
		set( ModemName , MUIA_Text_Contents, c->modemmodel);
	}
}

static void DisconnectFunc(struct Hook *hook, Object *app, APTR *arg)
{

	struct Conf *c = *arg;
	SendCtrlMsg( PPP_CTRL_SETPHASE , PPP_PHASE_TERMINATE , c );

}

void FindModemUnit(struct Conf *c){
	struct EasySerial *Ser=0;
	int result = -1;
	int i;

	if( c->SerUnitNum >= 0 ){
		return;
	}

	for (i = 0; i < 100; i++)
	{
		if( Ser = OpenSerial( c->DeviceName ,i ) ){
			if( TestModem( Ser , c ) ){
				result = i;
				DrainSerial( Ser );
				CloseSerial( Ser );
				break;
			}
			DrainSerial( Ser );
			CloseSerial( Ser );
		} else break;
	}
	c->SerUnitNum = result;
}


static void ConnectFunc(struct Hook *hook, Object *app, APTR *arg)
{
	struct Conf *c = *arg;
	struct EasySerial *Ser=0;

	if( c->state == STATE_PLUGGED ){

		// is interfacename configured?
		if( c->InterfaceName[0] == 0 ){
			set( IN_Info , MUIA_Text_Contents, (IPTR)"ppp Interface not configured!");
			return;
		}

		// check if arostcp is running
		if( FindTask("bsdsocket.library") == NULL ){
			set( IN_Info , MUIA_Text_Contents, (IPTR)"AROSTCP is not running!");
			if( StartStack() ){
				set( OUT_Info , MUIA_Text_Contents, (IPTR)"Starting AROSTCP OK");
			}else{
				set( OUT_Info , MUIA_Text_Contents, (IPTR)"Starting AROSTCP FAIL!");
				return;
			}
		}

		// check ppp.device
		if( FindPort("ppp-control") ){
			// send info request to ppp.device
			SendRequest();
		}else{
			set( OUT_Info, MUIA_Text_Contents, (IPTR)"Can't find ppp.device!");
			set( IN_Info , MUIA_Text_Contents, (IPTR)"Not configured?");
			return;
		}

		if( c->SerUnitNum >=0 ){
			set( IN_Info , MUIA_Text_Contents, (IPTR)"Open Serial Device...");
			set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
			if( Ser = OpenSerial( c->DeviceName ,c->SerUnitNum ) ){
				set( IN_Info , MUIA_Text_Contents, (IPTR)"Modem Test...");
				//set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
				if( TestModem( Ser , c ) ){
					UpdateModemInfo( SignalBM , c );
					set( IN_Info , MUIA_Text_Contents, (IPTR)"DialUp...");
					//set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
					if( DialUp(Ser,c) ){
						CloseSerial(Ser);
						if( SendCtrlMsg( PPP_CTRL_OPEN_SERIAL , 0 , c )){
							c->state = STATE_OPENDEV;
							set( OUT_Info , MUIA_Text_Contents, (IPTR)"OK,Starting PPP...");
						}
					}
				}
				CloseSerial(Ser);
			}else c->state = STATE_UNPLUGGED;
		}else c->state = STATE_UNPLUGGED;

		if( c->state != STATE_OPENDEV ){
			set( OUT_Info , MUIA_Text_Contents, (IPTR)"ERROR");
		}
	}
}


#define FILEBUFFSIZE 4000

void ConfNetWork(struct PPPcontrolMsg *msg,struct Conf *c){

	struct TagItem tags[] =
	{
		{ SYS_Input,  (IPTR)NULL  },
		{ SYS_Output, (IPTR)NULL  },
		{ SYS_Error,  (IPTR)NULL  },
		{ SYS_Asynch, (IPTR)FALSE },
		{ TAG_DONE,   0           }
	};

    struct Library *MiamiBase;
	TEXT arostcppath[256];
	UBYTE *buff;

	arostcppath[0]=0;
	GetVar( "SYS/Packages/AROSTCP" , arostcppath , 256 , LV_VAR);

	bug("\n###########################################################\n");
	bug("PPP is ONLINE !\n");
	bug("Local IP address %d.%d.%d.%d\n",msg->LocalIP[0],msg->LocalIP[1],msg->LocalIP[2],msg->LocalIP[3]);
	bug("Remote IP address %d.%d.%d.%d\n",msg->RemoteIP[0],msg->RemoteIP[1],msg->RemoteIP[2],msg->RemoteIP[3]);

	if( buff = AllocMem( FILEBUFFSIZE , MEMF_CLEAR|MEMF_PUBLIC ) ){

		bug("Primary DNS address %d.%d.%d.%d\n", msg->PrimaryDNS[0],msg->PrimaryDNS[1],
			msg->PrimaryDNS[2],msg->PrimaryDNS[3] );
		bug("Secondary DNS address %d.%d.%d.%d\n", msg->SecondaryDNS[0],msg->SecondaryDNS[1],
			msg->SecondaryDNS[2],msg->SecondaryDNS[3] );

        // Register nameservers with TCP/IP stack
		if( FindTask("bsdsocket.library") != NULL ) {
            MiamiBase = OpenLibrary("miami.library", 0);
            if(MiamiBase != NULL) {
                ClearDynNameServ();
                struct sockaddr_in ns_addr;

                ns_addr.sin_len = sizeof(ns_addr);
                ns_addr.sin_family = AF_INET;

                memcpy(&ns_addr.sin_addr.s_addr, msg->PrimaryDNS, 4);
                AddDynNameServ((struct sockaddr *)&ns_addr);

                memcpy(&ns_addr.sin_addr.s_addr, msg->SecondaryDNS, 4);
                AddDynNameServ((struct sockaddr *)&ns_addr);

                EndDynNameServ();
                CloseLibrary(MiamiBase);
            }
        }

		sprintf(buff,"%s/c/ifconfig %s %d.%d.%d.%d %d.%d.%d.%d",
				arostcppath,
				c->InterfaceName,
				msg->LocalIP[0],msg->LocalIP[1],
				msg->LocalIP[2],msg->LocalIP[3],
				msg->RemoteIP[0],msg->RemoteIP[1],
				msg->RemoteIP[2],msg->RemoteIP[3] );

		bug("Executing command:\"%s\"\n",buff);
		if( SystemTagList( buff , tags ) != 0 )
			bug("command FAIL !!!!\n");

		sprintf(buff,"%s/c/route add default %d.%d.%d.%d",
				arostcppath,
				msg->RemoteIP[0],msg->RemoteIP[1],
				msg->RemoteIP[2],msg->RemoteIP[3] );

		bug("Executing command:\"%s\"\n",buff);
		if(SystemTagList( buff , tags ) != 0 )
			bug("command FAIL !!!!\n");

		bug("\n############################################################*\n");

		FreeMem( buff , FILEBUFFSIZE );
	}
}

void HandleMessage(struct PPPcontrolMsg *InfoMsg,struct Conf *c){
	if( InfoMsg->Msg.mn_Length == sizeof(struct PPPcontrolMsg)
		&&  InfoMsg->Command == PPP_CTRL_INFO
	 ){
		if( exPhase != InfoMsg->Phase || exSer != InfoMsg->Ser || exstate !=  c->state ){
			exPhase = InfoMsg->Phase; exSer = InfoMsg->Ser; exstate =  c->state; 
			bug("ModemManager:handlemsg phase=%d,ser=%d,state=%d\n",exPhase,exSer,exstate);
		}
		// TERMINATE phase in progress ,dont do nothing
		if(  InfoMsg->Phase == PPP_PHASE_TERMINATE ){
			return;
		}

		// PPP initializing is ready
		if(  c->state == STATE_OPENDEV && InfoMsg->Phase == PPP_PHASE_NETWORK ){
			 ConfNetWork( InfoMsg ,c );
			 c->state = STATE_NETWORK;
		}

		// serial connection is lost (device unplugged)
		if( c->state != STATE_UNPLUGGED && (! InfoMsg->Ser) ){
			  c->state = STATE_PLUGGED;
		}

		// Connection is ok
		if( InfoMsg->Phase == PPP_PHASE_NETWORK &&  InfoMsg->Ser ){
			  c->state = STATE_NETWORK;
		}

		// net connection is lost
		if( c->state == STATE_NETWORK &&  InfoMsg->Phase != PPP_PHASE_NETWORK ){
		//	SendCtrlMsg( PPP_CTRL_CLOSE_SERIAL , 0 , c );
			c->state = STATE_PLUGGED;
		}
	}
}


int main(void)
{
   	struct Hook DisconnectHook,ConnectHook;
	struct EasyTimer *timer=0;
	BYTE buf[STRSIZE];
	buf[0]=0;
	ULONG sigs;
	struct PPPcontrolMsg *CtrlMsg=0;
	struct PPPcontrolMsg *InfoMsg=0;
	struct MsgPort *CtrlPort=0;
	struct EasySerial *Ser=0;
	struct Conf *c=0;

	struct EasyGraph *INegr=0;
	struct EasyGraph *OUTegr=0;

	Object *INGraphMUIbm,*OUTGraphMUIbm,*MUISignalBM;

	ULONG SpeedIn=0,SpeedOUT=0;

	if( ! FindPort(PortName)){
	if( timer=OpenTimer() ){
	if( CtrlPort = CreatePort(PortName,0) ){
	if( CtrlMsg = AllocMem(sizeof(struct PPPcontrolMsg),MEMF_PUBLIC | MEMF_CLEAR)){
	if( c = AllocMem(sizeof(struct Conf),MEMF_PUBLIC | MEMF_CLEAR)){

		NEWLIST(&c->atcl);

		for(;;)
		{

			ReadConfig(c);

			c->state = STATE_UNPLUGGED;
			c->signal = -1;
			c->AccessType = -1;
			SpeedIn =0;
			SpeedOUT = 0;

			SetTimer( timer , 0 );
			application = NULL;

			// send info request to ppp.device and wait response
			if( SendRequest() ){
				bug("ModemManager:wait response\n");
				sigs = Wait( SIGBREAKF_CTRL_C |
							(1L<< CtrlPort->mp_SigBit )
							);

				while( InfoMsg = (struct PPPcontrolMsg*)GetMsg(CtrlPort) ){
					HandleMessage(InfoMsg,c);
					ReplyMsg((struct Message *)InfoMsg);
				}

				if (sigs & SIGBREAKF_CTRL_C) goto shutdown;
			}

			SetTimer( timer , 1 );
			bug("ModemManager:wait until %s unit %d open.\n",c->DeviceName ,c->SerUnitNum);
			while(c->state == STATE_UNPLUGGED)
			{
				 sigs = Wait( SIGBREAKF_CTRL_C |
							(1L<< CtrlPort->mp_SigBit ) |
							(1L<< timer->TimeMsg->mp_SigBit )
							);

				// handle incoming messages
				while( InfoMsg = (struct PPPcontrolMsg*)GetMsg(CtrlPort) )
					ReplyMsg((struct Message *)InfoMsg);

				// Check if modem is plugged in.
				if(GetMsg(timer->TimeMsg)){
					FindModemUnit(c);
					if( c->SerUnitNum >= 0 ){
						if( Ser = OpenSerial( c->DeviceName ,c->SerUnitNum ) ){
							if( TestModem( Ser , c ) ){
								 c->state = STATE_PLUGGED;
							}
							CloseSerial(Ser);
						}
					}
					SetTimer( timer , 5 );
				}

				if (sigs & SIGBREAKF_CTRL_C) goto shutdown;
			}

			bug("ModemManager:Open GUI window\n");

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
						GroupFrame,
							Child, (IPTR) HGroup,
								Child, ModemName = SimpleText("1234567890123"),
								Child, MUISignalBM = BitmapObject,
									MUIA_FixWidth, 32,
									MUIA_FixHeight, 16,
								End,
								Child, AccessType = SimpleText("12345"),
							End,
							Child, (IPTR) HGroup,
								Child, IN_Info = SimpleText("12345678901"),
								Child, INGraphMUIbm = BitmapObject,
									MUIA_FixWidth, 80,
									MUIA_FixHeight, 16,
								End,
							End,
							Child, (IPTR) HGroup,
								Child, OUT_Info = SimpleText("12345678901"),
								Child, OUTGraphMUIbm = BitmapObject,
									MUIA_FixWidth, 80,
									MUIA_FixHeight, 16,
								End,
							End,

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
				sigs = 0;

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

				set( IN_Info , MUIA_Text_Contents, (IPTR)"");
				set( OUT_Info , MUIA_Text_Contents, (IPTR)"");

				SignalBM = MakeBitmap(32,16,window,MUISignalBM);
				INegr = MakeGraph(80,16,window,INGraphMUIbm);
				OUTegr = MakeGraph(80,16,window,OUTGraphMUIbm);
				set( INGraphMUIbm , MUIA_ShowMe , FALSE );
				set( OUTGraphMUIbm , MUIA_ShowMe , FALSE );

				SetTimer( timer , 1 );
				SendRequest();

				UpdateModemInfo( SignalBM , c );

				while(
						DoMethod(
							application, MUIM_Application_NewInput, (IPTR) &sigs
						) != MUIV_Application_ReturnID_Quit
					)
				{
					if (sigs){
						 sigs = Wait(	sigs |
										SIGBREAKF_CTRL_C |
										(1L<< CtrlPort->mp_SigBit ) |
										(1L<< timer->TimeMsg->mp_SigBit )
									);

						while( InfoMsg = (struct PPPcontrolMsg*)GetMsg(CtrlPort) ){
							//bug("ModemManager: received info message num %d\n",InfoMsg->num);

							HandleMessage(InfoMsg,c);

							if( c->state == STATE_UNPLUGGED ){
								set( IN_Info , MUIA_Text_Contents, (IPTR)"Unplugged");
								set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
								set(DisConBut, MUIA_Disabled , TRUE );
								set(   ConBut, MUIA_Disabled , TRUE );
							}
							else if( c->state == STATE_PLUGGED ){
								set( INGraphMUIbm , MUIA_ShowMe , FALSE );
								set( OUTGraphMUIbm , MUIA_ShowMe , FALSE );
								set( IN_Info , MUIA_Text_Contents, (IPTR)"");
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
								if( InfoMsg->Phase == PPP_PHASE_TERMINATE ){
									set(DisConBut, MUIA_Disabled , TRUE );
									set(   ConBut, MUIA_Disabled , TRUE );
									set( INGraphMUIbm , MUIA_ShowMe , FALSE );
									set( OUTGraphMUIbm , MUIA_ShowMe , FALSE );
									set( IN_Info , MUIA_Text_Contents, (IPTR)"Terminate...");
									set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
								} else{
									SpeedIn = InfoMsg->SpeedIn;
									SpeedOUT = InfoMsg->SpeedOut;

									speedstr(buf," In",SpeedIn);
									set( IN_Info , MUIA_Text_Contents, (IPTR)buf);
									speedstr(buf,"Out",SpeedOUT);
									set( OUT_Info , MUIA_Text_Contents, (IPTR)buf);

									set(DisConBut, MUIA_Disabled , FALSE );
									set(   ConBut, MUIA_Disabled , TRUE );

									set( INGraphMUIbm , MUIA_ShowMe , TRUE );
									set( OUTGraphMUIbm , MUIA_ShowMe , TRUE );
								}
							}

							ReplyMsg((struct Message *)InfoMsg);
						//bug("ModemManager: ReplyMsg OK\n");
						}


						if(GetMsg(timer->TimeMsg)){

							if( c->state == STATE_NETWORK ){
								speedstr(buf," In",SpeedIn);
								set( IN_Info , MUIA_Text_Contents, (IPTR)buf);
								speedstr(buf,"Out",SpeedOUT);
								set( OUT_Info , MUIA_Text_Contents, (IPTR)buf);
								UpdateGraph(INegr,(FLOAT)SpeedIn);
								UpdateGraph(OUTegr,(FLOAT)SpeedOUT);
							}

							// test if modem is unplugged
							if( c->state == STATE_PLUGGED ){
								if( Ser = OpenSerial( c->DeviceName ,c->SerUnitNum ) ){
									 CloseSerial(Ser);
									 //UpdateModemInfo( SignalBM , c );
								} else
									c->state = STATE_UNPLUGGED;
							}

							if( c->state == STATE_PLUGGED ){
								set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
								set( IN_Info , MUIA_Text_Contents, (IPTR)"");
								set(DisConBut, MUIA_Disabled , TRUE );
								set(   ConBut, MUIA_Disabled , FALSE );
							}

							SetTimer( timer , TIMERVALUE );
						}

						if (sigs & SIGBREAKF_CTRL_C) goto shutdown;
					}

					if( c->state == STATE_UNPLUGGED ) break;
				} //GUI loop

				// MUIV_Application_ReturnID_Quit ?
				if( c->state != STATE_UNPLUGGED ) break;
			}
			bug("ModemManager:Device Unplugged -> Close GUI window\n");
			SetTimer( timer , 0 );
			MUI_DisposeObject(application);
			application=NULL;
			CloseBitmap(SignalBM);
			CloseGraph(INegr);
			CloseGraph(OUTegr);
		} // main loop

	}}}}}

	// close all
	shutdown:
	bug("ModemManager:ShutDown\n");
	if(application) MUI_DisposeObject(application);
	CloseBitmap(SignalBM);
	CloseGraph(INegr);
	CloseGraph(OUTegr);
	CloseTimer(timer);

	struct at_command  *atc;
	while( atc = (struct at_command *)RemHead( &c->atcl ) ){
		FreeMem( atc , sizeof(struct at_command) );
	}

	if(CtrlPort){
		Forbid();
			while( GetMsg(CtrlPort) ) ReplyMsg((struct Message *)CtrlMsg);
			DeletePort(CtrlPort);
		Permit();
	}

	if( c ) FreeMem( c , sizeof(struct Conf));
	if( CtrlMsg ) FreeMem(CtrlMsg , sizeof(struct PPPcontrolMsg));

  return 0;
}
