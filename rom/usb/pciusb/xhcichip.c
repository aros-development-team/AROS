/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/pci.h>

#include <devices/usb_hub.h>

#include "uhwcmd.h"

#if (AROS_USB30_CODE)

APTR AllocVecAlignedOn4KPage(APTR *original, ULONG bytesize, ULONG alignment);

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase (hd->hd_HiddPCIDeviceAB)
#undef HiddAttrBase
#define HiddAttrBase (hd->hd_HiddAB)


static
AROS_UFH3(void, xhciResetHandler,
        AROS_UFHA(struct PCIController *, hc, A1),
        AROS_UFHA(APTR, unused, A5),
        AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

	/* Halt controller */
    #ifdef DEBUG
    if(!xhciHaltHC(hc))
        KPRINTF(1000, ("XHCI Halting HC failed, reset may result in undefined behavior!\n"));
    #else
    xhciHaltHC(hc);
    #endif

	/* Reset controller */
    xhciResetHC(hc);

    AROS_USERFUNC_EXIT
}

void xhciCompleteInt(struct PCIController *hc)
{
    KPRINTF(1, ("CompleteInt!\n"));

    KPRINTF(1, ("CompleteDone\n"));
}

void xhciIntCode(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    struct PCIController *hc = (struct PCIController *) irq->h_Data;
//    struct PCIDevice *base = hc->hc_Device;
//    struct PCIUnit *unit = hc->hc_Unit;

    ULONG intr, portn;

    intr = opreg_readl(XHCI_USBSTS);
    if(intr & XHCF_STS_EINT) {
        /* Clear (RW1C) Event Interrupt (EINT) */
        opreg_writel(XHCI_USBSTS, XHCF_STS_EINT);

        if (hc->hc_Flags & HCF_ONLINE)
        {
            if(intr & XHCF_STS_HSE) {
                KPRINTF(1000, ("Host System Error (HSE)!\n"));
            }

            if(intr & XHCF_STS_PCD) {
                /* There are seven status change bits in the PORTSC register,
                        Connect Status Change (CSC)
                        Port Enabled/Disabled Change (PEC)
                        Warm Port Reset Change (WRC)
                        Over-current Change (OCC)
                        Port Reset Change (PRC)
                        Port Link State Change (PLC)
                        Port Config Error Change (CEC)
                */
                for (portn = 1; portn <= hc->hc_NumPorts; portn++) {
                    if (opreg_readl(XHCI_PORTSC(portn)) & (XHCF_PS_CSC|XHCF_PS_PEC|XHCF_PS_OCC|XHCF_PS_WRC|XHCF_PS_PRC|XHCF_PS_PLC|XHCF_PS_CEC)) {
                            KPRINTF(1000,("port %d changed\n", portn));
                    }
                }
            }

            if(intr & XHCF_STS_SRE) {
                KPRINTF(1000, ("Host Controller Error (HCE)!\n"));
            }

        } // not online
    }
}

IPTR xhciExtCap(struct PCIController *hc, ULONG id, IPTR extcap) {

    IPTR extcapoff = (IPTR) 0;
    ULONG cnt = XHCI_EXT_CAPS_MAX;

    KPRINTF(100,("search for ext cap with id(%ld)\n", id));

    if(extcap) {
        KPRINTF(100, ("continue search from %p\n", extcap));
        extcap = (IPTR) XHCV_EXT_CAPS_NEXT(READMEM32_LE(extcap));
    } else {  
        extcap = (IPTR) hc->xhc_capregbase + XHCV_xECP(capreg_readl(XHCI_HCCPARAMS));
        KPRINTF(100, ("search from the beginning %p\n", extcap));
    }

    do {
        extcap += extcapoff;
        if((XHCV_EXT_CAPS_ID(READMEM32_LE(extcap)) == id)) {
            KPRINTF(100, ("found matching ext cap %lx\n", extcap));
            return (IPTR) extcap;
        }
        #if DEBUG
        if(extcap)
            KPRINTF(100, ("skipping ext cap with id(%ld)\n", XHCV_EXT_CAPS_ID(READMEM32_LE(extcap))));
        #endif
        extcapoff = (IPTR) XHCV_EXT_CAPS_NEXT(READMEM32_LE(extcap));
        cnt--;
    } while(cnt & extcapoff);

    KPRINTF(100, ("not found!\n"));
    return (IPTR) 0;
}

