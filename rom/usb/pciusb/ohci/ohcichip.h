#ifndef OHCICHIP_H
#define OHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for OHCI USB Controller
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include <exec/types.h>
#include <hardware/usb/ohci.h>
#include <devices/usbhardware.h>
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

struct OhciED
{
    struct OhciED           *oed_Succ;
    struct OhciED           *oed_Pred;
    ULONG                   oed_Self;       /* LE PHYSICAL pointer to self */
    /* On 64 bits a padding will be inserted here */
    struct IOUsbHWReq       *oed_IOReq;   /* IO Request this belongs to */

    struct OhciTD           *oed_FirstTD;    /* First TD */
    IPTR                    oed_Continue;   /* Flag for fragmented bulk transfer */
    APTR	                oed_Buffer;	    /* Mirror buffer for data outside of DMA-accessible area */
    struct UsbSetupData     *oed_SetupData; /* Mirror buffer for setup packet */

    /* HC data, aligned to 16 bytes */
    ULONG                   oed_EPCaps;     /* LE MaxPacketSize and other stuff */
    ULONG                   oed_TailPtr;    /* LE PHYSICAL TD Queue Tail Pointer */
    ULONG                   oed_HeadPtr;    /* LE PHYSICAL TD Queue Head Pointer */
    ULONG                   oed_NextED;     /* LE PHYSICAL Next Endpoint Descriptor */
};

struct OhciTD
{
    struct OhciTD           *otd_Succ;
    IPTR                    otd_Length;     /* Length of transfer */
    ULONG                   otd_Self;       /* LE PHYSICAL pointer to self */
    /* On 64 bits a padding will be inserted here */
    struct OhciED           *otd_ED;         /* Pointer to parent ED this TD belongs to */

    /* HC data, aligned to 16 bytes */
    ULONG                   otd_Ctrl;       /* LE Ctrl stuff */
    ULONG                   otd_BufferPtr;  /* LE PHYSICAL Current Buffer Pointer */
    ULONG                   otd_NextTD;     /* LE PHYSICAL Next TD */
    ULONG                   otd_BufferEnd;  /* LE PHYSICAL End of buffer */
};

struct OhciIsoTD
{
    struct OhciIsoTD        *oitd_Succ;
    IPTR                    oitd_Length;    /* Length of transfer */
    ULONG                   oitd_Self;      /* LE PHYSICAL pointer to self */
    /* On 64 bits a padding will be inserted here */
    struct OhciED           *oitd_ED;        /* Pointer to parent ED this TD belongs to */

    /* HC data, aligned to 16 bytes */
    ULONG                   oitd_Ctrl;       /* LE Ctrl stuff */
    ULONG                   oitd_BufferPage0;/* LE PHYSICAL Current Buffer Page Pointer */
    ULONG                   oitd_NextTD;     /* LE PHYSICAL Next TD */
    ULONG                   oitd_BufferEnd;  /* LE PHYSICAL End of buffer */
    ULONG                   oitd_Offset[8];  /* PSWs */
};

struct RTIsoNode;

struct OhciPTDPrivate
{
    struct IOUsbHWBufferReq     ptd_BufferReq;
    APTR                        ptd_BounceBuffer;
    struct RTIsoNode           *ptd_RTIsoNode;
};

struct OhciHCPrivate
{
    struct OhciED           *ohc_OhciCtrlHeadED;
    struct OhciED           *ohc_OhciCtrlTailED;
    struct OhciED           *ohc_OhciBulkHeadED;
    struct OhciED           *ohc_OhciBulkTailED;
    struct OhciED           *ohc_OhciIntED[5];
    struct OhciED           *ohc_OhciTermED;
    struct OhciTD           *ohc_OhciTermTD;
    struct OhciHCCA         *ohc_OhciHCCA;
    struct OhciED           *ohc_OhciEDPool;
    struct OhciTD           *ohc_OhciTDPool;
    struct OhciIsoTD        *ohc_OhciIsoTDPool;
    struct OhciED           *ohc_OhciAsyncFreeED;
    ULONG                   ohc_OhciDoneQueue;
    struct List             ohc_OhciRetireQueue;
};

#endif /* OHCICHIP_H */
