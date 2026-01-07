/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/hidd.h>
#include <hidd/usb.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <stdio.h>
#include <string.h>

#include "uhwcmd.h"
#include "xhciproto.h"
#include "pcixhci.h"

#ifdef base
#undef base
#endif
#define base (hc->hc_Device)
#if defined(AROS_USE_LOGRES)
#ifdef LogHandle
#undef LogHandle
#endif
#ifdef LogResBase
#undef LogResBase
#endif
#define LogHandle (hc->hc_LogRHandle)
#define LogResBase (base->hd_LogResBase)
#endif

static const char strXhciInitTaskName[] = "xHCI init";
static const char strHardwareNamePrefixFmt[] = "PCI USB %u.";
static const char strHardwareNameMinorUnknown[] = "x";
static const char strHardwareNameMinorFmt[] = "%u";
static const char strHardwareNameSuffix[] = " xHCI Host controller";

static BOOL XHCIController__Init(struct PCIController *hc)
{
    struct XhciHCPrivate *xhcic = NULL;
    volatile struct xhci_hccapr *xhciregs;
    ULONG xhciUSBLegSup = 0;
    ULONG xhciECPOff;
    ULONG val;
    ULONG cnt;
    UBYTE usbMajor = 0;
    UBYTE usbMinor = 0;
    UBYTE usbBestMajor = 0;
    UBYTE usbBestMinor = 0;
    UBYTE usb3Major = 0;
    UBYTE usb3Minor = 0;
    struct MsgPort *timerport = NULL;
    struct timerequest *timerreq = NULL;

    struct TagItem pciMemEnableAttrs[] = {
        { aHidd_PCIDevice_isMEM,    TRUE },
        { aHidd_PCIDevice_isMaster, TRUE },
        { TAG_DONE, 0UL },
    };

    if(!xhciOpenTaskTimer(&timerport, &timerreq, strXhciInitTaskName)) {
        return FALSE;
    }

    xhcic = AllocMem(sizeof(*xhcic), MEMF_CLEAR);
    if(!xhcic) {
        xhciCloseTaskTimer(&timerport, &timerreq);
        return FALSE;
    }
    hc->hc_CPrivate = xhcic;

    /* Initialize hardware... */
    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (IPTR *)&hc->hc_RegBase);
    xhciregs = (volatile struct xhci_hccapr *)hc->hc_RegBase;

    if(hc->hc_Unit) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Initializing hardware for unit #%d" DEBUGCOLOR_RESET" \n",
                        hc->hc_Unit->hu_UnitNo);
    } else {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Initializing hardware for controller @ 0x%p" DEBUGCOLOR_RESET" \n",
                        hc);
    }
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  CAPLENGTH: 0x%02x" DEBUGCOLOR_RESET" \n", xhciregs->caplength);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  DBOFF: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->dboff));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  RRSOFF: 0x%08x" DEBUGCOLOR_RESET" \n", AROS_LE2LONG(xhciregs->rrsoff));

    xhcic->xhc_XHCIOpR   = (APTR)((IPTR)xhciregs + xhciregs->caplength);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Operational Registers @ 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_XHCIOpR);
    xhcic->xhc_XHCIDB    = (APTR)((IPTR)xhciregs + AROS_LE2LONG(xhciregs->dboff));
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Doorbells @ 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_XHCIDB);
    xhcic->xhc_XHCIPorts = (APTR)((IPTR)xhcic->xhc_XHCIOpR + 0x400);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Port Registers @ 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_XHCIPorts);
    xhcic->xhc_XHCIIntR  = (APTR)((IPTR)xhciregs + AROS_LE2LONG(xhciregs->rrsoff) + 0x20);
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Interrupt Registers @ 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_XHCIIntR);

    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *)pciMemEnableAttrs); /* activate memory */

    /* Cache capability parameters once */
    ULONG hcsparams1 = AROS_LE2LONG(xhciregs->hcsparams1);
    ULONG hcsparams2 = AROS_LE2LONG(xhciregs->hcsparams2);
    ULONG hcsparams3 = AROS_LE2LONG(xhciregs->hcsparams3);
    ULONG hccparams1 = AROS_LE2LONG(xhciregs->hccparams1);
    ULONG hccparams2 = AROS_LE2LONG(xhciregs->hccparams2);
    ULONG xhciMaxIntrs = (hcsparams1 >> 8) & 0x7FF;
    UWORD xhciPortLimit = (UWORD)((hcsparams1 >> 24) & 0xFF);

    if(xhciPortLimit > MAX_ROOT_PORTS) {
        xhciPortLimit = MAX_ROOT_PORTS;
    }

    memset(xhcic->xhc_PortProtocol, XHCI_PORT_PROTOCOL_UNKNOWN, sizeof(xhcic->xhc_PortProtocol));
    xhcic->xhc_PortProtocolValid = FALSE;

    /* Extended Capabilities Pointer comes from HCCPARAMS1 (DWORD offset) */
    xhciECPOff = ((hccparams1 >> XHCIS_HCCPARAMS1_ECP) & XHCI_HCCPARAMS1_ECP_SMASK) << 2;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  Extended Capabilties Pointer = %04x" DEBUGCOLOR_RESET" \n", xhciECPOff);

    while(xhciECPOff >= 0x40) {
        volatile ULONG *capreg = (volatile ULONG *)((IPTR)xhciregs + xhciECPOff);
        ULONG caphdr = AROS_LE2LONG(*capreg);
        ULONG nextcap = (caphdr >> XHCIS_EXT_CAP_NEXT) & XHCI_EXT_CAP_NEXT_MASK;
        UBYTE capid = caphdr & 0xFF;

        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  ExtCap @%04lx: ID=%02x next=%02lx" DEBUGCOLOR_RESET" \n",
                        xhciECPOff, capid, nextcap);

        if(capid == XHCI_EXT_CAP_ID_LEGACY_SUPPORT) {
            xhciUSBLegSup = caphdr;
            pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  xhciUSBLegSup = $%08x" DEBUGCOLOR_RESET" \n", xhciUSBLegSup);

            if(xhciUSBLegSup & XHCIF_USBLEGSUP_BIOSOWNED) {
                ULONG ownershipval = xhciUSBLegSup | XHCIF_USBLEGSUP_OSOWNED;

                pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Taking ownership of xHCI from BIOS" DEBUGCOLOR_RESET" \n");
takeownership:
                cnt = 100;
                /*
                 * Change the ownership flag and read back to ensure it is written
                 */
                *capreg = AROS_LONG2LE(ownershipval);
                (void)*capreg;

                /*
                 * Wait for ownership change to take place.
                 * XHCI specification doesn't say how long it can take...
                 */
                while((AROS_LE2LONG(*capreg) & XHCIF_USBLEGSUP_BIOSOWNED) && (--cnt > 0)) {
                    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Waiting for ownership to change..." DEBUGCOLOR_RESET" \n");
                    uhwDelayMS(10, timerreq);
                }
                if((ownershipval != XHCIF_USBLEGSUP_OSOWNED) &&
                        (AROS_LE2LONG(*capreg) & XHCIF_USBLEGSUP_BIOSOWNED)) {
                    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Ownership of xHCI still with BIOS" DEBUGCOLOR_RESET" \n");

                    /* Try to force ownership */
                    ownershipval = XHCIF_USBLEGSUP_OSOWNED;
                    goto takeownership;
                }
            } else if(xhciUSBLegSup & XHCIF_USBLEGSUP_OSOWNED) {
                pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Ownership already with OS!" DEBUGCOLOR_RESET" \n");
            } else {
                pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Forcing ownership of xHCI from (unknown)" DEBUGCOLOR_RESET" \n");
                /* Try to force ownership */
                *capreg = AROS_LONG2LE(XHCIF_USBLEGSUP_OSOWNED);
                (void)*capreg;
            }

            /* Clear the SMI control bits */
            volatile ULONG *smictl = (volatile ULONG *)((IPTR)capreg + 4);
            *smictl = AROS_LONG2LE(0);
            (void)*smictl;
        } else if(capid == XHCI_EXT_CAP_ID_SUPPORTED_PROTOCOL) {
            ULONG caprev   = AROS_LE2LONG(*(capreg + 0));  /* DWORD0: revision */
            ULONG capname  = AROS_LE2LONG(*(capreg + 1));  /* DWORD1: name string */
            ULONG capports = AROS_LE2LONG(*(capreg + 2));  /* DWORD2: ports */

            UBYTE major = (caprev >> XHCIS_XCP_REV_MAJOR) & XHCI_XCP_REV_MAJOR_MASK;
            UBYTE minor_bcd = (caprev >> XHCIS_XCP_REV_MINOR) & XHCI_XCP_REV_MINOR_MASK;
            UBYTE minor = (minor_bcd >> 4) & 0x0F;
            if(minor == 0)
                minor = minor_bcd & 0x0F;

            UWORD name = (UWORD)(capname & XHCI_XCP_NAMESTRING_MASK);
            UBYTE portOffset = (capports >> XHCIS_XCP_PORT_OFFSET) & XHCI_XCP_PORT_OFFSET_MASK;
            UBYTE portCount = (capports >> XHCIS_XCP_PORT_COUNT) & XHCI_XCP_PORT_COUNT_MASK;

            pciusbWarn("xHCI",
                       DEBUGCOLOR_SET "  XCP protocol '%c%c' rev %u.%u ports %u..%u"
                       DEBUGCOLOR_RESET" \n",
                       (name & 0xFF),
                       (name >> 8) & 0xFF,
                       major, minor,
                       portOffset,
                       (UWORD)(portOffset + portCount - 1));

            if((major > usbBestMajor) || ((major == usbBestMajor) && (minor > usbBestMinor))) {
                usbBestMajor = major;
                usbBestMinor = minor;
            }
            if(major >= XHCI_PORT_PROTOCOL_USB3) {
                if((major > usb3Major) || ((major == usb3Major) && (minor > usb3Minor))) {
                    usb3Major = major;
                    usb3Minor = minor;
                }
            }

            if(portOffset > 0 && portCount > 0) {
                UWORD start = (UWORD)(portOffset - 1);
                UWORD end = (UWORD)(start + portCount);
                UBYTE proto = XHCI_PORT_PROTOCOL_UNKNOWN;

                if(major >= XHCI_PORT_PROTOCOL_USB3) {
                    proto = XHCI_PORT_PROTOCOL_USB3;
                } else if(major == XHCI_PORT_PROTOCOL_USB2) {
                    proto = XHCI_PORT_PROTOCOL_USB2;
                }

                if(proto != XHCI_PORT_PROTOCOL_UNKNOWN) {
                    for(UWORD port = start; port < end && port < xhciPortLimit; port++) {
                        xhcic->xhc_PortProtocol[port] = proto;
                    }
                    xhcic->xhc_PortProtocolValid = TRUE;
                }
            }
        }

        if(nextcap == 0) {
            break;
        }
        xhciECPOff = nextcap << 2;
    }

    UWORD xhciversion;
    xhciversion = AROS_LE2LONG(xhciregs->hciversion) & 0xFFFF;
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "  HCIVERSION: 0x%04x" DEBUGCOLOR_RESET" \n", xhciversion);
    hc->hc_HCIVersionMajor = (UBYTE)((xhciversion >> 8) & 0xFF);
    hc->hc_HCIVersionMinor = (UBYTE)((xhciversion >> 4) & 0x0F);
    if(usb3Major > 0) {
        usbMajor = usb3Major;
        usbMinor = usb3Minor;
    } else {
        usbMajor = usbBestMajor;
        usbMinor = usbBestMinor;
    }
    if(usbMajor == 0) {
        usbMajor = 3;
        usbMinor = 0xFF;
    }
    hc->hc_USBVersionMajor = usbMajor;
    hc->hc_USBVersionMinor = usbMinor;

    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  HCSPARAMS1: 0x%08x (MaxIntr %lu)" DEBUGCOLOR_RESET" \n", hcsparams1,
                     xhciMaxIntrs);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  HCSPARAMS2: 0x%08x" DEBUGCOLOR_RESET" \n", hcsparams2);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  HCSPARAMS3: 0x%08x" DEBUGCOLOR_RESET" \n", hcsparams3);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  HCCPARAMS1: 0x%08x" DEBUGCOLOR_RESET" \n", hccparams1);
    pciusbXHCIDebugV("xHCI",
                     DEBUGCOLOR_SET "  HCCPARAMS2: 0x%08x (PRS=%u CPSM=%u)" DEBUGCOLOR_RESET" \n",
                     hccparams2,
                     (hccparams2 & XHCIF_HCCPARAMS2_PRS) ? 1 : 0,
                     (hccparams2 & XHCIF_HCCPARAMS2_CPSM) ? 1 : 0);
    pciusbXHCIDebugV("xHCI", DEBUGCOLOR_SET "  OPR.CONFIG: 0x%08x" DEBUGCOLOR_RESET" \n",
                     AROS_LE2LONG(((volatile struct xhci_hcopr *)xhcic->xhc_XHCIOpR)->config));

    hc->hc_NumPorts = (ULONG)((hcsparams1 >> 24) & 0xFF);
    xhcic->xhc_NumSlots = (ULONG)(hcsparams1 & 0xFF);

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%d ports, %d slots" DEBUGCOLOR_RESET" \n",
                    hc->hc_NumPorts, xhcic->xhc_NumSlots);

    if(hccparams1 & XHCIF_HCCPARAMS1_CSZ)
        hc->hc_Flags |= HCF_CTX64;
    if(hccparams1 & XHCIF_HCCPARAMS1_AC64)
        hc->hc_Flags |= HCF_ADDR64;
    if(hccparams1 & XHCIF_HCCPARAMS1_PPC)
        hc->hc_Flags |= HCF_PPC;

    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "%d byte context(s), %ubit addressing" DEBUGCOLOR_RESET" \n",
                    (hc->hc_Flags & HCF_CTX64) ? 64 : 32,
                    (hc->hc_Flags & HCF_ADDR64) ? 64 : 32);

    /* Device Context Base Address Array (Chapter 6.1) */
    xhcic->xhc_DCBAAp = pciAllocAligned(hc, &xhcic->xhc_DCBAA,
                                        sizeof(UQUAD) * (xhcic->xhc_NumSlots + 1),
                                        ALIGN_DCBAA,
                                        xhciPageSize(hc));
    if(xhcic->xhc_DCBAAp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated DCBAA @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_DCBAAp, xhcic->xhc_DCBAA.me_Un.meu_Addr, xhcic->xhc_DCBAA.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMADCBAA = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)xhcic->xhc_DCBAAp);