BOOL xhciHaltHC(struct PCIController *hc) {

    struct PCIUnit *hu = hc->hc_Unit;

    ULONG timeout, temp;

    /* Halt the controller by clearing Run/Stop bit */
    temp = opreg_readl(XHCI_USBCMD);
    opreg_writel(XHCI_USBCMD, (temp & ~XHCF_CMD_RS));

    /*
        The xHC shall halt within 16 ms. after software clears the Run/Stop bit if certain conditions have been met.
        The HCHalted (HCH) bit in the USBSTS register indicates when the xHC has finished its
        pending pipelined transactions and has entered the stopped state. 
    */
    timeout = 250;  //FIXME: arbitrary value of 2500ms
    do {
        temp = opreg_readl(XHCI_USBSTS);
        if( (temp & XHCF_STS_HCH) ) {
            KPRINTF(1000, ("controller halted!\n"));
            return TRUE;
        }
        uhwDelayMS(10, hu);
    } while(--timeout);

    KPRINTF(1000, ("halt failed!\n"));
    return FALSE;
}

BOOL xhciResetHC(struct PCIController *hc) {

    struct PCIUnit *hu = hc->hc_Unit;

    ULONG timeout, temp;

    /* Reset controller by setting HCRST-bit */
    temp = opreg_readl(XHCI_USBCMD);
    opreg_writel(XHCI_USBCMD, (temp | XHCF_CMD_HCRST));

    /*
        Controller clears HCRST bit when reset is done, wait for it and for CNR-bit to be cleared
    */
    timeout = 250;  //FIXME: arbitrary value of 2500ms
    do {
        temp = opreg_readl(XHCI_USBCMD);
        if( !(temp & XHCF_CMD_HCRST) ) {
            /* Wait for CNR-bit to be 0 */
            timeout = 250;  //FIXME: arbitrary value of 2500ms
            do {
                temp = opreg_readl(XHCI_USBSTS);
                if( !(temp & XHCF_STS_CNR) ) {
                    KPRINTF(1000, ("reset succeeded!\n"));
                    return TRUE;
                }
                uhwDelayMS(10, hu);
            } while(--timeout);
            return FALSE;
        }
        uhwDelayMS(10, hu);
    } while(--timeout);

    KPRINTF(1000, ("reset failed!\n"));
    return FALSE;
}

