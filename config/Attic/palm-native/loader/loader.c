/*    
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Code for downloading an ELF file onto the Palm.
*/

#include <Pilot.h>
#include <System/SerialMgr.h>
#include <System/StringMgr.h>
#include <string.h>
#include "callback.h"
#define __PALM_CODE__
#include "../registers.h"

#include "loaderRsc.h"

ULong get_ssp(void);


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define UNPROTECT_MEMORY         \
	csa0 = RREG_L(CSA0);     \
	csa1 = RREG_L(CSA1);     \
	csa2 = RREG_L(CSA2);     \
	csa3 = RREG_L(CSA3);     \
	WREG_L(CSA0)=csa0 &(~8); \
	WREG_L(CSA1)=csa1 &(~8); \
	WREG_L(CSA2)=csa2 &(~8); \
	WREG_L(CSA3)=csa3 &(~8);

#define PROTECT_MEMORY    \
	WREG_L(CSA0)=csa0;\
	WREG_L(CSA1)=csa1;\
	WREG_L(CSA2)=csa2;\
	WREG_L(CSA3)=csa3;

#define BAUDRATE 57600

int _strncmp(const char * s1, const char * s2, int len)
{
	int c = 0;
	int d;
	while (c < len) {
		d = s1[c] - s2[c];
		if (d != 0)
			return d;
		c++;
	} 
	return 0;
}

void _strncpy(char * d, const char * s, int len)
{
	int c = 0;
	while (c < len) {
		d[c] = s[c];
		c++;
	}
}

void _delay(UInt ctr) 
{
	while (ctr > 0) {
		int i = 1000;
		int x = 2;
		while (i > 0) {
			i--;
			x = x*x;
		}
		ctr--;
	}
}

#define TYPE_HANDSHAKE         0x01
#define TYPE_READ_MEM          0x02
#define TYPE_WRITE_MEM         0x03
#define TYPE_MEM_CONTENT       0x04
#define TYPE_WRITE_MEM_ACK     0x05
#define TYPE_GET_SPECIAL_INFO  0x06
#define TYPE_START_PROGRAM     0x07
#define TYPE_START_PROGRAM_ACK 0x08
#define TYPE_QUIT              0x10

struct Packet
{
	unsigned short  payload_length;
	unsigned char   payload_type;
	char *          payload;
	int             state;
	unsigned short  pos;
};

enum {
	GET_FRAME_BEGINNING = 0x01,
	GET_PAYLOAD_LENGTH_HI,
	GET_PAYLOAD_LENGTH_LO,
	GET_TYPE,
	GET_PAYLOAD,
	GET_FRAME_END
};

void free_packet(struct Packet * packet)
{
	if (packet->payload)
		MemPtrFree(packet->payload);
	MemPtrFree(packet);
}

struct Packet * ready_packet = NULL;
struct Packet * current_packet = NULL;

void free_all_packets(void)
{
	if (NULL != current_packet) {
		free_packet(current_packet);
	}

	if (NULL != ready_packet)
		free_packet(ready_packet);
}


struct Packet * get_next_packet(UInt refnum)
{
	struct Packet * p = NULL;
	ULong len;
	Err err;
	
	err = SerReceiveCheck(refnum, &len);

