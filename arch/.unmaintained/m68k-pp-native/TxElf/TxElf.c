/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <devices/serial.h>

#include <aros/macros.h>

#include <stdio.h>
#include <string.h>

#define PARAM "FILE/A,UNIT=UNITNUM/K/N,START/S"
#define ARG_FILENAME    0
#define ARG_UNITNUM     1
#define ARG_START       2
#define TOTAL_ARGS      3

#define BAUDRATE        57600

int start_protocol(struct IOExtSer *, ULONG, char * );

struct Packet;

void free_packet(struct Packet * packet);
void free_all_packets(void);

struct List PacketList;
struct List PilotMemList;

int main(void)
{
	struct RDArgs  * rda;
	IPTR           * args[TOTAL_ARGS] = {NULL, NULL};
	int              unitnum = 0;
	int              baudrate = BAUDRATE;
	struct 	         IOExtSer * IORequest;	
	int              err;
	struct           MsgPort * SerPort;
	int              flags;
	struct Interrupt SerInterrupt;

	NEWLIST(&PacketList);
	NEWLIST(&PilotMemList);

	rda = ReadArgs(PARAM, (IPTR *)args, NULL);
	if (rda) {
		if (NULL != args[ARG_UNITNUM])
			unitnum = *args[ARG_UNITNUM];

printf("Opening serial port unit %d with %d baud.\n",
       unitnum,
       baudrate);

		SerPort = CreatePort("serial port message port",0);
		IORequest = (struct IOExtSer *)CreateExtIO(SerPort, sizeof(struct IOExtSer));
		
		err = OpenDevice("serial.device",unitnum,(struct IORequest *)IORequest,flags);

		if (0 == err) {
			/*
			 * Try to set the baudrate.
			 */
			IORequest->IOSer.io_Command = SDCMD_SETPARAMS;
			IORequest->io_Baud          = baudrate;
//			DoIO((struct IORequest *)IORequest);
			if (0 == ((struct IORequest *)IORequest)->io_Error) {
				/*
				 * serial device is opened.
				 * baud rate is set.
				 * so let's start.
				 */
				printf("All set!\n");
				err = start_protocol(IORequest, args[ARG_START], args[ARG_FILENAME]);
			} else {
				printf("Could not set baudrate!\n");
			}
			free_all_packets();
			CloseDevice((struct IORequest *)IORequest);
		} else {
			printf("Could not open serial device unit %d.\n",unitnum);
		}
		DeleteExtIO((struct IORequest *)IORequest);
		FreeArgs(rda);
	} else {
		printf("Forgot to give a mandator argument?\n");
	}
	
	return 0;
}

#define TYPE_HANDSHAKE	        0x01
#define TYPE_READ_MEM	        0x02
#define TYPE_WRITE_MEM	        0x03
#define TYPE_MEM_CONTENT        0x04
#define TYPE_WRITE_MEM_ACK	0x05
#define TYPE_GET_SPECIAL_INFO   0x06
#define TYPE_START_PROGRAM      0x07
#define TYPE_START_PROGRAM_ACK  0x08
#define TYPE_QUIT               0x10

struct Packet
{
	struct Node     next_packet;
	UWORD           payload_length;
	UBYTE           payload_type;
	BYTE *          payload;
	int             state;
	UWORD           pos;
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
		FreeMem(packet->payload,packet->payload_length);
	FreeMem(packet, sizeof(struct Packet));
}

struct Packet * current_packet;

void free_all_packets(void)
{
	struct Packet * p;
	if (NULL != current_packet) {
		free_packet(current_packet);
	}
	
	while (NULL != (p = (struct Packet *)RemHead(&PacketList)))
		free_packet(p);
}

struct Packet * get_next_packet(struct IOExtSer * IORequest)
{
	struct Packet * p = NULL;
	ULONG len;
	IORequest->IOSer.io_Command = SDCMD_QUERY;
	DoIO((struct IORequest *)IORequest);

