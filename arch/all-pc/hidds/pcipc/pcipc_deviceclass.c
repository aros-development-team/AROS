/*
    Copyright (C) 2020-2025, The AROS Development Team. All rights reserved.

    Desc: i386/x86_64 native PCI device support routines.
*/

#include <aros/debug.h>

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/acpica.h>

#include <aros/cpu.h>
#include <exec/types.h>
#include <utility/tagitem.h>
#include <oop/oop.h>
#include <hidd/pci.h>
#include <hardware/pci.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#include <string.h>

#include "pcipc.h"

#define DMSI(x)

OOP_Object *PCIPCDev__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object *driver = (OOP_Object *)GetTagData(aHidd_PCIDevice_Driver, 0, msg->attrList);
    ULONG deviceBus = (ULONG)GetTagData(aHidd_PCIDevice_Bus, 0, msg->attrList);
    ULONG deviceDev = (ULONG)GetTagData(aHidd_PCIDevice_Dev, 0, msg->attrList);
    ULONG deviceSub = (ULONG)GetTagData(aHidd_PCIDevice_Sub, 0, msg->attrList);
    struct pRoot_New pcidevNew;
    struct PCIPCBusData *ddata;
    struct TagItem pcidevTags[] =
    {
        { aHidd_Name,                           (IPTR)"pcipc.hidd"      },
        { aHidd_PCIDevice_ExtendedConfig,       0                       },
        { TAG_DONE,                             0                       }
    };
    IPTR mmconfig = 0;
    OOP_Object *deviceObj;

    ddata = OOP_INST_DATA(PSD(cl)->pcipcDriverClass, driver);

    pcidevNew.mID      = msg->mID;
    pcidevNew.attrList = pcidevTags;

    if (msg->attrList)
    {
        pcidevTags[2].ti_Tag  = TAG_MORE;
        pcidevTags[2].ti_Data = (IPTR)msg->attrList;
    }
 
    if(PSD(cl)->pcipc_acpiMcfgTbl) {
        ACPI_MCFG_ALLOCATION *mcfg_alloc;
        int i, nsegs = 0;
        ULONG offset;

        offset = sizeof(ACPI_TABLE_MCFG);
        mcfg_alloc = ACPI_ADD_PTR(ACPI_MCFG_ALLOCATION, PSD(cl)->pcipc_acpiMcfgTbl, offset);

        D(bug("[PCIPC:Device] %s: Parsing MCFG Table allocations...\n", __func__);)
        for (i = 0; offset + sizeof(ACPI_MCFG_ALLOCATION) <= PSD(cl)->pcipc_acpiMcfgTbl->Header.Length; i++)
        {
            D(bug("[PCIPC:Device] %s:     #%u %p - segment %d, bus %d-%d, address 0x%p\n",
                    __func__, i, mcfg_alloc, mcfg_alloc->PciSegment, mcfg_alloc->StartBusNumber, mcfg_alloc->EndBusNumber,
                    mcfg_alloc->Address);
            )
            nsegs++;
            if ((deviceBus <= mcfg_alloc->EndBusNumber) && (deviceBus >= mcfg_alloc->StartBusNumber))
            {
                D(bug("[PCIPC:Device] %s:       * bus %d, dev %d, sub %d\n", __func__, deviceBus, deviceDev, deviceSub);)

                mmconfig = ((IPTR)mcfg_alloc->Address) | (((deviceBus - mcfg_alloc->StartBusNumber) & 255)<<20) | ((deviceDev & 31) << 15) | ((deviceSub & 7) << 12);
                D(bug("[PCIPC:Device] %s:             MMIO @ 0x%p\n", __func__, mmconfig);)
                if (ddata->ecam)
                {
                    D(bug("[PCIPC:Device] %s:             ECAM Access\n", __func__);)
                    pcidevTags[1].ti_Data = mmconfig;
                }
                break;
            }
            offset += sizeof(ACPI_MCFG_ALLOCATION);
            mcfg_alloc = ACPI_ADD_PTR(ACPI_MCFG_ALLOCATION, PSD(cl)->pcipc_acpiMcfgTbl, offset);
        }
        D(bug("[PCIPC:Device] %s: checked %u segment allocation(s)\n", __func__, nsegs);)
    }

    deviceObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, &pcidevNew.mID);
    if (deviceObj)
    {
        struct PCIPCDeviceData *data = OOP_INST_DATA(cl, deviceObj);

        D(bug("[PCIPC:Device] %s: Device Object created @ 0x%p\n", __func__, deviceObj);)

        data->mmconfig = (APTR)mmconfig;
        /* If we didnt detect ECAM support from ACPI, try
         * to detct it now. */
        if (!deviceBus && !deviceDev && !deviceSub && !ddata->ecam)
        {
            struct pHidd_PCIDriver_ReadConfigLong msg;

            msg.mID = HiddPCIDeviceBase + moHidd_PCIDriver_ReadConfigLong;
            msg.device = deviceObj;
            msg.bus = msg.dev = msg.sub = 0;
            msg.reg = PCIEXBAR;
            ddata->ecam = (APTR)OOP_DoMethod(driver, (OOP_Msg)&msg);
            D(bug("[PCIPC:Device] %s: ECAM @ 0x%p\n", __func__, ddata->ecam);)
            if ((ddata->ecam) && (data->mmconfig))
            {
                pcidevTags[1].ti_Data = mmconfig;
                D(bug("[PCIPC:Device] %s: disposing original device object @ 0x%p\n", __func__, deviceObj);)
                OOP_DisposeObject(deviceObj) ;
                D(bug("[PCIPC:Device] %s: creating new instance ...\n", __func__);)
                deviceObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, &pcidevNew.mID);
                if (deviceObj)
                {
                    struct PCIPCDeviceData *data = OOP_INST_DATA(cl, deviceObj);
                    D(bug("[PCIPC:Device] %s: New Host Bridge Device @ 0x%p\n", __func__, deviceObj);)
                    data->mmconfig = (APTR)mmconfig;
                }
            }
        }
    }
    return deviceObj;
}

