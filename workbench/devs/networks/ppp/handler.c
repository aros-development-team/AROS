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


VOID PerformIO(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2){

	// D(bug("PerformIO Command-->"));

	ios2->ios2_Req.io_Error = 0;

	switch (ios2->ios2_Req.io_Command){
	case CMD_READ:
		ReadPacket(LIBBASE,ios2);
		break;

	case CMD_WRITE:
		WritePacket(LIBBASE,ios2);
		break;

	case S2_DEVICEQUERY:
		DeviceQuery(LIBBASE,ios2);
		break;

	case S2_GETSTATIONADDRESS:
		GetStationAddress(LIBBASE,ios2);
		break;

	case S2_BROADCAST:
		WritePacket(LIBBASE,ios2);
		break;

	case S2_ONLINE:
		Online(LIBBASE,ios2);
		break;

	case S2_OFFLINE:
		Offline(LIBBASE,ios2);
		break;

	case S2_CONFIGINTERFACE:
		ConfigInterface(LIBBASE,ios2);
		break;

	default:
		D(bug("Unknown Command %d\n",ios2->ios2_Req.io_Command));
		ios2->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
		ios2->ios2_WireError = S2WERR_GENERIC_ERROR;
		TermIO(LIBBASE,ios2);
		break;
	}
//  case S2_TRACKTYPE:
//  case S2_UNTRACKTYPE:
//  case S2_GETTYPESTATS:
//  case S2_GETGLOBALSTATS:
//  case S2_ONEVENT:
//  case S2_READORPHAN:
//  case S2_ADDMULTICASTADDRESS:
//  case S2_DELMULTICASTADDRESS:
//  case S2_MULTICAST:
//  case S2_GETSPECIALSTATS:

}


VOID TermIO(LIBBASETYPEPTR LIBBASE,struct IOSana2Req *ios2){
	if(!(ios2->ios2_Req.io_Flags & IOF_QUICK)){
		ReplyMsg((struct Message *)ios2);
	}
}


VOID ConfigInterface(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *ios2){
	D(bug("ConfigInterface\n"));
	//CopyMem(&sdu->sdu_StAddr,&ios2->ios2_SrcAddr,4);
	//ios2->ios2_Req.io_Error = S2ERR_BAD_STATE;
	//ios2->ios2_WireError = S2WERR_IS_CONFIGURED;
	TermIO(LIBBASE,ios2);
}


VOID GetStationAddress(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *ios2){
	D(bug("GetStationAddress\n"));
	memset(ios2->ios2_DstAddr, 0, SANA2_MAX_ADDR_BYTES);
	memset(ios2->ios2_SrcAddr, 0, SANA2_MAX_ADDR_BYTES);
	TermIO(LIBBASE,ios2);
}


VOID DeviceQuery(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *ios2){
	struct Sana2DeviceQuery *sdq;
	D(bug("DeviceQuery\n"));
	sdq = (struct Sana2DeviceQuery *)ios2->ios2_StatData;

	sdq->DevQueryFormat = 0;
	sdq->DeviceLevel = 0;
	sdq->AddrFieldSize = 32;
	sdq->MTU = PPP_MTU;
	sdq->BPS = 1000000;
	sdq->HardwareType = S2WireType_PPP;
	sdq->SizeSupplied = sizeof(*sdq);

	TermIO(LIBBASE,ios2);
}


VOID WritePacket(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *ios2){
	//  D(bug("WritePacket\n"));
	/* Make sure that we are online. */
	if( LIBBASE->device_up &&  Phase() == PPP_PHASE_NETWORK && LIBBASE->ser ){
		/* Make sure it's a legal length. */
		if(ios2->ios2_DataLength <= PPP_MTU){
			/* See if our serial CMD_WRITE command is busy.  If it's not, send
			   the IO request to SendPacket. */
			if(CheckIO((struct IORequest *)LIBBASE->ser->SerTx)){
				WaitIO((struct IORequest *)LIBBASE->ser->SerTx);
				SendPacket(LIBBASE, ios2);
			}else{
				/* We'll have to queue the packet for later...*/
				ios2->ios2_Req.io_Flags &= ~IOF_QUICK;
				ObtainSemaphore(&LIBBASE->sdu_ListLock);
				AddTail((struct List *)&LIBBASE->Tx_List,(struct Node *)ios2);
				ReleaseSemaphore(&LIBBASE->sdu_ListLock);
			}

		}else{
			/* Sorry, the packet is too long! */
			D(bug("WritePacket TOO LONG !\n"));
			ios2->ios2_Req.io_Error = S2ERR_MTU_EXCEEDED;
			ios2->ios2_WireError = S2WERR_GENERIC_ERROR;
			TermIO(LIBBASE,ios2);
			//  DoEvent(LIBBASE,sdu,S2EVENT_TX);
		}

	}else{
		/* Sorry, we're offline */
		ios2->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ios2->ios2_WireError = S2WERR_UNIT_OFFLINE;
		TermIO(LIBBASE,ios2);
	}
}