	if (0 != (len = IORequest->IOSer.io_Actual)) {
		char * buf;
		int i;
//		printf("Reading %ld bytes from serial port.\n",
//		       IORequest->IOSer.io_Actual);

		buf = AllocMem(len, MEMF_ANY);
		if (NULL != buf) {

			IORequest->IOSer.io_Command = CMD_READ;
			IORequest->IOSer.io_Flags   = IOF_QUICK;
			IORequest->IOSer.io_Length  = len;
			IORequest->IOSer.io_Data    = buf;

			DoIO((struct IORequest *)IORequest);
		}
		
		i = 0;
		while (i < len)	{

			if (NULL == current_packet) {
				current_packet = AllocMem(sizeof(struct Packet),
				                          MEMF_CLEAR);
				current_packet->state = GET_FRAME_BEGINNING;
			}
		
			switch (current_packet->state) {
				case GET_FRAME_BEGINNING:
					if (0x7f == buf[i])
						current_packet->state = GET_PAYLOAD_LENGTH_HI;
					else
						printf("Could not find beginning of frame. Dropping byte (x%02x)!\n",
						       buf[i]);
					i++;
				break;
			
				case GET_PAYLOAD_LENGTH_HI:
					current_packet->payload_length = ((UWORD)buf[i]) << 8;
					current_packet->state = GET_PAYLOAD_LENGTH_LO;
					i++;
				break;
			
				case GET_PAYLOAD_LENGTH_LO:
					current_packet->payload_length |= ((UWORD)buf[i]);
					current_packet->state = GET_TYPE;
					current_packet->payload = AllocMem(current_packet->payload_length, MEMF_ANY);
					i++;
					//printf("Reading packet with length %d\n",current_packet->payload_length);
				break;
				
				case GET_TYPE:
					current_packet->payload_type = buf[i];
					current_packet->state = GET_PAYLOAD;
					i++;
				break;
				
				case GET_PAYLOAD:
					while (i < len && 
					       current_packet->pos < current_packet->payload_length) {
						if (0x7f == buf[i]) {
							/*
							 * Must not appear here!
							 */
							//printf("Received faulty packet (x%02x). Dropping it.\n",buf[i]);
							free_packet(current_packet);
							current_packet = NULL;
							break;				
						}
						current_packet->payload[current_packet->pos++] = buf[i];
						i++;
						//printf("%c\n",buf[i]);
					}
					
					if (current_packet->pos == current_packet->payload_length)
						current_packet->state = GET_FRAME_END;
				break;

				case GET_FRAME_END:
					if (0x7f == buf[i]) {
						AddTail(&PacketList, 
						        &current_packet->next_packet);
						//printf("Got a complete packet!\n");
					} else {
						//printf("Received faulty packet (x%02x). Dropping it.\n",buf[i]);
						free_packet(current_packet);
						current_packet = NULL;
					}
					i++;
					current_packet = NULL;
				break;
				
				default:
					printf("Error!\n");
					i++;
			}
		}
	}

	/*
	 * Try to get the first complete packet in the list.
	 */
	p = (struct Packet *)RemHead(&PacketList);
	if (NULL != p) {
		int i = 0;
		int c = 0;

//printf("Will return a packet!\n");
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
			 * Un-convert the payload
			 */
			char * pl = AllocMem(p->payload_length - c, MEMF_ANY);
printf("Unconverting payload!\n");
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
			FreeMem(p->payload, p->payload_length);
			p->payload = pl;
			p->payload_length -= c;
		}
	}
	return p;
}



