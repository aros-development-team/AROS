#ifndef XHCICHIP_H
#define XHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for XHCI USB Controller
 *----------------------------------------------------------------------------
 */

#define XHCI_CAPLENGTH  0x00
#define XHCI_HCIVERSION 0x02 
#define XHCI_HCSPARAMS1 0x04
#define XHCI_HCSPARAMS2 0x08
#define XHCI_HCSPARAMS3 0x0C
#define XHCI_HCCPARAMS  0x10
#define XHCI_DBOFF      0x14
#define XHCI_RTSOFF     0x18


/* Extended capability IDs */
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


/* XHCI_HCSPARAMS1 defines */
#define XHCB_MaxSlots   0
#define XHCB_MaxIntrs   8
#define XHCB_MaxPorts   24

#define XHCM_MaxPorts (((1UL<<8)-1)<<XHCB_MaxPorts)
#define XHCM_MaxIntrs (((1UL<<11)-1)<<XHCB_MaxIntrs)
#define XHCM_MaxSlots (((1UL<<8)-1)<<XHCB_MaxSlots)


/* XHCI_HCCPARAMS defines */
#define XHCB_AC64       0
#define XHCB_BNC        1
#define XHCB_CSZ        2
#define XHCB_PPC        3
#define XHCB_PIND       4
#define XHCB_LHRC       5
#define XHCB_LTC        6
#define XHCB_NSS        7
#define XHCS_MaxPSASize 12
#define XHCS_xECP       16

#define XHCF_AC64       (1UL<<XHCB_AC64)
#define XHCF_BNC        (1UL<<XHCB_BNC)
#define XHCF_CSZ        (1UL<<XHCB_CSZ)
#define XHCF_PPC        (1UL<<XHCB_PPC)
#define XHCF_PIND       (1UL<<XHCB_PIND)
#define XHCF_LHRC       (1UL<<XHCB_LHRC)
#define XHCF_LTC        (1UL<<XHCB_LTC)
#define XHCF_NSS        (1UL<<XHCB_NSS)
#define XHCM_MaxPSASize (((1UL<<4)-1)<<XHCS_MaxPSASize)
#define XHCM_xECP       (((1UL<<16)-1)<<XHCS_xECP)

#endif /* XHCICHIP_H */