	if (0 != len) {
		char * buf;
		int i;

		buf = MemPtrNew(len);
		if (NULL != buf) {

			len = SerReceive(refnum,
			                 buf,
			                 len,
			                 1000,
			                 &err);
		}
		
		i = 0;
		while (i < len)	{

			if (NULL == current_packet) {
				current_packet = MemPtrNew(sizeof(struct Packet));
				current_packet->state = GET_FRAME_BEGINNING;
				current_packet->pos = 0;
			}
		
			switch (current_packet->state) {
				case GET_FRAME_BEGINNING:
					if (0x7f == buf[i])
						current_packet->state = GET_PAYLOAD_LENGTH_HI;
					i++;
				break;
			
				case GET_PAYLOAD_LENGTH_HI:
					current_packet->payload_length = ((unsigned short)buf[i]) << 8;
					current_packet->state = GET_PAYLOAD_LENGTH_LO;
					i++;
				break;
			
				case GET_PAYLOAD_LENGTH_LO:
					current_packet->payload_length |= ((unsigned short)buf[i]);
					current_packet->state = GET_TYPE;
					current_packet->payload = MemPtrNew(current_packet->payload_length);
					i++;
				break;

				case GET_TYPE:
					current_packet->payload_type = buf[i];
					current_packet->state = GET_PAYLOAD;
					i++;
				break;
				
				case GET_PAYLOAD:
					while (i < len && 
					       current_packet->pos < current_packet->payload_length) {

						current_packet->payload[current_packet->pos++] = buf[i];
						i++;
					}
					
					if (current_packet->pos == current_packet->payload_length)
						current_packet->state = GET_FRAME_END;
				break;

				case GET_FRAME_END:
					if (0x7f == buf[i]) {
						ready_packet = current_packet;
					} else {
						free_packet(current_packet);
					}
					i++;
					current_packet = NULL;
				break;
				
				default:
					i++;
			}
		}
	}

	/*
	 * Try to get the first complete packet in the list.
	 */
	p = (struct Packet *)ready_packet;
	if (NULL != p) {
		int i = 0;
		int c = 0;

		ready_packet = NULL;
		/*
		 * Reconvert the payload (if necessary)
		 */
		while (i < p->payload_length) {
			if (0x27 == p->payload[i])
				c++;
			i++;
		}
		
		if (c != 0) {
			/*
			 * Convert the payload
			 */
			char * pl = MemPtrNew(p->payload_length - c);
			if (NULL != pl) {
				int _i = 0;
				i = 0;
				while (i < p->payload_length) {
					if (0x27 == p->payload[i]) {
						if (0x28 == p->payload[i+1]) {
							pl[_i] = 0x27;
						} else
							pl[_i] = 0x7f;
						i++;
					} else
						pl[_i] = p->payload[i]; 
					i++;
					_i++;
				}
			}
			MemPtrFree(p->payload);
			p->payload = pl;
			p->payload_length -= c;
		}
	}
	return p;
}


void send_packet(UInt refnum, char * buffer, unsigned short len, unsigned char type)
{
	ULong i = 0, c = 0;
	char * packet;
	Err err;
	
	while (i < len) {
		/*
		 * Count number of characters to escape.
		 */
		if (0x7f == buffer[i] || 0x27 == buffer[i])
			c++;
		i++;
	}
	
	packet = MemPtrNew(1 + /* frame begin */
	                   2 + /* length of payload indicator */
	                   1 + /* type */
	                   len + c + /* payload */
	                   1 /* frame end */);

	if (NULL != packet) {
		int pos = 4;
		packet[0] = 0x7f;
		packet[1] = ((len+c) >> 8) & 0xff;
		packet[2] = ((len+c)     ) & 0xff;
		packet[3] = type;
		
		i = 0;
		/*
		 * convert forbidden bytes.
		 */
		while (i < len) {
			if (0x7f == buffer[i]) {
				packet[pos++] = 0x27;
				packet[pos] = 0x29; /* = 0x7f */
			} else if (0x27 == buffer[i]) {
				packet[pos++] = 0x27;
				packet[pos] = 0x28; /* = 0x27 */
			} else {
				packet[pos] = buffer[i];
			}
			pos++;
			i++;
		}
		packet[pos] = 0x7f;
		
		/*
		 * Send this packet
		 */
		SerSend(refnum,packet,pos+1,&err);
		SerSendWait(refnum,-1);

		MemPtrFree(packet);
	}
}

/*
 * Do a handshake. I send "AROS?", expect "AROS!" and send back
 * "greets!".
 */
