/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#ifndef PCIEHCI_HC_H
#define PCIEHCI_HC_H

/*
 *----------------------------------------------------------------------------
 *             Includes for EHCI USB Controller
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#undef SYNC
#ifdef __powerpc__
#define SYNC asm volatile("eieio");
#else
#define SYNC
#endif

#define READMEM16_LE(rb) AROS_WORD2LE(*((volatile UWORD *) (rb)))
#define READMEM32_LE(rb) AROS_LONG2LE(*((volatile ULONG *) (rb)))
#define	WRITEMEM32_LE(adr, value)	   *((volatile ULONG *) (adr)) = AROS_LONG2LE(value)
#define CONSTWRITEMEM32_LE(adr, value) *((volatile ULONG *) (adr)) = AROS_LONG2LE(value)

#define CONSTWRITEREG16_LE(rb, offset, value) *((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_WORD2LE(value)
#define CONSTWRITEREG32_LE(rb, offset, value) *((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_LONG2LE(value)
#define WRITEREG16_LE(rb, offset, value)      *((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_WORD2LE(value)
#define WRITEREG32_LE(rb, offset, value)      *((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_LONG2LE(value)
#define WRITEREG64_LE(rb, offset, value)      *((volatile UQUAD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))) = AROS_QUAD2LE(value)

#define READREG16_LE(rb, offset) AROS_WORD2LE(*((volatile UWORD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))))
#define READREG32_LE(rb, offset) AROS_LONG2LE(*((volatile ULONG *) (((UBYTE *) (rb)) + ((ULONG) (offset)))))
#define READREG64_LE(rb, offset) AROS_QUAD2LE(*((volatile UQUAD *) (((UBYTE *) (rb)) + ((ULONG) (offset)))))

#define READIO16_LE(rb, offset) AROS_WORD2LE(WORDIN((((UBYTE *) (rb)) + ((ULONG) (offset)))))
#define WRITEIO16_LE(rb, offset, value) WORDOUT((((UBYTE *) (rb)) + ((ULONG) (offset))), AROS_WORD2LE(value))
#define WRITEIO32_LE(rb, offset, value) LONGOUT((((UBYTE *) (rb)) + ((ULONG) (offset))), AROS_WORD2LE(value))

/* Macros to swap bit defines, constants & variables */
#define SWB(x) ((x+16) & 31)
#define SWC(x) ((x>>16)|((x & 0xffff)<<16))
#define SWW(x) ((x>>16)|(x<<16))
#define L2U(x) (x<<16)
#define U2L(x) (x>>16)

#define PID_IN              0x69
#define PID_OUT             0xe1
#define PID_SETUP           0x2d

/*
 * --------------------- EHCI registers ------------------------
 * Warning: These are BYTE offsets!
 */

#define EHCI_CAPLENGTH      0x000 /* Offset for operational registers */
#define EHCI_HCIVERSION     0x002 /* HC Version Number */
#define EHCI_HCSPARAMS      0x004 /* HC Structural Parameters */
#define EHCI_HCCPARAMS      0x008 /* HC Capability Parameters */
#define EHCI_HCSPPORTROUTE  0x00c /* HC Companion Port Route Description */

/* EHCI_HCSPARAMS defines */
#define EHSB_PORTPOWERCTRL   4    /* Support for Port Power Control */
#define EHSB_EXTPORTROUTING  7    /* Routing to companion ports via HCSSPORTROUTE array */
#define EHSB_PORTINDICATORS 16    /* Support for Port Indicators */

#define EHSS_NUM_PORTS       0    /* Number of ports */
#define EHSS_PORTS_PER_COMP  8    /* Ports per companion controller */
#define EHSS_NUM_COMPANIONS 12    /* Number of companion controllers */

#define EHSF_PORTPOWERCTRL  (1UL<<EHSB_PORTPOWERCTRL)
#define EHSF_EXTPORTROUTING (1UL<<EHSB_EXTPORTROUTING)
#define EHSF_PORTINDICATORS (1UL<<EHSB_PORTINDICATORS)

#define EHSM_NUM_PORTS      (((1UL<<4)-1)<<EHSS_NUM_PORTS)
#define EHSM_PORTS_PER_COMP (((1UL<<4)-1)<<EHSS_PORTS_PER_COMP)
#define EHSM_NUM_COMPANIONS (((1UL<<4)-1)<<EHSS_NUM_COMPANIONS)

