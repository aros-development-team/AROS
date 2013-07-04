#ifndef UHCICHIP_H
#define UHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for UHCI USB Controller
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include <exec/types.h>
#include "hccommon.h"

/* PCI Class: PCI_CLASS_SERIAL_USB */

/* Framelist stuff

  1. Standard approach
     - Framelist contains all the same entries pointing to ISO-TD
     - ISO-TD: is inactive by default. Links to Control-QH
     - Control-QH: - Head links to Int-Queue
                   - Element: Links to dummy-TD if empty (inactive)
                   - Element: otherwise links to QH for control transfer
     - Int-Queue : - Head links to Bulk-Queue
  2. Not quite conform but better approach:
     - Framelist contains pointers to the correct interrupt queue head,
       depending on the interval (9 different QHs). The last 1ms qh points to the iso-td
     - the iso-td points to the first Control-QH
     - the control qh has vertical TDs for each transfer, and points to the next control qh
     - the last control qh points to bulk qh.
     - the bulk qh has vertical TDs for each (partial) transfer, and points to the next bulk qh
     - the last bulk qh points to the terminating qh with terminating bits set.
*/

/*
 * --------------------- UHCI registers ------------------------
 * Warning: These are BYTE offsets!
 */

#define UHCI_USBCMD         0x000 /* USB Command (r/w) */
#define UHCI_USBSTATUS      0x002 /* USB Status (r/wc) */
#define UHCI_USBINTEN       0x004 /* USB Interrupt Enable (r/w) */
#define UHCI_FRAMECOUNT     0x006 /* Frame Number (r/w) */
#define UHCI_FRAMELISTADDR  0x008 /* Framelist Base Address (LONGWORD!), 4KB aligned! (r/w) */
#define UHCI_SOFMOD         0x00c /* Start Of Frame Modify (upper byte?) (r/w) */
#define UHCI_PORT1STSCTRL   0x010 /* Port 1 Status/Control (r/wc) */
#define UHCI_PORT2STSCTRL   0x012 /* Port 2 Status/Control (r/wc) */
#define UHCI_USBLEGSUP      0x0c0 /* legacy support */

struct UHCIRegs
{
    volatile UWORD uhr_USBCmd;         /* USB Command (r/w) */
    volatile UWORD uhr_USBStatus;      /* USB Status (r/wc) */
    volatile UWORD uhr_USBIntEn;       /* USB Interrupt Enable (r/w) */
    volatile UWORD uhr_FrameCount;     /* Frame Number (r/w) */
    volatile APTR  uhr_FrameListAddr;  /* Framelist Base Address (LONGWORD!) (r/w) */
    volatile UBYTE uhr_SOFMod;         /* Start Of Frame Modify (upper byte?) (r/w) */
    volatile UBYTE uhr_Reserved0;
    volatile UWORD uhr_Reserved1;
    volatile UWORD uhr_PortStsCtrl[2]; /* Port 1/2 Status/Control (r/wc) */
};

/* UHCI_USBCMD defines */
#define UHCB_RUNSTOP         0    /* 1=Run, 0=Stop */
#define UHCB_HCRESET         1    /* Host Controller Reset */
#define UHCB_GLOBALRESET     2    /* Reset everything */
#define UHCB_USBSUSPEND      3    /* Send USB Suspend */
#define UHCB_USBRESUME       4    /* Send USB Resume */
#define UHCB_DEBUG           5    /* Software Debug */
#define UHCB_CONFIGURE       6    /* Semaphore */
#define UHCB_MAXPACKET64     7    /* 1=64 bytes, 0=32 bytes */

#define UHCF_RUNSTOP        (1UL<<UHCB_RUNSTOP)
#define UHCF_HCRESET        (1UL<<UHCB_HCRESET)
#define UHCF_GLOBALRESET    (1UL<<UHCB_GLOBALRESET)
#define UHCF_USBSUSPEND     (1UL<<UHCB_USBSUSPEND)
#define UHCF_USBRESUME      (1UL<<UHCB_USBRESUME)
#define UHCF_DEBUG          (1UL<<UHCB_DEBUG)
#define UHCF_CONFIGURE      (1UL<<UHCB_CONFIGURE)
#define UHCF_MAXPACKET64    (1UL<<UHCB_MAXPACKET64)

/* UHCI_USBSTATUS defines */
#define UHSB_USBINT          0    /* TD completed */
#define UHSB_USBERRORINT     1    /* TD resulted in an error condition */
#define UHSB_RESUMEDTX       2    /* Resume detected */
#define UHSB_HCSYSERROR      3    /* HC PCI error */
#define UHSB_HCPROCERROR     4    /* HC has found a TD error */
#define UHSB_HCHALTED        5    /* HC has stopped execution */

