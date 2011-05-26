#ifndef OHCICHIP_H
#define OHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for OHCI USB Controller
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include <exec/types.h>
#include "hccommon.h"

/* PCI Class: PCI_CLASS_SERIAL_USB */

/* Framelist stuff

  - Framelist contains all the same entries pointing to ISO-TD
  - ISO-TD: is inactive by default. Links to Control-QH
  - Control-QH: - Head links to Int-Queue
                - Element: Links to dummy-TD if empty (inactive)
                - Element: otherwise links to QH for control transfer
  - Int-Queue : - Head links to Bulk-Queue


*/

/*
 * --------------------- OHCI registers ------------------------
 * Warning: These are BYTE offsets!
 */

/* operational registers */
#define OHCI_REVISION       0x000 /* Host Controller Revision (r) */
#define OHCI_CONTROL        0x004 /* Control register (r/w) */
#define OHCI_CMDSTATUS      0x008 /* Command and Status */
#define OHCI_INTSTATUS      0x00c /* Interrupt Status (r/wc) */
#define OHCI_INTEN          0x010 /* Interrupt Enable (r/ws) */
#define OHCI_INTDIS         0x014 /* Interrupt Disable (r/wc) */
#define OHCI_HCCA           0x018 /* Pointer to HCCA */
#define OHCI_PERIODIC_ED    0x01c /* Current periodic ED */
#define OHCI_CTRL_HEAD_ED   0x020 /* Control Head ED */
#define OHCI_CTRL_ED        0x024 /* Current control ED */
#define OHCI_BULK_HEAD_ED   0x028 /* Bulk Head ED */
#define OHCI_BULK_ED        0x02c /* Current bulk ED */
#define OHCI_DONEHEAD       0x030 /* Done head pointer */
#define OHCI_FRAMEINTERVAL  0x034 /* Frame interval */
#define OHCI_FRAMEREMAINING 0x038 /* Frame remaining time */
#define OHCI_FRAMECOUNT     0x03c /* Frame number */
#define OHCI_PERIODICSTART  0x040 /* Periodic start (usually 10% of 12000 == 0x3e67) */
#define OHCI_LSTHRESHOLD    0x044 /* Lowspeed threshold (usually 0x0628) */
#define OHCI_HUBDESCA       0x048 /* Root Hub Descriptor A */
#define OHCI_HUBDESCB       0x04c /* Root Hub Descriptor B */
#define OHCI_HUBSTATUS      0x050 /* Root Hub Status */
#define OHCI_PORTSTATUS     0x054 /* Port Status */

/* OHCI_CONTROL defines */
#define OCLS_CBSR            0    /* Control Bulk Service Ratio */
#define OCLB_PERIODICENABLE  2    /* Periodic Enable */
#define OCLB_ISOENABLE       3    /* Isochronous Enable */
#define OCLB_CTRLENABLE      4    /* Control List enable */
#define OCLB_BULKENABLE      5    /* Bulk List enable */
#define OCLS_USBSTATE        6    /* Host controller functional state */
#define OCLB_SMIINT          8    /* SMI Interrupt routing */
#define OCLB_REMOTEWAKEUP   10    /* Remote wakeup enabled */

#define OCLF_PERIODICENABLE (1UL<<OCLB_PERIODICENABLE)
#define OCLF_ISOENABLE      (1UL<<OCLB_ISOENABLE)
#define OCLF_CTRLENABLE     (1UL<<OCLB_CTRLENABLE)
#define OCLF_BULKENABLE     (1UL<<OCLB_BULKENABLE)
#define OCLF_SMIINT         (1UL<<OCLB_SMIINT)
#define OCLF_REMOTEWAKEUP   (1UL<<OCLB_REMOTEWAKEUP)

