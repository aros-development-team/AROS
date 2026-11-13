#ifndef OHCI_H_
#define OHCI_H_

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

#include <aros/asmcall.h>
#include <aros/macros.h>

#include <exec/semaphores.h>
#include <devices/timer.h>

#include LC_LIBDEFS_FILE

#ifndef BIG_ENDIAN_OHCI
#define BIG_ENDIAN_OHCI 0
#endif

#if BIG_ENDIAN_OHCI
#define AROS_LONG2OHCI(x)	(AROS_LONG2BE(x))
#define AROS_WORD2OHCI(x)	(AROS_WORD2BE(x))
#define AROS_OHCI2LONG(x)	(AROS_BE2LONG(x))
#define AROS_OHCI2WORD(x)	(AROS_BE2WORD(x))
#else
#define AROS_LONG2OHCI(x)	(AROS_LONG2LE(x))
#define AROS_WORD2OHCI(x)	(AROS_WORD2LE(x))
#define AROS_OHCI2LONG(x)	(AROS_LE2LONG(x))
#define AROS_OHCI2WORD(x)	(AROS_LE2WORD(x))
#endif

#define mmio(var) (*(volatile uint32_t *)&(var))

typedef struct ohci_registers {
    uint32_t    HcRevision;
    uint32_t    HcControl;
    uint32_t    HcCommandStatus;
    uint32_t    HcInterruptStatus;
    uint32_t    HcInterruptEnable;
    uint32_t    HcInterruptDisable;
    uint32_t    HcHCCA;
    uint32_t    HcPeriodCurrentED;
    uint32_t    HcControlHeadED;
    uint32_t    HcControlCurrentED;
    uint32_t    HcBulkHeadED;
    uint32_t    HcBulkCurrentED;
    uint32_t    HcDoneHead;
    uint32_t    HcFmInterval;
    uint32_t    HcFmRemaining;
    uint32_t    HcFmNumber;
    uint32_t    HcPeriodicStart;
    uint32_t    HcLSThreshold;
    uint32_t    HcRhDescriptorA;
    uint32_t    HcRhDescriptorB;
    uint32_t    HcRhStatus;
    uint32_t    HcRhPortStatus[0];
} ohci_registers_t;

/* HcControl */
#define HC_CTRL_CBSR_MASK       0x00000003
#define HC_CTRL_CBSR_1_1        0x00000000
#define HC_CTRL_CBSR_1_2        0x00000001
#define HC_CTRL_CBSR_1_3        0x00000002
#define HC_CTRL_CBSR_1_4        0x00000003
#define HC_CTRL_PLE             0x00000004
#define HC_CTRL_IE              0x00000008
#define HC_CTRL_CLE             0x00000010
#define HC_CTRL_BLE             0x00000020
#define HC_CTRL_HCFS_MASK       0x000000c0
#define HC_CTRL_HCFS_RESET      0x00000000
#define HC_CTRL_HCFS_RESUME     0x00000040
#define HC_CTRL_HCFS_OPERATIONAL 0x00000080
#define HC_CTRL_HCFS_SUSPENDED  0x000000c0
#define HC_CTRL_IR              0x00000100
#define HC_CTRL_RWC             0x00000200
#define HC_CTRL_RWE             0x00000400

/* HcCommandStatus */
#define HC_CS_HCR               0x00000001
#define HC_CS_CLF               0x00000002
#define HC_CS_BLF               0x00000004
#define HC_CS_OCR               0x00000008
#define HC_CS_SOC_MASK          0x00030000

/* HcInterruptStatus, HcInterruptDisable */
#define HC_INTR_SO              0x00000001
#define HC_INTR_WDH             0x00000002
#define HC_INTR_SF              0x00000004
#define HC_INTR_RD              0x00000008
#define HC_INTR_UE              0x00000010
#define HC_INTR_FNO             0x00000020
#define HC_INTR_RHSC            0x00000040
#define HC_INTR_OC              0x40000000
#define HC_INTR_MIE             0x80000000

/* HcFmInterval */
#define HC_FM_GET_IVAL(v)       ((v) & 0x3fff)
#define HC_FM_GET_FSMPS(v)      (((v) >> 16) & 0x7fff)
#define HC_FM_FIT               0x80000000

#define HC_FM_FSMPS(v)          ((((v)-210)*6/7) << 16)
#define HC_PERIODIC(v)          ((v)*9/10)

/* HcRhDescriptorA */
#define HC_RHA_GET_NDP(v)       ((v) & 0xff)
#define HC_RHA_PSM              0x00000100
#define HC_RHA_NPS              0x00000200
#define HC_RHA_DT               0x00000400
#define HC_RHA_OCPM             0x00000800
#define HC_RHA_NOCP             0x00001000
#define HC_RHA_GET_POTPGT(v)    ((v) >> 24)

