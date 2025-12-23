#ifndef XHCICHIP_H
#define XHCICHIP_H

/*
 *----------------------------------------------------------------------------
 *             xHCI Controller Private definitions
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

#define XHCI_PORT_PROTOCOL_UNKNOWN 0
#define XHCI_PORT_PROTOCOL_USB2    2
#define XHCI_PORT_PROTOCOL_USB3    3

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

/*
 * RINGFROMTRB() is used as a fast-path in the interrupt handler to map an
 * in-ring TRB pointer (reported by Transfer Event TRBs) back to its containing
 * ring. This is only valid if the ring allocation is aligned to XHCI_RING_BYTES.
 *
 * xHCI itself only mandates 64-byte ring segment alignment, so we explicitly
 * enforce the stronger alignment for all rings allocated by this driver.
 */
#define XHCI_RING_ALIGN   (XHCI_RING_BYTES)
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
    APTR                                dpBounceBuf;
    APTR                                dpBounceData;
    ULONG                               dpBounceLen;
    UWORD                               dpBounceDir;
    BYTE                                dpCC;
};

/* Misc Stuff */

/*
 * Memory barrier before MMIO doorbells:
  * ensure TRB contents and software bookkeeping (ringio[]) are globally visible
  * before we notify the controller. This prevents "fast completion" races on
  * some hypervisors/emulators (e.g. VirtualBox) where an interrupt may be raised
  * immediately after the doorbell is written.
  *
 * On ARM/RISC-V we use an explicit device-capable barrier. On x86, a release
 * fence typically compiles to a compiler barrier (store->store ordering holds).
  */
#if defined(__GNUC__) || defined(__clang__)
  /* ARM (AArch64 / ARMv7+): ensure all prior writes are observable before MMIO. */
# if defined(__aarch64__) || defined(__arm__)
#  define XHCI_MMIO_BARRIER() __asm__ __volatile__("dmb ishst" ::: "memory")

  /* RISC-V: order all IO + memory accesses around MMIO stores. */
# elif defined(__riscv)
   /*
    * "iorw, iorw" is the conservative form for device/MMIO.
    * If your assembler/toolchain rejects iorw, switch to "fence rw, rw".
    */
#  define XHCI_MMIO_BARRIER() __asm__ __volatile__("fence iorw, iorw" ::: "memory")

  /* Other architectures: use a release fence (compiler + arch barrier as needed). */
# else
#  define XHCI_MMIO_BARRIER() __atomic_thread_fence(__ATOMIC_RELEASE)
# endif
#else
  /* Fallback: at least prevent compiler reordering if nothing better exists. */
# define XHCI_MMIO_BARRIER() do { } while (0)
#endif

#endif /* XHCICHIP_H */
