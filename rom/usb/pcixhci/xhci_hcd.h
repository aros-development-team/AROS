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
#include "pcixhci.h"
#include "hccommon.h"

struct MsgPort;
struct timerequest;

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

struct pciusbXHCIDMAAlloc {
    struct MemEntry                     dmaa_Entry;
    APTR                                dmaa_Ptr;
    APTR                                dmaa_DMA;
};

struct pciusbXHCITRBParams {
    ULONG                       tparams;
    ULONG                       flags;
    ULONG                       status;
};

struct XhciPortTaskPrivate {
    struct Task                    *xpt_Task;
    BYTE                            xpt_PortChangeSignal;
    struct MsgPort                 *xpt_TimerPort;
    struct timerequest             *xpt_TimerReq;
};

struct XhciEventTaskPrivate {
    struct Task                    *xet_Task;
    BYTE                            xet_ProcessEventsSignal;
    struct MsgPort                 *xet_TimerPort;
    struct timerequest             *xet_TimerReq;
};

struct XhciHCPrivate {
    UWORD                               xhc_NumSlots;
    UWORD                               xhc_NumDevs;
    UWORD                               xhc_NumScratchPads;
    /* Device Context Base Address Array */
    struct MemEntry                     xhc_DCBAA;
    APTR                                xhc_DCBAAp;
    APTR                                xhc_DMADCBAA;
    /* Scratchpad Buffer Array and buffers */
    struct MemEntry                     xhc_SPBA;
    APTR                                xhc_SPBAp;
    APTR                                xhc_DMASPBA;
    struct MemEntry                     xhc_SPBuffers;
    APTR                                xhc_SPBuffersp;
    APTR                                xhc_DMASPBuffers;
    /* Event Ring Segment Table */
    struct MemEntry                     xhc_ERST;
    APTR                                xhc_ERSTp;
    APTR                                xhc_DMAERST;
    /* Command Ring */
    struct MemEntry                     xhc_OPR;
    APTR                                xhc_OPRp;
    APTR                                xhc_DMAOPR;
    /* Event Ring */
    struct MemEntry                     xhc_ERS;
    APTR                                xhc_ERSp;
    APTR                                xhc_DMAERS;

    APTR                                xhc_XHCIOpR;
    APTR                                xhc_XHCIDB;
    APTR                                xhc_XHCIPorts;
    APTR                                xhc_XHCIIntR;

    BYTE                                xhc_ReadySignal;
    UWORD                               xhc_RootPortChanges;
    struct Task                        *xhc_ReadySigTask;
    struct XhciPortTaskPrivate         xhc_PortTask;
    struct XhciEventTaskPrivate        xhc_EventTask;
    struct pciusbXHCIDevice            *xhc_Devices[USB_DEV_MAX];
    volatile struct pciusbXHCITRBParams xhc_CmdResults[USB_DEV_MAX];
    UBYTE                               xhc_PortProtocol[MAX_ROOT_PORTS];
    BOOL                                xhc_PortProtocolValid;
};

static inline struct XhciHCPrivate *xhciGetHCPrivate(struct PCIController *hc)
{
    return (struct XhciHCPrivate *)hc->hc_CPrivate;
}

/*
 * Root port state helpers
 *
 * Poseidon's hub model expects USB2-style PORT_ENABLE semantics. On SuperSpeed
 * ports, the xHCI PED bit is not a reliable equivalent, so we treat a USB3
 * port as "enabled" when it is connected and in U0 (operational).
 */
static inline ULONG xhciPortSpeedBits(ULONG portsc)
{
    return (portsc & XHCI_PR_PORTSC_SPEED_MASK);
}

static inline ULONG xhciPortLinkState(ULONG portsc)
{
    return (portsc & XHCI_PR_PORTSC_PLS_MASK) >> XHCIS_PR_PORTSC_PLS;
}

static inline BOOL xhciIsUsb3SpeedBits(ULONG portsc)
{
    ULONG speedBits = xhciPortSpeedBits(portsc);
    return (speedBits == XHCIF_PR_PORTSC_SUPERSPEED) ||
           (speedBits == XHCIF_PR_PORTSC_SUPERSPEEDPLUS);
}

/*
 * Identify whether a given port register represents a USB3 protocol port.
 *
 * When speed bits are zero (disconnected, RxDetect, etc.), consult the
 * Supported Protocol capability-derived table built during init.
 */
static inline BOOL xhciPortIsUsb3(struct PCIController *hc, UWORD hciport, ULONG portsc)
{
    struct XhciHCPrivate *xhcic = xhciGetHCPrivate(hc);

    if(xhcic && xhcic->xhc_PortProtocolValid && (hciport < MAX_ROOT_PORTS)) {
        if(xhcic->xhc_PortProtocol[hciport] == XHCI_PORT_PROTOCOL_USB3)
            return TRUE;
        if(xhcic->xhc_PortProtocol[hciport] == XHCI_PORT_PROTOCOL_USB2)
            return FALSE;
    }

    return xhciIsUsb3SpeedBits(portsc);
}

static inline BOOL xhciUsb3Operational(ULONG portsc)
{
    /* USB3 ports do not use PED; treat U0 + CCS as "enabled". */
    return ((portsc & XHCIF_PR_PORTSC_CCS) != 0) && (xhciPortLinkState(portsc) == 0);
}

static inline BOOL xhciHubPortEnabled(struct PCIController *hc, UWORD hciport, ULONG portsc)
{
    if(xhciPortIsUsb3(hc, hciport, portsc))
        return xhciUsb3Operational(portsc);

    return (portsc & XHCIF_PR_PORTSC_PED) != 0;
}

static inline BOOL xhciHubPortConnected(ULONG portsc)
{
    return (portsc & XHCIF_PR_PORTSC_CCS) != 0;
}

/*
 * Private Transfer Request Block Ring desciption
 */
struct pcisusbXHCIRing {
    struct xhci_trb                     ring[XHCI_EVENT_RING_TRBS];                 // (!!) volatile area accessed by the controller (!!)
    struct IORequest                    **ringio;                                   // non-DMA bookkeeping
    struct pciusbXHCIDMAAlloc           rnext;                                      // Next ring in this sequence
    struct xhci_trb                     current;                                    // cached copy of the current trb
    volatile UWORD                      next, end;                                  // current queue locations
    // NB: the cycle bit is cached in the highest bit of end
};
#define RINGENDCFLAG                            (1 << 15)

static inline void xhciRingLock(void)
{
    Disable();
}

static inline void xhciRingUnlock(void)
{
    Enable();
}

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

struct pciusbXHCIEndpointCtx {
    struct pciusbXHCIDevice            *ectx_Device;
    ULONG                               ectx_EPID;
    struct MsgPort                     *ectx_TimerPort;
    struct timerequest                 *ectx_TimerReq;
};

struct pciusbXHCIIODevPrivate {
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