/* HcRhStatus */
#define HC_RHS_LPS              0x00000001
#define HC_RHS_OCI              0x00000002
#define HC_RHS_DRWE             0x00008000
#define HC_RHS_LPSC             0x00010000
#define HC_RHS_OCIC             0x00020000
#define HC_RHS_CRWE             0x80000000

/* HcRhPortStatus */
#define HC_PS_CCS               0x00000001
#define HC_PS_PES               0x00000002
#define HC_PS_PSS               0x00000004
#define HC_PS_POCI              0x00000008
#define HC_PS_PRS               0x00000010
#define HC_PS_PPS               0x00000100
#define HC_PS_LSDA              0x00000200
#define HC_PS_CSC               0x00010000
#define HC_PS_PESC              0x00020000
#define HC_PS_PSSC              0x00040000
#define HC_PS_OCIC              0x00080000
#define HC_PS_PRSC              0x00100000

typedef struct ohci_pipe ohci_pipe_t;

typedef struct ohci_hcca {
    uint32_t    hccaIntrTab[32];
//#if AROS_BIG_ENDIAN
//    uint16_t    pad;
//    uint16_t    hccaFrNum;
//#else
    uint16_t    hccaFrNum;
    uint16_t    pad;
//#endif
    uint32_t    hccaDoneHead;
    uint8_t     hccaRsvd[116];
} __attribute__((packed)) ohci_hcca_t;

typedef struct ohci_ed {
    uint32_t    edFlags;
    uint32_t    edTailP;
    uint32_t    edHeadP;
    uint32_t    edNextED;
    ohci_pipe_t *edPipe;
    uint32_t    edUsage;
} __attribute__((aligned(32))) ohci_ed_t;

#define ED_FA_MASK      0x0000007f
#define ED_EN_MASK      0x00000780
#define ED_D_MASK       0x00001800
#define ED_S            0x00002000
#define ED_K            0x00004000
#define ED_F            0x00008000
#define ED_MPS_MASK     0x07ff0000
#define ED_C            0x00000002
#define ED_H            0x00000001

#define ED_GET_USAGE(ed) ((ed)->edUsage)
#define ED_SET_USAGE(ed,u) ((ed)->edUsage = (u))

typedef struct ohci_td {
    uint32_t    tdFlags;
    uint32_t    tdCurrentBufferPointer;
    uint32_t    tdNextTD;
    uint32_t    tdBufferEnd;
    ohci_pipe_t *tdPipe;
} __attribute__((aligned(32))) ohci_td_t;

#define TD_R            0x00040000
#define TD_DP_MASK      0x00180000
#define TD_DI_MASK      0x00e00000
#define TD_T_MASK       0x03000000
#define TD_EC_MASK      0x0c000000
#define TD_CC_MASK      0xf0000000

#define HC_FSMPS(i) (((i-210) * 6 / 7)<<16)

#define CLID_Drv_USB_OHCI "Bus::Drv::OHCI"
#define IID_Drv_USB_OHCI  "Bus::Drv::OHCI"

#undef HiddPCIDeviceAttrBase
#undef HiddUSBDeviceAttrBase
#undef HiddUSBHubAttrBase
#undef HiddUSBDrvAttrBase
#undef HiddOHCIAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (SD(cl)->HiddPCIDeviceAB)
#define HiddUSBDeviceAttrBase (SD(cl)->HiddUSBDeviceAB)
#define HiddUSBHubAttrBase (SD(cl)->HiddUSBHubAB)
#define HiddUSBDrvAttrBase (SD(cl)->HiddUSBDrvAB)
#define HiddOHCIAttrBase (SD(cl)->HiddOHCIAB)
#define HiddAttrBase (SD(cl)->HiddAB)

#define MAX_OHCI_DEVICES        8

typedef struct ohci_intr {
    struct MinNode      node;
    struct Interrupt    *intr;
    ohci_td_t           *td;
    void                *buffer;
    uint32_t            length;
} ohci_intr_t;

struct ohci_pipe {
    struct Node         node;
    struct SignalSemaphore lock;

    ohci_intr_t         *interrupt;
    ohci_ed_t           *ed;
    ohci_ed_t           *location;

    ohci_td_t           *tail;

    uint16_t			maxpacket;
    uint8_t             type;
    uint8_t             interval;
    uint8_t             endpoint;
    uint8_t             address;