void send_packet(struct IOExtSer * IORequest, char * buffer, UWORD len, UBYTE type)
{
	ULONG i = 0, c = 0;
	char * packet;
	while (i < len) {
		/*
		 * Count number of characters to escape.
		 */
		if (0x7f == buffer[i] || 0x27 == buffer[i])
			c++;
		i++;
	}
	
	packet = AllocMem(1 + /* frame begin */
	                  2 + /* length of payload indicator */
	                  1 + /* type */
	                  len + c + /* payload */
	                  1 /* frame end */, 
	                  MEMF_ANY);

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
printf("Converting packet to send!\n");		
			} else if (0x27 == buffer[i]) {
				packet[pos++] = 0x27;
				packet[pos] = 0x28; /* = 0x27 */
printf("Converting packet to send!\n");		
			} else {
				packet[pos] = buffer[i];
			}
			pos++;
			i++;
		}
		/*
		 * End marker for the packet.
		 */
		packet[pos] = 0x7f;
		
		/*
		 * Send this packet
		 */
		IORequest->IOSer.io_Command = CMD_WRITE;
		IORequest->IOSer.io_Flags   = 0;
		IORequest->IOSer.io_Length  = pos+1;
		IORequest->IOSer.io_Data    = packet;
		
		DoIO((struct IORequest *)IORequest);
		FreeMem(packet, pos+1);
	}
	                  
}

/*
 * Do a handshake. Other side sends "AROS?", I send back "AROS!" and get back
 * "greets!".
 */
int do_handshake(struct IOExtSer * IORequest)
{
	struct Packet * packet;
	int ctr = 0;
	BOOL found = FALSE;
	
	while (ctr < 10) {
		packet = get_next_packet(IORequest);
		if (NULL != packet) {
			if (0 == strncmp("AROS?",&packet->payload[0],5) &&
			    5 == packet->payload_length) {
			    	printf("Got 'AROS?' Sending 'AROS!'\n");
				free_packet(packet);
				send_packet(IORequest,"AROS!",5, TYPE_HANDSHAKE);
				found = TRUE;
				break;
			} else {
				printf("Wrong payload: %s\n",packet->payload);
			}
			free_packet(packet);
		}
		ctr++;
		Delay(50);
	}
	
	if (FALSE == found) {
		return -1;
	}

	found = FALSE;
	ctr = 0;
	
	while (ctr < 4) {
		packet = get_next_packet(IORequest);
		if (NULL != packet) {
			if (0 == strncmp("greets!",&packet->payload[0],7) &&
			    7 == packet->payload_length) {
				free_packet(packet);
				printf("\t\tHandshake complete!!\n");
				found = TRUE;
				break;
			} else {
				//printf("2. Wrong payload: %s\n",packet->payload);
			}

			free_packet(packet);
		} else {
			//printf("Could not get any packet at all!\n");
		}
		ctr++;
		Delay(50);
	}
	
	if (TRUE == found)
		return 0;
		
	return -1;
}

struct rm {
	ULONG addr;
	UWORD num_bytes;	
} /* __attribute__((packed)) */;

char * read_mem(struct IOExtSer * IORequest, ULONG addr, UWORD num_bytes)
{
	struct rm _rm;
	int ctr = 0;
	int send = TRUE;

	_rm.num_bytes = AROS_WORD2BE(num_bytes);
	_rm.addr      = AROS_LONG2BE(addr);

#if 0
	printf("Sending request for memory content!\n");
#endif

	while (ctr < 10) {
		struct Packet * packet;
		if (TRUE == send) {
			send_packet(IORequest, (char *)&_rm, 6, TYPE_READ_MEM);
			Delay(50);
		} else {
			send = FALSE;
			Delay(20);
		}
		packet  = get_next_packet(IORequest);
		if (NULL != packet) {
			if (TYPE_MEM_CONTENT == packet->payload_type &&
			    0 == memcmp(&_rm.addr,&packet->payload[0],sizeof(addr)) &&
			    packet->payload_length == sizeof(addr) + num_bytes) {
				char * buf;
				int i = 4;
#if 0
				while (i < packet->payload_length) {
					printf("x%02x ",(UBYTE)packet->payload[i]);
					i++;
					if (0 == (i % 16))
						printf("\n");
				}
				printf("\n");
#endif
				buf = AllocMem(packet->payload_length-4,MEMF_ANY);
				if (NULL != buf)
					memcpy(buf, &packet->payload[4], packet->payload_length-4);

				free_packet(packet);
				return buf;
			} else {
				//printf("%s: Wrong packet.\n",__FUNCTION__);
				send = FALSE;
				ctr--;
			}

			free_packet(packet);
		} else {
			//printf("Could not get any packet at all!\n");
			send = TRUE;
		}
		ctr++;
	}
	return NULL;
}