/* EHCI_HCCPARAMS defines */
#define EHCB_64BITS          0    /* Use 64 Bit pointers and structures */
#define EHCB_PROGFRAMELIST   1    /* Programmable Frame list size */
#define EHCB_ASYNCSCHEDPARK  2    /* Park feature for highspeed QH supported */
#define EHCS_EXTCAPOFFSET    8    /* Offset to extended capabilities registers */

#define EHCF_64BITS         (1UL<<EHCB_64BITS)
#define EHCF_PROGFRAMELIST  (1UL<<EHCB_PROGFRAMELIST)
#define EHCF_ASYNCSCHEDPARK (1UL<<EHCB_ASYNCSCHEDPARK)
#define EHCM_EXTCAPOFFSET   (((1UL<<8)-1)<<EHCS_EXTCAPOFFSET)

/* Operational Registers */
#define EHCI_USBCMD         0x000 /* USB Command (r/w) */
#define EHCI_USBSTATUS      0x004 /* USB Status (r/wc) */
#define EHCI_USBINTEN       0x008 /* USB Interrupt enable (r/w) */
#define EHCI_FRAMECOUNT     0x00c /* Frame Number (r/w) */
#define EHCI_CTRLDSSEGMENT  0x010 /* Upper 32 bits in 64 Bit mode */
#define EHCI_PERIODICLIST   0x014 /* Periodic Frame List Base Address Register (4K aligned) */
#define EHCI_ASYNCADDR      0x018 /* Asynchronous List Address Register (32 byte aligned) */
#define EHCI_CONFIGFLAG     0x040 /* Configure flag (r/w) */
#define EHCI_PORTSC1        0x044 /* Port Status & Control 1 (r/w) */

/* EHCI_USBCMD defines */
#define EHUB_RUNSTOP         0    /* 1=Run, 0=Stop */
#define EHUB_HCRESET         1    /* Host Controller Reset */
#define EHUB_PERIODICENABLE  4    /* Enable Periodic Schedule */
#define EHUB_ASYNCENABLE     5    /* Enable Async Schedule */
#define EHUB_ASYNCDOORBELL   6    /* Cause interrupt on next Async schedule advance */
#define EHUB_LIGHTHCRESET    7    /* Light Host Controller Reset */
#define EHUB_ASYNCSCHEDPARK 11    /* Park Asynchroneous schedule */

#define EHUS_FRAMELISTSIZE   2    /* Size of framelist (divisor) */
#define EHUS_ASYNCPARKCOUNT  8    /* Number of successive transactions before continuing async */
#define EHUS_INTTHRESHOLD   16    /* Interrupt threshold control */

#define EHUF_RUNSTOP        (1UL<<EHUB_RUNSTOP)
#define EHUF_HCRESET        (1UL<<EHUB_HCRESET)
#define EHUF_PERIODICENABLE (1UL<<EHUB_PERIODICENABLE)
#define EHUF_ASYNCENABLE    (1UL<<EHUB_ASYNCENABLE)
#define EHUF_ASYNCDOORBELL  (1UL<<EHUB_ASYNCDOORBELL)
#define EHUF_LIGHTHCRESET   (1UL<<EHUB_LIGHTHCRESET)
#define EHUF_ASYNCSCHEDPARK (1UL<<EHUB_ASYNCSCHEDPARK)

#define EHUM_FRAMELISTSIZE  (((1UL<<2)-1)<<EHUS_FRAMELISTSIZE)
#define EHUM_ASYNCPARKCOUNT (((1UL<<2)-1)<<EHUS_ASYNCPARKCOUNT)
#define EHUM_INTTHRESHOLD   (((1UL<<8)-1)<<EHUS_INTTHRESHOLD)

/* EHCI_USBSTS and EHCI_USBINTEN (0-5) defines */
#define EHSB_TDDONE          0    /* Transfer descriptor done */
#define EHSB_TDERROR         1    /* Some TD has errored */
#define EHSB_PORTCHANGED     2    /* Port Change detected */
#define EHSB_FRAMECOUNTOVER  3    /* Frame List Rollover */
#define EHSB_HOSTERROR       4    /* Host System Error */
#define EHSB_ASYNCADVANCE    5    /* Async Schedule has advanced */
#define EHSB_HCHALTED       12    /* Host controller halted */
#define EHSB_RECLAMATION    13    /* Empty asynchrous schedule */
#define EHSB_PERIODICACTIVE 14    /* Periodic schedule is running */
#define EHSB_ASYNCACTIVE    15    /* Async schedule is running */