#define UHSF_USBINT         (1UL<<UHSB_USBINT)
#define UHSF_USBERRORINT    (1UL<<UHSB_USBERRORINT)
#define UHSF_RESUMEDTX      (1UL<<UHSB_RESUMEDTX)
#define UHSF_HCSYSERROR     (1UL<<UHSB_HCSYSERROR)
#define UHSF_HCPROCERROR    (1UL<<UHSB_HCPROCERROR)
#define UHSF_HCHALTED       (1UL<<UHSB_HCHALTED)

/* UHCI_USBINTEN defines */
#define UHIB_TIMEOUTCRC      0    /* Timeout or CRC Interrupt Enable */
#define UHIB_RESUME          1    /* Resume Interrupt Enable */
#define UHIB_INTONCOMPLETE   2    /* Interrupt on Complete (IOC) Enable */
#define UHIB_SHORTPACKET     3    /* Short Packet Interrupt Enable */

#define UHIF_TIMEOUTCRC     (1UL<<UHIB_TIMEOUTCRC)
#define UHIF_RESUME         (1UL<<UHIB_RESUME)
#define UHIF_INTONCOMPLETE  (1UL<<UHIB_INTONCOMPLETE)
#define UHIF_SHORTPACKET    (1UL<<UHIB_SHORTPACKET)

/* UHCI_PORTxSTSCTRL defines */
#define UHPB_PORTCONNECTED   0    /* Port Connection status */
#define UHPB_CONNECTCHANGE   1    /* Port Connection change */
#define UHPB_PORTENABLE      2    /* Enable Port */
#define UHPB_ENABLECHANGE    3    /* Port Enable/Disable change */
#define UHPB_STATUSDPLUS     4    /* Status of D+ line */
#define UHPB_STATUSDMINUS    5    /* Status of D- line */
#define UHPB_RESUMEDTX       6    /* Resume detected */
#define UHPB_LOWSPEED        8    /* Low speed device connected */
#define UHPB_PORTRESET       9    /* Port is in reset state */
#define UHPB_PORTSUSPEND    12    /* Suspend Mode */

#define UHPF_PORTCONNECTED  (1UL<<UHPB_PORTCONNECTED)
#define UHPF_CONNECTCHANGE  (1UL<<UHPB_CONNECTCHANGE)
#define UHPF_PORTENABLE     (1UL<<UHPB_PORTENABLE)
#define UHPF_ENABLECHANGE   (1UL<<UHPB_ENABLECHANGE)
#define UHPF_STATUSDPLUS    (1UL<<UHPB_STATUSDPLUS)
#define UHPF_STATUSDMINUS   (1UL<<UHPB_STATUSDMINUS)
#define UHPF_RESUMEDTX      (1UL<<UHPB_RESUMEDTX)
#define UHPF_LOWSPEED       (1UL<<UHPB_LOWSPEED)
#define UHPF_PORTRESET      (1UL<<UHPB_PORTRESET)
#define UHPF_PORTSUSPEND    (1UL<<UHPB_PORTSUSPEND)

/* data structures */

#define UHCI_FRAMELIST_SIZE      1024
#define UHCI_FRAMELIST_ALIGNMENT 0x0fff

#define UHCI_TDQH_ALIGNMENT      0x0007

#define UHCI_QH_POOLSIZE         128
#define UHCI_TD_POOLSIZE         1024

#define UHCI_TD_CTRL_LIMIT       512   // limit for one batch of CTRL data TDs
#define UHCI_TD_INT_LIMIT        128   // limit for one batch of INT data TDs
#define UHCI_TD_BULK_LIMIT       32768 // limit for one batch of BULK data TDs

struct UhciXX
{
    struct UhciXX  *uxx_Succ;
    struct UhciXX  *uxx_Pred;
    ULONG           uxx_Self;       /* LE PHYSICAL pointer to self + UHCI_TDSELECT */
    APTR            uxx_Private;
    /* aligned to 16 bytes */
    ULONG           uxx_Link;       /* LE PHYSICAL link pointer */
};

struct UhciTD
{
    struct UhciXX  *utd_Succ;
    ULONG           utd_Unused0;
    //struct UhciXX  *utd_Pred;
    ULONG           utd_Self;       /* LE PHYSICAL pointer to self + UHCI_TDSELECT */
    //struct UhciQH  *utd_QueueHead;  /* Pointer to queue head this thing belongs to (only for Ctrl/Bulk) */
    ULONG           utd_Unused1;
    /* aligned to 16 bytes */
    ULONG           utd_Link;       /* LE PHYSICAL TD Link Pointer (+BFS/DFS+QH/TD+TERM) */
    ULONG           utd_CtrlStatus; /* LE Control and Status word */
    ULONG           utd_Token;      /* LE Token (Transfer length) */
    ULONG           utd_BufferPtr;  /* LE PHYSICAL Data Buffer */
};

