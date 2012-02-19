#define  DEBUG  1
#include <aros/debug.h>

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <utility/utility.h>
#include <devices/serial.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/commodities.h>

#include <devices/serial.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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


struct mouse_action {
	BYTE	dx;
	BYTE	dy;
	ULONG	flags;
};

struct protocol {
	const char * signature;
	ULONG signature_length;
	ULONG packet_length;
	void (*handler)(char *, ULONG, struct mouse_action *);
	const char * name;
};


/*
 * The following flags are defined:
 */
#define MOUSE_LEFT_BUTTON	0x01
#define MOUSE_RIGHT_BUTTON	0x02
#define MOUSE_MIDDLE_BUTTON	0x04

#define MOUSE_DATA_VALID	0x8000

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

static void ms_mouse_protocol(char *, ULONG, struct mouse_action * );


/*
 * All known protocols and their handlers
 */
const struct protocol protocols[] = {
	{ms_mouse, sizeof(ms_mouse), 3, ms_mouse_protocol, "ms-mouse"},
	{NULL    , 0               , 0, NULL             , NULL}
};


static struct NewBroker nb =
{
	NB_VERSION,
	NULL,
	NULL,
	NULL,
	NBU_NOTIFY | NBU_UNIQUE, 
	0,
	0,
	NULL,                             
	0 
};

static CxObj * cxbroker;




static void ms_mouse_protocol(char * buffer, 
                              ULONG len,
                              struct mouse_action * ma)
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
					ma->flags = MOUSE_DATA_VALID;
					if ((mousebuffer[2] & 0x20))
						ma->flags |= MOUSE_LEFT_BUTTON;
					if ((mousebuffer[2] & 0x10))
						ma->flags |= MOUSE_RIGHT_BUTTON;
					ma->dy = (mousebuffer[1] & 0x20) 
					           ? (mousebuffer[1]-0x40)
					           : (mousebuffer[1]);
					ma->dx = (mousebuffer[0] & 0x20) 
					           ? (mousebuffer[0]-0x40)
					           : (mousebuffer[0]);
					
				}
			break;
		
			default:
				bufptr = 0;
		}
		i++;
		len--;
	}
}

static void check_mouse_action(struct mouse_action * old_action,
                               struct mouse_action * cur_action)
{
	if (cur_action->flags & MOUSE_DATA_VALID &&
	    old_action->flags & MOUSE_DATA_VALID) {
		/*
		 * Check buttons
		 */
		if (old_action->flags & MOUSE_LEFT_BUTTON) {
			if (0 == (cur_action->flags & MOUSE_LEFT_BUTTON)) {
		    		printf("Left mouse button released!\n");
			}
		} else
		if (0 == (old_action->flags & MOUSE_LEFT_BUTTON)) {
			if (cur_action->flags & MOUSE_LEFT_BUTTON) {
				printf("Left mouse button pressed!\n");
			}
		}

		if (old_action->flags & MOUSE_RIGHT_BUTTON) {
			if (0 == (cur_action->flags & MOUSE_RIGHT_BUTTON)) {
		    		printf("Right mouse button released!\n");
			}
		} else
		if (0 == (old_action->flags & MOUSE_RIGHT_BUTTON)) {
			if (cur_action->flags & MOUSE_RIGHT_BUTTON) {
				printf("Right mouse button pressed!\n");
			}
		}

		if (old_action->flags & MOUSE_MIDDLE_BUTTON) {
			if (0 == (cur_action->flags & MOUSE_MIDDLE_BUTTON)) {
		    		printf("Middle mouse button released!\n");
			}
		} else
		if (0 == (old_action->flags & MOUSE_MIDDLE_BUTTON)) {
			if (cur_action->flags & MOUSE_MIDDLE_BUTTON) {
				printf("Middle mouse button pressed!\n");
			}
		}

		if (cur_action->dx) {
			printf("Mouse movement left/right: %d\n",cur_action->dx);
		}
		if (cur_action->dy) {
			printf("Mouse movement up/down: %d\n",cur_action->dy);
		}
	}
}