#else
        xhcic->xhc_DMADCBAA = xhcic->xhc_DCBAAp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMADCBAA);
        memset(xhcic->xhc_DCBAAp, 0, sizeof(UQUAD) * (xhcic->xhc_NumSlots + 1));
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate DCBAA DMA Memory" DEBUGCOLOR_RESET" \n");
        goto init_fail;
    }

    /* Event Ring Segment Table (Chapter 6.5) */
    xhcic->xhc_ERSTp = pciAllocAligned(hc, &xhcic->xhc_ERST,
                                       sizeof(struct xhci_er_seg),
                                       ALIGN_EVTRING_TBL,
                                       ALIGN_EVTRING_TBL);
    if(xhcic->xhc_ERSTp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated ERST @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_ERSTp, xhcic->xhc_ERST.me_Un.meu_Addr, xhcic->xhc_ERST.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMAERST = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)xhcic->xhc_ERSTp);
#else
        xhcic->xhc_DMAERST = xhcic->xhc_ERSTp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERST);
        memset((void *)xhcic->xhc_ERSTp, 0, sizeof(struct xhci_er_seg));
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate ERST DMA Memory" DEBUGCOLOR_RESET" \n");
        goto init_fail;
    }

    /* Command Ring */
    xhcic->xhc_OPRp = pciAllocAligned(hc, &xhcic->xhc_OPR,
                                      sizeof(struct pcisusbXHCIRing),
                                      XHCI_RING_ALIGN,
                                      (1 << 16));
    if(xhcic->xhc_OPRp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated OPR @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_OPRp, xhcic->xhc_OPR.me_Un.meu_Addr, xhcic->xhc_OPR.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMAOPR = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)xhcic->xhc_OPRp);
