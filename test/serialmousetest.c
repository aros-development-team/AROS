#define  DEBUG  1
#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <utility/utility.h>
#include <devices/serial.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <devices/serial.h>

#include <stdio.h>

#define ARG_TEMPLATE "KILL/S,UNIT/N,PROBE/S"

enum 
{
	ARG_KILL = 0,
	ARG_UNIT,
	ARG_PROBE,
	NOOFARGS
};

static UBYTE mousebuffer[3];
static ULONG bufptr;


struct protocol {
	const char * signature;
	ULONG signature_length;
	ULONG packet_length;
	void (* handler)(char *, ULONG);
	const char * name;
};

const char ms_mouse[] = 
{
	0x4d,0x40,0x00,
	0x00,0x08,0x01,
	0x24,0x30,0x2e,
	0x30,0x10,0x26,
	0x10,0x21,0x3c,
	0x10,0x10,0x10,
	0x15,0x10,0x12,
	0x10,0x10,0x3c,
	0x2d,0x2f,0x35,
	0x33,0x25,0x3c,
	0x30,0x2e,0x30,
	0x10,0x26,0x10,
	0x21,0x3c,0x2d,
	0x29,0x23,0x32,
	0x2f,0x33,0x2f,
	0x26,0x34,0x00,
	0x33,0x25,0x32,
	0x29,0x21,0x2c,
	0x00,0x2d,0x2f,
	0x35,0x33,0x25,
	0x00,0x12,0x0e,
	0x11,0x21,0x15,
	0x11,0x09
};

static void ms_mouse_protocol(char * buffer, ULONG len);


/*
 * All known protocols and their handlers
 */
const struct protocol protocols[] = {
	{ms_mouse, sizeof(ms_mouse), 3, ms_mouse_protocol, "ms-mouse"},
	{NULL    , 0               , 0, NULL             , NULL}
};

static void ms_mouse_protocol(char * buffer, ULONG len)
{
	ULONG i = 0;

#if 1
	ULONG j = 0;
	while (j < len) {
		printf("0x%02x,",buffer[j++]);
	}
	printf("\n");
#endif
	
	while (len > 0) {
		BYTE dx,dy;
		switch (bufptr) {
			case 0:
			case 1:
				if (0 == (buffer[i] & 0x40)) {
					mousebuffer[bufptr++] = buffer[i];
				}
			break;
		
			case 2:
				mousebuffer[bufptr] = buffer[i];

				bufptr = 0;
					
				if ((mousebuffer[2] & 0x40)) {
				
					if ((mousebuffer[2] & 0x20))
						printf("Left button down.\n");
					if ((mousebuffer[2] & 0x10))
						printf("Right button down.\n");
					dy = (mousebuffer[1] & 0x20) 
					           ? (mousebuffer[1]-0x40)
					           : (mousebuffer[1]);
					dx = (mousebuffer[0] & 0x20) 
					           ? (mousebuffer[0]-0x40)
					           : (mousebuffer[0]);
					
					if (dx || dy)
						printf("Movement: dx %d, dy %d\n",dx,dy);
				}
			break;
		
			default:
				bufptr = 0;
		}
		i++;
		len--;
	}
}

static void read_input(struct IOExtSer * IORequest, 
                       struct MsgPort * notifport,
                       void (* handler)(char *, ULONG))
{
	int n = 3;
	while (1) {
		BYTE buf[10];
		struct Message * msg;
		BOOL IODone = FALSE;
		ULONG sigs;
		
		memset(buf, 0x00, 10);
		IORequest->IOSer.io_Command = CMD_READ;
		IORequest->IOSer.io_Flags   = IOF_QUICK;
		IORequest->IOSer.io_Length  = n;
		IORequest->IOSer.io_Data    = buf;
		SendIO((struct IORequest *)IORequest);
		sigs = Wait((1 << ((struct IORequest *)IORequest)->io_Message.mn_ReplyPort->mp_SigBit) |
		            (1 << notifport->mp_SigBit) |
		            SIGBREAKF_CTRL_C );
		if (NULL != CheckIO((struct IORequest *)IORequest)) {
			if (NULL == handler) {
				printf("No handler given. Calling def. handler!\n");
				ms_mouse_protocol(buf, n);
			} else {
				handler(buf, n);
			}
			IODone = TRUE;
		}
		
		if (NULL != (msg = GetMsg(notifport))) {
			printf("Serial mouse driver ends.\n");
			if (FALSE == IODone)
				AbortIO((struct IORequest *)IORequest);
			FreeMem(msg, sizeof(struct Message));
			break;
		}
		
		if (sigs & SIGBREAKF_CTRL_C) {
			break;
		}
	}
} /* read_input */