void PCIPCDev__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    BOOL handled = FALSE;
    ULONG idx;

    if (IS_PCIDEV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_PCIDevice_MSICount:
                {
                    IPTR capmsi;

                    handled = TRUE;
                    *msg->storage = 1;

                    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSI, &capmsi);
                    if (capmsi)
                    {
                        struct pHidd_PCIDevice_ReadConfigWord cmeth;
                        UWORD msiflags;

                        cmeth.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
                        cmeth.reg = capmsi + PCIMSI_FLAGS;
                        msiflags = (UWORD)OOP_DoMethod(o, &cmeth.mID);
                        *msg->storage = ( 1 << ((msiflags & PCIMSIF_MMC_MASK) >> 1));
                    }
                    break;
                }
        }
    }

    if (!handled)
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    }
}

APTR pci_get_bar_va(OOP_Class *cl, OOP_Object *o, UBYTE bar)
{
    IPTR base_phys = 0, bar_type = 0, bar_size = 0;

    /* Query base address, size, and type */
    switch (bar)
    {
        case 0:
            OOP_GetAttr(o, aHidd_PCIDevice_Base0, &base_phys);
            OOP_GetAttr(o, aHidd_PCIDevice_Size0, &bar_size);
            OOP_GetAttr(o, aHidd_PCIDevice_Type0, &bar_type);
            break;
        case 1:
            OOP_GetAttr(o, aHidd_PCIDevice_Base1, &base_phys);
            OOP_GetAttr(o, aHidd_PCIDevice_Size1, &bar_size);
            OOP_GetAttr(o, aHidd_PCIDevice_Type1, &bar_type);
            break;
        case 2:
            OOP_GetAttr(o, aHidd_PCIDevice_Base2, &base_phys);
            OOP_GetAttr(o, aHidd_PCIDevice_Size2, &bar_size);
            OOP_GetAttr(o, aHidd_PCIDevice_Type2, &bar_type);
            break;
        case 3:
            OOP_GetAttr(o, aHidd_PCIDevice_Base3, &base_phys);
            OOP_GetAttr(o, aHidd_PCIDevice_Size3, &bar_size);
            OOP_GetAttr(o, aHidd_PCIDevice_Type3, &bar_type);
            break;
        case 4:
            OOP_GetAttr(o, aHidd_PCIDevice_Base4, &base_phys);
            OOP_GetAttr(o, aHidd_PCIDevice_Size4, &bar_size);
            OOP_GetAttr(o, aHidd_PCIDevice_Type4, &bar_type);
            break;
        case 5:
            OOP_GetAttr(o, aHidd_PCIDevice_Base5, &base_phys);
            OOP_GetAttr(o, aHidd_PCIDevice_Size5, &bar_size);
            OOP_GetAttr(o, aHidd_PCIDevice_Type5, &bar_type);
            break;
        default:
            return NULL;
    }

    /* Ignore I/O BARs */
    if (bar_type & PCIBAR_TYPE_IO)
        return NULL;

    D(bug("[PCIPC:Device] %s: BAR%u phys=%p size=%08x virt=%p\n",
          __func__, bar, (APTR)base_phys, (ULONG)bar_size, virt);)

    return (APTR)base_phys;
}