#else
        xhcic->xhc_DMAOPR = xhcic->xhc_OPRp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAOPR);
        xhciInitRing(hc, (struct pcisusbXHCIRing *)xhcic->xhc_OPRp);
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate OPR DMA Memory" DEBUGCOLOR_RESET" \n");
        goto init_fail;
    }

    /* Event Ring */
    xhcic->xhc_ERSp = pciAllocAligned(hc, &xhcic->xhc_ERS,
                                      sizeof(struct pcisusbXHCIRing),
                                      XHCI_RING_ALIGN,
                                      (1 << 16));
    if(xhcic->xhc_ERSp) {
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Allocated ERS @ 0x%p <0x%p, %u>" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_ERSp, xhcic->xhc_ERS.me_Un.meu_Addr, xhcic->xhc_ERS.me_Length);
#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMAERS = CPUTOPCI(hc, hc->hc_PCIDriverObject, (APTR)xhcic->xhc_ERSp);
#else
        xhcic->xhc_DMAERS = xhcic->xhc_ERSp;
#endif
        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Mapped to 0x%p" DEBUGCOLOR_RESET" \n", xhcic->xhc_DMAERS);
        xhciInitRing(hc, (struct pcisusbXHCIRing *)xhcic->xhc_ERSp);
    } else {
        pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate ERS DMA Memory" DEBUGCOLOR_RESET" \n");
        goto init_fail;
    }

    /* Scratchpad buffer count decode (Hi/Lo per spec) */
    val = hcsparams2;
    {
        ULONG sp_lo = (val >> 21) & 0x1F;
        ULONG sp_hi = (val >> 27) & 0x1F;
        xhcic->xhc_NumScratchPads = (sp_hi << 5) | sp_lo;
    }
    pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "SPB = %u" DEBUGCOLOR_RESET" \n", xhcic->xhc_NumScratchPads);

    if(xhcic->xhc_NumScratchPads) {
        ULONG pagesize   = xhciPageSize(hc);
        ULONG spba_size  = sizeof(struct xhci_address) * xhcic->xhc_NumScratchPads;
        ULONG spb_size   = pagesize * xhcic->xhc_NumScratchPads;

        xhcic->xhc_SPBAp = pciAllocAligned(hc, &xhcic->xhc_SPBA,
                                           spba_size,
                                           ALIGN_SPBA,
                                           pagesize);
        if(!xhcic->xhc_SPBAp) {
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate SPBA DMA Memory" DEBUGCOLOR_RESET" \n");
            goto init_fail;
        }

#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMASPBA = CPUTOPCI(hc, hc->hc_PCIDriverObject, xhcic->xhc_SPBAp);
#else
        xhcic->xhc_DMASPBA = xhcic->xhc_SPBAp;
#endif
        memset(xhcic->xhc_SPBAp, 0, spba_size);

        xhcic->xhc_SPBuffersp = pciAllocAligned(hc, &xhcic->xhc_SPBuffers,
                                                spb_size,
                                                pagesize,
                                                pagesize);
        if(!xhcic->xhc_SPBuffersp) {
            pciusbError("xHCI", DEBUGWARNCOLOR_SET "xHCI: Unable to allocate Scratchpad Buffers" DEBUGCOLOR_RESET" \n");
            if(xhcic->xhc_SPBA.me_Un.meu_Addr)
                FREEPCIMEM(hc, hc->hc_PCIDriverObject, xhcic->xhc_SPBA.me_Un.meu_Addr);
            goto init_fail;
        }

#if !defined(PCIUSB_NO_CPUTOPCI)
        xhcic->xhc_DMASPBuffers = CPUTOPCI(hc, hc->hc_PCIDriverObject, xhcic->xhc_SPBuffersp);
#else
        xhcic->xhc_DMASPBuffers = xhcic->xhc_SPBuffersp;
#endif
        memset(xhcic->xhc_SPBuffersp, 0, spb_size);

        {
            volatile struct xhci_address *spba = (volatile struct xhci_address *)xhcic->xhc_SPBAp;
            ULONG i;
            for(i = 0; i < xhcic->xhc_NumScratchPads; i++) {
                xhciSetPointer(hc, spba[i],
                               (IPTR)xhcic->xhc_DMASPBuffers + ((IPTR)pagesize * i));
            }
        }

        xhciSetPointer(hc,
                       ((volatile struct xhci_address *)xhcic->xhc_DCBAAp)[0],
                       xhcic->xhc_DMASPBA);

        pciusbXHCIDebug("xHCI", DEBUGCOLOR_SET "Scratch buffers allocated (%u x 0x%x)" DEBUGCOLOR_RESET" \n",
                        xhcic->xhc_NumScratchPads, pagesize);
    }

    xhciCloseTaskTimer(&timerport, &timerreq);
    return TRUE;

