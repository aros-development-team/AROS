#include <asm/io.h>
#include <asm/irq.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <proto/exec.h>
#define DEBUG 1
#include <aros/debug.h>
#include "hardware.h"
#include "bitmap.h"
#include "usb.h"

void executeSchedule(struct UHCIData *data, UWORD frameindex) {

	outw(frameindex, data->baseport+PORT_FRAME_NUMBER);
	outw(inw(data->baseport+PORT_COMMAND) | CMD_RUNSTOP, data->baseport+PORT_COMMAND);
}

void waitms(struct PortUnit *port, ULONG ms) {
	port->treq->tr_node.io_Command = TR_ADDREQUEST;
	port->treq->tr_time.tv_secs = 0;
	port->treq->tr_time.tv_micro = ms;
	DoIO(&port->treq->tr_node);
}

#define DTCL_DONE      (1)
#define DTCL_NOT_DONE  (0)
#define DTCL_ERROR    (-1)

LONG doneTD(struct PortUnit *port, ULONG *td) {

	if (!(td[1] & TD_STATUS_ACTIVE))
		return DTCL_DONE;
	if (td[1] & TD_STATUS_STALLED)
	{
		kprintf("UHCI: ftd - stalled\n");
		return DTCL_ERROR;
	}
	if (td[1] & TD_STATUS_DATA_BUFFER_ERROR)
	{
		kprintf("UHCI: ftd - data buffer error\n");
		return DTCL_ERROR;
	}
	if (td[1] & TD_STATUS_BABBLE_DETECTED)
	{
		kprintf("UHCI: ftd - babble detect\n");
		return DTCL_ERROR;
	}
	if (td[1] & TD_STATUS_NAK_RECEIVED)
	{
		kprintf("UHCI: ftd - NAK receive\n");
		return DTCL_ERROR;
	}
	if (td[1] & TD_STATUS_CRC_TIMEOUT_ERROR)
	{
		kprintf("UHCI: ftd - CRC/TIMEOUT error\n");
		return DTCL_ERROR;
	}
	if (td[1] & TD_STATUS_BITSTUFF_ERROR)
	{
		kprintf("UHCI: ftd - bitstuff error\n");
		return DTCL_ERROR;
	}
	return DTCL_NOT_DONE;
}

LONG doneTCList(struct PortUnit *port, struct List *list) {
struct TransferContext *tc;
UWORD status;
LONG ret;

	tc = (struct TransferContext *)list->lh_Head;
	while (tc->ln.ln_Succ != NULL)
	{
		if (tc->ln.ln_Type == TCT_QH)
		{
			if (!doneTCList(port, &tc->qlist))
				return DTCL_NOT_DONE;
		}
		else if (tc->ln.ln_Type == TCT_TD)
		{
			status = inw(port->uhci->baseport+PORT_STATUS);
			/* clear all set bits */
			outw(status, port->uhci->baseport+PORT_STATUS);
			if (status & STATUS_HC_PROCESS_ERROR)
			{
				kprintf("UHCI: wt - HC process error\n");
				return DTCL_ERROR;
			}
			if (status & STATUS_HS_ERROR)
			{
				kprintf("UHCI: wt - host system error\n");
				return DTCL_ERROR;
			}
			if (status & STATUS_ERROR_INTERRUPT)
			{
				kprintf("UHCI: wt - error interrupt\n");
				return DTCL_ERROR;
			}
			if (status & STATUS_USB_INTERRUPT)
			{
				/* either IOC or SPD */
//				kprintf("UHCI: wt - got USB Interrupt\n");
			}
			ret = doneTD(port, tc->td);
			if (ret != DTCL_DONE)
				return ret;
		}
		tc = (struct TransferContext *)tc->ln.ln_Succ;
	}
	return DTCL_DONE;
}

BOOL waitTransfer(struct PortUnit *port, struct TransferData *data) {
LONG ret;

	while	((ret = doneTCList(port, &data->list)) == DTCL_NOT_DONE)
		waitms(port, 500);
	if (ret == DTCL_ERROR)
		return FALSE;
	return TRUE;
}

struct TransferContext *allocTC(LONG type) {
struct TransferContext *tc;