struct UhciQH
{
    struct UhciXX  *uqh_Succ;
    struct UhciXX  *uqh_Pred;
    ULONG           uqh_Self;       /* LE PHYSICAL pointer to self + UHCI_QHSELECT */
    struct IOUsbHWReq *uqh_IOReq;   /* IO Request this belongs to */
    /* aligned to 16 bytes */
    ULONG           uqh_Link;       /* LE PHYSICAL QH Link Pointer (QH/TD+TERM) */
    ULONG           uqh_Element;    /* LE PHYSICAL Queue Element Link Pointer (QH/TD+TERM) */
    struct UhciTD  *uqh_FirstTD;    /* First TD */
    ULONG           uqh_Actual;     /* Number of bytes for successful completion in this QH */
    APTR            uqh_SetupBuffer;/* Bounce buffer */
    APTR            uqh_DataBuffer; /* Bounce buffer */
    ULONG           uqh_Unused1;    /* Make sure size of structure is aligned to 16 bytes */
    ULONG           uqh_Unused2;    /* Make sure size of structure is aligned to 16 bytes */
};

/* pointer defines */

#define UHCI_PTRMASK        0xfffffff0 /* frame list pointer mask */
#define UHCI_QHSELECT       0x00000002 /* pointer is a queue head */
#define UHCI_TDSELECT       0x00000000 /* pointer is a transfer descriptor */
#define UHCI_TERMINATE      0x00000001 /* terminate list here */
#define UHCI_DFS            0x00000004 /* depth first search (TD only) */
#define UHCI_BFS            0x00000000 /* breadth first search (TD only) */

/* TD control and status word defines */

#define UTSS_ACTUALLENGTH   0          /* actual length of data transferred */
#define UTSB_BITSTUFFERR    17         /* Bit-Stuffing error */
#define UTSB_CRCTIMEOUT     18         /* IN CRC error, OUT Timeout error */
#define UTSB_NAK            19         /* NAK received */
#define UTSB_BABBLE         20         /* Babble detected on the bus */
#define UTSB_DATABUFFERERR  21         /* Data Buffer Error */
#define UTSB_STALLED        22         /* TD stalled due to errors */
#define UTCB_ACTIVE         23         /* TD is active / enable TD */
#define UTCB_READYINTEN     24         /* enable interrupt on complete */
#define UTCB_ISOCHRONOUS    25         /* enable isochronous transfer */
#define UTCB_LOWSPEED       26         /* device is lowspeed */
#define UTCS_ERRORLIMIT     27         /* how many errors permitted */
#define UTCB_SHORTPACKET    29         /* enable short packet detection */

#define UTSM_ACTUALLENGTH   (((1UL<<11)-1)<<UTSS_ACTUALLENGTH)
#define UTSF_BITSTUFFERR    (1UL<<UTSB_BITSTUFFERR)
#define UTSF_CRCTIMEOUT     (1UL<<UTSB_CRCTIMEOUT)
#define UTSF_NAK            (1UL<<UTSB_NAK)
#define UTSF_BABBLE         (1UL<<UTSB_BABBLE)
#define UTSF_DATABUFFERERR  (1UL<<UTSB_DATABUFFERERR)
#define UTSF_STALLED        (1UL<<UTSB_STALLED)
#define UTCF_ACTIVE         (1UL<<UTCB_ACTIVE)
#define UTCF_READYINTEN     (1UL<<UTCB_READYINTEN)
#define UTCF_ISOCHRONOUS    (1UL<<UTCB_ISOCHRONOUS)
#define UTCF_LOWSPEED       (1UL<<UTCB_LOWSPEED)
#define UTCF_SHORTPACKET    (1UL<<UTCB_SHORTPACKET)

#define UTCM_ERRORLIMIT     (((1UL<<2)-1)<<UTCS_ERRORLIMIT)
#define UTCF_NOERRORLIMIT   (0UL<<UTCS_ERRORLIMIT)
#define UTCF_1ERRORLIMIT    (1UL<<UTCS_ERRORLIMIT)
#define UTCF_2ERRORSLIMIT   (2UL<<UTCS_ERRORLIMIT)
#define UTCF_3ERRORSLIMIT   (3UL<<UTCS_ERRORLIMIT)

/* TD Token word defines */

#define UTTS_PID            0          /* Packet ID */
#define UTTS_DEVADDR        8          /* Device address */
#define UTTS_ENDPOINT       15         /* Endpoint address */
#define UTTB_DATA1          19         /* DATA1 toggle */
#define UTTS_TRANSLENGTH    21         /* (maximum) length of the transfer */

#define UTTF_DATA0          (0UL<<UTTB_DATA1)
#define UTTF_DATA1          (1UL<<UTTB_DATA1)

#define UTTM_PID            (((1UL<<8)-1)<<UTTS_PID)
#define UTTM_DEVADDR        (((1UL<<7)-1)<<UTTS_DEVADDR)
#define UTTM_ENDPOINT       (((1UL<<4)-1)<<UTTS_ENDPOINT)
#define UTTM_TRANSLENGTH    (((1UL<<11)-1)<<UTTS_TRANSLENGTH)

#endif /* UHCICHIP_H */
