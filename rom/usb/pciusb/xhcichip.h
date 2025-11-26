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
    struct xhci_trb                     ring[USB_DEV_MAX];                          // (!!) volatile area accessed by the controller (!!)
    struct IORequest                    *ringio[USB_DEV_MAX];
    struct pciusbXHCIDMAAlloc           rnext;                                      // Next ring in this sequence
    struct xhci_trb                     current;                                    // cached copy of the current trb
    volatile UWORD                      next, end;                                  // current queue locations
                                                                                    // NB: the cycle bit is cached in the highest bit of end
};
#define RINGENDCFLAG                            (1 << 15)
#define RINGFROMTRB(x)                          ((struct pcisusbXHCIRing*)((IPTR)(x) & ~(sizeof(ering->ring)-1)))

/*
 * Private USB Device Context
 */
struct pcisusbXHCIDevice {
    struct pciusbXHCIDMAAlloc           dc_EPAllocs[MAX_DEVENDPOINTS];
    struct pciusbXHCIDMAAlloc           dc_IN;                                      // Device Input context
    struct pciusbXHCIDMAAlloc           dc_SlotCtx;                                 // Device Context (R/O)
    ULONG                               dx_TxMax;
    UBYTE                               dc_SlotID;
    UBYTE                               dc_DevAddr;
    UBYTE                               dc_RootPort;
};

struct pciusbXHCIIODevPrivate
{
    struct pcisusbXHCIDevice            *dpDevice;
    ULONG                               dpEPID;
    UWORD                               dpSTRB;                                     // Setup TRB
    UWORD                               dpTxSTRB;                                   // Transaction TRB
    UWORD                               dpTxETRB;                                   // Transaction TRB
    UWORD                               dpSttTRB;                                   // Status TRB
    BYTE                                dpCC;
};

/* Misc Stuff */

#define XHCIASCII_DEBUG

#if (DEBUG > 0) && !defined(AROS_USE_LOGRES) && defined(XHCIASCII_DEBUG)
#define DEBUGCOLOR_SET                          "\033[32m"
#define DEBUGWARNCOLOR_SET                      "\033[31m"
#define DEBUGFUNCCOLOR_SET                      "\033[32;1m"
#define DEBUGCOLOR_RESET                        "\033[0m"
#else
#define DEBUGCOLOR_SET
#define DEBUGWARNCOLOR_SET
#define DEBUGFUNCCOLOR_SET
#define DEBUGCOLOR_RESET
#endif

#endif /* XHCICHIP_H */