	tc = AllocMem(sizeof(struct TransferContext), MEMF_PUBLIC | MEMF_CLEAR);
	if (tc != NULL)
	{
		/* 16 bytes alignment */
		tc->td = (ULONG *)(((ULONG)tc->rtd+15) & ~15);
		tc->ln.ln_Type = type;
		NEWLIST(&tc->qlist);
		return tc;
	}
	return NULL;
}

void addTailTC(struct List *list, struct TransferContext *tc) {
struct TransferContext *last;

	last = (struct TransferContext *)list->lh_TailPred;
	ADDTAIL(list, &tc->ln);
	if (last->ln.ln_Pred)
	{
		switch (tc->ln.ln_Type)
		{
		case TCT_TD:
			last->td[0] = (ULONG)tc->td | TD_LP_TYPE_TD;
			break;
		case TCT_QH:
			last->td[0] = (ULONG)tc->td | TD_LP_TYPE_QH;
			tc->td[1] = TD_LP_TERMINATE;
			break;
		}
	}
	tc->td[0] = TD_LP_TERMINATE;
}

void addTailQH(struct TransferContext *qh, struct TransferContext *tc) {

	if (qh->qlist.lh_Head->ln_Succ == NULL)
	{
		/*
			list is empty so we have to store pointer of tc->td in the
			queued head directly
		*/
		ADDTAIL(&qh->qlist, &tc->ln);
		switch (tc->ln.ln_Type)
		{
		case TCT_TD:
			qh->td[1] = (ULONG)tc->td | TD_LP_TYPE_TD;
			break;
		case TCT_QH:
			qh->td[1] = (ULONG)tc->td | TD_LP_TYPE_QH;
			tc->td[1] = TD_LP_TERMINATE;
			break;
		}
		tc->td[0] = TD_LP_TERMINATE;
	}
	else
		addTailTC(&qh->qlist, tc);
}

void freeTransferList(struct List *list) {
struct TransferContext *tc;
struct TransferContext *next;

	tc = (struct TransferContext *)list->lh_Head;
	while (tc->ln.ln_Succ != NULL)
	{
		next = (struct TransferContext *)tc->ln.ln_Succ;
		REMOVE(&tc->ln);
		if (tc->ln.ln_Type == TCT_QH)
			freeTransferList(&tc->qlist);
		FreeMem(tc, sizeof(struct TransferContext));
		tc = next;
	}
}

/*******************************************************************/
void printData(ULONG *data, LONG size) {
int i;
int j;

	size += 1;
	if (size == 0x800)
		kprintf("\tno data to print\n");
	else
	{
		while (size)
		{
			kprintf("\t");
			for (i=0;(i<4) && (size);i++)
			{
				kprintf(" ");
				for (j=0;(j<4) && (size);j++)
				{
					kprintf("%02x", *((UBYTE *)data));
					data = (ULONG *)((UBYTE *)data+1);
					size--;
				}
			}
			kprintf("\n");
		}
	}
}

void printTD(ULONG lp) {
ULONG type;
ULONG *td;

	if (!(lp & TD_LP_TERMINATE))
	{
		type = lp & TD_LP_TYPE;
		td = (ULONG *)(lp & ~0xF);
		if (type == TD_LP_TYPE_TD)
			kprintf("TransferDescriptor\n");
		else if (type == TD_LP_TYPE_QH)
			kprintf("QueuedHead\n");
		kprintf("TD0=%08lx\n", td[0]);
		kprintf("TD1=%08lx\n", td[1]);
		if (type == TD_LP_TYPE_TD)
		{
			kprintf("TD2=%08lx\n", td[2]);
			kprintf("TD3=%08lx\n", td[3]);
			if (td[3])
				printData((ULONG *)td[3], td[2]>>21);
		}
		else if (type == TD_LP_TYPE_QH)
		{
			printTD(td[1]);
		}
		printTD(td[0]);
	}
}

void printFrameListPointer(ULONG data) {

	kprintf("FLP: %lx\n", data);
	if (!(data & FLP_TERMINATE))
	{
		if ((data & FLP_TYPE) == FLP_TYPE_TD)
			printTD(data);
		else
			kprintf("unknown pointer\n");
	}
}

