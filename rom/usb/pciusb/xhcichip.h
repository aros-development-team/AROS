#ifndef XHCICHIP_H
#define XHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for XHCI USB Controller
 *----------------------------------------------------------------------------
 */

/*
    XHCI_xxx's are offsets to something
    XHCB_xxx's are bitnumbers
    XHCF_xxx's are flags
    XHCM_xxx's are bitmasks
    XHCV_xxx(p)'s return shifted values from p
*/

#define opreg_readl(opreg) READREG32_LE(hc->xhc_opregbase, opreg)
#define opreg_writel(opreg, value) WRITEREG32_LE(hc->xhc_opregbase, opreg, value)
#define opreg_writeq(opreg, value) WRITEREG64_LE(hc->xhc_opregbase, opreg, value)

#define capreg_readl(capreg) READREG32_LE(hc->xhc_capregbase, capreg)
#define capreg_readw(capreg) READREG16_LE(hc->xhc_capregbase, capreg)
#define capreg_readb(capreg) (*((volatile UBYTE *) (((UBYTE *) (hc->xhc_capregbase)) + ((ULONG) (capreg))))) 


/* XHCI capability register defines */
#define XHCI_CAPLENGTH  0x00
#define XHCI_HCIVERSION 0x02 
#define XHCI_HCSPARAMS1 0x04
#define XHCI_HCSPARAMS2 0x08
#define XHCI_HCSPARAMS3 0x0C
#define XHCI_HCCPARAMS  0x10
#define XHCI_DBOFF      0x14
#define XHCI_RTSOFF     0x18


/* XHCI_HCSPARAMS1 defines */
#define XHCB_MaxSlots   0
#define XHCB_MaxIntrs   8
#define XHCB_MaxPorts   24

#define XHCM_MaxSlots (((1UL<<8)-1)<<XHCB_MaxSlots)
#define XHCM_MaxIntrs (((1UL<<11)-1)<<XHCB_MaxIntrs)
#define XHCM_MaxPorts (((1UL<<8)-1)<<XHCB_MaxPorts)

#define XHCV_MaxPorts(p)    (((p)&XHCM_MaxPorts)>>XHCB_MaxPorts)
#define XHCV_MaxIntrs(p)    (((p)&XHCM_MaxIntrs)>>XHCB_MaxIntrs)
#define XHCV_MaxSlots(p)    (((p)&XHCM_MaxSlots)>>XHCB_MaxSlots)


/* XHCI_HCSPARAMS2 defines */
#define	XHCB_IST        0
#define	XHCB_ERST_Max   4
#define	XHCB_SPR        26
#define	XHCB_SPB_Max    27

#define	XHCM_IST        (((1UL<<4)-1)<<XHCB_IST)
#define	XHCM_ERST_Max   ((1UL<<4)-1)<<XHCB_ERST_Max)
#define	XHCF_SPR        (1UL<<XHCB_SPR)
#define	XHCM_SPB_Max    (((1UL<<4)-1)<<XHCB_SPB_Max)
#define	XHCV_SPB_Max(p) (((p)&XHCM_SPB_Max)>>XHCB_SPB_Max)

/* XHCI_HCSPARAMS3 defines */
#define	XHCB_U1DEV_LAT  0
#define	XHCB_U2DEV_LAT  16

#define	XHCM_U1DEV_LAT  (((1UL<<8)-1)<<XHCB_U1DEV_LAT)
#define	XHCM_U2DEV_LAT  (((1UL<<16)-1)<<XHCB_U2DEV_LAT)


/* XHCI_HCCPARAMS defines */
#define XHCB_AC64       0
#define XHCB_BNC        1
#define XHCB_CSZ        2
#define XHCB_PPC        3
#define XHCB_PIND       4
#define XHCB_LHRC       5
#define XHCB_LTC        6
#define XHCB_NSS        7
#define XHCB_MaxPSASize 12
#define XHCB_xECP       16

#define XHCF_AC64       (1UL<<XHCB_AC64)
#define XHCF_BNC        (1UL<<XHCB_BNC)
#define XHCF_CSZ        (1UL<<XHCB_CSZ)
#define XHCF_PPC        (1UL<<XHCB_PPC)
#define XHCF_PIND       (1UL<<XHCB_PIND)
#define XHCF_LHRC       (1UL<<XHCB_LHRC)
#define XHCF_LTC        (1UL<<XHCB_LTC)
#define XHCF_NSS        (1UL<<XHCB_NSS)
#define XHCM_MaxPSASize (((1UL<<4)-1)<<XHCB_MaxPSASize)
#define XHCM_xECP       (((1UL<<16)-1)<<XHCB_xECP)
#define XHCV_xECP(p)    ((((p)&XHCM_xECP)>>XHCB_xECP)<<2)