init_fail:
    if(xhcic) {
        FreeMem(xhcic, sizeof(*xhcic));
        hc->hc_CPrivate = NULL;
    }
    xhciCloseTaskTimer(&timerport, &timerreq);
    return FALSE;
}

#ifdef base
#undef base
#endif
#define base ((struct PCIDevice *)(cl->UserData))

OOP_Object *XHCIController__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct PCIController *hc = (struct PCIController *)GetTagData(aHidd_DriverData, 0, msg->attrList);
    struct TagItem xhcic_tags[] = {
        {aHidd_HardwareName,    0       },
        {TAG_MORE,              0       }
    };
    struct pRoot_New xcNewMsg;
    xcNewMsg.mID = msg->mID;
    xcNewMsg.attrList = &xhcic_tags[0];
    if((xhcic_tags[1].ti_Data = (IPTR)msg->attrList) == 0)
        xhcic_tags[1].ti_Tag = TAG_DONE;

    if(!hc)
        return NULL;

    if(!hc->hc_CPrivate) {
        /*
         * Initialize the controllers structures,
         * and query supported features
         */
        if(!XHCIController__Init(hc)) {
            return NULL;
        }
    }

    /* Construct the hardware name ... */
    {
        char name_buf[64];
        char *hardware_name = NULL;

        sprintf(name_buf, strHardwareNamePrefixFmt, hc->hc_USBVersionMajor);
        if(hc->hc_USBVersionMinor == 0xFF)
            sprintf(name_buf + 10, strHardwareNameMinorUnknown);
        else
            sprintf(name_buf + 10, strHardwareNameMinorFmt, hc->hc_USBVersionMinor);
        sprintf(name_buf + 11, strHardwareNameSuffix);

        hardware_name = AllocVec(strlen(name_buf) + 1, MEMF_CLEAR);
        if(hardware_name != NULL) {
            strcpy(hardware_name, name_buf);
            xhcic_tags[0].ti_Data = (IPTR)hardware_name;
        }
    }

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&xcNewMsg);
}