void printTC(struct List *list) {
struct TransferContext *tc;

	tc = (struct TransferContext *)list->lh_Head;
	while (tc->ln.ln_Succ)
	{
		kprintf("TC: %p (%s)\n", tc, tc->ln.ln_Type == TCT_TD ? "TD" : "QH");
		kprintf("TD: %lx\n", tc->td);
		kprintf("TD0=%lx\n", tc->td[0]);
		kprintf("TD1=%lx\n", tc->td[1]);
		switch (tc->ln.ln_Type)
		{
		case TCT_TD:
			kprintf("TD2=%lx\n", tc->td[2]);
			kprintf("TD3=%lx\n", tc->td[3]);
			break;
		case TCT_QH:
			printTC(&tc->qlist);
			break;
		}
		tc = (struct TransferContext *)tc->ln.ln_Succ;
	}
}

/*******************************************************************/

void addSchedule(struct UHCIData *uhci, struct TransferData *data) {
struct TransferContext *tc;
ULONG *td;

//	printTC(&data->head);
//	printFrameListPointer(&data->list);
	tc = (struct TransferContext *)data->list.lh_Head;
//	printTD(tc->ln.ln_Type == TCT_TD ? (ULONG)tc->td | TD_LP_TYPE_TD : (ULONG)tc->td | TD_LP_TYPE_QH);
//	data->framenum = inw(uhci->baseport+PORT_FRAME_NUMBER)+1;
//	if (data->framenum == 1023)
//		data->framenum = 0;
	/* find last element */
	td = uhci->flp->td;
	while (!(td[0] & TD_LP_TERMINATE))
		td = (ULONG *)(td[0] & ~0xF);
	switch (tc->ln.ln_Type)
	{
	case TCT_TD:
		td[0] = (ULONG)tc->td | FLP_TYPE_TD;
		break;
	case TCT_QH:
		td[0] = (ULONG)tc->td | FLP_TYPE_QH;
		break;
	}
}

void remSchedule(struct UHCIData *uhci, struct TransferData *data) {
struct TransferContext *tc;
ULONG *td;
ULONG *ltd;

	tc = (struct TransferContext *)data->list.lh_Head;
	if (tc->ln.ln_Succ)
	{
		td = uhci->flp->td;
		while ((td[0] & ~0xF) != (ULONG)tc->td)
			td = (ULONG *)(td[0] & ~0xF);
		ltd = td;
		
		do
		{
			ltd = (ULONG *)(ltd[0] & ~0xF);
			tc = (struct TransferContext *)tc->ln.ln_Succ;
		} while ((tc->ln.ln_Succ != NULL) && (!(ltd[0] & TD_LP_TERMINATE)));
		td[0] = ltd[0];
	}
}

BOOL sendControlData(struct PortUnit *port, struct TransferData *data) {
struct TransferContext *tc;
struct TransferContext *qh;
ULONG dest;
ULONG status;
ULONG ls=0;

	if (data->size<=1023)
	{
		qh = allocTC(TCT_QH);
		if (qh != NULL)
		{
			addTailTC(&data->list, qh);
			tc = allocTC(TCT_TD);
			if (tc != NULL)
			{
				if (port->flags & PUF_LOW_SPEED)
					ls = TD_CTRL_LOW_SPEED;
				/* prepare SETUP frame*/
				dest = TD_TOKEN_DATA0 | port->Endpoint<<15 | port->DeviceAddress<<8;
				status = (3<<27) | ls | TD_STATUS_ACTIVE;
				tc->td[1] = status;
				tc->td[2] = (sizeof(struct USBDeviceRequest)-1)<<21 | dest | PID_SETUP;
				tc->td[3] = (LONG)data->req;
				addTailQH(qh, tc);
				tc = allocTC(TCT_TD);
				if (tc != NULL)
				{
					/* prepare IN frame */
					dest ^= TD_TOKEN_TOGGLE; /* use DATA1 */
					tc->td[1] = status;
					tc->td[2] = (data->size-1)<<21 | dest | PID_IN;
					tc->td[3] = (LONG)data->data;
					addTailQH(qh, tc);
					if (data->data == NULL)
					{
						/*
							we requested no data from the device (function)
							therefore the empty data packet sent to it is the
							acknowledge
						*/
						addSchedule(port->uhci, data);
						return TRUE;
					}
					else
					{
						/* we requested data from the device (function)
							therefore we need to prepare an empty OUT packet
							to acknowledge the received data
						*/
						tc = allocTC(TCT_TD);
						if (tc != NULL)
						{
							dest |= TD_TOKEN_DATA1; /* use DATA1 */
							tc->td[1] = status;
							tc->td[2] = 0x7FF<<21 | dest | PID_OUT;
							addTailQH(qh, tc);
							addSchedule(port->uhci, data);
							return TRUE;
						}
					}
				}
			}
		}
		freeTransferList(&data->list);
	}
	else
		kprintf("UHCI: sendControlData() size (%ld) too big (put into 2 frames)\n", data->size);
	return NULL;
}