#define EHSF_TDDONE         (1UL<<EHSB_TDDONE)
#define EHSF_TDERROR        (1UL<<EHSB_TDERROR)
#define EHSF_PORTCHANGED    (1UL<<EHSB_PORTCHANGED)
#define EHSF_FRAMECOUNTOVER (1UL<<EHSB_FRAMECOUNTOVER)
#define EHSF_HOSTERROR      (1UL<<EHSB_HOSTERROR)
#define EHSF_ASYNCADVANCE   (1UL<<EHSB_ASYNCADVANCE)
#define EHSF_HCHALTED       (1UL<<EHSB_HCHALTED)
#define EHSF_RECLAMATION    (1UL<<EHSB_RECLAMATION)
#define EHSF_PERIODICACTIVE (1UL<<EHSB_PERIODICACTIVE)
#define EHSF_ASYNCACTIVE    (1UL<<EHSB_ASYNCACTIVE)

#define EHSF_ALL_INTS       (EHSF_TDDONE|EHSF_TDERROR|EHSF_PORTCHANGED|EHSF_FRAMECOUNTOVER|EHSF_HOSTERROR|EHSF_ASYNCADVANCE)

/* EHCI_CONFIGFLAG defines */
#define EHCB_CONFIGURED      0
#define EHCF_CONFIGURED     (1UL<<EHCB_CONFIGURED)

/* EHCI_PORTSC defines */
#define EHPB_PORTCONNECTED   0    /* Port Connection status */
#define EHPB_CONNECTCHANGE   1    /* Port Connection change */
#define EHPB_PORTENABLE      2    /* Enable Port */
#define EHPB_ENABLECHANGE    3    /* Port eanbled status changed */
#define EHPB_OVERCURRENT     4    /* OVer current condition detected */
#define EHPB_OVERCURRENTCHG  5    /* Over current condition changed */
#define EHPB_RESUMEDTX       6    /* Resume detected */
#define EHPB_PORTSUSPEND     7    /* Port is suspended */
#define EHPB_PORTRESET       8    /* Port is in reset state */
#define EHPB_LINESTATUS_DM  10    /* Line Status D- */
#define EHPB_LINESTATUS_DP  11    /* Line Stauts D+ */
#define EHPB_PORTPOWER      12    /* Depends on PortPowerControl */
#define EHPB_NOTPORTOWNER   13    /* Inverse of CONFIGURED (0=Owner) */
#define EHPB_WAKECONNECT    20    /* Wake on Connect */
#define EHPB_WAKEDISCONNECT 21    /* Wake on Disconnect */
#define EHPB_WAKEOCENABLE   22    /* Wake on Over Current Condition */

#define EHPS_PORTINDICATOR  14    /* Port indicator leds (0=off, 1=amber, 2=green) */

#define EHPF_PORTCONNECTED  (1UL<<EHPB_PORTCONNECTED)
#define EHPF_CONNECTCHANGE  (1UL<<EHPB_CONNECTCHANGE)
#define EHPF_PORTENABLE     (1UL<<EHPB_PORTENABLE)
#define EHPF_ENABLECHANGE   (1UL<<EHPB_ENABLECHANGE)
#define EHPF_OVERCURRENT    (1UL<<EHPB_OVERCURRENT)
#define EHPF_OVERCURRENTCHG (1UL<<EHPB_OVERCURRENTCHG)
#define EHPF_RESUMEDTX      (1UL<<EHPB_RESUMEDTX)
#define EHPF_PORTSUSPEND    (1UL<<EHPB_PORTSUSPEND)
#define EHPF_PORTRESET      (1UL<<EHPB_PORTRESET)
#define EHPF_LINESTATUS_DM  (1UL<<EHPB_LINESTATUS_DM)
#define EHPF_LINESTATUS_DP  (1UL<<EHPB_LINESTATUS_DP)
#define EHPF_PORTPOWER      (1UL<<EHPB_PORTPOWER)
#define EHPF_NOTPORTOWNER   (1UL<<EHPB_NOTPORTOWNER)
#define EHPF_WAKECONNECT    (1UL<<EHPB_WAKECONNECT)
#define EHPF_WAKEDISCONNECT (1UL<<EHPB_WAKEDISCONNECT)
#define EHPF_WAKEOCENABLE   (1UL<<EHPB_WAKEOCENABLE)