int do_handshake(UInt refnum)
{
	struct Packet * packet;
	int ctr = 0;
	
	while (ctr < 10) {
		send_packet(refnum, "AROS?", 5, TYPE_HANDSHAKE);
		_delay(1000);
		packet = get_next_packet(refnum);
		if (NULL != packet) {
			if (0 == _strncmp("AROS!", &packet->payload[0], 5) &&
			    5 == packet->payload_length) {
				free_packet(packet);
				send_packet(refnum, "greets!", 7, TYPE_HANDSHAKE);
				return 0;
			}
		}
		ctr++;
	}

	return -1;
}

void howdy(UInt refnum)
{
	send_packet(refnum,"HOWDY!",6,0xff);
}

void read_mem(UInt refnum, struct Packet * packet)
{
	/*
	 * Layout of packet:
	 * ULong: Start address
	 * unsigned short: Num Bytes
	 */
	if (6 == packet->payload_length) {
		char * buf;
		unsigned short i;
		ULong ctr;
		ULong addr = (((ULong)packet->payload[0]) << 24) |
		             (((ULong)packet->payload[1]) << 16) |
		             (((ULong)packet->payload[2]) <<  8) |
		             (((ULong)packet->payload[3]) <<  0);  
		ULong numbytes = (((ULong)packet->payload[4]) << 8) |
		                 (((ULong)packet->payload[5])     );
		                 
		buf = MemPtrNew((ULong)numbytes+ sizeof(addr));
		
		_strncpy(buf, (char *)&addr, sizeof(addr));
		
		ctr = numbytes;
		i = sizeof(addr);
		if (NULL != buf) {
			while (ctr > 0) {
				buf[i] = RREG_B(addr);
				i++;
				addr++;
				ctr--;
			}
			send_packet(refnum,buf,numbytes+sizeof(addr),TYPE_MEM_CONTENT);
		}
	}
}

void write_mem(UInt refnum, struct Packet * packet)
{
	/*
	 * Layout of packet:
	 * ULong: Start address
	 */
	if (packet->payload_length >= 4) {
		ULong ctr, i;
		ULong csa0, csa1, csa2, csa3;
		ULong addr = (((ULong)packet->payload[0]) << 24) |
		             (((ULong)packet->payload[1]) << 16) |
		             (((ULong)packet->payload[2]) <<  8) |
		             (((ULong)packet->payload[3]) <<  0);
		i = packet->payload_length - 4;
		ctr = 4;
		UNPROTECT_MEMORY
		while (i > 0) {
			*(Byte *)addr = packet->payload[ctr];
			addr++;
			ctr++;
			i--;
		}
		PROTECT_MEMORY
		/*
		 * Send back the start address as ack!
		 */
		send_packet(refnum,packet->payload,4,TYPE_WRITE_MEM_ACK);
	}
}

void execute(void * func())
{
	(*func)();
}

void start_program(UInt refnum, struct Packet * packet)
{
	/*
	 * Layout of packet:
	 * ULong: Start address
	 */
	if (packet->payload_length == 4) {
		ULong csa0,csa1,csa2,csa3;
		ULong addr = (((ULong)packet->payload[0]) << 24) |
		             (((ULong)packet->payload[1]) << 16) |
		             (((ULong)packet->payload[2]) <<  8) |
		             (((ULong)packet->payload[3]) <<  0);
		/*
		 * Send back the start address as ack!
		 */
		send_packet(refnum,packet->payload,4,TYPE_START_PROGRAM_ACK);
		UNPROTECT_MEMORY
		execute((void *)addr);
		PROTECT_MEMORY
	}
}

struct special_info
{
	ULong	start;
	ULong	end;
	ULong	ssp;
};

extern void end_marker();
extern void start();

void get_special_info(UInt refnum)
{
	struct special_info si;
	si.start = (ULong)start;
	si.end   = (ULong)end_marker;
	si.ssp   = get_ssp();	
	send_packet(refnum,(char *)&si,sizeof(struct special_info),TYPE_GET_SPECIAL_INFO);
}