/* Extended capability IDs */
#define XHCB_EXT_CAPS_ID        0
#define XHCB_EXT_CAPS_NEXT      8
#define	XHCB_EXT_CAPS_VALUE     16

#define XHCM_EXT_CAPS_ID        (((1UL<<8)-1)<<XHCB_EXT_CAPS_ID)
#define XHCM_EXT_CAPS_NEXT      (((1UL<<8)-1)<<XHCB_EXT_CAPS_NEXT)
#define	XHCM_EXT_CAPS_VALUE     (((1UL<<16)-1)<<XHCB_EXT_CAPS_VALUE)

#define XHCV_EXT_CAPS_ID(p)     (((p)&XHCM_EXT_CAPS_ID)>>XHCB_EXT_CAPS_ID)
#define XHCV_EXT_CAPS_NEXT(p)	((((p)&XHCM_EXT_CAPS_NEXT)>>XHCB_EXT_CAPS_NEXT)<<2)
#define	XHCV_EXT_CAPS_VALUE(p)  (((p)&XHCM_EXT_CAPS_VALUE)>>XHCB_EXT_CAPS_VALUE)


/* Reserved ID 0 */
#define XHCI_EXT_CAPS_LEGACY    1
#define XHCI_EXT_CAPS_PROTOCOL  2
#define XHCI_EXT_CAPS_XPOWERMNG 3
#define XHCI_EXT_CAPS_IOVIRT    4
#define XHCI_EXT_CAPS_MSGINTR   5
#define XHCI_EXT_CAPS_LOCALMEM  6
/* Reserved IDs 7-9 */
#define XHCI_EXT_CAPS_USBDEBUG  10
/* Reserved IDs 11-16 */
#define XHCI_EXT_CAPS_XMSGINTR  17
/* Reserved IDs 18-191 */
/* Vendor defined IDs 192-255 */
#define XHCI_EXT_CAPS_MAX       255


/* These are for XHCI_EXT_CAPS_LEGACY */
/* USB Legacy Support Capability (USBLEGSUP) */
#define XHCI_USBLEGSUP   0x00
#define XHCB_BIOSOWNED   16
#define XHCB_OSOWNED     24
#define XHCF_BIOSOWNED  (1UL<<XHCB_BIOSOWNED)
#define XHCF_OSOWNED    (1UL<<XHCB_OSOWNED)

/* USB Legacy Support Control/Status (USBLEGCTLSTS) */
#define XHCI_USBLEGCTLSTS       0x04
#define XHCB_SMI_USBE           0
#define XHCB_SMI_HSEE           4
#define XHCB_SMI_OSOE           13
#define XHCB_SMI_PCICE          14
#define XHCB_SMI_BARE           15
#define XHCB_SMI_EI             16
#define XHCB_SMI_HSE            20
#define XHCB_SMI_OSOC           29
#define XHCB_SMI_PCIC           30
#define XHCB_SMI_BAR            31

#define XHCF_SMI_USBE           (1UL<<XHCB_SMI_USBE)
#define XHCF_SMI_HSEE           (1UL<<XHCB_SMI_HSEE)
#define XHCF_SMI_OSOE           (1UL<<XHCB_SMI_OSOE)
#define XHCF_SMI_PCICE          (1UL<<XHCB_SMI_PCICE)
#define XHCF_SMI_BARE           (1UL<<XHCB_SMI_BARE)
#define XHCF_SMI_EI             (1UL<<XHCB_SMI_EI)
#define XHCF_SMI_HSE            (1UL<<XHCB_SMI_HSE)
#define XHCF_SMI_OSOC           (1UL<<XHCB_SMI_OSOC)
#define XHCF_SMI_PCIC           (1UL<<XHCB_SMI_PCIC)
#define XHCF_SMI_BAR            (1UL<<XHCB_SMI_BAR)


/* These are for XHCI_EXT_CAPS_PROTOCOL */
/* xHCI Supported Protocol Capability Field Definitions */
#define XHCI_SPFD               0x00
#define XHCB_SPFD_RMINOR        16
#define XHCB_SPFD_RMAJOR        24

