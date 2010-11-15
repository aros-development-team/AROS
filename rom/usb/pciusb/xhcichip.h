#ifndef XHCICHIP_H
#define XHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for XHCI USB Controller
 *----------------------------------------------------------------------------
 */

#define XHCI_HCCPARAMS  0x10

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