int write_mem(struct IOExtSer * IORequest, ULONG addr, char * buf, UWORD num_bytes)
{
	int ctr = 0;
	int send = TRUE;
	char * _buf = AllocMem(num_bytes + sizeof(addr), MEMF_ANY);
	ULONG _addr = AROS_BE2LONG(addr);
	if (NULL == _buf)
		return -1;

	memcpy(_buf, &_addr, sizeof(addr));
	memcpy(_buf+sizeof(addr), buf, num_bytes);
	while (ctr < 20) {
		struct Packet * packet;
		if (TRUE == send) {
			send_packet(IORequest, _buf, num_bytes+sizeof(addr), TYPE_WRITE_MEM);
			Delay(50);
		} else {
			send = FALSE;
			Delay(20);
		}
		packet  = get_next_packet(IORequest);
		if (NULL != packet) {
			if (TYPE_WRITE_MEM_ACK == packet->payload_type &&
			    0 == memcmp(&_addr,&packet->payload[0],sizeof(addr))) {
				printf("\tSuccessfully wrote %d bytes starting at address 0x%x\n",num_bytes,addr);
				free_packet(packet);
				FreeMem(_buf, num_bytes + sizeof(addr));
				return 0;
			} else {
				//printf("%s: Wrong packet.\n",__FUNCTION__);
				send = FALSE;
				ctr--;
			}

			free_packet(packet);
		} else {
			//printf("Could not get any packet at all!\n");
			send = TRUE;
		}
		ctr++;
	}
	FreeMem(_buf, num_bytes + sizeof(addr));
	return -1;
}

int send_chunk(struct IOExtSer * IORequest, ULONG addr, char * buf, UWORD num_bytes)
{
	printf("Sending chunk of size %d to address %x\n",num_bytes,addr);
	while (num_bytes > 0) {
		UWORD send = 100;
		if (num_bytes < send)
			send = num_bytes;
		if (write_mem(IORequest, addr, buf, send) < 0) {
			printf("send_chunk failed!\n");
			return -1;
		}
		addr += send;
		buf  += send;
		num_bytes -= send;
		printf("%d more bytes.\n",num_bytes);
	}
	return 0;
}

int start_program(struct IOExtSer * IORequest, ULONG addr)
{
	int ctr = 0;
	int send = TRUE;
	ULONG _addr = AROS_BE2LONG(addr);

	printf("Sending request to start program at address 0x%x!\n",addr);

	while (ctr < 10) {
		struct Packet * packet;
		if (TRUE == send) {
			send_packet(IORequest, (char *)&_addr, sizeof(_addr), TYPE_START_PROGRAM);
			Delay(50);
		} else {
			send = FALSE;
			Delay(20);
		}
		packet  = get_next_packet(IORequest);
		if (NULL != packet) {
			if (TYPE_START_PROGRAM_ACK == packet->payload_type) {
				printf("Sucessfully started program!\n");
				free_packet(packet);
				return 0;
			} else {
				//printf("%s: Wrong packet.\n",__FUNCTION__);
				send = FALSE;
				ctr--;
			}

			free_packet(packet);
		} else {
			//printf("Could not get any packet at all!\n");
			send = TRUE;
		}
		ctr++;
	}
	return -1;
}


struct special_info
{
	ULONG	start;
	ULONG	end;
	ULONG	ssp;
};

