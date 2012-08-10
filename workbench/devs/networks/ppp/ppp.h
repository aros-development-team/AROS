/*
 * $Id$
 */

#ifndef _PPP_H
#define _PPP_H

#define DEBUG 1

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <dos/bptr.h>

#include <oop/oop.h>

#include <hidd/pci.h>

#include <devices/timer.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include LC_LIBDEFS_FILE


/*
** Maximum Transmission Unit
*/
#define PPP_MTU 1500

/*
** Max # of Units allowed
*/
#define SD_MAXUNITS 1

#define PPP_MAXBUFF 4096
#define SERIAL_BUFSIZE PPP_MAXBUFF
#define PPP_MAXARGLEN 100

// phases
#define PPP_PHASE_DEAD 1
#define PPP_PHASE_CONFIGURATION 2
#define PPP_PHASE_AUTHENTICATION 3
#define PPP_PHASE_PROTOCOL_CONF 4
#define PPP_PHASE_NETWORK 5
#define PPP_PHASE_TERMINATE 6

// PPPcontrolMsg commands
#define PPP_CTRL_INFO_REQUEST 1
#define PPP_CTRL_INFO 2
#define PPP_CTRL_SETPHASE 3
#define PPP_CTRL_OPEN_SERIAL 4
#define PPP_CTRL_CLOSE_SERIAL 5

 struct PPPcontrolMsg{
	 
	struct Message Msg;
	
	ULONG Command;      // command
	IPTR Arg;         // command argument
	
	UBYTE *DeviceName;  // serial device name
	ULONG UnitNum;     // serial device unit number
	UBYTE *username;
	UBYTE *password;
	
	// info response part: 
	UBYTE Phase;  // ppp phase
	BOOL Ser;	 // serial device status
	BOOL Up;    // ppp device up/down
	ULONG BytesIn;
	ULONG BytesOut;
	ULONG SpeedIn;
	ULONG SpeedOut;
	ULONG UpTime;
	
	UBYTE LocalIP[4];
	UBYTE RemoteIP[4];
	UBYTE PrimaryDNS[4];
	UBYTE SecondaryDNS[4];

	ULONG num; // message number (debug purposes)
};

struct at_command {
	struct Node cNode;
	BYTE command,arg;
	BYTE str[PPP_MAXARGLEN];
};

 struct EasyTimer{
	struct MsgPort  *TimeMsg;
	struct timerequest *TimeReq;
};

 struct EasySerial{
	struct IOExtSer	*SerRx;   /* Serial IORequest for CMD_READ's */
	struct IOExtSer	*SerTx;   /* Serial IORequest for CMD_WRITE's */
	struct MsgPort	*RxPort;  /* Serial CMD_READ IORequest reply port */
	struct MsgPort	*TxPort;  /* Serial CMD_WRITE IORequest reply port */
	UBYTE			*RxBuff;	/* Buffer for holding incoming data */
	UBYTE			*TxBuff;	/* Buffer for hold outgoing packets */
	BOOL Ok; // is device ok (= not unplugged)
};

struct PPPBase {
	struct Device	sd_Device;
	ULONG			sd_OpenCnt;
	UBYTE			sd_Flags;

	BOOL device_up;
	BOOL sdu_Proc_run;

	ULONG gui_message;

	struct Unit  *sd_Unit;
	struct SignalSemaphore sd_Lock;

	struct Process	*sdu_Proc;

	struct EasySerial *ser;

	BYTE DeviceName[PPP_MAXARGLEN];
	BYTE SerUnitNum;
	BYTE username[PPP_MAXARGLEN];
	BYTE password[PPP_MAXARGLEN];

	BOOL		(*CopyFromBuffer)(APTR, APTR, ULONG);
	BOOL		(*CopyToBuffer)(APTR, APTR, ULONG);

	struct SignalSemaphore   sdu_ListLock;	/* A Semaphore for access to all of our queues. */
	struct MinList	 Rx_List;				/* Pending CMD_READ's */
	struct MinList	 Tx_List;				/* Pending CMD_WRITE's */

	ULONG BytesIn;
	ULONG BytesOut;
	ULONG SpeedIn;
	ULONG SpeedOut;
	ULONG UpTime;
	
	UBYTE LocalIP[4];
	UBYTE RemoteIP[4];
	UBYTE PrimaryDNS[4];
	UBYTE SecondaryDNS[4];
};



#endif

