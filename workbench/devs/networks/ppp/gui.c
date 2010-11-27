/*
 * $Id: gui.c 34949 2010-10-25 18:27:45Z Sami $
 */

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
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <utility/hooks.h>
#include <libraries/mui.h>

#include "ppp.h"
#include "device_protos.h"

#define SimpleText(text) TextObject, MUIA_Text_Contents, (IPTR) text, End

#define TIMERVALUE 2
#define STRSIZE 100

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
	LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)*arg;

	LIBBASE->gui_message = GUIM_DISCONNECT;
	if(LIBBASE->sdu_Proc) Signal( (struct Task*)LIBBASE->sdu_Proc , SIGBREAKF_CTRL_F );

}

static void ConnectFunc(struct Hook *hook, Object *app, APTR *arg)
{
	LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)*arg;

	LIBBASE->gui_message = GUIM_CONNECT;
	if(LIBBASE->sdu_Proc) Signal( (struct Task*)LIBBASE->sdu_Proc , SIGBREAKF_CTRL_F );
}

void ppp_gui(void)
{
	Object *application, *window, *DisConBut, *ConBut;
	Object *IN_Info,*OUT_Info;

   	struct Hook DisconnectHook,ConnectHook;
	LIBBASETYPEPTR LIBBASE;
	struct MsgPort	 *TimeMsg=0;
	struct timerequest *TimeReq=0;
	ULONG oldin=0,speedin=0;
	ULONG oldout=0,speedout=0;
	BYTE buf[STRSIZE];


	LIBBASE = FindTask(NULL)->tc_UserData;

	if( TimeMsg = CreateMsgPort() ){
		if( TimeReq = CreateIORequest((struct MsgPort *)TimeMsg, sizeof(struct timerequest))){
			if(!OpenDevice("timer.device", UNIT_VBLANK,(struct IORequest *)TimeReq, 0)){


				bug("PPP GUI process  hello!\n");
				DisconnectHook.h_Entry = HookEntry;
				DisconnectHook.h_SubEntry = (HOOKFUNC) DisconnectFunc;
				ConnectHook.h_Entry = HookEntry;
				ConnectHook.h_SubEntry = (HOOKFUNC) ConnectFunc;

				application = ApplicationObject,
				SubWindow, window = WindowObject,
					MUIA_Window_Title,	(IPTR) "ppp",
					MUIA_Window_Activate,TRUE,
						WindowContents, (IPTR) VGroup,
							Child, (IPTR) VGroup,
								Child, IN_Info = SimpleText(""),
								Child, OUT_Info = SimpleText(""),
							End,
							Child, (IPTR) HGroup,
								Child, (IPTR) (   ConBut = SimpleButton(" Connect  ")),
								Child, (IPTR) (DisConBut = SimpleButton("Disconnect")),
							End,
						End,
					End,
				End;

				if (application)
				{
				ULONG sigs = 0;
				LIBBASE->gui_run = TRUE;

				DoMethod(
						window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
						(IPTR) application, 2, MUIM_Application_ReturnID,
						MUIV_Application_ReturnID_Quit
					);

					DoMethod(
						ConBut,MUIM_Notify,MUIA_Pressed,FALSE,
						application, (IPTR) 3,
						MUIM_CallHook, &ConnectHook,LIBBASE
					);

					DoMethod(
						DisConBut,MUIM_Notify,MUIA_Pressed,FALSE,
						application, (IPTR) 3,
						MUIM_CallHook, &DisconnectHook,LIBBASE
					);

				set(window,MUIA_Window_Open,TRUE);

				set(DisConBut,MUIA_Disabled,TRUE);
				set(   ConBut,MUIA_Disabled,TRUE);

				TimeReq->tr_time.tv_secs = TIMERVALUE;
				TimeReq->tr_time.tv_micro = 0;
				((struct IORequest *)TimeReq)->io_Command = TR_ADDREQUEST;
				SendIO( (struct IORequest *)TimeReq );

				while(
						DoMethod(
							application, MUIM_Application_NewInput, (IPTR) &sigs
						) != MUIV_Application_ReturnID_Quit
					)
				{
					if (sigs){
						sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F | (1L<< TimeMsg->mp_SigBit ) );
						if (sigs & SIGBREAKF_CTRL_C) break;

						if( ! LIBBASE->device_up ){
							set( IN_Info , MUIA_Text_Contents, (IPTR)"Disconnected");
							set( OUT_Info , MUIA_Text_Contents, (IPTR)"");
							set(DisConBut, MUIA_Disabled , TRUE );
							set(   ConBut, MUIA_Disabled , FALSE );
						}

						else if( ! LIBBASE->ser ){
							set( IN_Info , MUIA_Text_Contents, (IPTR)"OpenDevice:");
							snprintf( buf, STRSIZE , "%s unit %d",LIBBASE->DeviceName,LIBBASE->SerUnitNum);
							set( OUT_Info , MUIA_Text_Contents, (IPTR)buf);
							set(DisConBut, MUIA_Disabled , FALSE );
							set(   ConBut, MUIA_Disabled , TRUE );
						}

						else if( LIBBASE->ser &&  Phase() != PPP_PHASE_NETWORK  ){
							set( IN_Info , MUIA_Text_Contents, (IPTR)"Connection in progress:");
							UBYTE phase = Phase();

							set( OUT_Info , MUIA_Text_Contents,
								(IPTR)( phase == PPP_PHASE_CONFIGURATION  ? "LCP conficuration" :
										phase == PPP_PHASE_AUTHENTICATION ? "Authentication" :
										phase == PPP_PHASE_PROTOCOL_CONF  ? "Protocol configuration" :
																			"DialUp"
								));

							set(DisConBut, MUIA_Disabled , FALSE );
							set(   ConBut, MUIA_Disabled , TRUE );
							set(window,MUIA_Window_Title,(IPTR)LIBBASE->modemmodel);
						}

						else if( Phase() == PPP_PHASE_NETWORK  ){

							set(DisConBut, MUIA_Disabled , FALSE );
							set(   ConBut, MUIA_Disabled , TRUE );

							speedstr(buf," In",speedin);
							set( IN_Info , MUIA_Text_Contents, (IPTR)buf);

							speedstr(buf,"Out",speedout);
							set( OUT_Info , MUIA_Text_Contents, (IPTR)buf);
						}

						if(GetMsg(TimeMsg)){

							speedin =  ( LIBBASE->bytes_in - oldin ) / TIMERVALUE;
							speedout =  ( LIBBASE->bytes_out - oldout ) / TIMERVALUE;
							oldin = LIBBASE->bytes_in;
							oldout = LIBBASE->bytes_out;

							TimeReq->tr_time.tv_secs = TIMERVALUE;
							TimeReq->tr_time.tv_micro = 0;
							((struct IORequest *)TimeReq)->io_Command = TR_ADDREQUEST;
							SendIO( (struct IORequest *)TimeReq );
						}

					}
				}

				MUI_DisposeObject(application);
				}

			}
		}
	}

	bug("PPP GUI process  shotdown\n");

	if( TimeReq ){
		AbortIO((struct IORequest *)TimeReq);
		WaitIO((struct IORequest *)TimeReq);
		while(GetMsg(TimeMsg));
		CloseDevice((struct IORequest *)TimeReq);
	}
	if(TimeReq) DeleteIORequest(TimeReq);
	if(TimeMsg) DeleteMsgPort(TimeMsg);

	Forbid();
	LIBBASE->gui_run = FALSE;
}



void open_gui(LIBBASETYPEPTR LIBBASE){

	if( LIBBASE->gui_run )return;

	D(bug("New ppp gui process:\n"));
	if(LIBBASE->gui_process = CreateNewProcTags(
								NP_Entry, ppp_gui,
								NP_Name, "PPP GUI process",
								NP_Synchronous , FALSE,
								NP_Priority, 0,
								NP_UserData, LIBBASE,
								NP_StackSize, 4096,
								TAG_DONE))
	{
	while( ! LIBBASE->gui_run ) Delay(5);
		D(bug("...ok\n"));
	}else{
		D(bug("New process:FAILL !!!\n"));
	}
}

void close_gui(LIBBASETYPEPTR LIBBASE){
	if( LIBBASE->gui_run ){
		Signal( (struct Task*)LIBBASE->gui_process , SIGBREAKF_CTRL_C );
		while(  LIBBASE->gui_run ) Delay(5);
	}
}
