#ifndef XHCICHIP_H
#define XHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for XHCI USB Controller
 *----------------------------------------------------------------------------
 */

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

#define XHCM_MaxPorts (((1UL<<8)-1)<<XHCB_MaxPorts)
#define XHCM_MaxIntrs (((1UL<<11)-1)<<XHCB_MaxIntrs)
#define XHCM_MaxSlots (((1UL<<8)-1)<<XHCB_MaxSlots)


/* XHCI_HCSPARAMS2 defines */
#define	XHCB_IST        0
#define	XHCB_ERST_Max   4
#define	XHCB_SPR        26
#define	XHCB_SPB_Max    27

#define	XHCM_IST        (((1UL<<4)-1)<<XHCB_IST)
#define	XHCM_ERST_Max   ((1UL<<4)-1)<<XHCB_ERST_Max)
#define	XHCF_SPR        (1UL<<XHCB_SPR)
#define	XHCM_SPB_Max    (((1UL<<4)-1)<<XHCB_SPB_Max)


/* XHCI_HCSPARAMS3 defines */
#define	XHCB_U1DEV_LAT  0
#define	XHCB_U2DEV_LAT  16

#define	XHCM_U1DEV_LAT  (((1UL<<8)-1)<<XHCB_U1DEV_LAT)
#define	XHCM_U2DEV_LAT  (((1UL<<8)-1)<<XHCB_U2DEV_LAT)


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
#define XHCI_xECP(p)    ((((p)&XHCM_xECP)>>XHCB_xECP)<<2)


/* Extended capability IDs */
#define XHCB_EXT_CAPS_ID        0
#define XHCB_EXT_CAPS_NEXT      8
#define	XHCB_EXT_CAPS_VALUE     16

#define XHCM_EXT_CAPS_ID        (((1UL<<8)-1)<<XHCB_EXT_CAPS_ID)
#define XHCM_EXT_CAPS_NEXT      (((1UL<<8)-1)<<XHCB_EXT_CAPS_NEXT)
#define	XHCM_EXT_CAPS_VALUE     (((1UL<<16)-1)<<XHCB_EXT_CAPS_VALUE)

#define XHCI_EXT_CAPS_ID(p)     (((p)&XHCM_EXT_CAPS_ID)>>XHCB_EXT_CAPS_ID)
#define XHCI_EXT_CAPS_NEXT(p)	((((p)&XHCM_EXT_CAPS_NEXT)>>XHCB_EXT_CAPS_NEXT)<<2)
#define	XHCI_EXT_CAPS_VALUE(p)  (((p)&XHCM_EXT_CAPS_VALUE)>>XHCB_EXT_CAPS_VALUE)

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
#define	XHCF_STS_EINT   (1UL<<XHCB_STS_EINT
#define	XHCF_STS_PCD    (1UL<<XHCB_STS_PCD)
#define	XHCF_STS_SSS    (1UL<<XHCB_STS_SSS)
#define	XHCF_STS_RSS    (1UL<<XHCB_STS_RSS)
#define	XHCF_STS_SRE    (1UL<<XHCB_STS_SRE)
#define	XHCF_STS_CNR    (1UL<<XHCB_STS_CNR)
#define	XHCF_STS_HCE    (1UL<<XHCB_STS_HCE)


/* USB Legacy Support Capability */
#define XHCB_HC_BIOS_OWNED      16
#define XHCB_HC_OS_OWNED        24

#define XHCF_HC_BIOS_OWNED      (1UL<<XHCB_HC_BIOS_OWNED)
#define XHCF_HC_OS_OWNED        (1UL<<XHCB_HC_OS_OWNED)

#endif /* XHCICHIP_H */