#define OCLM_CBSR           (((1UL<<2)-1)<<OCLS_CBSR)
#define OCLM_USBSTATE       (((1UL<<2)-1)<<OCLS_USBSTATE)
#define OCLF_USBRESET       (0UL<<OCLS_USBSTATE)
#define OCLF_USBRESUME      (1UL<<OCLS_USBSTATE)
#define OCLF_USBOPER        (2UL<<OCLS_USBSTATE)
#define OCLF_USBSUSPEND     (3UL<<OCLS_USBSTATE)

/* OHCI_CMDSTATUS defines */
#define OCSB_HCRESET         0    /* Host controller reset */
#define OCSB_CTRLENABLE      1    /* Enable Control List processing */
#define OCSB_BULKENABLE      2    /* Enable Bulk List processing */
#define OCSB_OWNERCHANGEREQ  3    /* Request change of ownership for BIOS handover */

#define OCSF_HCRESET        (1UL<<OCSB_HCRESET)
#define OCSF_CTRLENABLE     (1UL<<OCSB_CTRLENABLE)
#define OCSF_BULKENABLE     (1UL<<OCSB_BULKENABLE)
#define OCSF_OWNERCHANGEREQ (1UL<<OCSB_OWNERCHANGEREQ)

/* OHCI_INTSTATUS, OHCI_INTEN and OHCI_INTDIS defines */
#define OISB_SCHEDOVERRUN    0    /* Schedule overrun */
#define OISB_DONEHEAD        1    /* Writeback done head */
#define OISB_SOF             2    /* Start of Frame */
#define OISB_RESUMEDTX       3    /* Resume detected */
#define OISB_HOSTERROR       4    /* Unrecoverable error */
#define OISB_FRAMECOUNTOVER  5    /* Frame counter overrun (15 bit) */
#define OISB_HUBCHANGE       6    /* Root Hub status change */
#define OISB_OWNERCHANGE    30    /* Ownership changed */
#define OISB_MASTERENABLE   31    /* Master Interrupt enable (INTEN only) */

#define OISF_SCHEDOVERRUN   (1UL<<OISB_SCHEDOVERRUN)
#define OISF_DONEHEAD       (1UL<<OISB_DONEHEAD)
#define OISF_SOF            (1UL<<OISB_SOF)
#define OISF_RESUMEDTX      (1UL<<OISB_RESUMEDTX)
#define OISF_HOSTERROR      (1UL<<OISB_HOSTERROR)
#define OISF_FRAMECOUNTOVER (1UL<<OISB_FRAMECOUNTOVER)
#define OISF_HUBCHANGE      (1UL<<OISB_HUBCHANGE)
#define OISF_OWNERCHANGE    (1UL<<OISB_OWNERCHANGE)
#define OISF_MASTERENABLE   (1UL<<OISB_MASTERENABLE)

#define OISF_ALL_INTS       (OISF_SCHEDOVERRUN|OISF_DONEHEAD|OISF_SOF|OISF_RESUMEDTX|OISF_HOSTERROR|OISF_FRAMECOUNTOVER|OISF_HUBCHANGE|OISF_MASTERENABLE)

/* OHCI_INTERVAL defines */
#define OIVS_INTERVAL        0    /* Interval, usually 11999 == 0x2edf */
#define OIVS_BITSPERFRAME   16    /* Size of the frames in bits, usually ((12000 - 210) * 6) / 7 == 10105 */
#define OIVB_TOGGLE         31    /* Toggle on change of interval */

#define OIVM_INTERVAL       (((1UL<<14)-1)<<OIVS_INTERVAL)
#define OIVM_BITSPERFRAME   (((1UL<<15)-1)<<OIVS_BITSPERFRAME)
#define OIVF_TOGGLE         (1UL<<OIVB_TOGGLE)

#define OHCI_DEF_BITSPERFRAME 10105