int get_special_info(struct IOExtSer * IORequest, struct special_info * si)
{
	int ctr = 0;
	int send = TRUE;

	printf("Sending request for special info!\n");

	while (ctr < 10) {
		struct Packet * packet;
		if (TRUE == send) {
			send_packet(IORequest, (char *)&ctr, 1, TYPE_GET_SPECIAL_INFO);
			Delay(50);
		} else {
			send = FALSE;
			Delay(20);
		}
		packet  = get_next_packet(IORequest);
		if (NULL != packet) {
			if (TYPE_GET_SPECIAL_INFO == packet->payload_type &&
			    packet->payload_length == sizeof(struct special_info)) {
				memcpy(si, packet->payload, packet->payload_length);
				free_packet(packet);
				printf("\t\tGot special info!\n");
				return 0;
			} else {
				//printf("%s: Wrong packet. (%d)\n",__FUNCTION__,packet->payload_type);
				//printf("%d-%d\n",packet->payload_length,sizeof(struct special_info));
				ctr--;
				send = FALSE;
			}

			free_packet(packet);
		} else {
			//printf("Could not get any packet at all!\n");
			send = TRUE;
		}
		ctr++;
	}
	return -1;
}

struct PilotMem
{
	struct Node	node;
	ULONG		start;
	ULONG		end;
};

struct registers
{
	UWORD	GRPBASEA; /* 0xfffff100 */
	UWORD	GRPBASEB; /* 0xfffff102 */
	UWORD	GRPBASEC; /* 0xfffff104 */
	UWORD	GRPBASED; /* 0xfffff106 */

	UWORD	GRPMASKA; /* 0xfffff108 */
	UWORD	GRPMASKB; /* 0xfffff10a */
	UWORD	GRPMASKC; /* 0xfffff10c */
	UWORD	GRPMASKD; /* 0xfffff10e */

	ULONG	CSA0; /* 0xfffff110 */
	ULONG	CSA1; /* 0xfffff114 */
	ULONG	CSA2; /* 0xfffff118 */
	ULONG	CSA3; /* 0xfffff11c */

	ULONG	empty1; /* 0xfffff120 */
	ULONG	empty2; /* 0xfffff124 */
	ULONG	empty3; /* 0xfffff128 */
	ULONG	empty4; /* 0xfffff12c */

	ULONG	CSC0; /* 0xfffff130 */
	ULONG	CSC1; /* 0xfffff134 */
	ULONG	CSC2; /* 0xfffff138 */
	ULONG	CSC3; /* 0xfffff13c */
	
	ULONG	LSSA;

	struct	special_info	SI;	
} __attribute__((packed)) Regs;

/*
 * This function builds the current palm memory layout into a linked
 * list of simple structures. Later on the relocator will look at this
 * list and try to grab a chunk.
 */
void BuildPilotMem(struct registers * reg)
{
	struct PilotMem *pm;
	
	pm = AllocMem(sizeof(struct PilotMem),MEMF_CLEAR);
#warning Should really build a much better list here!!!
	pm->start = (AROS_LONG2BE(reg->SI.end) & 0xffff0000) + 0x4000;
	pm->end   = pm->start+0x100000;
	AddTail(&PilotMemList, &pm->node);
}

void * GetPilotMem(ULONG size)
{
	struct PilotMem * pm;

	/* For alignment purposes add 4 to the size */
	size += 4;
	printf("%s: Trying to allocate %ld bytes on palm!\n",__FUNCTION__,size);
	ForeachNode(&PilotMemList, pm) {
		if (pm->end - pm->start + 1 >= size) {
			void * ret;
			if (0 == (pm->start & 0x03))
				ret = pm->start;
			else
			 	ret = (void *)((pm->start & 0xfffffffc) + 4);
			printf("Found a memory chunk at 0x%x\n",ret);
			pm->start += size;
			return ret;
		}
		pm = (struct PilotMem *)pm->node.ln_Succ;
	}
	return NULL;
}

#include "myinternalloadseg_elf.c"

int upload_program(struct IOExtSer * IORequest, char * name, ULONG * startaddr)
{
	BPTR segs;
	BPTR file = Open(name, MODE_OLDFILE);
	
	if (file)
	{
		printf("Opened file!\n");
		segs = MyInternalLoadSeg_ELF(file, NULL, NULL, IORequest, startaddr);
		
		Close(file);
	} else {
		printf("Could not open file %s\n",name);
	}
	
	return 0;
}