static void read_input(struct IOExtSer * IORequest, 
                       struct MsgPort * notifport,
                       struct MsgPort * cxport,
                       void (*handler)(char *, ULONG, struct mouse_action *))
{
	struct mouse_action old_action, cur_action;
	BOOL end = FALSE;
	old_action.flags = 0;
	cur_action.flags = 0;
	int n = 3;
	while (FALSE == end) {
		BYTE buf[10];
		struct Message * msg;
		BOOL IODone = FALSE;
		ULONG sigs;
		ULONG cxsig = (NULL != cxport) ? (1 << cxport->mp_SigBit)
		                               : 0;
		
		memset(buf, 0x00, 10);
		IORequest->IOSer.io_Command = CMD_READ;
		IORequest->IOSer.io_Flags   = IOF_QUICK;
		IORequest->IOSer.io_Length  = n;
		IORequest->IOSer.io_Data    = buf;
		SendIO((struct IORequest *)IORequest);
		sigs = Wait((1 << ((struct IORequest *)IORequest)->io_Message.mn_ReplyPort->mp_SigBit) |
		            (1 << notifport->mp_SigBit) |
		            cxsig |
		            SIGBREAKF_CTRL_C );
		if (NULL != CheckIO((struct IORequest *)IORequest)) {
			
			if (NULL == handler) {
				printf("No handler given. Calling def. handler!\n");
				ms_mouse_protocol(buf, n, &cur_action);
			} else {
				handler(buf, n, &cur_action);
			}
			
			check_mouse_action(&old_action, &cur_action);

			if (cur_action.flags & MOUSE_DATA_VALID)
				old_action = cur_action;
			
			IODone = TRUE;
		}
		
		if (sigs & cxsig) {
			CxMsg * cxmsg;
			printf("Got a signal for me as commodity.\n");
			while (NULL != (cxmsg = (CxMsg *)GetMsg(cxport))) {
				switch (CxMsgType(cxmsg)) {
					case CXM_COMMAND:
						switch (CxMsgID(cxmsg)) {
							case CXCMD_DISABLE:
								ActivateCxObj(cxbroker, FALSE);
							break;
							
							case CXCMD_ENABLE:
								ActivateCxObj(cxbroker, TRUE);
							break;
							
							case CXCMD_KILL:
								end = TRUE;
							break;
						}
					break;
				}
				ReplyMsg((struct Message *)cxmsg);
			}
		}
		
		if (NULL != (msg = GetMsg(notifport))) {
			printf("Serial mouse driver ends.\n");
			if (FALSE == IODone)
				AbortIO((struct IORequest *)IORequest);
			FreeMem(msg, sizeof(struct Message));
			end = TRUE;
		}
		
		if (sigs & SIGBREAKF_CTRL_C) {
			end = TRUE;
		}
	} /* while (FALSE == end) */
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
				      (long)protocols[i].signature_length);
				
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

static void mouse_driver(ULONG unit, BOOL probe_proto,struct MsgPort * notifport,struct MsgPort *cxport)
{
	struct MsgPort * SerPort;
        ULONG unitnum = unit;
        
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
					void (*handler) (char*,ULONG,struct mouse_action *) = NULL;
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
					read_input(IORequest, notifport, cxport, handler);
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


static BOOL InitCommodity(void)
{
	BOOL rc = FALSE;
	
	if (NULL != CxBase) {
	
		nb.nb_Name  = strdup("Mouse Driver");
		nb.nb_Title = strdup("Mouse Driver");
		nb.nb_Descr = strdup("Mouse Driver for serial mice.");
	
		if (NULL != (nb.nb_Port = CreateMsgPort())) {
			if (NULL != (cxbroker = CxBroker(&nb, 0))) {
				ActivateCxObj(cxbroker, TRUE);
				rc = TRUE;
			} else {
				DeleteMsgPort(nb.nb_Port);
				nb.nb_Port = NULL;
			}
		}
	}
	return rc;
}


static void CleanupCommodity(void)
{
	if (NULL != CxBase) {
		if (NULL != cxbroker)
			DeleteCxObjAll(cxbroker);
		if (NULL != nb.nb_Port) {
			struct Message * msg;
			while (NULL != (msg = GetMsg(nb.nb_Port))) {
				ReplyMsg(msg);
			}
			DeleteMsgPort(nb.nb_Port);
			nb.nb_Port = NULL;
		}
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
				BOOL have_cx = InitCommodity();
				struct MsgPort * notifport = CreatePort(MSGPORT_NAME, 0);
				if (NULL != notifport) {
					mouse_driver(args[ARG_UNIT],
					             args[ARG_PROBE],
					             notifport,
					             nb.nb_Port);
					DeletePort(notifport);
					if (TRUE == have_cx) {
						CleanupCommodity();
					}
				} else {
					printf("Could not create notification port!\n");
				}
			}
		}
		FreeArgs(rda);
	}	
	return 0;
}