VOID XHCIController__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

void XHCIController__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

#   define XHCIController_DATA_SIZE (sizeof(struct XHCIController))

#ifdef base
#undef base
#endif
#define base hd

int XHCIControllerOOPStartup(struct PCIDevice *hd)
{
    if(!hd->hd_USBXHCIControllerClass) {
        OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
        OOP_Class *cl = NULL;

        struct OOP_MethodDescr XHCIController_Root_descr[] = {
            {(OOP_MethodFunc)XHCIController__Root__New, moRoot_New},
            {(OOP_MethodFunc)XHCIController__Root__Dispose, moRoot_Dispose},
            {(OOP_MethodFunc)XHCIController__Root__Get, moRoot_Get},
            {NULL, 0}
        };
#define NUM_XHCIController_Root_METHODS 3

        struct OOP_InterfaceDescr XHCIController_ifdescr[] = {
            {XHCIController_Root_descr, IID_Root, NUM_XHCIController_Root_METHODS},
            {NULL, NULL}
        };

        struct TagItem XHCIController_tags[] = {
            {aMeta_SuperID, (IPTR)CLID_Hidd_USBController},
            {aMeta_InterfaceDescr, (IPTR)XHCIController_ifdescr},
            {aMeta_InstSize, (IPTR)XHCIController_DATA_SIZE},
            {TAG_DONE, (IPTR)0}
        };

        if(MetaAttrBase == 0)
            return FALSE;

        cl = OOP_NewObject(NULL, CLID_HiddMeta, XHCIController_tags);
        if(cl != NULL) {
            cl->UserData = (APTR)hd;
            hd->hd_USBXHCIControllerClass = cl;
            OOP_AddClass(cl);
        }

        OOP_ReleaseAttrBase(IID_Meta);
    }
    return hd->hd_USBXHCIControllerClass != NULL;
}

static void XHCIControllerOOPShutdown(struct PCIDevice *hd)
{
    if(hd->hd_USBXHCIControllerClass != NULL) {
        OOP_RemoveClass(hd->hd_USBXHCIControllerClass);
        OOP_DisposeObject((OOP_Object *)hd->hd_USBXHCIControllerClass);
    }
}
