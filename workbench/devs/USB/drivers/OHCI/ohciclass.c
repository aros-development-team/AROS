/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define DEBUG 1

#include <inttypes.h>

#include <exec/types.h>
#include <exec/ports.h>
#include <oop/oop.h>
#include <usb/usb.h>
#include <utility/tagitem.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <devices/timer.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/irq.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include "ohci.h"

static const usb_hub_descriptor_t hub_descriptor = {
    bDescLength:        sizeof(usb_hub_descriptor_t) - 31,
    bDescriptorType:    UDESC_HUB,
    bNbrPorts:          2,
    wHubCharacteristics:0,
    bPwrOn2PwrGood:     50,
    bHubContrCurrent:   0,
    DeviceRemovable:    {0,},
};

OOP_Object *METHOD(OHCI, Root, New)
{
    int success = 0;
    D(bug("[OHCI] OHCI::New()\n"));

    BASE(cl->UserData)->LibNode.lib_OpenCnt++;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        ohci_data_t *ohci = OOP_INST_DATA(cl, o);
        int i;

        ohci->pendingRHSC = 1;
        ohci->tr = ohci_CreateTimer();
        ohci->running = 0;

        NEWLIST(&ohci->intList);

        NEWLIST(&ohci->timerPort.mp_MsgList);
        ohci->timerPort.mp_Flags = PA_SOFTINT;
        ohci->timerPort.mp_Node.ln_Type = NT_MSGPORT;
        ohci->timerPort.mp_SigTask = &ohci->timerInt;
        ohci->timerInt.is_Code = OHCI_HubInterrupt;
        ohci->timerInt.is_Data = ohci;

        ohci->timerReq = CreateIORequest(&ohci->timerPort, sizeof(struct timerequest));
        OpenDevice((STRPTR)"timer.device", UNIT_VBLANK, (struct IORequest *)ohci->timerReq, 0);

        ohci->regs = (ohci_registers_t *)GetTagData(aHidd_OHCI_MemBase, 0, msg->attrList);
        ohci->pciDriver = (OOP_Object *)GetTagData(aHidd_OHCI_PCIDriver, 0, msg->attrList);
        ohci->pciDevice = (OOP_Object *)GetTagData(aHidd_OHCI_PCIDevice, 0, msg->attrList);
        ohci->irqNum = GetTagData(aHidd_OHCI_IRQ, 0, msg->attrList);

        ohci->hcca = HIDD_PCIDriver_AllocPCIMem(ohci->pciDriver, 4096);

        CopyMem(&hub_descriptor, &ohci->hubDescr, sizeof(usb_hub_descriptor_t));
        ohci->hubDescr.bNbrPorts = GetTagData(aHidd_USBHub_NumPorts, 0, msg->attrList);
        ohci->hubDescr.wHubCharacteristics = AROS_WORD2LE(UHD_PWR_NO_SWITCH | UHD_OC_INDIVIDUAL);

        ohci->irqHandler = AllocPooled(SD(cl)->memPool, sizeof(HIDDT_IRQ_Handler));

        ohci->irqHandler->h_Node.ln_Name = "UHCI Intr";
        ohci->irqHandler->h_Node.ln_Pri = 127;
        ohci->irqHandler->h_Code = ohci_Handler;
        ohci->irqHandler->h_Data = ohci;

        HIDD_IRQ_AddHandler(SD(cl)->irq, ohci->irqHandler, ohci->irqNum);
        D(bug("[OHCI] IRQHandler = %08x int = %d\n", ohci->irqHandler, ohci->irqNum));

        D(bug("[OHCI] New(): o=%p, ports=%d, regs=%p, drv=%p, dev=%p, hcca=%p\n", o,
              ohci->hubDescr.bNbrPorts, ohci->regs, ohci->pciDriver, ohci->pciDevice,
              ohci->hcca));

        ohci->sd = SD(cl);

        if (ohci->tmp)
            AddTail(&ohci->intList, &ohci->tmp->is_Node);

        /* Allocate empty endpoint descriptors for chaining */
        ohci->ctrl_head = ohci_AllocED(cl, o);
        ohci->ctrl_head->edFlags = AROS_LONG2OHCI(ED_K);
        ohci->ctrl_head->edNextED = 0;
        CacheClearE(ohci->ctrl_head, sizeof(ohci_ed_t), CACRF_ClearD);

        ohci->bulk_head = ohci_AllocED(cl, o);
        ohci->bulk_head->edFlags = AROS_LONG2OHCI(ED_K);
        ohci->bulk_head->edNextED = 0;
        CacheClearE(ohci->bulk_head, sizeof(ohci_ed_t), CACRF_ClearD);

        ohci->isoc_head = ohci_AllocED(cl, o);
        ohci->isoc_head->edFlags = AROS_LONG2OHCI(ED_K);
        ohci->isoc_head->edNextED = 0;
        CacheClearE(ohci->isoc_head, sizeof(ohci_ed_t), CACRF_ClearD);

        /*
         * The endpoints for interrupts.
         *
         * There are 63 endpoint descriptors used for interrupt
         * transfers. They form a tree, with several pooling rates
         * ranging from 1ms to 32ms. The 1ms endpoint points to the
         * isochronous queue.
         */
        ohci->int01 = ohci_AllocED(cl, o);
        ohci->int01->edFlags = AROS_LONG2OHCI(ED_K);
        ohci->int01->edNextED = AROS_LONG2OHCI((uint32_t)ohci->isoc_head);
        CacheClearE(ohci->int01, sizeof(ohci_ed_t), CACRF_ClearD);

        for (i=0; i < 2; i++)
        {
            ohci->int02[i] = ohci_AllocED(cl, o);
            ohci->int02[i]->edFlags = AROS_LONG2OHCI(ED_K);
            ohci->int02[i]->edNextED = AROS_LONG2OHCI((uint32_t)ohci->int01);
            CacheClearE(ohci->int02[i], sizeof(ohci_ed_t), CACRF_ClearD);
        }

        for (i=0; i < 4; i++)
        {
            ohci->int04[i] = ohci_AllocED(cl, o);
            ohci->int04[i]->edFlags = AROS_LONG2OHCI(ED_K);
            ohci->int04[i]->edNextED = AROS_LONG2OHCI((uint32_t)ohci->int02[i & 0x01]);
            CacheClearE(ohci->int04[i], sizeof(ohci_ed_t), CACRF_ClearD);
        }

        for (i=0; i < 8; i++)
        {
            ohci->int08[i] = ohci_AllocED(cl, o);
            ohci->int08[i]->edFlags = AROS_LONG2OHCI(ED_K);
            ohci->int08[i]->edNextED = AROS_LONG2OHCI((uint32_t)ohci->int04[i & 0x03]);
            CacheClearE(ohci->int08[i], sizeof(ohci_ed_t), CACRF_ClearD);
        }

        for (i=0; i < 16; i++)
        {
            ohci->int16[i] = ohci_AllocED(cl, o);
            ohci->int16[i]->edFlags = AROS_LONG2OHCI(ED_K);
            ohci->int16[i]->edNextED = AROS_LONG2OHCI((uint32_t)ohci->int08[i & 0x07]);
            CacheClearE(ohci->int16[i], sizeof(ohci_ed_t), CACRF_ClearD);
        }

        for (i=0; i < 32; i++)
        {
            ohci->int32[i] = ohci_AllocED(cl, o);
            ohci->int32[i]->edFlags = AROS_LONG2OHCI(ED_K);
            ohci->int32[i]->edNextED = AROS_LONG2OHCI((uint32_t)ohci->int16[i & 0x0f]);
            CacheClearE(ohci->int32[i], sizeof(ohci_ed_t), CACRF_ClearD);

            /* Link this pointers with HCCA table */
            ohci->hcca->hccaIntrTab[i] = AROS_LONG2OHCI((uint32_t)ohci->int32[i]);
        }
        CacheClearE((APTR)ohci->hcca, sizeof(ohci_hcca_t), CACRF_ClearD);

        /* Reset OHCI */

        /*
         * Preserve some registers which were set by BIOS. I will have
         * to get rid of it pretty soon
         */
        uint32_t ctl = AROS_OHCI2LONG(mmio(ohci->regs->HcControl));
        uint32_t rwc = ctl | HC_CTRL_RWC;
        uint32_t fm = AROS_OHCI2LONG(mmio(ohci->regs->HcFmInterval));
        uint32_t desca = AROS_OHCI2LONG(mmio(ohci->regs->HcRhDescriptorA));
        uint32_t descb = AROS_OHCI2LONG(mmio(ohci->regs->HcRhDescriptorB));

        D(bug("[OHCI] ctl=%08x fm=%08x desca=%08x descb=%08x\n", ctl,fm,desca,descb));

        desca &= ~HC_RHA_NPS;
        desca |= HC_RHA_PSM;
        descb |= 0x7fff0000;


        for (i=0; i < ohci->hubDescr.bNbrPorts; i++)
        {
            mmio(ohci->regs->HcRhPortStatus[i]) = AROS_LONG2OHCI(UPS_LOW_SPEED);
            ohci_Delay(ohci->tr, ohci->hubDescr.bPwrOn2PwrGood * UHD_PWRON_FACTOR + USB_EXTRA_POWER_UP_TIME);
        }

        mmio(ohci->regs->HcControl) = AROS_LONG2OHCI(rwc | HC_CTRL_HCFS_RESET);
        ohci_Delay(ohci->tr, USB_BUS_RESET_DELAY);

        mmio(ohci->regs->HcCommandStatus) = AROS_LONG2OHCI(HC_CS_HCR);
        for (i=0; i < 10; i++)
        {
            ohci_Delay(ohci->tr, 1);
            if (!(mmio(ohci->regs->HcCommandStatus) & AROS_LONG2OHCI(HC_CS_HCR)))
                break;
        }

        if (i==10)
            D(bug("[OHCI] Reset not ready...\n"));

        /* Initial setup of OHCI */
        D(bug("[OHCI] Initial setup\n"));

        mmio(ohci->regs->HcHCCA) = AROS_LONG2OHCI((uint32_t)ohci->hcca);
        mmio(ohci->regs->HcBulkHeadED) = AROS_LONG2OHCI((uint32_t)ohci->bulk_head);
        mmio(ohci->regs->HcControlHeadED) = AROS_LONG2OHCI((uint32_t)ohci->ctrl_head);
        mmio(ohci->regs->HcInterruptDisable) = AROS_LONG2OHCI(0xc000007f);

        ctl = AROS_OHCI2LONG(mmio(ohci->regs->HcControl));
        ctl &= ~(HC_CTRL_CBSR_MASK | HC_CTRL_HCFS_MASK | HC_CTRL_PLE |
                 HC_CTRL_IE | HC_CTRL_CLE | HC_CTRL_BLE | HC_CTRL_IR );
        ctl |= HC_CTRL_PLE | HC_CTRL_IE | HC_CTRL_CLE | HC_CTRL_BLE |
               rwc | HC_CTRL_CBSR_1_4 | HC_CTRL_HCFS_SUSPENDED;

        /* Start OHCI */
        mmio(ohci->regs->HcControl) = AROS_LONG2OHCI(ctl);

        uint32_t ival = HC_FM_GET_IVAL(fm);
        D(bug("[OHCI] ival=%08x\n", ival));
        fm = (AROS_OHCI2LONG(mmio(ohci->regs->HcFmRemaining)) & HC_FM_FIT) ^ HC_FM_FIT;
        fm |= HC_FM_FSMPS(ival) | ival;
        D(bug("[OHCI] fm=%08x\n", fm));
        mmio(ohci->regs->HcFmInterval) = AROS_LONG2OHCI(fm);
        uint32_t per = HC_PERIODIC(ival);
        mmio(ohci->regs->HcPeriodicStart) = AROS_LONG2OHCI(per);
	D(bug("[OHCI] periodic start=%08x\n", per));

        mmio(ohci->regs->HcRhDescriptorA) = AROS_LONG2OHCI(desca | HC_RHA_NOCP);
        mmio(ohci->regs->HcRhStatus) = AROS_LONG2OHCI(HC_RHS_LPS);
        ohci_Delay(ohci->tr, 5);
        mmio(ohci->regs->HcRhDescriptorA) = AROS_LONG2OHCI(desca);
        mmio(ohci->regs->HcRhDescriptorB) = AROS_LONG2OHCI(descb);

        if (HC_RHA_GET_POTPGT(desca) != 0)
        {
            D(bug("[OHCI] delay=%d\n", HC_RHA_GET_POTPGT(desca) * UHD_PWRON_FACTOR));
            ohci_Delay(ohci->tr, HC_RHA_GET_POTPGT(desca) * UHD_PWRON_FACTOR);
        }

        /* Enable interrupts */
        mmio(ohci->regs->HcInterruptEnable) = AROS_LONG2OHCI(HC_INTR_MIE | HC_INTR_SO | HC_INTR_WDH |
            HC_INTR_RD | HC_INTR_UE | HC_INTR_RHSC);

        D(bug("[OHCI] OHCI controller up and running.\n"));
        success = TRUE;
    }

    if (!success)
    {
        OOP_MethodID mID = OOP_GetMethodID((STRPTR)IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg)&mID);
        o = NULL;
    }

    D(bug("[OHCI] OHCI::New() = %p\n",o));

    if (!o)
        BASE(cl->UserData)->LibNode.lib_OpenCnt--;

    return o;
}

