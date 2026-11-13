/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Intel VMD PCI device class - the devices behind a VMD.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "pcivmd.h"

#define DMSI(x)

#define PCICMF_INTDISABLE       (1 << 10)

#define rd16(reg)       VMD_RawRead(dom, data->bus, data->dev, data->sub, (reg), 2)
#define rd32(reg)       VMD_RawRead(dom, data->bus, data->dev, data->sub, (reg), 4)
#define wr16(reg, val)  VMD_RawWrite(dom, data->bus, data->dev, data->sub, (reg), 2, (val))
#define wr32(reg, val)  VMD_RawWrite(dom, data->bus, data->dev, data->sub, (reg), 4, (val))

OOP_Object *PCIVMDDev__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pcivmd_staticdata *psd = PSD(cl);
    OOP_Object *driver = (OOP_Object *)GetTagData(aHidd_PCIDevice_Driver, 0, msg->attrList);
    UBYTE bus = (UBYTE)GetTagData(aHidd_PCIDevice_Bus, 0, msg->attrList);
    UBYTE dev = (UBYTE)GetTagData(aHidd_PCIDevice_Dev, 0, msg->attrList);
    UBYTE sub = (UBYTE)GetTagData(aHidd_PCIDevice_Sub, 0, msg->attrList);
    struct VMDDomain *dom;
    struct pRoot_New pcidevNew;
    struct TagItem pcidevTags[] =
    {
        { aHidd_Name,                           (IPTR)"pcivmd.hidd"     },
        { aHidd_PCIDevice_ExtendedConfig,       0                       },
        { TAG_MORE,                             (IPTR)msg->attrList     }
    };
    OOP_Object *deviceObj;

    if (!driver)
        return NULL;
    dom = ((struct VMDBusData *)OOP_INST_DATA(psd->vmdDriverClass, driver))->domain;

    /* The whole of every function's configuration space is in the window */
    pcidevTags[1].ti_Data = (IPTR)VMD_ConfigAddr(dom, bus, dev, sub);

    pcidevNew.mID      = msg->mID;
    pcidevNew.attrList = pcidevTags;

    deviceObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, &pcidevNew.mID);
    if (deviceObj)
    {
        struct VMDDeviceData *data = OOP_INST_DATA(cl, deviceObj);

        data->domain = dom;
        data->bus = bus;
        data->dev = dev;
        data->sub = sub;
        data->vecMode = VMDVEC_NONE;
        data->vecCount = 0;
    }

    return deviceObj;
}