VOID SendPacket( LIBBASETYPEPTR LIBBASE ,struct IOSana2Req *ios2 ){

	//  D(bug("SendPacket lenght %d\n",ios2->ios2_DataLength));

	if( ( Phase() == PPP_PHASE_NETWORK ) && LIBBASE->device_up && LIBBASE->ser ){

		if( LIBBASE->CopyFromBuffer( LIBBASE->ser->TxBuff , ios2->ios2_Data , ios2->ios2_DataLength ) ){
			send_IP_packet( LIBBASE->ser->TxBuff , ios2->ios2_DataLength );		
            LIBBASE->BytesOut += ios2->ios2_DataLength;
		}else{
			bug( "SendPacket CopyFromBuffer FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			ios2->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
			ios2->ios2_WireError = S2WERR_BUFF_ERROR;
		}

	}else{
		ios2->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ios2->ios2_WireError = S2WERR_UNIT_OFFLINE;
	}

	TermIO(LIBBASE,ios2);

}


VOID ReadPacket(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *ios2){

//   D(bug("ReadPacket type %d ",ios2->ios2_PacketType));

	if( ( Phase() == PPP_PHASE_NETWORK ) && LIBBASE->device_up && LIBBASE->ser ){
		//   D(bug("Add to list...\n"));
		ObtainSemaphore(&LIBBASE->sdu_ListLock);
		AddTail((struct List *)&LIBBASE->Rx_List,(struct Node *)ios2);
		ReleaseSemaphore(&LIBBASE->sdu_ListLock);
	}else{
		// D(bug("Sorry,We're offline..\n"));
		ios2->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
		ios2->ios2_WireError = S2WERR_UNIT_OFFLINE;
		TermIO(LIBBASE,ios2);
	}
}



VOID Online(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *ios2){
	D(bug("Online\n"));
	LIBBASE->device_up = TRUE;
	TermIO(LIBBASE,ios2);
}


VOID Offline(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *ios2){

	D(bug("Offline\n"));
	Set_phase( PPP_PHASE_TERMINATE );
	LIBBASE->device_up = FALSE;
	TermIO(LIBBASE,ios2);

}



VOID CMD_WRITE_Ready(LIBBASETYPEPTR LIBBASE){
	struct IOSana2Req *ios2;

	/* See if we have any pending CMD_WRITE requests. */
	ObtainSemaphore(&LIBBASE->sdu_ListLock);
	ios2 = (struct IOSana2Req *)RemHead((struct List *)&LIBBASE->Tx_List);
	ReleaseSemaphore(&LIBBASE->sdu_ListLock);

	if(ios2) SendPacket( LIBBASE , ios2 );

}


VOID CMD_READ_Ready(LIBBASETYPEPTR LIBBASE, struct IOExtSer *ioSer){
	UBYTE  *ptr;
	ULONG length;
    if(LIBBASE->ser){
		ptr = LIBBASE->ser->RxBuff;
		length = ioSer->IOSer.io_Actual;
		bytes_received( ptr,length );
		LIBBASE->BytesIn += length;
		QueueSerRequest( LIBBASE->ser ,  PPP_MAXBUFF );
	}
}


// ppp.c call this if IP packet is received.

VOID Incoming_IP_Packet(LIBBASETYPEPTR LIBBASE, BYTE *data , ULONG length){
	struct IOSana2Req *ios2;
	struct PPP_DevUnit *sdu;

// bug("GotPacket %d bytes \n",length);

	sdu =   (struct PPP_DevUnit *)LIBBASE->sd_Unit;

	if(length){ // ignore zero-length packets.

		ObtainSemaphore(&LIBBASE->sdu_ListLock);
		ios2 = (struct IOSana2Req *)RemHead((struct List *)&LIBBASE->Rx_List);
		ReleaseSemaphore(&LIBBASE->sdu_ListLock);

		if(ios2){
			if( LIBBASE->CopyToBuffer(ios2->ios2_Data, data ,length)){
				//    bug("CopyToBuffer OK\n");
				ios2->ios2_DataLength = length;
			}else{
				bug("CopyToBuffer FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
				ios2->ios2_DataLength = 0;
				ios2->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
				ios2->ios2_WireError = S2WERR_BUFF_ERROR;
			}
			TermIO(LIBBASE,ios2);
		}else{
			bug( "Orphan Packet Receivet !!! %d bytes\n" , length );
		}
	}

}