int download_AROS(void)
{
	int rc = 0;
	Err err;
	UInt refNum;


	err = SysLibFind("Serial Library",&refNum);
	if (0 == err) {
		err = SerOpen(refNum, 0, BAUDRATE);
		if (0 == err) {
			VoidPtr serbuf = MemPtrNew(0x400);
			if (NULL != serbuf)
				SerSetReceiveBuffer(refNum, serbuf, 0x400);
			/*
			 * Serial Port is opened. So let's try a handshake
			 */
			if (0 == do_handshake(refNum)) {
				/*
				 * Let's read all packets and act upon.
				 */
				int end = FALSE;
				while (FALSE == end) {
					struct Packet * packet;
					_delay(100);
					packet = get_next_packet(refNum);
					if (NULL != packet) {
						switch (packet->payload_type) {
							case TYPE_READ_MEM:
								read_mem(refNum,packet);
							break;
							
							case TYPE_WRITE_MEM:
								write_mem(refNum,packet);
							break;
							
							case TYPE_GET_SPECIAL_INFO:
								get_special_info(refNum);
							break;
							
							case TYPE_START_PROGRAM:
								start_program(refNum, packet);
							break;
							
							case TYPE_QUIT:
								end = TRUE;
							break;
						}
						free_packet(packet);
					}
				}
			}

			if (NULL != serbuf) {
				SerSetReceiveBuffer(refNum, serbuf, 0);
				MemPtrFree(serbuf);
			}
			
			SerClose(refNum);
		}
		else
			return -1;
	} else
		rc = -1;		
	return rc;
}

static Boolean MainFormHandleEvent (EventPtr e)
{
    Boolean handled = false;
    FormPtr frm;
    int rc;
    CALLBACK_PROLOGUE

    switch (e->eType) {
    case frmOpenEvent:
	frm = FrmGetActiveForm();
	FrmDrawForm(frm);

	handled = true;
	break;

    case menuEvent:
	MenuEraseStatus(NULL);

	switch(e->data.menu.itemID) {
	}

    	handled = true;
	break;

    case ctlSelectEvent:
	switch(e->data.ctlSelect.controlID) {
		case Button1:
			rc = download_AROS();
		break;
	}
	break;

    default:
        break;
    }

    CALLBACK_EPILOGUE

    return handled;
}

static Boolean ApplicationHandleEvent(EventPtr e)
{
    FormPtr frm;
    Word    formId;
    Boolean handled = false;

    if (e->eType == frmLoadEvent) {
	formId = e->data.frmLoad.formID;
	frm = FrmInitForm(formId);
	FrmSetActiveForm(frm);

	switch(formId) {
	case MainForm:
	    FrmSetEventHandler(frm, MainFormHandleEvent);
	    break;
	}
	handled = true;
    }

    return handled;
}

/* Get preferences, open (or create) app database */
static Word StartApplication(void)
{
    FrmGotoForm(MainForm);
    return 0;
}

/* Save preferences, close forms, close app database */
static void StopApplication(void)
{
    FrmSaveAllForms();
    FrmCloseAllForms();
}

/* The main event loop */
static void EventLoop(void)
{
    Word err;
    EventType e;

    do {
	EvtGetEvent(&e, evtWaitForever);
	if (! SysHandleEvent (&e))
	    if (! MenuHandleEvent (NULL, &e, &err))
		if (! ApplicationHandleEvent (&e))
		    FrmDispatchEvent (&e);
    } while (e.eType != appStopEvent);
}



/* Main entry point; it is unlikely you will need to change this except to
   handle other launch command codes */
DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    Word err;


    if (cmd == sysAppLaunchCmdNormalLaunch) {

	err = StartApplication();
	if (err) return err;

	EventLoop();
	StopApplication();
	
    } else {
	return sysErrParamErr;
    }

    return 0;
}
