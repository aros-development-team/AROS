#ifndef UHCI_H_
#define UHCI_H_

/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <inttypes.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <oop/oop.h>

#include <usb/usb.h>
#include <usb/usb_core.h>

#include <devices/timer.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

#define CLID_Drv_USB_UHCI "Bus::Drv::UHCI"
#define IID_Drv_USB_UHCI  "Bus::Drv::UHCI"

#undef HiddPCIDeviceAttrBase
#undef HiddUSBDeviceAttrBase
#undef HiddUSBHubAttrBase
#undef HiddUSBDrvAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (SD(cl)->HiddPCIDeviceAB)
#define HiddUSBDeviceAttrBase (SD(cl)->HiddUSBDeviceAB)
#define HiddUSBHubAttrBase (SD(cl)->HiddUSBHubAB)
#define HiddUSBDrvAttrBase (SD(cl)->HiddUSBDrvAB)
#define HiddAttrBase (SD(cl)->HiddAB)

typedef struct {
    uint32_t    qh_HLink;
    uint32_t    qh_VLink;
    uint32_t    qh_Data1;
    uint32_t    qh_Data2;
} UHCI_QueueHeader;

typedef struct {
    uint32_t 	td_LinkPtr;
    uint32_t	td_Status;
    uint32_t	td_Token;
    uint32_t	td_Buffer;
} UHCI_TransferDesc;

#define MAX_DEVS			6

struct uhci_staticdata
{
    struct SignalSemaphore  global_lock;

    APTR                    MemPool;

    IPTR                        iobase[MAX_DEVS];
    int					num_devices;

    OOP_Object				*uhciDevice[MAX_DEVS];
    OOP_Object				*uhciPCIDriver[MAX_DEVS];

    OOP_Object				*usb;
    OOP_Object				*pci;
    OOP_Object				*irq;
    OOP_Class               *uhciClass;

    struct List				td_list;

    OOP_AttrBase        HiddPCIDeviceAB;
    OOP_AttrBase        HiddUSBDeviceAB;
    OOP_AttrBase        HiddUSBHubAB;
    OOP_AttrBase        HiddUSBDrvAB;
    OOP_AttrBase        HiddAB;
};

struct TDNode {
    struct MinNode	td_Node;
    uint32_t			td_Bitmap[8];
    uint8_t			*td_Page;
};

struct uhcibase
{
    struct Library          LibNode;
    struct uhci_staticdata   sd;
};

#define UHCI_BITMAP_SIZE	128

typedef struct UHCI_Interrupt {
    struct MinNode              i_node;
    struct Interrupt            *i_intr;
    UHCI_TransferDesc           *i_td;
} UHCI_Interrupt_t;

typedef struct UHCI_Pipe {
    struct MinNode		p_Node;
    struct MinList              p_Intr;
    volatile UHCI_QueueHeader	*p_Queue;
    enum USB_PipeType	        p_Type;

    uint8_t                     p_FullSpeed;
    uint8_t                     p_DevAddr;
    uint8_t                     p_EndPoint;
    uint8_t                     p_NextToggle;
    uint16_t                    p_MaxTransfer;
    uint8_t                     p_Interval;

    uint8_t                     p_QHNode;
    uint8_t                     p_QHLocation;

    uint32_t                    p_ErrorCode;

    struct timerequest          *p_Timeout;
    uint32_t                    p_TimeoutVal;
    struct Task                 *p_SigTask;
    uint8_t                     p_Signal;

    volatile UHCI_TransferDesc  *p_FirstTD;
    volatile UHCI_TransferDesc  *p_LastTD;
} UHCI_Pipe;

typedef struct {
    struct uhci_staticdata	*sd;

    struct List         intList;
    struct Interrupt    *tmp;

    uint8_t             reset;
    uint8_t             running;

    struct MsgPort      mport;
    struct timerequest  *timereq;
    struct Interrupt    timerint;

    IPTR		iobase;
    IPTR		irq;

    uint32_t		*Frame;

    OOP_Object	*pciDriver;
    OOP_Object	*device;

    struct Interrupt   irqHandler;

    struct timerequest	*tr;

    struct MinList		Isochronous;
    struct MinList		Interrupts;
    struct MinList		ControlLS;
    struct MinList		ControlFS;
    struct MinList		Bulk;

    UHCI_TransferDesc	*dummy_td;

    /* Interrupt queue headers */
    UHCI_QueueHeader	*qh01;
    UHCI_QueueHeader	*qh02[2];
    UHCI_QueueHeader	*qh04[4];
    UHCI_QueueHeader	*qh08[8];
    UHCI_QueueHeader	*qh16[16];
    UHCI_QueueHeader	*qh32[32];
} UHCIData;