BOOL sendData(struct PortUnit *port, struct TransferData *data) {
BOOL retval = FALSE;

	switch (data->mode)
	{
	case TT_CONTROL:
		retval = sendControlData(port, data);
		break;
	default:
		kprintf("UHCI: unsupported data transfer\n");
	}
	return retval;
}

BOOL getDescriptor(struct PortUnit *port, struct DescriptorHeader *d, UWORD dsize, UBYTE descr, UBYTE index, UWORD langid) {
struct USBDeviceRequest req;
struct TransferData data;

	req.bmRequestType = UDRRT_DIR_DEVICE_TO_HOST | UDRRT_RT_STANDARD | UDRRT_REC_DEVICE;
	req.bRequest = UDRR_GET_DESCRIPTOR;
	req.wValue = (descr<<8) | index;
	req.wIndex = langid;
	req.wLength = dsize;
	data.req = &req;
	NEWLIST(&data.list);
	data.data = d;
	data.size = dsize;
	data.mode = TT_CONTROL;
	if (sendData(port, &data))
	{
		if (waitTransfer(port, &data))
		{
			kprintf("UHCI: wait success\n");
			remSchedule(port->uhci, &data);
			freeTransferList(&data.list);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL setAddress(struct PortUnit *port, UBYTE address) {
struct USBDeviceRequest req;
struct TransferData data;

	kprintf("set address %d\n", address);
	req.bmRequestType = UDRRT_DIR_HOST_TO_DEVICE | UDRRT_RT_STANDARD | UDRRT_REC_DEVICE;
	req.bRequest = UDRR_SET_ADDRESS;
	req.wValue = address;
	req.wIndex = 0;
	req.wLength = 0;
	data.req = &req;
	NEWLIST(&data.list);
	data.data = NULL;
	data.size = 0;
	data.mode = TT_CONTROL;
	if (sendData(port, &data))
	{
		if (waitTransfer(port, &data))
		{
			kprintf("UHCI: wait success\n");
			port->DeviceAddress = address;
			remSchedule(port->uhci, &data);
			freeTransferList(&data.list);
			return TRUE;
		}
	}
	return FALSE;
}

void waitConnected(struct PortUnit *port) {
UWORD portnum;
UWORD status;

	portnum = port->uhci->baseport+port->portnum;
	for (;;)
	{
		do
		{
			status = inw(portnum);
		} while (!(status & PSC_CONNECT_STATUS_CHANGE));
		status &= ~PSC_ENABLE_CHANGE; /* do not clear that one if set */
		outw(status, portnum);
		if (status & PSC_CONNECT_STATUS)
			break;
	}
}

void enablePort(struct PortUnit *port) {
UWORD portnum;

	portnum = port->uhci->baseport+port->portnum;
	outw(inw(portnum) | PSC_ENABLE, portnum);
}

void resetPort(struct PortUnit *port) {
UWORD portnum;

	portnum = port->uhci->baseport+port->portnum;
	outw(inw(portnum) | PSC_RESET, portnum);
	waitms(port, 10); /* 10-20 ms */
	outw(inw(portnum) & ~PSC_RESET, portnum);
}

struct PortUnit *newPort(struct UHCIData *data, UBYTE portnum) {
struct PortUnit *port;

	port = AllocMem(sizeof(struct PortUnit), MEMF_PUBLIC | MEMF_CLEAR);
	if (port != NULL)
	{
		port->tmp = CreateMsgPort();
		if (port->tmp != NULL)
		{
			port->treq = (struct timerequest *)CreateIORequest(port->tmp, sizeof(struct timerequest));
			if (port->treq)
			{
				if (OpenDevice(TIMERNAME, UNIT_VBLANK, &port->treq->tr_node, 0) == 0)
				{
					port->uhci = data;
					port->portnum = portnum;
					if (inw(data->baseport+portnum) & PSC_LOW_SPEED);
						port->flags |= PUF_LOW_SPEED;
					return port;
				}
				DeleteIORequest((struct IORequest *)port->treq);
			}
			DeleteMsgPort(port->tmp);
		}
		FreeMem(port, sizeof(struct PortUnit));
	}
	return NULL;
}

void disposePort(struct PortUnit *port) {
	CloseDevice(&port->treq->tr_node);
	DeleteIORequest((struct IORequest *)port->treq);
	DeleteMsgPort(port->tmp);
	FreeMem(port, sizeof(struct PortUnit));
}

void testPort(struct UHCIData *data, UBYTE portnum) {
struct PortUnit *port;
//struct TransferContext *tc;

	port = newPort(data, portnum);
	if (port != NULL)
	{
		kprintf("wait connect\n");
		waitConnected(port);
		waitms(port, 100); /* at least 100 ms */
		resetPort(port);
		enablePort(port);
		kprintf("get device descriptor\n");
		if (setAddress(port, allocAddress(data->abitmap)))
		{
			kprintf("UHCI: address set to %d\n", port->DeviceAddress);
			if (getDescriptor(port, &port->dd.header, sizeof(struct DescriptorDevice), GDT_DEVICE, 0, 0))
			{
				kprintf("UHCI: Vendor=%d Product=%d\n",port->dd.idVendor, port->dd.idProduct);
				kprintf("Configs=%d\n", port->dd.bNumConfigurations);
				/* read configurations */
				/* read interface(s) */
			}
		}
		disposePort(port);
	}
}



void uhci_int(int cpl, void *data, struct pt_regs *regs);

void irqSet(int, struct irqServer *);
static struct irqServer uhci = {uhci_int, "uhci", NULL};

BOOL initFrameLP(struct UHCIData *data) {
LONG i;

	data->flp = allocTC(TCT_TD);
	if (data->flp != NULL)
	{
		data->flp->td[0] = TD_LP_TERMINATE;
		data->flp->td[1] = 0;
		data->flp->td[2] = 0x7FF<<21 | 0x7F<<8 | PID_IN;
		data->flp->td[3] = 0;
		for (i=0;i<1024;i++)
			data->framebase[i] = (ULONG)data->flp->td | TD_LP_TYPE_TD;
		return TRUE;
	}
	return FALSE;
}

BOOL initUSBUHCIHW(struct UHCIData *data, HIDDT_PCI_Device *device) {

	if ((ULONG)device->BaseAddress[4] & 0x01)
	{
		data->baseport = (ULONG)device->BaseAddress[4] & ~0x01;
		kprintf("UHCI: %p\n", data->baseport);
		data->realframebase = AllocMem(1024*sizeof(ULONG)+4096, MEMF_PUBLIC | MEMF_CLEAR);
		if (data->realframebase != NULL)
		{
			/* we need a 4k aligned pointer */
			data->framebase = (ULONG *)(((ULONG)data->realframebase+4095) & ~4095);
			if (initFrameLP(data))
			{
				outl((ULONG)data->framebase, data->baseport+PORT_FRAME_LIST_BASE);
				irqSet(device->INTLine, &uhci);
				outw(IE_TIMEOUT_CRC | IE_RESUME | IE_ON_COMPLETE | IE_SHORT_PACKET, data->baseport+PORT_INTERRUPT_ENABLE);
				outw(inw(data->baseport+PORT_COMMAND) | CMD_CONFIGURE_FLAG, data->baseport+PORT_COMMAND);
				executeSchedule(data, 0);
				allocAddress(data->abitmap); /* allocate address 0 */
#warning "continue here"
//				testPort(data, PORT_PORT1_STATUS_CTRL);
//				for(;;);
				return TRUE;
			}
			FreeMem(data->realframebase, 1024*sizeof(ULONG)+4096);
		}
	}
	return FALSE;
}

#undef SysBase
extern struct ExecBase *SysBase;

void uhci_int(int cpl, void *data, struct pt_regs *regs) {

	kprintf("got int\n");
}