VOID PCIPCDev__Hidd_PCIDevice__GetVectorAttribs(
    OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_GetVectorAttribs *msg)
{
    struct PCIPCDeviceData *data = OOP_INST_DATA(cl, o);
    IPTR capmsi = 0, capmsix = 0;

    D(bug("[PCIPC:Device] %s()\n", __func__);)

    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSI,  &capmsi);
    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSIX, &capmsix);

    /* Prefer MSI-X if present */
    if (capmsix)
    {
        struct pHidd_PCIDevice_ReadConfigWord wcw;
        struct pHidd_PCIDevice_ReadConfigLong wcl;
        UWORD msix_flags;
        ULONG table_dw;
        APTR bar_va = NULL;

        wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        wcw.reg = capmsix + PCIMSIX_FLAGS;
        msix_flags = (UWORD)OOP_DoMethod(o, &wcw.mID);

        if (msix_flags & (1 << 15))  /* MSI-X enabled? */
        {
            /* Read Table info (offset + BIR) */
            wcl.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigLong;
            wcl.reg = capmsix + PCIMSIX_TABLE;
            table_dw = (ULONG)OOP_DoMethod(o, &wcl.mID);

            UBYTE bir   = (UBYTE)(table_dw & 0x7);
            ULONG toff  = (table_dw & ~0x7u);

            /* Map the BAR */
            bar_va = pci_get_bar_va(cl, o, bir);
            if (bar_va)
            {
                volatile struct msix_entry *mtab =
                    (volatile struct msix_entry *)((UBYTE *)bar_va + toff);

                /* Check TableSize from Flags (bits 0..10) */
                UWORD table_size = (msix_flags & 0x07FF) + 1;
                if (msg->vectorno < table_size)
                {
                    ULONG msg_data = mtab[msg->vectorno].msg_data;
                    ULONG vector   = (msg_data & 0xFF);

                    struct TagItem *tag, *tags = (struct TagItem *)msg->attribs;
                    while ((tag = NextTagItem(&tags)))
                    {
                        switch (tag->ti_Tag)
                        {
                            case tHidd_PCIVector_Int:
                                tag->ti_Data = vector - HW_IRQ_BASE;
                                break;

                            case tHidd_PCIVector_Native:
                                tag->ti_Data = vector;
                                break;

                            default:
                                break;
                        }
                    }

                    DMSI(bug("[PCIPC:Device] %s: #%u MSI-X vector read from table -> %u\n",
                             __func__, msg->vectorno, vector);)
                    return;
                }
                else
                {
                    bug("[PCIPC:Device] %s: Illegal MSI-X vector %u (max=%u)\n",
                        __func__, msg->vectorno, table_size);
                    return;
                }
            }
            else
            {
                bug("[PCIPC:Device] %s: Could not map BAR %u for MSI-X table\n",
                    __func__, bir);
            }
        }
    }

    /* Fallback to MSI */
    if (capmsi)
    {
        struct pHidd_PCIDevice_ReadConfigWord cmeth;
        UWORD msiflags;

        cmeth.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        cmeth.reg = capmsi + PCIMSI_FLAGS;
        msiflags = (UWORD)OOP_DoMethod(o, &cmeth.mID);

        if (msiflags & PCIMSIF_ENABLE)
        {
            UWORD vecCount = (1 << ((msiflags & PCIMSIF_MMEN_MASK) >> 4));
            if (msg->vectorno < vecCount)
            {
                UWORD msg_data;
                struct pHidd_PCIDevice_ReadConfigWord rmsg;

                /* Read message data directly from config space */
                if (msiflags & PCIMSIF_64BIT)
                    rmsg.reg = capmsi + PCIMSI_DATA64;
                else
                    rmsg.reg = capmsi + PCIMSI_DATA32;

                rmsg.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
                msg_data = (UWORD)OOP_DoMethod(o, &rmsg.mID);

                UWORD vector = (msg_data & 0xFF);

                struct TagItem *tag, *tags = (struct TagItem *)msg->attribs;
                while ((tag = NextTagItem(&tags)))
                {
                    switch (tag->ti_Tag)
                    {
                        case tHidd_PCIVector_Int:
                            tag->ti_Data = vector - HW_IRQ_BASE;
                            break;

                        case tHidd_PCIVector_Native:
                            tag->ti_Data = vector;
                            break;

                        default:
                            break;
                    }
                }

                DMSI(bug("[PCIPC:Device] %s: #%u MSI vector read from config -> %u\n",
                         __func__, msg->vectorno, vector);)
                return;
            }
            else
            {
                bug("[PCIPC:Device] %s: Illegal MSI vector %u\n",
                    __func__, msg->vectorno);
            }
        }
    }
    D(bug("[PCIPC:Device] %s: Device has no active MSI/MSI-X\n", __func__);)
}