/* OHCI_HUBDESCA defines */
#define OHAS_NUMPORTS        0    /* Number of downstream ports */
#define OHAB_INDIVIDUALPS    8    /* Power switching per port */
#define OHAB_NOPOWERSWITCH   9    /* Ports always powered */
#define OHAB_INDIVIDUALOC   11    /* Overcurrent Detection per port */
#define OHAB_NOOVERCURRENT  12    /* No over-current detection */
#define OHAS_POWERGOOD      24    /* Power-good delay */

#define OHAM_NUMPORTS       (((1UL<<8)-1)<<OHAS_NUMPORTS)
#define OHAF_INDIVIDUALPS   (1UL<<OHAB_INDIVIDUALPS)
#define OHAF_NOPOWERSWITCH  (1UL<<OHAB_NOPOWERSWITCH)
#define OHAF_INDIVIDUALOC   (1UL<<OHAB_INDIVIDUALOC)
#define OHAF_NOOVERCURRENT  (1UL<<OHAB_NOOVERCURRENT)
#define OHAM_POWERGOOD      (((1UL<<8)-1)<<OHAS_POWERGOOD)

/* OHCI_HUBDESCB defines */
#define OHBS_DEVREMOVABLE    0    /* Bitmask of removable devices at roothub (port 1 == bit 1) */
#define OHBS_PORTPOWERCTRL  16    /* Bitmask of global power controlled ports */

#define OHBM_DEVREMOVABLE   (((1UL<<16)-1)<<OHBS_DEVREMOVABLE)
#define OHBM_PORTPOWERCTRL  (((1UL<<16)-1)<<OHBS_PORTPOWERCTRL)

/* OHCI_HUBSTATUS defines */
#define OHSB_UNPOWERHUB      0    /* Clear global Hub power */
#define OHSB_OVERCURRENT     1    /* Global over-current reported */
#define OHSB_POWERHUB       16    /* Set global Hub power */
#define OHSB_OVERCURRENTCHG 17    /* Reports change in over-current situation */

#define OHSF_UNPOWERHUB     (1UL<<OHSB_UNPOWERHUB)
#define OHSF_OVERCURRENT    (1UL<<OHSB_OVERCURRENT)
#define OHSF_POWERHUB       (1UL<<OHSB_POWERHUB)
#define OHSF_OVERCURRENTCHG (1UL<<OHSB_OVERCURRENTCHG)

/* OHCI_PORTSTATUS defines */
#define OHPB_PORTCONNECTED   0    /* Port Connection status (r) */
#define OHPB_PORTDISABLE     0    /* Clear Port enable (w) */
#define OHPB_PORTENABLE      1    /* Port Enabled (r), Enable Port (w) */
#define OHPB_PORTSUSPEND     2    /* Port Suspended (r), Suspend Port (w) */
#define OHPB_OVERCURRENT     3    /* Port Overcurrent detected (r) */
#define OHPB_RESUME          3    /* Resume from suspend (w) */
#define OHPB_PORTRESET       4    /* Port in reset (r), Reset port (w) */
#define OHPB_PORTPOWER       8    /* Power powered (r), Power port (w) */
#define OHPB_LOWSPEED        9    /* Low speed device connected (r) */
#define OHPB_PORTUNPOWER     9    /* Clear Port power (w) */
#define OHPB_CONNECTCHANGE  16    /* Port Connection change */
#define OHPB_ENABLECHANGE   17    /* Port Enable/Disable change */
#define OHPB_RESUMEDTX      18    /* Resume detected */
#define OHPB_OVERCURRENTCHG 19    /* Over-current change */
#define OHPB_RESETCHANGE    20    /* Reset complete */