int start_protocol(struct IOExtSer * IORequest, ULONG start, char * filename)
{
	int err;
	char * buf;
	ULONG startaddr;

	err = do_handshake(IORequest);
	if (0 != err) {
		printf("Handshake failed!\n");
		return err;
	}
	
	buf = read_mem(IORequest,0xfffff100,0x40);
	if (buf) {
		memcpy(&Regs, buf, 0x40);
		FreeMem(buf,0x40);
	}

	buf = read_mem(IORequest,0xfffffA00,0x4);
	if (buf) {
		memcpy(&Regs.LSSA, buf, 0x4);
		FreeMem(buf,0x4);
	}

	err = get_special_info(IORequest,&Regs.SI);

	printf("start: %lx\n",AROS_LONG2BE(Regs.SI.start));
	printf("end:   %lx\n",AROS_LONG2BE(Regs.SI.end));
	printf("ssp:   %lx\n",AROS_LONG2BE(Regs.SI.ssp));

	printf("GRPBASEA    : %x\n",AROS_WORD2BE(Regs.GRPBASEA));
	printf("GRPBASEB    : %x\n",AROS_WORD2BE(Regs.GRPBASEB));
	printf("GRPBASEC    : %x\n",AROS_WORD2BE(Regs.GRPBASEC));
	printf("GRPBASED    : %x\n",AROS_WORD2BE(Regs.GRPBASED));

	printf("GRPMASKA    : %x\n",AROS_WORD2BE(Regs.GRPMASKA));
	printf("GRPMASKB    : %x\n",AROS_WORD2BE(Regs.GRPMASKB));
	printf("GRPMASKC    : %x\n",AROS_WORD2BE(Regs.GRPMASKC));
	printf("GRPMASKD    : %x\n",AROS_WORD2BE(Regs.GRPMASKD));

	printf("CSA0        : %lx\n",AROS_LONG2BE(Regs.CSA0));
	printf("CSA1        : %lx\n",AROS_LONG2BE(Regs.CSA1));
	printf("CSA2        : %lx\n",AROS_LONG2BE(Regs.CSA2));
	printf("CSA3        : %lx\n",AROS_LONG2BE(Regs.CSA3));

	printf("CSC0        : %lx\n",AROS_LONG2BE(Regs.CSC0));
	printf("CSC1        : %lx\n",AROS_LONG2BE(Regs.CSC1));
	printf("CSC2        : %lx\n",AROS_LONG2BE(Regs.CSC2));
	printf("CSC3        : %lx\n",AROS_LONG2BE(Regs.CSC3));

#if 0
	Not doing this anymore. Will do this on the palm.
	/*
	 * Disable the read only bit in the CSA1 and CSA3 register.
	 */
	Regs.CSA1 &= AROS_LONG2BE(0xfffffff7);
	printf("Changing CSA1 register to new value %x!\n",AROS_LONG2BE(Regs.CSA1));
	err = write_mem(IORequest, 0xfffff110, (char *)&Regs.CSA1, sizeof(Regs.CSA1));
	if (0 == err) {
		printf("Successfully changed register CSA1!\n");
	}

	Regs.CSA3 &= AROS_LONG2BE(0xfffffff7);
	printf("Changing CSA3 register to new value %x!\n",AROS_LONG2BE(Regs.CSA3));
	err = write_mem(IORequest, 0xfffff110, (char *)&Regs.CSA3, sizeof(Regs.CSA3));
	if (0 == err) {
		printf("Successfully changed register CSA3!\n");
	}
#endif
	BuildPilotMem(&Regs);
	printf("Trying to upload program now!\n");
	if (0 != upload_program(IORequest,filename, &startaddr)) {
		printf("upload_program returned error!\n");
	}

	if (TRUE == start)
		start_program(IORequest, startaddr);

	send_packet(IORequest, (char *)&err, 1, TYPE_QUIT);

	return err;
}