#define EHPM_PORTINDICATOR  (((1UL<<2)-1)<<EHPS_PORTINDICATOR)
#define EHPF_PORTIND_OFF    (0UL<<EHPS_PORTINDICATOR)
#define EHPF_PORTIND_AMBER  (1UL<<EHPS_PORTINDICATOR)
#define EHPF_PORTIND_GREEN  (2UL<<EHPS_PORTINDICATOR)

/* Legacy support register */
#define EHLS_CAP_ID          0
#define EHLB_BIOS_OWNER     16
#define EHLB_OS_OWNER       24

#define EHLM_CAP_ID         (((1UL<<8)-1)<<EHLS_CAP_ID)
#define EHLF_BIOS_OWNER     (1UL<<EHLB_BIOS_OWNER)
#define EHLF_OS_OWNER       (1UL<<EHLB_OS_OWNER)

/* Legacy support control / status */

/* data structures */

#define EHCI_FRAMELIST_SIZE      1024
#define EHCI_FRAMELIST_ALIGNMENT 0x0fff

#define EHCI_PAGE_SIZE           4096
#define EHCI_PAGE_MASK           0xfffff000
#define EHCI_OFFSET_MASK         0x00000fff

#define EHCI_TDQH_ALIGNMENT      0x001f

#define EHCI_QH_POOLSIZE         128
#define EHCI_TD_POOLSIZE         512

#define EHCI_TD_BULK_LIMIT       (128<<10) // limit for one batch of BULK data TDs

/* pointer defines */

#define EHCI_PTRMASK        0xffffffe0 /* frame list pointer mask */
#define EHCI_TERMINATE      0x00000001 /* terminate list here */
#define EHCI_ISOTD          0x00000000 /* isochronous TD */
#define EHCI_QUEUEHEAD      0x00000002 /* pointer is a queue head */
#define EHCI_SPLITISOTD     0x00000004 /* split transaction isochronous TD */
#define EHCI_FRAMESPAN      0x00000006 /* frame span traversal node */

/* TD control and status word defines */

#define ETSB_PING            0    /* PING state instead of OUT */
#define ETSB_SPLITERR        0    /* periodic split transaction error handshake */
#define ETSB_COMPLETESPLIT   1    /* In complete-split state */
#define ETSB_MISSEDCSPLIT    2    /* Missed Micro-frame for complete-split */
#define ETSB_TRANSERR        3    /* Transaction error (Timeout, CRC, PID) */
#define ETSB_BABBLE          4    /* Babble detected on the bus */
#define ETSB_DATABUFFERERR   5    /* Data Buffer Error (Overrun / Underrun) */
#define ETSB_HALTED          6    /* TD has been halted */
#define ETCB_ACTIVE          7    /* TD is active / enable TD */
#define ETCB_READYINTEN     15    /* Interrupt on Complete enable */
#define ETCB_DATA1          31    /* Data toggle bit */

#define ETCS_PIDCODE         8    /* PID code */
#define ETCS_ERRORLIMIT     10    /* how many errors permitted */
#define ETSS_CURRENTPAGE    12    /* current page offset */
#define ETSS_TRANSLENGTH    16    /* bytes to transfer */

#define ETSF_PING           (1UL<<ETSB_PING)
#define ETSF_SPLITERR       (1UL<<ETSB_SPLITERR)
#define ETSF_COMPLETESPLIT  (1UL<<ETSB_COMPLETESPLIT)
#define ETSF_MISSEDCSPLIT   (1UL<<ETSB_MISSEDCSPLIT)
#define ETSF_TRANSERR       (1UL<<ETSB_TRANSERR)
#define ETSF_BABBLE         (1UL<<ETSB_BABBLE)
#define ETSF_DATABUFFERERR  (1UL<<ETSB_DATABUFFERERR)
#define ETSF_HALTED         (1UL<<ETSB_HALTED)
#define ETCF_ACTIVE         (1UL<<ETCB_ACTIVE)
#define ETCF_READYINTEN     (1UL<<ETCB_READYINTEN)
#define ETCF_DATA1          (1UL<<ETCB_DATA1)

