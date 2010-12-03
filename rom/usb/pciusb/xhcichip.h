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


/* USB Legacy Support Capability */
#define XHCB_HC_BIOS_OWNED      16
#define XHCB_HC_OS_OWNED        24

#define XHCF_HC_BIOS_OWNED      (1UL<<XHCB_HC_BIOS_OWNED)
#define XHCF_HC_OS_OWNED        (1UL<<XHCB_HC_OS_OWNED)

#endif /* XHCICHIP_H */