static const struct protocol * probe_protocol(struct IOExtSer * IORequest, struct MsgPort * notifport)
{
	const struct protocol * p = NULL;
	ULONG n;
	Delay(50);
	printf("Supposed to probe for protocol!\n");
	IORequest->IOSer.io_Command = SDCMD_QUERY;
	DoIO((struct IORequest *)IORequest);
	printf("Number of bytes in buffer: %d\n",(int)IORequest->IOSer.io_Actual);
	if (0 != (n = IORequest->IOSer.io_Actual)) {
		UBYTE * buffer = AllocMem(n, MEMF_CLEAR);
		if (NULL != buffer) {
			ULONG i = 0;
			IORequest->IOSer.io_Command = CMD_READ;
			IORequest->IOSer.io_Flags   = IOF_QUICK;
			IORequest->IOSer.io_Length  = n;
			IORequest->IOSer.io_Data    = buffer;
			DoIO((struct IORequest *)IORequest);
			
			while (protocols[i].signature) {
				printf("Possible: %s, sign_length=%ld\n",
				      protocols[i].name,
				      protocols[i].signature_length);
				
				if (n >= protocols[i].signature_length) {
					ULONG d = n - protocols[i].signature_length;
					ULONG k  = 0;
					while (k <= d) {
						if (0 == memcmp(&buffer[k], 
						                protocols[i].signature,
						                protocols[i].signature_length)) {
							printf("Found signature for %s.\n",protocols[i].name);
							p = &protocols[i];
							break;
						}
						k++;
					}
				}
				i++;
			}
			
			FreeMem(buffer, n);
		}
	}
	return p;
}

static void mouse_driver(ULONG unit, BOOL probe_proto,struct MsgPort * notifport)
{
	struct MsgPort * SerPort;
        ULONG unitnum = unit;
                	
        printf("unit %ld\n",unitnum);
	SerPort = CreatePort(NULL,0);
	if (NULL != SerPort)  {
		struct IOExtSer * IORequest;
		IORequest = (struct IOExtSer *)CreateExtIO(SerPort, sizeof(struct IOExtSer));
		if (NULL != IORequest) {
			BYTE err = OpenDevice("serial.device", unitnum, (struct IORequest *)IORequest, 0);
			if (0 == err) {
				/*
				 * Set parameters to read from mouse.
				 */
				IORequest->IOSer.io_Command = SDCMD_SETPARAMS;
				IORequest->io_Baud          = 1200;
				IORequest->io_ReadLen       = 7;
				IORequest->io_WriteLen      = 7;
				IORequest->io_StopBits      = 1;
				IORequest->io_RBufLen       = 512;
				IORequest->io_ExtFlags      = 0;
				IORequest->IOSer.io_Flags   = 0;
				DoIO((struct IORequest *)IORequest);
				
				
				if (0 == ((struct IORequest *)IORequest)->io_Error) {
					void (*handler) (char*,ULONG) = NULL;
					if (TRUE == probe_proto) {
						const struct protocol * p;
						p = probe_protocol(IORequest, notifport);
						if (p) {
							handler = p->handler;
						} else {
							printf("Could not detect mouse protocol!\n");
							goto probe_fail;
						}
					}
					read_input(IORequest, notifport, handler);
				} else {
					printf("Could not set parameters for serial port.\n");
					printf("Error code: %d\n",((struct IORequest *)IORequest)->io_Error);
				}
probe_fail:				
				CloseDevice((struct IORequest *)IORequest);
			}
			DeleteExtIO((struct IORequest *)IORequest);
		}
		DeletePort(SerPort);
	}

}

#define MSGPORT_NAME "serial_mouse_driver"

int main(int argc, char **argv)
{
	IPTR args[NOOFARGS] = {FALSE,  // ARG_KILL
	                       0,      // ARG_UNIT
	                       FALSE   // ARG_PROBE
	                     };
	struct RDArgs *rda;
printf("!!\n");
	rda = ReadArgs(ARG_TEMPLATE, args, NULL);
printf("??\n");	
	if (NULL != rda) {
		if (TRUE == args[ARG_KILL]) {
			struct MsgPort * mport = FindPort(MSGPORT_NAME);
			if (NULL == mport) {
				printf("Program seems not to be running. Cannot kill it.\n");
				
			} else {
				struct Message * msg = AllocMem(sizeof(struct Message), MEMF_CLEAR);
				if (NULL != msg) {
					/*
					 * Just send a message to the port.
					 * The content does not matter so far.
					 */
					PutMsg(mport, msg);
				}
			}
		} else {
			struct MsgPort * mport = FindPort(MSGPORT_NAME);
			if (NULL != mport) {
				printf("Program already running!\n");
			} else {
				struct MsgPort * notifport = CreatePort(MSGPORT_NAME, 0);
				if (NULL != notifport) {
					mouse_driver(args[ARG_UNIT],
					             args[ARG_PROBE],
					             notifport);
					DeletePort(notifport);
				} else {
					printf("Could not create notification port!\n");
				}
			}
		}
		FreeArgs(rda);
	}	
	return 0;
}
