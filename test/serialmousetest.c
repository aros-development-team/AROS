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

#define ARG_TEMPLATE "KILL/S,UNIT/N"

enum 
{
	ARG_KILL = 0,
	ARG_UNIT,
	NOOFARGS
};

static char mousebuffer[3];
static int bufptr;

static void mouse_buffer(char * buffer, ULONG len)
{
	ULONG i = 0;
	while (len > 0) {
		switch (bufptr) {
			case 0:
				if (buffer[i] & 0x40) {
					mousebuffer[0] = buffer[i];
					bufptr++;
				}
			break;
		
			case 1:
			case 2:
				mousebuffer[bufptr] = buffer[1];
				if (2 == bufptr) {
					BYTE dx,dy;
					bufptr = 0;
					
					if ((mousebuffer[0] & 0x20))
						printf("Left mouse button.\n");
					if ((mousebuffer[1] & 0x10))
						printf("Right mouse button.\n");
					dx = (mousebuffer[0] & 0x03) << 6 | (mousebuffer[1] & 0x3f);
					dy = (mousebuffer[0] & 0x0c) << 4 | (mousebuffer[2] & 0x3f);
					if (dx || dy)
						printf("Movement: dx %d, dy %d\n",dx,dy);
				} else
					bufptr++;
			break;
		
			default:
				bufptr = 0;
		}
		i++;
		len--;
	}
}

static void read_input(struct IOExtSer * IORequest, struct MsgPort * notifport)
{
	int n = 3;
	while (1) {
		char buf[10];
		struct Message * msg;
		BOOL IODone = FALSE;
		memset(buf, 0x00, 10);
		IORequest->IOSer.io_Command = CMD_READ;
		IORequest->IOSer.io_Flags   = IOF_QUICK;
		IORequest->IOSer.io_Length  = n;
		IORequest->IOSer.io_Data    = buf;
		SendIO((struct IORequest *)IORequest);
		Wait((1 << ((struct IORequest *)IORequest)->io_Message.mn_ReplyPort->mp_SigBit) |
		     (1 << notifport->mp_SigBit));
		if (NULL != CheckIO((struct IORequest *)IORequest)) {
			mouse_buffer(buf, n);
			IODone = TRUE;
		}
		
		if (NULL != (msg = GetMsg(notifport))) {
			printf("Serial mouse driver ends.\n");
			if (FALSE == IODone)
				AbortIO((struct IORequest *)IORequest);
			FreeMem(msg, sizeof(struct Message));
			break;
		}
	}
} /* read_input */

static void mouse_driver(IPTR * unit, struct MsgPort * notifport)
{
	struct MsgPort * SerPort;
        ULONG unitnum = 0;
        
        if (NULL != unit)
        	unitnum = *unit;

printf("Unit=%ld\n",unitnum);    
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
					read_input(IORequest, notifport);
				} else {
					printf("Could not set parameters for serial port.\n");
					printf("Error code: %d\n",((struct IORequest *)IORequest)->io_Error);
				}
				
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
	                       0             // ARG_UNIT
	                     };
	struct RDArgs *rda;

	rda = ReadArgs(ARG_TEMPLATE, args, NULL);
	
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
					mouse_driver((IPTR *)args[ARG_UNIT],notifport);
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
