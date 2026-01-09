#ifndef UHCICHIP_H
#define UHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for UHCI USB Controller
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include <exec/types.h>
#include <hardware/usb/uhci.h>
#include <devices/usbhardware.h>
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

struct UhciXX
{
    struct UhciXX               *uxx_Succ;
    struct UhciXX               *uxx_Pred;
    ULONG                       uxx_Self;           /* LE PHYSICAL pointer to self + UHCI_TDSELECT */
#if __WORDSIZE == 64
    ULONG                       uxx_Unused0;
#endif
    APTR                        uxx_Private;
    /* aligned to 16 bytes */
    ULONG                       uxx_Link;           /* LE PHYSICAL link pointer */
};

struct UhciTD
{
    struct UhciXX               *utd_Succ;
    IPTR                        utd_Unused0;
    ULONG                       utd_Self;           /* LE PHYSICAL pointer to self + UHCI_TDSELECT */
#if __WORDSIZE == 64
    ULONG                       utd_Unused2;
#endif
    IPTR                        utd_Unused1;
    /* aligned to 16 bytes */
    ULONG                       utd_Link;           /* LE PHYSICAL TD Link Pointer (+BFS/DFS+QH/TD+TERM) */
    ULONG                       utd_CtrlStatus;     /* LE Control and Status word */
    ULONG                       utd_Token;          /* LE Token (Transfer length) */
    ULONG                       utd_BufferPtr;      /* LE PHYSICAL Data Buffer */
};

struct UhciQH
{
    struct UhciXX               *uqh_Succ;
    struct UhciXX               *uqh_Pred;
    ULONG                       uqh_Self;           /* LE PHYSICAL pointer to self + UHCI_QHSELECT */
#if __WORDSIZE == 64
    ULONG                       utd_Unused3;
#endif    
    struct IOUsbHWReq           *uqh_IOReq;         /* IO Request this belongs to */
    /* aligned to 16 bytes */
    ULONG                       uqh_Link;           /* LE PHYSICAL QH Link Pointer (QH/TD+TERM) */
    ULONG                       uqh_Element;        /* LE PHYSICAL Queue Element Link Pointer (QH/TD+TERM) */
    struct UhciTD               *uqh_FirstTD;       /* First TD */
    ULONG                       uqh_Actual;         /* Number of bytes for successful completion in this QH */
    APTR                        uqh_SetupBuffer;    /* Bounce buffer */
    APTR                        uqh_DataBuffer;     /* Bounce buffer */
    ULONG                       uqh_Unused1;        /* Make sure size of structure is aligned to 16 bytes */
    ULONG                       uqh_Unused2;        /* Make sure size of structure is aligned to 16 bytes */
};

struct UhciHCPrivate
{
    ULONG                       *uhc_UhciFrameList;
    struct UhciQH               *uhc_UhciQHPool;
    struct UhciTD               *uhc_UhciTDPool;

    struct PTDNode             **uhc_IsoHead;
    struct PTDNode             **uhc_IsoTail;

    struct UhciQH               *uhc_UhciCtrlQH;
    struct UhciQH               *uhc_UhciBulkQH;
    struct UhciQH               *uhc_UhciIntQH[9];
    struct UhciTD               *uhc_UhciIsoTD;
    struct UhciQH               *uhc_UhciTermQH;
};

struct UhciPTDPrivate
{
    ULONG                       ptd_NextPhys;
    struct IOUsbHWBufferReq     ptd_BufferReq;
    APTR                        ptd_BounceBuffer;
};

#define HCQB_UHCI_VIA_BABBLE    5
#define HCQ_UHCI_VIA_BABBLE     (1 << HCQB_UHCI_VIA_BABBLE)


#endif /* UHCICHIP_H */
