#ifndef EHCICHIP_H
#define EHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             Includes for EHCI USB Controller
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include <exec/types.h>
#include <hardware/usb/ehci.h>
#include "hccommon.h"

#define EHCI_QH_POOLSIZE         128
#define EHCI_TD_POOLSIZE         512
#define EHCI_ITD_POOLSIZE        128
#define EHCI_SITD_POOLSIZE       128

#define EHCI_TD_BULK_LIMIT       (128<<10) // limit for one batch of BULK data TDs

struct PTDNode;

struct EhciPTDPrivate
{
    ULONG           ptd_NextPhys;
    UWORD           ptd_PktCount;
    UWORD           ptd_PktLength[8];
};

struct EhciTD
{
    struct EhciTD  *etd_Succ;
    IPTR            etd_Unused0;
    ULONG           etd_Self;       /* LE PHYSICAL pointer to self */
    ULONG           etd_Length;     /* Number of bytes to transfer within this */
#if __WORDSIZE == 64
    IPTR            etd_Unused1;
#endif
    IPTR            etd_Unused[4];

    /* aligned to 32 bytes */
    ULONG           etd_NextTD;     /* LE PHYSICAL pointer to next qTD */
    ULONG           etd_AltNextTD;  /* LE PHYSICAL alternate pointer to next qTD on short packet */
    ULONG           etd_CtrlStatus; /* LE Control and Status word */
    ULONG           etd_BufferPtr[5]; /* LE Buffer Pointers */

    ULONG           etd_ExtBufferPtr[5]; /* LE Buffer Pointers (upper 32 bit) */
    ULONG           etd_Unused2[3];
};

struct EhciITD
{
    struct EhciITD  *itd_Succ;
    ULONG           itd_Self;
#if __WORDSIZE == 64
    ULONG           itd_Unused0;
#endif
    ULONG           itd_Next;
    ULONG           itd_Transaction[8];
    ULONG           itd_BufferPtr[7];
#if __WORDSIZE == 64
    ULONG           itd_ExtBufferPtr[7];
#endif
};

struct EhciSiTD
{
    struct EhciSiTD *sitd_Succ;
    ULONG            sitd_Self;
#if __WORDSIZE == 64
    ULONG            sitd_Unused0;
#endif
    ULONG            sitd_Next;
    ULONG            sitd_EPCaps;
    ULONG            sitd_Token;
    ULONG            sitd_BufferPtr[2];
#if __WORDSIZE == 64
    ULONG            sitd_ExtBufferPtr[2];
#endif
    ULONG            sitd_BackPointer;
};

struct EhciQH
{
    struct EhciQH  *eqh_Succ;
    struct EhciQH  *eqh_Pred;
    ULONG           eqh_Self;       /* LE PHYSICAL pointer to self + UHCI_QHSELECT */
#if __WORDSIZE == 64
    ULONG           eqh_Unused1;    // Fill te gap
#endif
    struct IOUsbHWReq *eqh_IOReq;   /* IO Request this belongs to */

    struct EhciTD  *eqh_FirstTD;    /* First TD */
    IPTR            eqh_Actual;     /* Number of bytes for successful completion in this QH */
    APTR            eqh_Buffer;	    /* Mirror buffer for data outside of DMA-accessible area */
    APTR	    eqh_SetupBuf;   /* Mirror buffer for setup packet */

    /* aligned to 32 bytes */
    ULONG           eqh_NextQH;     /* LE PHYSICAL horizontal pointer to next QH */
    ULONG           eqh_EPCaps;     /* LE Endpoint Capabilities/Characteristics word */
    ULONG           eqh_SplitCtrl;  /* LE Split and Int control stuff */
    ULONG           eqh_CurrTD;     /* LE PHYSICAL current TD pointer */

    /* Transaction working space for host controller */
    ULONG           eqh_NextTD;     /* LE PHYSICAL pointer to next qTD */
    ULONG           eqh_AltNextTD;  /* LE PHYSICAL alternate pointer to next qTD on short packet */
    ULONG           eqh_CtrlStatus; /* LE Control and Status word */
    ULONG           eqh_BufferPtr[5]; /* LE Buffer Pointers */

    ULONG           eqh_ExtBufferPtr[5]; /* LE Buffer Pointers (upper 32 bit) */
    ULONG           eqh_Unused[7];
};

struct EhciHCPrivate
{
    ULONG                       *ehc_EhciFrameList;
    ULONG                       *ehc_IsoAnchor;
    struct PTDNode             **ehc_IsoHead;
    struct PTDNode             **ehc_IsoTail;

    struct EhciQH               *ehc_EhciQHPool;
    struct EhciTD               *ehc_EhciTDPool;
    struct EhciITD              *ehc_EhciITDPool;
    struct EhciSiTD             *ehc_EhciSiTDPool;

    struct EhciQH               *ehc_EhciAsyncQH;
    struct EhciQH               *ehc_EhciIntQH[11];
    struct EhciQH               *ehc_EhciTermQH;

    struct EhciQH               *ehc_EhciAsyncFreeQH;
    struct EhciTD               *ehc_ShortPktEndTD;

    ULONG                       ehc_EhciUsbCmd;

    /* EHCI companion routing information (decoded from HCSPARAMS).
     * These values are required to correctly map global root-hub ports to
     * companion controllers when multiple HCI functions exist on a single
     * PCI device.
     */
    UWORD                       ehc_EhciNumCompanions;
    UWORD                       ehc_EhciPortsPerComp;

    UWORD                       ehc_EhciTimeoutShift;

    UWORD                       ehc_FrameListSize;
    ULONG                       ehc_FrameListMask;

    volatile BOOL               ehc_AsyncAdvanced;
    BOOL                        ehc_64BitCapable;
    BOOL                        ehc_ComplexRouting;
};

/* Support functions */
#if __WORDSIZE == 64
#define ehciSetPointer(hcpriv, baseptr, extptr, value) \
    do { \
        WRITEMEM32_LE(&(baseptr), (ULONG)((IPTR)(value) & 0xFFFFFFFF)); \
        WRITEMEM32_LE(&(extptr), (hcpriv)->ehc_64BitCapable ? (ULONG)(((IPTR)(value) >> 32) & 0xFFFFFFFF) : 0); \
    } while (0)
#else
#define ehciSetPointer(hcpriv, baseptr, extptr, value) \
    WRITEMEM32_LE(&(baseptr), (ULONG)((IPTR)(value) & 0xFFFFFFFF))
#endif

/* hc_Quirks */
#define HCQB_EHCI_OVERLAY_CTRL_FILL     0
#define HCQ_EHCI_OVERLAY_CTRL_FILL      (1 << HCQB_EHCI_OVERLAY_CTRL_FILL)
#define HCQB_EHCI_OVERLAY_INT_FILL      1
#define HCQ_EHCI_OVERLAY_INT_FILL       (1 << HCQB_EHCI_OVERLAY_INT_FILL)
#define HCQB_EHCI_OVERLAY_BULK_FILL     2
#define HCQ_EHCI_OVERLAY_BULK_FILL      (1 << HCQB_EHCI_OVERLAY_BULK_FILL)
#define HCQB_EHCI_VBOX_FRAMEROOLOVER    3
#define HCQ_EHCI_VBOX_FRAMEROOLOVER     (1 << HCQB_EHCI_VBOX_FRAMEROOLOVER)
#define HCQB_EHCI_MOSC_FRAMECOUNTBUG    4
#define HCQ_EHCI_MOSC_FRAMECOUNTBUG     (1 << HCQB_EHCI_MOSC_FRAMECOUNTBUG)

#endif /* EHCICHIP_H */
