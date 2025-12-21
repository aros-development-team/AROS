#ifndef XHCICHIP_H
#define XHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             XHCI Controller Private definitions
 *----------------------------------------------------------------------------
 *
 */

#include <exec/types.h>
#include <hardware/usb/xhci.h>
#include "hccommon.h"

#ifndef DMAFLAGS_PREREAD
#define DMAFLAGS_PREREAD     0
#endif

#ifndef DMAFLAGS_PREWRITE
#define DMAFLAGS_PREWRITE    DMA_ReadFromRAM
#endif

#ifndef DMAFLAGS_POSTREAD
#define DMAFLAGS_POSTREAD    (1 << 31)
#endif

#ifndef DMAFLAGS_POSTWRITE
#define DMAFLAGS_POSTWRITE   (1 << 31) | DMA_ReadFromRAM
#endif

/* Number of TRBs in the single event ring segment. */
#define XHCI_EVENT_RING_TRBS  USB_DEV_MAX    /* 128/256/512 */

/*
 * Maximum TRB transfer length (Transfer Length field is 17 bits).
 * Keeping large payloads in fewer TRBs improves robustness on controllers/emulators
 * that are sensitive to missing ISP/chain semantics.
 */
#define XHCI_TRB_MAX_XFER      0x1FFFFUL


#define SLOT_CTX_ENTRIES_SHIFT 27
#define SLOT_CTX_ENTRIES_MASK  0x1F

struct pciusbXHCIEndpointCtx;

struct pciusbXHCIDMAAlloc
{
    struct MemEntry                     dmaa_Entry;
    APTR                                dmaa_Ptr;
    APTR                                dmaa_DMA;
};

/*
 * Private Transfer Request Block Ring desciption
 */
struct pcisusbXHCIRing
{
    struct xhci_trb                     ring[XHCI_EVENT_RING_TRBS];                 // (!!) volatile area accessed by the controller (!!)
    struct IORequest                    *ringio[XHCI_EVENT_RING_TRBS];
    struct pciusbXHCIDMAAlloc           rnext;                                      // Next ring in this sequence
    struct xhci_trb                     current;                                    // cached copy of the current trb
    volatile UWORD                      next, end;                                  // current queue locations
                                                                                    // NB: the cycle bit is cached in the highest bit of end
};
#define RINGENDCFLAG                            (1 << 15)
/* Derive the ring base from an in-ring TRB pointer (ring size is power-of-two). */
#define XHCI_RING_BYTES   (sizeof(((struct pcisusbXHCIRing *)0)->ring))
#define RINGFROMTRB(x)    ((struct pcisusbXHCIRing *)((IPTR)(x) & ~(XHCI_RING_BYTES - 1)))

/*
 * Private USB Device Context
 */
struct pciusbXHCIDevice {
    struct pciusbXHCIDMAAlloc           dc_EPAllocs[MAX_DEVENDPOINTS];
    struct pciusbXHCIEndpointCtx       *dc_EPContexts[MAX_DEVENDPOINTS];
    struct pciusbXHCIDMAAlloc           dc_IN;                                      // Device Input context
    struct pciusbXHCIDMAAlloc           dc_SlotCtx;                                 // Device Context (R/O)
    ULONG                               dc_EP0MaxPacket;
    ULONG                               dc_TxMax;
    ULONG                               dc_RouteString;
    UBYTE                               dc_SlotID;
    UBYTE                               dc_DevAddr;
    UBYTE                               dc_RootPort;
};

struct pciusbXHCIEndpointCtx
{
    struct pciusbXHCIDevice            *ectx_Device;
    ULONG                               ectx_EPID;
};

struct pciusbXHCIIODevPrivate
{
    struct pciusbXHCIDevice            *dpDevice;
    ULONG                               dpEPID;
    UWORD                               dpSTRB;                                     // Setup TRB
    UWORD                               dpTxSTRB;                                   // Transaction TRB
    UWORD                               dpTxETRB;                                   // Transaction TRB
    UWORD                               dpSttTRB;                                   // Status TRB
    BYTE                                dpCC;
};

/* Misc Stuff */

#endif /* XHCICHIP_H */