enum {
    aoHidd_UHCI_IOBase,
    aoHidd_UHCI_PCIDriver,
    aoHidd_UHCI_PCIDevice,

    num_Hidd_UHCI_Attrs
};


#define aHidd_UHCI_IOBase		(HiddUHCIAttrBase + aoHidd_UHCI_IOBase)
#define aHidd_UHCI_PCIDriver	(HiddUHCIAttrBase + aoHidd_UHCI_PCIDriver)
#define aHidd_UHCI_PCIDevice	(HiddUHCIAttrBase + aoHidd_UHCI_PCIDevice)
#define IS_UHCI_ATTR(attr, idx) (((idx)=(attr)-HiddUHCIAttrBase) < num_Hidd_UHCI_Attrs)

#define BASE(lib)((struct uhcibase*)(lib))
#define SD(cl) (&BASE(cl->UserData)->sd)

#define METHOD(base, id, name) \
    base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

/*** PCI config registers ***/

#define PCI_USBREV              0x60    /* USB protocol revision */
#define  PCI_USBREV_MASK        0xff
#define  PCI_USBREV_PRE_1_0     0x00
#define  PCI_USBREV_1_0         0x10
#define  PCI_USBREV_1_1         0x11

#define PCI_LEGSUP              0xc0    /* Legacy Support register */
#define  PCI_LEGSUP_USBPIRQDEN  0x2000  /* USB PIRQ D Enable */

#define PCI_CBIO                0x20    /* configuration base IO */

#define PCI_BASE_CLASS_SERIAL	0x0c
#define PCI_SUB_CLASS_USB		0x03
#define PCI_INTERFACE_UHCI      0x00

/*** UHCI registers ***/

#define UHCI_CMD                0x00
#define  UHCI_CMD_RS            0x0001
#define  UHCI_CMD_HCRESET       0x0002
#define  UHCI_CMD_GRESET        0x0004
#define  UHCI_CMD_EGSM          0x0008
#define  UHCI_CMD_FGR           0x0010
#define  UHCI_CMD_SWDBG         0x0020
#define  UHCI_CMD_CF            0x0040
#define  UHCI_CMD_MAXP          0x0080

#define UHCI_RESET_TIMEOUT		100

#define UHCI_STS                0x02
#define  UHCI_STS_USBINT        0x0001
#define  UHCI_STS_USBEI         0x0002
#define  UHCI_STS_RD            0x0004
#define  UHCI_STS_HSE           0x0008
#define  UHCI_STS_HCPE          0x0010
#define  UHCI_STS_HCH           0x0020
#define  UHCI_STS_ALLINTRS      0x003f

#define UHCI_INTR               0x04
#define  UHCI_INTR_TOCRCIE      0x0001
#define  UHCI_INTR_RIE          0x0002
#define  UHCI_INTR_IOCE         0x0004
#define  UHCI_INTR_SPIE         0x0008

#define UHCI_FRNUM              0x06
#define  UHCI_FRNUM_MASK        0x03ff

#define UHCI_FLBASEADDR         0x08

#define UHCI_SOF                0x0c
#define  UHCI_SOF_MASK          0x7f

#define UHCI_PORTSC1            0x010
#define UHCI_PORTSC2            0x012
#define UHCI_PORTSC_CCS         0x0001
#define UHCI_PORTSC_CSC         0x0002
#define UHCI_PORTSC_PE          0x0004
#define UHCI_PORTSC_POEDC       0x0008
#define UHCI_PORTSC_LS          0x0030
#define UHCI_PORTSC_LS_SHIFT    4
#define UHCI_PORTSC_RD          0x0040
#define UHCI_PORTSC_LSDA        0x0100
#define UHCI_PORTSC_PR          0x0200
#define UHCI_PORTSC_OCI         0x0400
#define UHCI_PORTSC_OCIC        0x0800
#define UHCI_PORTSC_SUSP        0x1000

#define URWMASK(x) \
    ((x) & (UHCI_PORTSC_SUSP | UHCI_PORTSC_PR | UHCI_PORTSC_RD | UHCI_PORTSC_PE))

#define UHCI_FRAMELIST_COUNT    1024
#define UHCI_FRAMELIST_ALIGN    4096

#define UHCI_TD_ALIGN           16
#define UHCI_QH_ALIGN           16

#define UHCI_PTR_T	0x00000001
#define UHCI_PTR_TD	0x00000000
#define UHCI_PTR_QH	0x00000002
#define UHCI_PTR_VF	0x00000004