#define OHPF_PORTCONNECTED  (1UL<<OHPB_PORTCONNECTED)
#define OHPF_PORTDISABLE    (1UL<<OHPB_PORTDISABLE)
#define OHPF_PORTENABLE     (1UL<<OHPB_PORTENABLE)
#define OHPF_PORTSUSPEND    (1UL<<OHPB_PORTSUSPEND)
#define OHPF_OVERCURRENT    (1UL<<OHPB_OVERCURRENT)
#define OHPF_RESUME         (1UL<<OHPB_RESUME)
#define OHPF_PORTRESET      (1UL<<OHPB_PORTRESET)
#define OHPF_PORTPOWER      (1UL<<OHPB_PORTPOWER)
#define OHPF_LOWSPEED       (1UL<<OHPB_LOWSPEED)
#define OHPF_PORTUNPOWER    (1UL<<OHPB_PORTUNPOWER)
#define OHPF_CONNECTCHANGE  (1UL<<OHPB_CONNECTCHANGE)
#define OHPF_ENABLECHANGE   (1UL<<OHPB_ENABLECHANGE)
#define OHPF_RESUMEDTX      (1UL<<OHPB_RESUMEDTX)
#define OHPF_OVERCURRENTCHG (1UL<<OHPB_OVERCURRENTCHG)
#define OHPF_RESETCHANGE    (1UL<<OHPB_RESETCHANGE)

/* data structures */

/* HCCA registers */

#define OHCI_HCCA_SIZE        256 /* size of HCCA section */
#define OHCI_HCCA_ALIGNMENT 0x0ff /* alignment of HCCA section */

struct OhciHCCA
{
    ULONG           oha_IntEDs[32]; /* LE PHYSICAL pointer to Interrupt EDs */
    UWORD           oha_FrameCount; /* LE Framecounter */
    UWORD           oha_FrmCntChg;  /* Set 0 when framecounter was updated */
    ULONG           oha_DoneHead;   /* LE PHYSICAL pointer to Head of ED finished + unmasked Int */
};


#define OHCI_PAGE_SIZE           4096

#define OHCI_TDQH_ALIGNMENT      0x001f

#define OHCI_ED_POOLSIZE         128
#define OHCI_TD_POOLSIZE         512

#define OHCI_TD_BULK_LIMIT       (128<<10) // limit for one batch of BULK data TDs


struct OhciED
{
    struct OhciED  *oed_Succ;
    struct OhciED  *oed_Pred;
    ULONG           oed_Self;       /* LE PHYSICAL pointer to self */
    /* On 64 bits a padding will be inserted here */
    struct IOUsbHWReq *oed_IOReq;   /* IO Request this belongs to */

    struct OhciTD  *oed_FirstTD;    /* First TD */
    IPTR            oed_Continue;   /* Flag for fragmented bulk transfer */
    APTR	    oed_Buffer;	    /* Mirror buffer for data outside of DMA-accessible area */
    struct UsbSetupData *oed_SetupData; /* Mirror buffer for setup packet */

    /* HC data, aligned to 16 bytes */
    ULONG           oed_EPCaps;     /* LE MaxPacketSize and other stuff */
    ULONG           oed_TailPtr;    /* LE PHYSICAL TD Queue Tail Pointer */
    ULONG           oed_HeadPtr;    /* LE PHYSICAL TD Queue Head Pointer */
    ULONG           oed_NextED;     /* LE PHYSICAL Next Endpoint Descriptor */
};

struct OhciTD
{
    struct OhciTD  *otd_Succ;
    IPTR            otd_Length;     /* Length of transfer */
    ULONG           otd_Self;       /* LE PHYSICAL pointer to self */
    /* On 64 bits a padding will be inserted here */
    struct OhciED  *otd_ED;         /* Pointer to parent ED this TD belongs to */

    /* HC data, aligned to 16 bytes */
    ULONG           otd_Ctrl;       /* LE Ctrl stuff */
    ULONG           otd_BufferPtr;  /* LE PHYSICAL Current Buffer Pointer */
    ULONG           otd_NextTD;     /* LE PHYSICAL Next TD */
    ULONG           otd_BufferEnd;  /* LE PHYSICAL End of buffer */
};

/* pointer defines */

#define OHCI_PTRMASK        0xfffffff0 /* frame list pointer mask */

/* ED EPCaps defines */