struct pRoot_Dispose {
    OOP_MethodID        mID;
};

void METHOD(OHCI, Root, Dispose)
{
    ohci_data_t *ohci = OOP_INST_DATA(cl, o);
    struct Library *base = &BASE(cl->UserData)->LibNode;

    ohci_DeleteTimer(ohci->tr);

    D(bug("[OHCI] OHCI::Dispose\n"));

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    base->lib_OpenCnt--;
}

void METHOD(OHCI, Root, Get)
{
    uint32_t idx;

    if (IS_USBDEVICE_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_USBDevice_Address:
                *msg->storage = 1;
                break;
            case aoHidd_USBDevice_Hub:
                *msg->storage = 0;
                break;
            case aoHidd_USBDevice_Bus:
                *msg->storage = (intptr_t)o;
                break;
            default:
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
    else
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* Class initialization and destruction */

#undef SD
#define SD(x) (&LIBBASE->sd)

static int OHCI_InitClass(LIBBASETYPEPTR LIBBASE)
{
    int i;
    D(bug("[OHCI] InitClass\n"));

    HiddOHCIAttrBase = OOP_ObtainAttrBase(IID_Drv_USB_OHCI);
    LIBBASE->sd.irq = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);

    if (HiddOHCIAttrBase)
    {
        struct TagItem tags[] = {
                { aHidd_OHCI_MemBase,           0UL },
                { aHidd_OHCI_PCIDevice,         0UL },
                { aHidd_OHCI_PCIDriver,         0UL },
                { aHidd_OHCI_IRQ,				0UL },
                { aHidd_USBHub_NumPorts,        0UL },
                { aHidd_USBHub_IsRoot,          1UL },
                { aHidd_USBDevice_Address,      1UL },
                { TAG_DONE, 0UL },
        };

        for (i=0; i < LIBBASE->sd.numDevices; i++)
        {
            tags[0].ti_Data = LIBBASE->sd.ramBase[i];
            tags[1].ti_Data = (intptr_t)LIBBASE->sd.pciDevice[i];
            tags[2].ti_Data = (intptr_t)LIBBASE->sd.pciDriver[i];
            tags[3].ti_Data = (intptr_t)LIBBASE->sd.irqNum[i];
            tags[4].ti_Data = (intptr_t)LIBBASE->sd.numPorts[i];

            D(bug("[OHCI] Initializing driver object: dev=%p, drv=%p, %d ports @ %p\n",
                  LIBBASE->sd.pciDevice[i], LIBBASE->sd.pciDriver[i], LIBBASE->sd.numPorts[i],
                  LIBBASE->sd.ramBase[i]));

            LIBBASE->sd.ohciDevice[i] = OOP_NewObject(NULL, CLID_Drv_USB_OHCI, tags);
            HIDD_USB_AttachDriver(LIBBASE->sd.usb, LIBBASE->sd.ohciDevice[i]);
        }
    }

    return TRUE;
}

static int OHCI_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[OHCI] ExpungeClass\n"));

    OOP_ReleaseAttrBase(IID_Drv_USB_OHCI);

    return TRUE;
}

ADD2INITLIB(OHCI_InitClass, 0)
ADD2EXPUNGELIB(OHCI_ExpungeClass, 0)