BOOL PCIPCDev__Hidd_PCIDevice__ObtainVectors(OOP_Class *cl, OOP_Object *o,
                                             struct pHidd_PCIDevice_ObtainVectors *msg)
{
    struct PCIPCDeviceData *data = OOP_INST_DATA(cl, o);
    IPTR capmsi = 0, capmsix = 0;

    D(bug("[PCIPC:Device] %s()\n", __func__);)

    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSI,  &capmsi);
    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSIX, &capmsix);

    /* Prefer MSI-X if present */
    if (capmsix) {
        union {
            struct pHidd_PCIDevice_WriteConfigWord wcw;
            struct pHidd_PCIDevice_WriteConfigLong wcl;
        } cmeth;
        UWORD msix_ctl;
        ULONG table_dw, pba_dw;
        UWORD vectmin = (UWORD)GetTagData(tHidd_PCIVector_Min, 1, msg->requirements);
        UWORD vectmax = (UWORD)GetTagData(tHidd_PCIVector_Max, 1, msg->requirements);

        /* Read MSI-X control and table pointers */
        cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        cmeth.wcw.reg = capmsix + PCIMSIX_FLAGS;
        msix_ctl = (UWORD)OOP_DoMethod(o, &cmeth.wcw.mID);

        cmeth.wcl.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigLong;
        cmeth.wcl.reg = capmsix + PCIMSIX_TABLE;
        table_dw = (ULONG)OOP_DoMethod(o, &cmeth.wcl.mID);

        cmeth.wcl.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigLong;
        cmeth.wcl.reg = capmsix + PCIMSIX_PBA;
        pba_dw = (ULONG)OOP_DoMethod(o, &cmeth.wcl.mID);

        /* Decode BIR and offsets; map the table BAR to VA */
        UBYTE table_bir   = (UBYTE)(table_dw & 0x7);
        ULONG table_off   = (table_dw & ~0x7u);
        volatile UBYTE *bar_va = pci_get_bar_va(cl, o, table_bir);
        if (!bar_va) {
            DMSI(bug("[PCIPC:Device] %s: MSI-X table BAR VA not mapped\n", __func__);)
        } else {
            /* Table size = (TableSize field + 1). Bits 0..10 of msix_ctl carry TableSize. */
            UWORD table_size = (msix_ctl & 0x07FF) + 1;

            /* Clamp requested vector count to the table size */
            if (vectmax == 0 || vectmax > table_size) vectmax = table_size;
            if (vectmin == 0) vectmin = 1;
            if (vectmin > vectmax) vectmin = vectmax;

            /* Choose count (naively, highest within range) */
            UWORD vectcnt;
            ULONG apicIRQBase = (ULONG)-1;

            /* Allocate a contiguous block of IRQs for MSI-X (one vector per entry) */
            for (vectcnt = vectmax; vectcnt >= vectmin; --vectcnt) {
                apicIRQBase = KrnAllocIRQ(IRQTYPE_APIC, vectcnt);
                if (apicIRQBase != (ULONG)-1) break;
            }
            if (apicIRQBase == (ULONG)-1) {
                DMSI(bug("[PCIPC:Device] %s: MSI-X IRQ allocation failed\n", __func__);)
                /* fall through to MSI path below */
            } else {
                volatile struct msix_entry *mtab =
                    (volatile struct msix_entry *)(bar_va + table_off);

                /* 1) Mask function, then enable later (spec-compliant sequence) */
                UWORD new_ctl = msix_ctl | (1u << 14);                              /* Function Mask = 1 */
                new_ctl      &= ~(1u << 15);                                        /* Ensure Enable=0 for programming */
                cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
                cmeth.wcw.reg = capmsix + PCIMSIX_FLAGS;
                cmeth.wcw.val = new_ctl;
                OOP_DoMethod(o, &cmeth.wcw.mID);

                /* 2) Program N table entries (masked individually) */
                ULONG apic_id = 0;                                                  /* we currently only handle APIC 0 */
                ULONG msg_addr_lo = MSI_ADDR_LO(apic_id);
                ULONG msg_addr_hi = MSI_ADDR_HI;

                for (UWORD i = 0; i < vectcnt; ++i) {
                    ULONG vector = (ULONG)(apicIRQBase + i + HW_IRQ_BASE);

                    mtab[i].vector_ctrl = 1;                                        /* mask this vector during setup */
                    mtab[i].msg_addr_lo = msg_addr_lo;
                    mtab[i].msg_addr_hi = msg_addr_hi;
                    mtab[i].msg_data    = MSI_DATA(vector);

                    /* Read back to post writes if needed (optional) */
                    (void)mtab[i].msg_data;
                }

                /* 3) Enable MSI-X with function masked */
                new_ctl |= (1u << 15);                                              /* Enable */
                cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
                cmeth.wcw.reg = capmsix + PCIMSIX_FLAGS;
                cmeth.wcw.val = new_ctl;
                OOP_DoMethod(o, &cmeth.wcw.mID);

                /* 4) Unmask table entries that the driver will actually use */
                for (UWORD i = 0; i < vectcnt; ++i) {
                    mtab[i].vector_ctrl = 0;                                        /* unmask */
                    (void)mtab[i].vector_ctrl;
                }

                /* 5) Finally clear Function Mask so vectors can fire */
                new_ctl &= ~(1u << 14);
                cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
                cmeth.wcw.reg = capmsix + PCIMSIX_FLAGS;
                cmeth.wcw.val = new_ctl;
                OOP_DoMethod(o, &cmeth.wcw.mID);

                DMSI(bug("[PCIPC:Device] %s: Enabled MSI-X %u vecs @ base IRQ %u (IDT vec %u)\n",
                         __func__, vectcnt, apicIRQBase, (apicIRQBase + HW_IRQ_BASE));)
                return TRUE;
            }
        }
    }

    /* MSI fallback */
    if (capmsi) {
        union {
            struct pHidd_PCIDevice_WriteConfigWord wcw;
            struct pHidd_PCIDevice_WriteConfigLong wcl;
        } cmeth;
        UWORD msiflags;
        UWORD vectmin, vectmax, vectcnt, vecthw;
        ULONG apicIRQBase = 0;

        cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
        msiflags = (UWORD)OOP_DoMethod(o, &cmeth.wcw.mID);

        if (msiflags & PCIMSIF_ENABLE)
            return FALSE;

        vecthw = (1 << ((msiflags & PCIMSIF_MMC_MASK) >> 1));
        vectmin = (UWORD)GetTagData(tHidd_PCIVector_Min, 1, msg->requirements);
        vectmax = (UWORD)GetTagData(tHidd_PCIVector_Max, 1, msg->requirements);
        if (vectmin > vecthw) return FALSE;
        if (vectmax == 0 || vectmax > vecthw) vectmax = vecthw;

        for (vectcnt = vectmax; vectcnt >= vectmin; vectcnt--) {
            apicIRQBase = KrnAllocIRQ(IRQTYPE_APIC, vectcnt);
            if (apicIRQBase != (ULONG)-1) break;
        }
        if (apicIRQBase != (ULONG)-1) {
            /* Program message address (xAPIC) */
            cmeth.wcl.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigLong;
            cmeth.wcl.reg = capmsi + PCIMSI_ADDRESSLO;
            cmeth.wcl.val = (0xFEEu << 20);                                         /* dest APIC ID = 0 in bits 19:12 */
            OOP_DoMethod(o, &cmeth.wcl.mID);

            data->msimsg = (UWORD)apicIRQBase + HW_IRQ_BASE;

            if (msiflags & PCIMSIF_64BIT) {
                cmeth.wcl.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigLong;
                cmeth.wcl.reg = capmsi + PCIMSI_ADDRESSHI;
                cmeth.wcl.val = 0;
                OOP_DoMethod(o, &cmeth.wcl.mID);

                cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
                cmeth.wcw.reg = capmsi + PCIMSI_DATA64;
                cmeth.wcw.val = data->msimsg;
                OOP_DoMethod(o, &cmeth.wcw.mID);
            } else {
                cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
                cmeth.wcw.reg = capmsi + PCIMSI_DATA32;
                cmeth.wcw.val = data->msimsg;
                OOP_DoMethod(o, &cmeth.wcw.mID);
            }

            /* Set Multiple Message Enable to the count we actually allocated */
            UWORD mme = ilog2(roundup_pow_of_two(vectcnt));
            msiflags = (msiflags & ~PCIMSIF_MMEN_MASK) | (mme << 4);
            msiflags |= PCIMSIF_ENABLE;

            cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
            cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
            cmeth.wcw.val = msiflags;
            OOP_DoMethod(o, &cmeth.wcw.mID);

            /* Verify */
            cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
            cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
            msiflags = (UWORD)OOP_DoMethod(o, &cmeth.wcw.mID);
            if (msiflags & PCIMSIF_ENABLE) {
                return TRUE;
            }
        }
        DMSI(bug("[PCIPC:Device] %s: Failed to obtain/enable MSI vectors\n", __func__);)
    }

    return FALSE;
}