#define OECB_LOWSPEED       13    /* Lowspeed */
#define OECB_SKIP           14    /* Skip ED */
#define OECB_ISO            15    /* Isochronous endpoint */

#define OECS_DEVADDR         0    /* Device Address */
#define OECS_ENDPOINT        7    /* Endpoint number */
#define OECS_DIRECTION      11    /* Direction */
#define OECS_MAXPKTLEN      16    /* MaxPacketLength */

#define OECF_LOWSPEED       (1UL<<OECB_LOWSPEED)
#define OECF_SKIP           (1UL<<OECB_SKIP)
#define OECF_ISO            (1UL<<OECB_ISO)

#define OECM_DEVADDR        (((1UL<<7)-1)<<OECS_DEVADDR)
#define OECM_ENDPOINT       (((1UL<<4)-1)<<OECS_ENDPOINT)
#define OECM_MAXPKTLEN      (((1UL<<11)-1)<<OECS_MAXPKTLEN)

#define OECM_DIRECTION      (((1UL<<2)-1)<<OECS_DIRECTION)
#define OECF_DIRECTION_TD   (0UL<<OECS_DIRECTION)
#define OECF_DIRECTION_OUT  (1UL<<OECS_DIRECTION)
#define OECF_DIRECTION_IN   (2UL<<OECS_DIRECTION)

/* ED HeadPtr defines */

#define OEHB_HALTED          0    /* TD Queue is halted */
#define OEHB_DATA1           1    /* Data 1 Toggle bit */

#define OEHF_HALTED         (1UL<<OEHB_HALTED)
#define OEHF_DATA1          (1UL<<OEHB_DATA1)

/* TD Ctrl defines */

#define OTCB_ALLOWSHORTPKT  18    /* Allow short packets */
#define OTCB_DATA1          24    /* Data 1 toggle bit */
#define OTCB_TOGGLEFROMTD   25    /* Data toggle comes from TD */

#define OTCS_PIDCODE        19    /* PID code */
#define OTCS_DELAYINT       21    /* Delay interrupt by given amount of frames */
#define OTCS_ERRORCOUNT     26    /* Number of errors occurred so far */
#define OTCS_COMPLETIONCODE 28    /* Error codes */

#define OTCF_ALLOWSHORTPKT  (1UL<<OTCB_ALLOWSHORTPKT)
#define OTCF_DATA0          (0UL<<OTCB_DATA1)
#define OTCF_DATA1          (1UL<<OTCB_DATA1)
#define OTCF_TOGGLEFROMTD   (1UL<<OTCB_TOGGLEFROMTD)

#define OTCM_PIDCODE        (((1UL<<2)-1)<<OTCS_PIDCODE)
#define OTCF_PIDCODE_SETUP  (0UL<<OTCS_PIDCODE)
#define OTCF_PIDCODE_OUT    (1UL<<OTCS_PIDCODE)
#define OTCF_PIDCODE_IN     (2UL<<OTCS_PIDCODE)

#define OTCM_DELAYINT       (((1UL<<3)-1)<<OTCS_DELAYINT)
#define OTCF_NOINT          (7UL<<OTCS_DELAYINT)

#define OTCM_ERRORCOUNT     (((1UL<<2)-1)<<OTCS_ERRORCOUNT)
#define OTCM_COMPLETIONCODE (((1UL<<4)-1)<<OTCS_COMPLETIONCODE)
#define OTCF_CC_NOERROR     (0UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_CRCERROR    (1UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_BABBLE      (2UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_WRONGTOGGLE (3UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_STALL       (4UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_TIMEOUT     (5UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_PIDCORRUPT  (6UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_WRONGPID    (7UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_OVERFLOW    (8UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_SHORTPKT    (9UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_OVERRUN     (12UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_UNDERRUN    (13UL<<OTCS_COMPLETIONCODE)
#define OTCF_CC_INVALID     (15UL<<OTCS_COMPLETIONCODE)

#endif /* OHCICHIP_H */