#define UHCI_TD_GET_ACTLEN(s)   (((s) + 1) & 0x3ff)
#define UHCI_TD_ZERO_ACTLEN(t)  ((t) | 0x3ff)
#define UHCI_TD_BITSTUFF        0x00020000
#define UHCI_TD_CRCTO           0x00040000
#define UHCI_TD_NAK             0x00080000
#define UHCI_TD_BABBLE          0x00100000
#define UHCI_TD_DBUFFER         0x00200000
#define UHCI_TD_STALLED         0x00400000
#define UHCI_TD_ACTIVE          0x00800000
#define UHCI_TD_IOC             0x01000000
#define UHCI_TD_IOS             0x02000000
#define UHCI_TD_LS              0x04000000
#define UHCI_TD_GET_ERRCNT(s)   (((s) >> 27) & 3)
#define UHCI_TD_SET_ERRCNT(n)   ((n) << 27)
#define UHCI_TD_SPD             0x20000000

#define UHCI_TD_PID_IN          0x00000069
#define UHCI_TD_PID_OUT         0x000000e1
#define UHCI_TD_PID_SETUP       0x0000002d
#define UHCI_TD_GET_PID(s)      ((s) & 0xff)
#define UHCI_TD_SET_DEVADDR(a)  ((a) << 8)
#define UHCI_TD_GET_DEVADDR(s)  (((s) >> 8) & 0x7f)
#define UHCI_TD_SET_ENDPT(e)    (((e)&0xf) << 15)
#define UHCI_TD_GET_ENDPT(s)    (((s) >> 15) & 0xf)
#define UHCI_TD_SET_DT(t)       ((t) << 19)
#define UHCI_TD_GET_DT(s)       (((s) >> 19) & 1)
#define UHCI_TD_SET_MAXLEN(l)   (((l)-1) << 21)
#define UHCI_TD_GET_MAXLEN(s)   ((((s) >> 21) + 1) & 0x7ff)
#define UHCI_TD_MAXLEN_MASK     0xffe00000

#define UHCI_TD_ERROR (UHCI_TD_BITSTUFF|UHCI_TD_CRCTO|UHCI_TD_BABBLE|UHCI_TD_DBUFFER|UHCI_TD_STALLED)

#define UHCI_TD_SETUP(len, endp, dev) (UHCI_TD_SET_MAXLEN(len) | \
    UHCI_TD_SET_ENDPT(endp) | UHCI_TD_SET_DEVADDR(dev) | UHCI_TD_PID_SETUP)
#define UHCI_TD_OUT(len, endp, dev, dt) (UHCI_TD_SET_MAXLEN(len) | \
    UHCI_TD_SET_ENDPT(endp) | UHCI_TD_SET_DEVADDR(dev) | \
    UHCI_TD_PID_OUT | UHCI_TD_SET_DT(dt))
#define UHCI_TD_IN(len, endp, dev, dt) (UHCI_TD_SET_MAXLEN(len) | \
    UHCI_TD_SET_ENDPT(endp) | UHCI_TD_SET_DEVADDR(dev) | UHCI_TD_PID_IN | \
    UHCI_TD_SET_DT(dt))


UHCI_TransferDesc *uhci_AllocTD(OOP_Class *cl, OOP_Object *o);
UHCI_QueueHeader *uhci_AllocQH(OOP_Class *cl, OOP_Object *o);
void uhci_FreeTD(OOP_Class *cl, OOP_Object *o, UHCI_TransferDesc *td);
void uhci_FreeQH(OOP_Class *cl, OOP_Object *o, UHCI_QueueHeader *qh);
void uhci_FreeTDQuick(UHCIData *uhci, UHCI_TransferDesc *td);
void uhci_FreeQHQuick(UHCIData *uhci, UHCI_QueueHeader *qh);
void uhci_sleep(OOP_Class *cl, OOP_Object *o, uint32_t msec);
void uhci_globalreset(OOP_Class *cl, OOP_Object *o);
void uhci_reset(OOP_Class *cl, OOP_Object *o);
BOOL uhci_run(OOP_Class *cl, OOP_Object *o, BOOL run);

void uhci_RebuildList(OOP_Class *cl, OOP_Object *o);
UHCI_Pipe *uhci_CreatePipe(OOP_Class *cl, OOP_Object *o, enum USB_PipeType type, BOOL fullspeed,
        uint8_t addr, uint8_t endp, uint8_t period, uint32_t maxp, uint32_t timeout);
void uhci_DeletePipe(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe);
void uhci_QueuedTransfer(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe, VOID *buffer, uint32_t length, BOOL in);
void uhci_QueuedWrite(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe, VOID *buffer, uint32_t length);
void uhci_QueuedRead(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe, VOID *buffer, uint32_t length);
void uhci_ControlTransfer(OOP_Class *cl, OOP_Object *o, UHCI_Pipe *pipe,
        USBDevice_Request *request, VOID *buffer, uint32_t length);
BOOL uhci_PortReset(OOP_Class *cl, OOP_Object *o, uint8_t p);

#endif /*UHCI_H_*/