BOOL xhciInit(struct PCIController *hc, struct PCIUnit *hu) {

    struct PCIDevice *hd = hu->hu_Device;

    ULONG cnt, timeout, temp;
    IPTR extcap;
    APTR memptr;
    volatile APTR pciregbase;

    struct TagItem pciActivateMemAndBusmaster[] =
    {
            { aHidd_PCIDevice_isIO,     FALSE },
            { aHidd_PCIDevice_isMEM,    TRUE },
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    /* Activate Mem and Busmaster as pciFreeUnit will disable them! (along with IO, but we don't have that...) */
    OOP_SetAttrs(hc->hc_PCIDeviceObject, (struct TagItem *) pciActivateMemAndBusmaster);

    OOP_GetAttr(hc->hc_PCIDeviceObject, aHidd_PCIDevice_Base0, (APTR) &pciregbase);
//    KPRINTF(1000, ("XHCI MMIO address space (%p)\n",pciregbase));

    // Store capregbase in xhc_capregbase
    hc->xhc_capregbase = (APTR) pciregbase;
    KPRINTF(1000, ("xhc_capregbase (%p)\n",hc->xhc_capregbase));

    // Store opregbase in xhc_opregbase
    hc->xhc_opregbase = (APTR) ((ULONG) pciregbase + capreg_readb(XHCI_CAPLENGTH));
    KPRINTF(1000, ("xhc_opregbase (%p)\n",hc->xhc_opregbase));

//    KPRINTF(1000, ("XHCI CAPLENGTH (%02x)\n",   capreg_readb(XHCI_CAPLENGTH)));
//    KPRINTF(1000, ("XHCI Version (%04x)\n",     capreg_readw(XHCI_HCIVERSION)));
//    KPRINTF(1000, ("XHCI HCSPARAMS1 (%08x)\n",  capreg_readl(XHCI_HCSPARAMS1)));
//    KPRINTF(1000, ("XHCI HCSPARAMS2 (%08x)\n",  capreg_readl(XHCI_HCSPARAMS2)));
//    KPRINTF(1000, ("XHCI HCSPARAMS3 (%08x)\n",  capreg_readl(XHCI_HCSPARAMS3)));
//    KPRINTF(1000, ("XHCI HCCPARAMS (%08x)\n",   capreg_readl(XHCI_HCCPARAMS)));

    /*
        This field defines the page size supported by the xHC implementation.
        This xHC supports a page size of 2^(n+12) if bit n is Set. For example,
        if bit 0 is Set, the xHC supports 4k byte page sizes.
    */
    cnt = 12;
    temp = opreg_readl(XHCI_PAGESIZE)&0xffff;
    while((~temp&1) & temp){
        temp = temp>>1;
        cnt++;
    }
    hc->xhc_pagesize = 1<<(cnt);
    KPRINTF(1000, ("Pagesize 2^(n+12) = 0x%lx\n", hc->xhc_pagesize));

    hc->xhc_scratchbufs = XHCV_SPB_Max(capreg_readl(XHCI_HCSPARAMS2));
    KPRINTF(1000, ("Max Scratchpad Buffers %lx\n",hc->xhc_scratchbufs));

    hc->hc_NumPorts = XHCV_MaxPorts(capreg_readl(XHCI_HCSPARAMS1));
    KPRINTF(1000, ("MaxPorts %lx\n",hc->hc_NumPorts));

    /*
        Number of Device Slots (MaxSlots). This field specifies the maximum number of Device
        Context Structures and Doorbell Array entries this host controller can support. Valid values are
        in the range of 1 to 255. The value of ‘0’ is reserved.
        FIXME: Fail gracefully on error (0)
    */
    hc->xhc_maxslots = XHCV_MaxSlots(capreg_readl(XHCI_HCSPARAMS1));
    KPRINTF(1000, ("MaxSlots %lx\n",hc->xhc_maxslots));

    KPRINTF(1000, ("MaxIntrs %lx\n",XHCV_MaxIntrs(capreg_readl(XHCI_HCSPARAMS1))));

    /* 64 byte or 32 byte context data structures? */
    if(capreg_readl(XHCI_HCCPARAMS) & XHCF_CSZ) {
        hc->xhc_contextsize64=TRUE; 
    }

    /* xHCI Extended Capabilities, search for USB Legacy Support */
    extcap = xhciExtCap(hc, XHCI_EXT_CAPS_LEGACY, 0);
    if(extcap) {

        temp = READMEM32_LE(extcap);
        if( (temp & XHCF_BIOSOWNED) ){
           KPRINTF(1000, ("controller owned by BIOS\n"));

           /* Spec says "no more than a second", we give it a little more */
           timeout = 250;

           WRITEMEM32_LE(extcap, (temp | XHCF_OSOWNED) );
           do {
               temp = READMEM32_LE(extcap);
               if( !(temp & XHCF_BIOSOWNED) ) {
                   KPRINTF(1000, ("BIOS gave up on XHCI. Pwned!\n"));
                   break;
               }
               uhwDelayMS(10, hu);
            } while(--timeout);

            if(!timeout) {
                KPRINTF(1000, ("BIOS didn't release XHCI. Forcing and praying...\n"));
                WRITEMEM32_LE(extcap, (temp & ~XHCF_BIOSOWNED) );
            }
        }
    }

    /* xHCI Extended Capabilities, search for Supported Protocol */
    extcap = 0;
    do {
        extcap = xhciExtCap(hc, XHCI_EXT_CAPS_PROTOCOL, extcap);
        if(extcap) {
            ;
        }
    } while (extcap);

    if(xhciHaltHC(hc)) {
        if(xhciResetHC(hc)) {

            for(cnt = 1; cnt <=hc->hc_NumPorts; cnt++) {
                temp = opreg_readl(XHCI_PORTSC(cnt));
                KPRINTF(1000, ("Port #%d speed is %d\n",cnt, (temp&XHCM_PS_SPEED)>>XHCB_PS_SPEED ));
            }

            hc->hc_PCIMemSize = 1024;   //Arbitrary number
            hc->hc_PCIMemSize += ((hc->xhc_scratchbufs) * (hc->xhc_pagesize));

            memptr = HIDD_PCIDriver_AllocPCIMem(hc->hc_PCIDriverObject, hc->hc_PCIMemSize);
            hc->hc_PCIMem = (APTR) memptr;

            if(memptr) {
                // PhysicalAddress - VirtualAdjust = VirtualAddress
                // VirtualAddress  + VirtualAdjust = PhysicalAddress
                hc->hc_PCIVirtualAdjust = ((ULONG) pciGetPhysical(hc, memptr)) - ((ULONG) memptr);
                KPRINTF(10, ("VirtualAdjust 0x%08lx\n", hc->hc_PCIVirtualAdjust));

                hc->hc_CompleteInt.is_Node.ln_Type = NT_INTERRUPT;
                hc->hc_CompleteInt.is_Node.ln_Name = "XHCI CompleteInt";
                hc->hc_CompleteInt.is_Node.ln_Pri  = 0;
                hc->hc_CompleteInt.is_Data = hc;
                hc->hc_CompleteInt.is_Code = (void (*)(void)) &xhciCompleteInt;

                // add reset handler
                hc->hc_ResetInt.is_Code = xhciResetHandler;
                hc->hc_ResetInt.is_Data = hc;
                AddResetCallback(&hc->hc_ResetInt);

                // add interrupt handler
                hc->hc_PCIIntHandler.h_Node.ln_Name = "XHCI PCI (pciusb.device)";
                hc->hc_PCIIntHandler.h_Node.ln_Pri = 5;
                hc->hc_PCIIntHandler.h_Code = xhciIntCode;
                hc->hc_PCIIntHandler.h_Data = hc;
                HIDD_IRQ_AddHandler(hd->hd_IRQHidd, &hc->hc_PCIIntHandler, hc->hc_PCIIntLine);

                /* Clears (RW1C) Host System Error(HSE), Event Interrupt(EINT), Port Change Detect(PCD) and Save/Restore Error(SRE) */
                temp = opreg_readl(XHCI_USBSTS);
                opreg_writel(XHCI_USBSTS, temp);

                /* After reset all notifications should be automatically disabled but ensure anyway */
                opreg_writel(XHCI_DNCTRL, 0);

                /* Program the Max Device Slots Enabled (MaxSlotsEn) field */
                opreg_writel(XHCI_CONFIG, ((opreg_readl(XHCI_CONFIG)&~XHCM_CONFIG_MaxSlotsEn) | XHCV_MaxSlots(capreg_readl(XHCI_HCSPARAMS1))) );

                /*
                    Program the Device Context Base Address Array Pointer (DCBAAP) register (5.4.6) with a 64-bit
                    address pointing to where the Device Context Base Address Array is located.

                    - The Device Context Base Address Array shall be indexed by the Device Slot ID.
                    - The Device Context Base Address Array shall be aligned to a 64 byte boundary.
                    - The Device Context Base Address Array shall be physically contiguous within a page.
                    - The Device Context Base Address Array shall contain MaxSlotsEn + 1 entries. The maximum size of the
                    - Device Context Base Address Array is 256 64-bit entries, or 2K Bytes.
                    - Software shall set Device Context Base Address Array entries for unallocated Device Slots to ‘0’.
                    - Software shall set Device Context Base Address Array entries for allocated Device Slots to point to the
                      Device Context data structure associated with the device.
                    FIXME: Fail gracefully on error (NULL memptr)
                */
                hc->xhc_dcbaa = AllocVecAlignedOn4KPage(&hc->xhc_dcbaa_original, (hc->xhc_maxslots + 1)*8, 64);
                KPRINTF(1000, ("XHCI: Allocated DCBAA\n"));

                /*
                    If the Max Scratchpad Buffers field of the HCSPARAMS2 register is > ‘0’, then the first entry (entry_0) in
                    the DCBAA shall contain a pointer to the Scratchpad Buffer Array. If the Max Scratchpad Buffers field of
                    the HCSPARAMS2 register is = ‘0’, then the first entry (entry_0) in the DCBAA is reserved and shall be
                    cleared to ‘0’ by software.

                        Chapter 4.20
                            System software shall allocate the Scratchpad Buffer(s) before placing the xHC in Run mode (Run/Stop(R/S) = ‘1’).

                            The following operations take place to allocate Scratchpad Buffers to the xHC:
                            1) Software examines the Max Scratchpad Buffers field in the HCSPARAMS2 register.
                            2) Software allocates a Scratchpad Buffer Array with Max Scratchpad Buffers entries.
                            3) Software writes the base address of the Scratchpad Buffer Array to the DCBAA (Slot 0) entry.
                            4) For each entry in the Scratchpad Buffer Array:
                                a. Software allocates a PAGESIZE Scratchpad Buffer.
                                b. Software writes the base address of the allocated Scratchpad Buffer to associated entry in the Scratchpad Buffer Array.

                    FIXME: Allocate Scratchpad Buffers and Scratchpad Buffer Array and fill it. Program DCBAA array entry_0 or clear it (MEMF_CLEAR takes care of that)
                */
                if(!hc->xhc_scratchbufs) {
                    ;
                }

                /* FIXME: Allocate device context data structures and fill rest of the DCBAA array*/

                /* Define the Command Ring Dequeue Pointer by programming the Command Ring Control Register */

                /* Set Run/Stop(R/S), Interrupter Enable(INTE) and Host System Error Enable(HSEE) */
                opreg_writel(XHCI_USBCMD, (XHCF_CMD_RS | XHCF_CMD_INTE | XHCF_CMD_HSEE) );

                KPRINTF(1000, ("xhciInit returns TRUE...\n"));
                return TRUE;
            }
        }
    }

    KPRINTF(1000, ("xhciInit returns FALSE...\n"));
    return FALSE;
}

void xhciFree(struct PCIController *hc, struct PCIUnit *hu) {

    hc = (struct PCIController *) hu->hu_Controllers.lh_Head;
    while(hc->hc_Node.ln_Succ)
    {
        switch(hc->hc_HCIType)
        {
            case HCITYPE_XHCI:
            {
                xhciHaltHC(hc);
                KPRINTF(1000, ("Shutting down XHCI %08lx\n", hc));
                uhwDelayMS(50, hu);
                SYNC;
                KPRINTF(1000, ("Shutting down XHCI done.\n"));
                break;
            }
        }

        hc = (struct PCIController *) hc->hc_Node.ln_Succ;
    }
}


/*
    Allocate aligned memory on 4K page boundaries
    -Could of cource use hc->xhc_pagesize...
    -Free with FreeVec on original ptr
    -Does not check if bytesize exceeds pagesize
*/
APTR AllocVecAlignedOn4KPage(APTR *original, ULONG bytesize, ULONG alignment) {

#define PAGE_SHIFT          12
#define PAGE_SIZE           (1 << PAGE_SHIFT)
#define PAGE_MASK           (~(PAGE_SIZE-1))
#define ALIGN_MASK          (~(alignment-1))
#define PAGE_ALIGN(addr)    (((addr)+PAGE_SIZE-1)&PAGE_MASK)

    APTR aligned = NULL;
    IPTR tmp;

    if (original) {
        *original = AllocVec(PAGE_SIZE + bytesize + alignment, (MEMF_PUBLIC | MEMF_CLEAR));
        KPRINTF(1000, ("4KAlloc: Original %lx\n", original));

        if (original) {
            tmp = (IPTR) original;
            tmp = (tmp + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            KPRINTF(1000, ("4KAlloc: Page alignment %lx\n", tmp));
            tmp = (tmp + alignment - 1) & ~(alignment - 1);
            KPRINTF(1000, ("4KAlloc: Final alignment %lx\n", tmp));
            aligned = (APTR) tmp;
        }
    }

    return aligned;
}

#endif
