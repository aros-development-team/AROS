/*
 * $Id$
 */

#ifndef _PPP_H
#define _PPP_H

#define PPP_MAXBUFF 4096
#define SERIAL_BUFSIZE PPP_MAXBUFF

// phases
#define PPP_PHASE_DEAD 1
#define PPP_PHASE_CONFIGURATION 2
#define PPP_PHASE_AUTHENTICATION 3
#define PPP_PHASE_PROTOCOL_CONF 4
#define PPP_PHASE_NETWORK 5
#define PPP_PHASE_TERMINATE 6

// config stuff
#define PPP_MAXARGLEN 100

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
	
	BYTE *username;
	BYTE *password;
	
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
	//BYTE command,arg;
	BYTE str[PPP_MAXARGLEN];
};




#endif