#define ETCM_PIDCODE        (((1UL<<2)-1)<<ETCS_PIDCODE)
#define ETCF_PIDCODE_OUT    (0UL<<ETCS_PIDCODE)
#define ETCF_PIDCODE_IN     (1UL<<ETCS_PIDCODE)
#define ETCF_PIDCODE_SETUP  (2UL<<ETCS_PIDCODE)

#define ETCM_ERRORLIMIT     (((1UL<<2)-1)<<ETCS_ERRORLIMIT)
#define ETCF_NOERRORLIMIT   (0UL<<ETCS_ERRORLIMIT)
#define ETCF_1ERRORLIMIT    (1UL<<ETCS_ERRORLIMIT)
#define ETCF_2ERRORSLIMIT   (2UL<<ETCS_ERRORLIMIT)
#define ETCF_3ERRORSLIMIT   (3UL<<ETCS_ERRORLIMIT)

#define ETSM_CURRENTPAGE    (((1UL<<3)-1)<<ETSS_CURRENTPAGE)
#define ETSM_TRANSLENGTH    (((1UL<<15)-1)<<ETSS_TRANSLENGTH)

/* QH EP Capabilitities */

#define EQEB_INACTIVATENEXT  7    /* Inactivate on next transaction for periodic schedule (FS/LS) */
#define EQEB_LOWSPEED       12    /* Lowspeed transaction */
#define EQEB_HIGHSPEED      13    /* Highspeed transaction */
#define EQEB_TOGGLEFROMTD   14    /* Data toggle comes from TD */
#define EQEB_RECLAMHEAD     15    /* Head Of Reclamation List Flag */
#define EQEB_SPLITCTRLEP    27    /* For Fullspeed/Lowspeed, signal Control Endpoint */

#define EQES_DEVADDR         0    /* Device Address */
#define EQES_ENDPOINT        8    /* Endpoint number */
#define EQES_MAXPKTLEN      16    /* Maximum Packet Length */
#define EQES_RELOAD         28    /* 0=Ignore NAKCOUNT, NAKCOUNT is loaded with this */

#define EQEF_INACTIVATENEXT (1UL<<EQEB_INACTIVATENEXT)
#define EQEF_LOWSPEED       (1UL<<EQEB_LOWSPEED)
#define EQEF_HIGHSPEED      (1UL<<EQEB_HIGHSPEED)
#define EQEF_TOGGLEFROMTD   (1UL<<EQEB_TOGGLEFROMTD)
#define EQEF_RECLAMHEAD     (1UL<<EQEB_RECLAMHEAD)
#define EQEF_SPLITCTRLEP    (1UL<<EQEB_SPLITCTRLEP)

#define EQEM_DEVADDR        (((1UL<<7)-1)<<EQES_DEVADDR)
#define EQEM_ENDPOINT       (((1UL<<4)-1)<<EQES_ENDPOINT)
#define EQEM_MAXPKTLEN      (((1UL<<11)-1)<<EQES_MAXPKTLEN)
#define EQEM_RELOAD         (((1UL<<4)-1)<<EQES_RELOAD)

/* QH Split Ctrl */
#define EQSS_MUSOFACTIVE     0    /* µSOF Active */
#define EQSS_MUSOFCSPLIT     8    /* When to send the complete split */

#define EQSS_HUBADDRESS     16    /* Hub Device Address for Split Transaction */
#define EQSS_PORTNUMBER     23    /* Port Number of hub for Split Transaction */
#define EQSS_MULTIPLIER     30    /* Multiplier, how many successive packets are sent */

#define EQSM_MUSOFACTIVE    (((1UL<<8)-1)<<EQSS_MUSOFACTIVE)
#define EQSM_MUSOFCSPLIT    (((1UL<<8)-1)<<EQSS_MUSOFCSPLIT)
#define EQSM_HUBADDRESS     (((1UL<<7)-1)<<EQSS_HUBADDRESS)
#define EQSM_PORTNUMBER     (((1UL<<7)-1)<<EQSS_PORTNUMBER)

#define EQSM_MULTIPLIER     (((1UL<<2)-1)<<EQSS_MULTIPLIER)
#define EQSF_MULTI_1        (1UL<<EQSS_MULTIPLIER)
#define EQSF_MULTI_2        (2UL<<EQSS_MULTIPLIER)
#define EQSF_MULTI_3        (3UL<<EQSS_MULTIPLIER)

#endif /* PCIEHCI_HC_H */
