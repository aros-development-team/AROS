/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386/x86_64 native PCI device support routines.
    Lang: English
*/

#include <aros/debug.h>

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/types.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <string.h>

#include "pcipc.h"

#define DMSI(x) x

UBYTE PCIPCDev__Hidd_PCIDevice__VectorIRQ(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_VectorIRQ *msg)
{
    IPTR capmsi, driver;
    UBYTE vectirq = 0;

    D(bug("[PCIPC:Device] %s()\n", __func__);)

    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSI, &capmsi);
    OOP_GetAttr(o, aHidd_PCIDevice_Driver, &driver);

    /* Is MSI even supported? */
    if (capmsi)
    {
        struct pHidd_PCIDevice_ReadConfigWord cmeth;
        UWORD msiflags;

        cmeth.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        cmeth.reg = capmsi + PCIMSI_FLAGS;
        msiflags = (UWORD)OOP_DoMethod(o, &cmeth.mID);
        if (msiflags & PCIMSIF_ENABLE)
        {
            DMSI(bug("[PCIPC:Device] %s: MSI Queue size = %u\n", __func__, ((msiflags & PCIMSIF_MMEN_MASK) >> 4));)
            /* MSI is enabled .. but is the requested vector valid? */
            if (msg->vector < ((msiflags & PCIMSIF_MMEN_MASK) >> 4))
            {
                UWORD msimdr;

                if (!(msiflags & PCIMSIF_64BIT))
                {
                    cmeth.reg = capmsi + PCIMSI_DATA32;
                    msimdr = (UWORD)OOP_DoMethod(o, &cmeth.mID);
                }
                else
                {
                    cmeth.reg = capmsi + PCIMSI_DATA64;
                    msimdr = (UWORD)OOP_DoMethod(o, &cmeth.mID);
                }
                DMSI(bug("[PCIPC:Device] %s: msimdr = %04x\n", __func__, msimdr);)

                vectirq = (msimdr & 0xFF) + msg->vector;
            }
            else bug("[PCIPC:Device] %s: Illegal MSI vector %u\n", __func__, msg->vector);
        }
        else bug("[PCIPC:Device] %s: MSI is dissabled for the device\n", __func__);
    }
    else bug("[PCIPC:Device] %s: Device doesn't support MSI\n", __func__);
    /* If MSI wasnt enabled and they have just asked for the first vector - return the PCI int line */
    if (!vectirq && msg->vector == 0)
    {
        struct pHidd_PCIDevice_ReadConfigByte cmeth;
        cmeth.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigByte;
        cmeth.reg = capmsi + PCICS_INT_LINE;
        vectirq = (UBYTE)OOP_DoMethod(o, &cmeth.mID);
    }
    return vectirq;
}

BOOL PCIPCDev__Hidd_PCIDevice__ObtainVectors(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_ObtainVectors *msg)
{
    IPTR capmsi, capmsix, driver;

    D(bug("[PCIPC:Device] %s()\n", __func__);)

    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSI, &capmsi);
    OOP_GetAttr(o, aHidd_PCIDevice_CapabilityMSIX, &capmsix);
    OOP_GetAttr(o, aHidd_PCIDevice_Driver, &driver);

    if (capmsix)
    {
        DMSI(bug("[PCIPC:Device] %s: Device has MSI-X capability @ %u\n", __func__, capmsix);)
    }
    if (capmsi)
    {
        union {
            struct pHidd_PCIDevice_WriteConfigWord wcw;
            struct pHidd_PCIDevice_WriteConfigLong wcl;
        } cmeth;
        UWORD vectmin, vectmax, vectcnt;
        ULONG apicIRQBase = 0;
        UWORD msiflags;

        DMSI(bug("[PCIPC:Device] %s: Device has MSI capability @ %u\n", __func__, capmsi);)

        cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_ReadConfigWord;
        cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
        msiflags = (UWORD)OOP_DoMethod(o, &cmeth.wcw.mID);

        DMSI(bug("[PCIPC:Device] %s: Max Device MSI vectors = %u\n", __func__, (msiflags & PCIMSIF_MMC_MASK) >> 1);)

        vectmin = (UWORD)GetTagData(tHidd_PCIVector_Min, 0, msg->requirements);
        if (vectmin > (msiflags & PCIMSIF_MMC_MASK) >> 1)
            return FALSE;

        vectmax = (UWORD)GetTagData(tHidd_PCIVector_Max, 0, msg->requirements);
        if (vectmax > (msiflags & PCIMSIF_MMC_MASK) >> 1)
            vectmax = (msiflags & PCIMSIF_MMC_MASK) >> 1;

        for (vectcnt = vectmax; vectcnt >= vectmax; vectcnt--)
        {
            if ((apicIRQBase = KrnAllocIRQ(IRQTYPE_MSI, vectcnt)) != (ULONG)-1)
            {
                DMSI(bug("[PCIPC:Device] %s: Allocated %u IRQs starting at #%u\n", __func__, vectmin, apicIRQBase);)
                break;
            }
        }
        if (apicIRQBase)
        {
            DMSI(bug("[PCIPC:Device] %s: Configuring Device with %u MSI vectors...\n", __func__, vectcnt);)

            msiflags &= ~(PCIMSIF_ENABLE | PCIMSIF_MMEN_MASK);

            cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
            cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
            cmeth.wcw.val = msiflags;
            OOP_DoMethod(o, &cmeth.wcw.mID); 

            msiflags |= (vectcnt << 4);
            DMSI(bug("[PCIPC:Device] %s: flags = %04x\n", __func__, msiflags);)

            cmeth.wcl.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigLong;
            cmeth.wcl.reg = capmsi + PCIMSI_ADDRESSLO;
            cmeth.wcl.val = (0xFEE << 20) | (0 << 12) | (0x3 << 2);
            OOP_DoMethod(o, &cmeth.wcw.mID);

            if (msiflags & PCIMSIF_64BIT)
            {
                DMSI(bug("[PCIPC:Device] %s: #64bit device!\n", __func__);)
                cmeth.wcl.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigLong;
                cmeth.wcl.reg = capmsi + PCIMSI_ADDRESSHI;
                cmeth.wcl.val = 0;
                OOP_DoMethod(o, &cmeth.wcw.mID);
            }
            cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
            cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
            cmeth.wcw.val = msiflags;
            OOP_DoMethod(o, &cmeth.wcw.mID);

            if (!(msiflags & PCIMSIF_64BIT))
            {
                cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
                cmeth.wcw.reg = capmsi + PCIMSI_DATA32;
                cmeth.wcw.val = apicIRQBase;
                OOP_DoMethod(o, &cmeth.wcw.mID);
            }
            else
            {
                cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
                cmeth.wcw.reg = capmsi + PCIMSI_DATA64;
                cmeth.wcw.val = apicIRQBase;
                OOP_DoMethod(o, &cmeth.wcw.mID);
            }

            msiflags |= PCIMSIF_ENABLE;
            cmeth.wcw.mID = HiddPCIDeviceBase + moHidd_PCIDevice_WriteConfigWord;
            cmeth.wcw.reg = capmsi + PCIMSI_FLAGS;
            cmeth.wcw.val = msiflags;
            OOP_DoMethod(o, &cmeth.wcw.mID);

            return TRUE;
        }
    }
    DMSI(bug("[PCIPC:Device] %s: Failed to obtain/enable MSI vectors\n", __func__);)

    return FALSE;
}

VOID PCIPCDev__Hidd_PCIDevice__ReleaseVectors(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_ReleaseVectors *msg)
{
    D(bug("[PCIPC:Device] %s()\n", __func__);)
    return;
}