#define	XHCM_SPFD_RMINOR        (((1UL<<8)-1)<<XHCB_SPFD_RMINOR)
#define	XHCM_SPFD_RMAJOR        (((1UL<<8)-1)<<XHCB_SPFD_RMAJOR)

#define XHCV_SPFD_RMINOR(p)    (((p)&XHCM_SPFD_RMINOR)>>XHCB_SPFD_RMINOR)
#define XHCV_SPFD_RMAJOR(p)    (((p)&XHCM_SPFD_RMAJOR)>>XHCB_SPFD_RMAJOR)

#define XHCI_SPNAMESTRING       0x04

#define XHCI_SPPORT             0x08
#define XHCB_SPPORT_CPO         0       /* Compatible Port Offset */
#define XHCB_SPPORT_CPCNT       8       /* Compatible Port Count */
#define XHCB_SPPORT_PD          16      /* Protocol Defined */
#define XHCB_SPPORT_PSIC        28      /* Protocol Speed ID Count */

#define XHCM_SPPORT_CPO         (((1UL<<8)-1)<<XHCB_SPPORT_CPO)
#define XHCM_SPPORT_CPCNT       (((1UL<<8)-1)<<XHCB_SPPORT_CPCNT)
#define XHCM_SPPORT_PD          (((1UL<<12)-1)<<XHCB_SPPORT_PD)
#define XHCM_SPPORT_PSIC        (((1UL<<4)-1)<<XHCB_SPPORT_PSIC)

#define XHCV_SPPORT_CPO(p)      (((p)&XHCM_SPPORT_CPO)>>XHCB_SPPORT_CPO)
#define XHCV_SPPORT_CPCNT(p)    (((p)&XHCM_SPPORT_CPCNT)>>XHCB_SPPORT_CPCNT)
#define XHCV_SPPORT_PD(p)       (((p)&XHCM_SPPORT_PD)>>XHCB_SPPORT_PD)
#define XHCV_SPPORT_PSIC(p)     (((p)&XHCM_SPPORT_PSIC)>>XHCB_SPPORT_PSIC)

#define XHCI_SPPSI(psic) ((psic *4) + 0x10 )

/* XHCI operational register defines */
/* USB Command Register (USBCMD) */
#define	XHCI_USBCMD     0x00

#define	XHCB_CMD_RS     0
#define	XHCB_CMD_HCRST  1
#define	XHCB_CMD_INTE   2
#define	XHCB_CMD_HSEE   3
#define	XHCB_CMD_LHCRST 7
#define	XHCB_CMD_CSS    8
#define	XHCB_CMD_CRS    9
#define	XHCB_CMD_EWE    10
#define	XHCB_CMD_EU3S   11

#define	XHCF_CMD_RS     (1UL<<XHCB_CMD_RS)
#define	XHCF_CMD_HCRST  (1UL<<XHCB_CMD_HCRST)
#define	XHCF_CMD_INTE   (1UL<<XHCB_CMD_INTE)
#define	XHCF_CMD_HSEE   (1UL<<XHCB_CMD_HSEE)
#define	XHCF_CMD_LHCRST (1UL<<XHCB_CMD_LHCRST)
#define	XHCF_CMD_CSS    (1UL<<XHCB_CMD_CSS)
#define	XHCF_CMD_CRS    (1UL<<XHCB_CMD_CRS)
#define	XHCF_CMD_EWE    (1UL<<XHCB_CMD_EWE)
#define	XHCF_CMD_EU3S   (1UL<<XHCB_CMD_EU3S)

/* USB Status Register (USBSTS) */
#define	XHCI_USBSTS     0x04
#define	XHCB_STS_HCH    0
#define	XHCB_STS_HSE	2
#define	XHCB_STS_EINT   3
#define	XHCB_STS_PCD    4
#define	XHCB_STS_SSS    8
#define	XHCB_STS_RSS    9
#define	XHCB_STS_SRE    10
#define	XHCB_STS_CNR    11
#define	XHCB_STS_HCE    12