    struct timerequest  *timeout;
    uint32_t            timeoutVal;
    struct Task         *sigTask;
    uint8_t             signal;

    uint32_t            errorCode;
};

struct ohci_staticdata
{
    OOP_Class           *ohciClass;

    OOP_Object          *usb;
    OOP_Object          *pci;

    OOP_AttrBase        HiddPCIDeviceAB;
    OOP_AttrBase        HiddUSBDeviceAB;
    OOP_AttrBase        HiddUSBHubAB;
    OOP_AttrBase        HiddUSBDrvAB;
    OOP_AttrBase        HiddOHCIAB;
    OOP_AttrBase        HiddAB;

    void                *memPool;

    struct SignalSemaphore      tdLock;
    struct List                 tdList;

    uint8_t             numDevices;
    intptr_t            ramBase[MAX_OHCI_DEVICES];
    uint8_t             numPorts[MAX_OHCI_DEVICES];
    uint8_t				irqNum[MAX_OHCI_DEVICES];
    OOP_Object          *pciDevice[MAX_OHCI_DEVICES];
    OOP_Object          *pciDriver[MAX_OHCI_DEVICES];
    OOP_Object          *ohciDevice[MAX_OHCI_DEVICES];
};

struct ohcibase
{
    struct Library          LibNode;
    struct ohci_staticdata  sd;
};

typedef struct ohci_data {
    struct ohci_staticdata      *sd;
    volatile ohci_registers_t   *regs;
    volatile ohci_hcca_t        *hcca;
    usb_hub_descriptor_t        hubDescr;
    uint8_t                     running;
    uint8_t                     pendingRHSC;

    struct List                 intList;
    struct Interrupt            *tmp;

    struct MsgPort              timerPort;
    struct Interrupt            timerInt;
    struct timerequest          *timerReq;

    struct timerequest          *tr;

    struct Interrupt            irqHandler;
    intptr_t                    irqNum;

    OOP_Object                  *pciDriver;
    OOP_Object                  *pciDevice;

    ohci_ed_t                   *ctrl_head;
    ohci_ed_t                   *bulk_head;
    ohci_ed_t                   *isoc_head;

    ohci_ed_t                   *int01;
    ohci_ed_t                   *int02[2];
    ohci_ed_t                   *int04[4];
    ohci_ed_t                   *int08[8];
    ohci_ed_t                   *int16[16];
    ohci_ed_t                   *int32[32];

} ohci_data_t;

typedef struct td_node {
    struct MinNode      tdNode;
    uint32_t            tdBitmap[4];
    ohci_td_t           *tdPage;
} td_node_t;

#define BASE(lib)((struct ohcibase*)(lib))
#define SD(cl) (&BASE(cl->UserData)->sd)

#define METHOD(base, id, name) \
    base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

enum {
    aoHidd_OHCI_MemBase,
    aoHidd_OHCI_PCIDriver,
    aoHidd_OHCI_PCIDevice,
    aoHidd_OHCI_IRQ,

    num_Hidd_OHCI_Attrs
};

#define aHidd_OHCI_MemBase      (HiddOHCIAttrBase + aoHidd_OHCI_MemBase)
#define aHidd_OHCI_PCIDriver    (HiddOHCIAttrBase + aoHidd_OHCI_PCIDriver)
#define aHidd_OHCI_PCIDevice    (HiddOHCIAttrBase + aoHidd_OHCI_PCIDevice)
#define aHidd_OHCI_IRQ			 (HiddOHCIAttrBase + aoHidd_OHCI_IRQ)
#define IS_OHCI_ATTR(attr, idx) (((idx)=(attr)-HiddOHCIAttrBase) < num_Hidd_OHCI_Attrs)

#define PCI_BASE_CLASS_SERIAL   0x0c
#define PCI_SUB_CLASS_USB       0x03
#define PCI_INTERFACE_OHCI      0x10

ohci_td_t *ohci_AllocTD(OOP_Class *cl, OOP_Object *o);
ohci_ed_t *ohci_AllocED(OOP_Class *cl, OOP_Object *o);
void ohci_FreeTDQuick(ohci_data_t *ohci, ohci_td_t *td);
void ohci_FreeEDQuick(ohci_data_t *ohci, ohci_ed_t *ed);
void ohci_FreeTD(OOP_Class *cl, OOP_Object *o, ohci_td_t *td);
void ohci_FreeED(OOP_Class *cl, OOP_Object *o, ohci_ed_t *ed);

void ohci_Delay(struct timerequest *tr, uint32_t msec);
struct timerequest *ohci_CreateTimer();
void ohci_DeleteTimer(struct timerequest *tr);

#endif /*OHCI_H_*/