VOID PCIPCDev__Hidd_PCIDevice__ReleaseVectors(
    OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_ReleaseVectors *msg)
{
    D(bug("[PCIPC:Device] %s()\n", __func__);)

    struct PCIPCDeviceData *data = OOP_INST_DATA(cl, o);
    IPTR capmsi = 0, capmsix = 0;

    /* Query both MSI and MSI-X capability offsets */
    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSI,  &capmsi);
    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSIX, &capmsix);

    /* Disable MSI-X first if present */
    if (capmsix) {
        union {
            struct pHidd_PCIDevice_WriteConfigWord wcw;
        } cmeth;
        UWORD msixflags;

        cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        cmeth.wcw.reg = capmsix + PCIMSIX_FLAGS;
        msixflags = (UWORD)OOP_DoMethod(o, &cmeth.wcw.mID);

        if (msixflags & (1 << 15)) {                                                /* MSI-X enabled? */
            DMSI(bug("[PCIPC:Device] %s: Disabling MSI-X (flags=%04x)\n",
                     __func__, msixflags);)

            msixflags |=  (1 << 14);                                                /* Mask all vectors */
            msixflags &= ~(1 << 15);                                                /* Disable MSI-X    */

            cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
            cmeth.wcw.reg = capmsix + PCIMSIX_FLAGS;
            cmeth.wcw.val = msixflags;
            OOP_DoMethod(o, &cmeth.wcw.mID);
        }
    }

    /* Disable legacy MSI if present */
    if (capmsi) {
        union {
            struct pHidd_PCIDevice_WriteConfigWord wcw;
        } cmeth;
        UWORD msiflags;

        cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
        msiflags = (UWORD)OOP_DoMethod(o, &cmeth.wcw.mID);

        if (msiflags & PCIMSIF_ENABLE) {
            DMSI(bug("[PCIPC:Device] %s: Disabling MSI (flags=%04x)\n",
                     __func__, msiflags);)

            msiflags &= ~PCIMSIF_ENABLE;

            cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
            cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
            cmeth.wcw.val = msiflags;
            OOP_DoMethod(o, &cmeth.wcw.mID);
        }
    }

    /* Re-enable INTx for legacy fallback after MSI off */
    {
        union { struct pHidd_PCIDevice_WriteConfigWord wcw; } cmeth;
        UWORD cmd;

        cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        cmeth.wcw.reg = PCICS_COMMAND;
        cmd = (UWORD)OOP_DoMethod(o, &cmeth.wcw.mID);

        cmd &= ~(1 << 10);                                                          /* clear Interrupt Disable bit */

        cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
        cmeth.wcw.reg = PCICS_COMMAND;
        cmeth.wcw.val = cmd;
        OOP_DoMethod(o, &cmeth.wcw.mID);
    }

    D(bug("[PCIPC:Device] %s: MSI/MSI-X vectors released for %02x:%02x.%x\n",
        __func__, data->bus, data->dev, data->func));
}