#define	XHCF_STS_HCH    (1UL<<XHCB_STS_HCH)
#define	XHCF_STS_HSE    (1UL<<XHCB_STS_HSE)
#define	XHCF_STS_EINT   (1UL<<XHCB_STS_EINT)
#define	XHCF_STS_PCD    (1UL<<XHCB_STS_PCD)
#define	XHCF_STS_SSS    (1UL<<XHCB_STS_SSS)
#define	XHCF_STS_RSS    (1UL<<XHCB_STS_RSS)
#define	XHCF_STS_SRE    (1UL<<XHCB_STS_SRE)
#define	XHCF_STS_CNR    (1UL<<XHCB_STS_CNR)
#define	XHCF_STS_HCE    (1UL<<XHCB_STS_HCE)

/* Page Size Register (PAGESIZE) */
#define	XHCI_PAGESIZE   0x08

/* Device Notification Control Register (DNCTRL) */
#define	XHCI_DNCTRL     0x14

/* Command Ring Control Register (CRCR) */
#define XHCI_CRCR       0x18

/* Device Context Base Address Array Pointer Register (DCBAAP) */
#define XHCI_DCBAAP     0x30

/* Configure Register (CONFIG) */
#define XHCI_CONFIG     0x38
#define	XHCB_CONFIG_MaxSlotsEn  0
#define	XHCM_CONFIG_MaxSlotsEn  (((1UL<<8)-1)<<XHCB_CONFIG_MaxSlotsEn)

/* Port Status and Control Register (PORTSC) */
#define XHCI_PORTSC(port) (0x400 + (0x10 * (port-1)))

#define	XHCB_PS_CCS     0
#define	XHCB_PS_PED     1
#define	XHCB_PS_OCA     3
#define	XHCB_PS_PR      4
#define	XHCB_PS_PLS     5
#define	XHCB_PS_PP      9
#define	XHCB_PS_SPEED   10
#define	XHCB_PS_PIC     14
#define	XHCB_PS_LWS     16
#define	XHCB_PS_CSC     17
#define	XHCB_PS_PEC     18
#define	XHCB_PS_WRC     19
#define	XHCB_PS_OCC     20
#define	XHCB_PS_PRC     21
#define	XHCB_PS_PLC     22
#define	XHCB_PS_CEC     23
#define	XHCB_PS_CAS     24
#define	XHCB_PS_WCE     25
#define	XHCB_PS_WDE     26
#define	XHCB_PS_WOE     27
#define	XHCB_PS_DR      30
#define	XHCB_PS_WPR     31

#define	XHCF_PS_CCS     (1UL<<XHCB_PS_CCS)
#define	XHCF_PS_PED     (1UL<<XHCB_PS_PED)
#define	XHCF_PS_OCA     (1UL<<XHCB_PS_OCA)
#define	XHCF_PS_PR      (1UL<<XHCB_PS_PR)
#define	XHCM_PS_PLS     (((1UL<<4)-1)<<XHCB_PS_PLS)
#define	XHCF_PS_PP      (1UL<<XHCB_PS_PP)
#define	XHCM_PS_SPEED   (((1UL<<4)-1)<<XHCB_PS_SPEED)
#define	XHCM_PS_PIC     (((1UL<<2)-1)<<XHCB_PS_PIC)
#define	XHCF_PS_LWS     (1UL<<XHCB_PS_LWS)
#define	XHCF_PS_CSC     (1UL<<XHCB_PS_CSC)
#define	XHCF_PS_PEC     (1UL<<XHCB_PS_PEC)
#define	XHCF_PS_WRC     (1UL<<XHCB_PS_WRC)
#define	XHCF_PS_OCC     (1UL<<XHCB_PS_OCC)
#define	XHCF_PS_PRC     (1UL<<XHCB_PS_PRC)
#define	XHCF_PS_PLC     (1UL<<XHCB_PS_PLC)
#define	XHCF_PS_CEC     (1UL<<XHCB_PS_CEC)
#define	XHCF_PS_CAS     (1UL<<XHCB_PS_CAS)
#define	XHCF_PS_WCE     (1UL<<XHCB_PS_WCE)
#define	XHCF_PS_WDE     (1UL<<XHCB_PS_WDE)
#define	XHCF_PS_WOE     (1UL<<XHCB_PS_WOE)
#define	XHCF_PS_DR      (1UL<<XHCB_PS_DR)
#define	XHCF_PS_WPR     (1UL<<XHCB_PS_WPR)

#define XHCV_PS_SPEED(p)    (((p)&XHCM_PS_SPEED)>>XHCB_PS_SPEED)

#endif /* XHCICHIP_H */