void PCIVMDDev__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct pcivmd_staticdata *psd = PSD(cl);
    struct VMDDeviceData *data = OOP_INST_DATA(cl, o);
    struct VMDDomain *dom = data->domain;
    ULONG idx;

    if (IS_PCIDEV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_PCIDevice_IRQLine:
                /*
                 * Whatever pin the device claims, it is wired to nothing:
                 * a VMD does not pass legacy interrupts on. Saying so
                 * keeps anyone from waiting for it to be routed.
                 */
                *msg->storage = 0;
                return;

            case aoHidd_PCIDevice_MSICount:
                {
                    UBYTE capmsix = VMD_FindCapability(dom, data->bus, data->dev, data->sub, PCICAP_MSIX);

                    /* Plain MSI has a single address, so it gets a single vector */
                    *msg->storage = 1;
                    if (capmsix)
                    {
                        UWORD tsize = (rd16(capmsix + PCIMSIX_FLAGS) & PCIMSIXF_QSIZE) + 1;

                        *msg->storage = (tsize < VMD_CHILD_MAXVECS) ? tsize : VMD_CHILD_MAXVECS;
                    }
                    return;
                }
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* The address a base register decodes, as the device sees it */
static UQUAD vmd_baraddr(struct VMDDeviceData *data, UBYTE bar)
{
    struct VMDDomain *dom = data->domain;
    UWORD reg = PCICS_BAR0 + (bar << 2);
    ULONG lo;
    UQUAD addr;

    if (bar > 5)
        return 0;

    lo = rd32(reg);
    if ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
        return 0;

    addr = lo & PCIBAR_MASK_MEM;
    if (((lo & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT) && (bar < 5))
        addr |= (UQUAD)rd32(reg + 4) << 32;

    return addr;
}

/* Choose the VMD vector that has the least routed to it */
static UBYTE vmd_pickvector(struct VMDDomain *dom)
{
    UWORD i, best = dom->vd_VecFirst;

    for (i = dom->vd_VecFirst; i < dom->vd_VecCount; i++)
    {
        if (dom->vd_Vec[i].users < dom->vd_Vec[best].users)
            best = i;
    }
    dom->vd_Vec[best].users++;

    return (UBYTE)best;
}

static void vmd_dropvectors(struct VMDDeviceData *data)
{
    struct VMDDomain *dom = data->domain;
    UBYTE i;

    for (i = 0; i < data->vecCount; i++)
    {
        if (dom->vd_Vec[data->vecMap[i]].users)
            dom->vd_Vec[data->vecMap[i]].users--;
    }
    data->vecCount = 0;
    data->vecMode = VMDVEC_NONE;
}

/*
 * Point the device's messages at the VMD. It does not deliver them
 * anywhere itself: it takes the destination out of the address, and
 * raises that one of its own vectors.
 */
BOOL VMD_ObtainVectors(struct VMDDeviceData *data, UWORD vectmin, UWORD vectmax)
{
    struct VMDDomain *dom = data->domain;
    UBYTE capmsi, capmsix;
    UWORD cmd;

    if (!dom->vd_VecCount)
        return FALSE;

    if (vectmin == 0)
        vectmin = 1;
    if (vectmax < vectmin)
        vectmax = vectmin;

    capmsi = VMD_FindCapability(dom, data->bus, data->dev, data->sub, PCICAP_MSI);
    capmsix = VMD_FindCapability(dom, data->bus, data->dev, data->sub, PCICAP_MSIX);

    Forbid();

    if (capmsix)
    {
        UWORD flags = rd16(capmsix + PCIMSIX_FLAGS);
        UWORD tsize = (flags & PCIMSIXF_QSIZE) + 1;
        ULONG table = rd32(capmsix + PCIMSIX_TABLE);
        UWORD cnt = vectmax, i;
        volatile struct vmd_msix_entry *mtab = NULL;
        UQUAD base;

        if (cnt > tsize)
            cnt = tsize;
        if (cnt > VMD_CHILD_MAXVECS)
            cnt = VMD_CHILD_MAXVECS;

        base = vmd_baraddr(data, table & PCIMSIXF_BIRMASK);
        if (base && (cnt >= vectmin))
            mtab = VMD_MapBus(dom, base + (table & ~PCIMSIXF_BIRMASK),
                              tsize * sizeof(struct vmd_msix_entry));

        if (mtab)
        {
            vmd_dropvectors(data);

            /* The table can only be reached while memory is decoded */
            cmd = rd16(PCICS_COMMAND);
            wr16(PCICS_COMMAND, cmd | PCICMF_MEMDECODE | PCICMF_INTDISABLE);

            /* Keep everything quiet while the table changes */
            flags |= PCIMSIXF_ENABLE | PCIMSIXF_MASKALL;
            wr16(capmsix + PCIMSIX_FLAGS, flags);

            for (i = 0; i < tsize; i++)
            {
                mtab[i].vector_ctrl = PCIMSIX_ENTRY_CTRL_MASKBIT;
                if (i < cnt)
                {
                    UBYTE vec = vmd_pickvector(dom);

                    data->vecMap[i] = vec;
                    mtab[i].msg_addr_lo = VMD_MSI_ADDR(vec);
                    mtab[i].msg_addr_hi = 0;
                    mtab[i].msg_data    = 0;
                    mtab[i].vector_ctrl = 0;
                    (void)mtab[i].vector_ctrl;
                }
            }

            if (capmsi)
                wr16(capmsi + PCIMSI_FLAGS, rd16(capmsi + PCIMSI_FLAGS) & ~PCIMSIF_ENABLE);

            flags &= ~PCIMSIXF_MASKALL;
            wr16(capmsix + PCIMSIX_FLAGS, flags);

            data->vecMode = VMDVEC_MSIX;
            data->vecCount = (UBYTE)cnt;

            Permit();

            DMSI(bug("[PCIVMD:Device] %s: %02x:%02x.%x %u MSI-X vector(s)\n", __func__,
                data->bus, data->dev, data->sub, cnt);)
            return TRUE;
        }
    }

    if (capmsi && (vectmin == 1))
    {
        UWORD flags = rd16(capmsi + PCIMSI_FLAGS);
        UBYTE vec;

        vmd_dropvectors(data);
        vec = vmd_pickvector(dom);
        data->vecMap[0] = vec;

        flags &= ~PCIMSIF_ENABLE;
        wr16(capmsi + PCIMSI_FLAGS, flags);

        wr32(capmsi + PCIMSI_ADDRESSLO, VMD_MSI_ADDR(vec));
        if (flags & PCIMSIF_64BIT)
        {
            wr32(capmsi + PCIMSI_ADDRESSHI, 0);
            wr16(capmsi + PCIMSI_DATA64, 0);
        }
        else
        {
            wr16(capmsi + PCIMSI_DATA32, 0);
        }

        cmd = rd16(PCICS_COMMAND);
        wr16(PCICS_COMMAND, cmd | PCICMF_INTDISABLE);

        /* One message only */
        flags &= ~PCIMSIF_MMEN_MASK;
        flags |= PCIMSIF_ENABLE;
        wr16(capmsi + PCIMSI_FLAGS, flags);

        data->vecMode = VMDVEC_MSI;
        data->vecCount = 1;

        Permit();

        DMSI(bug("[PCIVMD:Device] %s: %02x:%02x.%x MSI\n", __func__,
            data->bus, data->dev, data->sub);)
        return TRUE;
    }

    Permit();

    return FALSE;
}

void VMD_ReleaseVectors(struct VMDDeviceData *data)
{
    struct VMDDomain *dom = data->domain;
    UBYTE capmsi, capmsix;
    UWORD flags;

    capmsi = VMD_FindCapability(dom, data->bus, data->dev, data->sub, PCICAP_MSI);
    capmsix = VMD_FindCapability(dom, data->bus, data->dev, data->sub, PCICAP_MSIX);

    Forbid();

    if (capmsix)
    {
        flags = rd16(capmsix + PCIMSIX_FLAGS);
        if (flags & PCIMSIXF_ENABLE)
        {
            flags |= PCIMSIXF_MASKALL;
            flags &= ~PCIMSIXF_ENABLE;
            wr16(capmsix + PCIMSIX_FLAGS, flags);
        }
    }

    if (capmsi)
    {
        flags = rd16(capmsi + PCIMSI_FLAGS);
        if (flags & PCIMSIF_ENABLE)
            wr16(capmsi + PCIMSI_FLAGS, flags & ~PCIMSIF_ENABLE);
    }

    wr16(PCICS_COMMAND, rd16(PCICS_COMMAND) & ~PCICMF_INTDISABLE);

    vmd_dropvectors(data);

    Permit();
}

VOID PCIVMDDev__Hidd_PCIDevice__GetVectorAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_GetVectorAttribs *msg)
{
    struct pcivmd_staticdata *psd = PSD(cl);
    struct VMDDeviceData *data = OOP_INST_DATA(cl, o);
    struct VMDDomain *dom = data->domain;
    struct TagItem *tag, *tags = (struct TagItem *)msg->attribs;
    struct VMDVector *vec;

    if (msg->vectorno >= data->vecCount)
    {
        D(bug("[PCIVMD:Device] %s: no vector %u\n", __func__, msg->vectorno);)
        return;
    }
    vec = &dom->vd_Vec[data->vecMap[msg->vectorno]];

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case tHidd_PCIVector_Int:
                tag->ti_Data = vec->irq;
                break;

            case tHidd_PCIVector_Native:
                tag->ti_Data = vec->native;
                break;
        }
    }
}

BOOL PCIVMDDev__Hidd_PCIDevice__ObtainVectors(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_ObtainVectors *msg)
{
    struct pcivmd_staticdata *psd = PSD(cl);
    struct VMDDeviceData *data = OOP_INST_DATA(cl, o);
    UWORD vectmin = (UWORD)GetTagData(tHidd_PCIVector_Min, 1, msg->requirements);
    UWORD vectmax = (UWORD)GetTagData(tHidd_PCIVector_Max, 1, msg->requirements);

    return VMD_ObtainVectors(data, vectmin, vectmax);
}

VOID PCIVMDDev__Hidd_PCIDevice__ReleaseVectors(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_ReleaseVectors *msg)
{
    struct VMDDeviceData *data = OOP_INST_DATA(cl, o);

    VMD_ReleaseVectors(data);
}
