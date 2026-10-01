/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Intel VMD PCI bus driver - initialisation.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/bootloader.h>

#include <aros/asmcall.h>
#include <aros/bootloader.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include LC_LIBDEFS_FILE
#include "pcivmd.h"

#define VMD_FEATS_CLIENT        (VMDF_MEMBAR_SHADOW_VSCAP | VMDF_BUS_RESTRICTIONS | \
                                 VMDF_OFFSET_FIRST_VECTOR)

static const struct
{
    UWORD product;
    ULONG features;
} vmd_ids[] =
{
    { 0x201d, VMDF_MEMBAR_SHADOW_VSCAP                                              },
    { 0x28c0, VMDF_MEMBAR_SHADOW | VMDF_BUS_RESTRICTIONS | VMDF_CAN_BYPASS_MSI_REMAP },
    { 0x467f, VMD_FEATS_CLIENT                                                      },
    { 0x4c3d, VMD_FEATS_CLIENT                                                      },
    { 0xa77f, VMD_FEATS_CLIENT                                                      },
    { 0x7d0b, VMD_FEATS_CLIENT                                                      },
    { 0xad0b, VMD_FEATS_CLIENT                                                      },
    { 0x9a0b, VMD_FEATS_CLIENT                                                      },
    { 0xb06f, VMD_FEATS_CLIENT                                                      },
    { 0xb60b, VMD_FEATS_CLIENT                                                      },
    { 0,      0                                                                     }
};

struct vmd_candidate
{
    struct MinNode  node;
    OOP_Object      *device;
    ULONG           features;
};

struct vmd_enumdata
{
    struct pcivmd_staticdata    *psd;
    struct MinList              found;
};

static const char vmd_owner[] = "pcivmd.hidd";

AROS_UFH3(static void, vmd_enumhook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, device, A2),
    AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    struct vmd_enumdata *ed = hook->h_Data;
    struct pcivmd_staticdata *psd = ed->psd;
    IPTR product = 0;
    int i;

    OOP_GetAttr(device, aHidd_PCIDevice_ProductID, &product);

    for (i = 0; vmd_ids[i].product; i++)
    {
        if (vmd_ids[i].product == product)
        {
            struct vmd_candidate *c = AllocMem(sizeof(struct vmd_candidate), MEMF_CLEAR);

            if (c)
            {
                c->device = device;
                c->features = vmd_ids[i].features;
                ADDTAIL(&ed->found, &c->node);
            }
            break;
        }
    }

    AROS_USERFUNC_EXIT
}

/* Has the user asked for the VMDs to be left alone? ("novmd") */
static BOOL vmd_disabled(void)
{
    APTR BootLoaderBase = OpenResource("bootloader.resource");
    struct List *args;
    struct Node *node;

    if (!BootLoaderBase)
        return FALSE;

    args = (struct List *)GetBootInfo(BL_Args);
    if (!args)
        return FALSE;

    ForeachNode(args, node)
    {
        static const char key[] = "novmd";
        const char *s = node->ln_Name;
        int i;

        if (!s)
            continue;
        for (i = 0; key[i] && (s[i] == key[i]); i++);
        if (!key[i] && ((s[i] == '\0') || (s[i] == ' ')))
            return TRUE;
    }

    return FALSE;
}

#undef OOPBase

static int PCIVMD_Init(LIBBASETYPEPTR LIBBASE)
{
    struct pcivmd_staticdata *psd = &LIBBASE->psd;
    struct Library *OOPBase = psd->OOPBase;
    struct vmd_enumdata ed;
    struct vmd_candidate *c;
    struct Hook hook;
    struct TagItem reqs[] =
    {
        { tHidd_PCI_VendorID,   0x8086  },
        { TAG_DONE,             0       }
    };
    OOP_Object *pci;

    D(bug("[PCIVMD] %s()\n", __func__);)

    NEWLIST(&psd->domains);

    psd->kernelBase = OpenResource("kernel.resource");
    if (!psd->kernelBase)
        return FALSE;

    psd->utilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!psd->utilityBase)
        return FALSE;

    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    psd->hiddPCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (!psd->hiddAB || !psd->hiddPCIDriverAB || !psd->hiddPCIDeviceAB)
    {
        bug("[PCIVMD] %s: ObtainAttrBases failed\n", __func__);
        return FALSE;
    }

    psd->hiddPCIMB = OOP_GetMethodID(IID_Hidd_PCI, 0);
    psd->hiddPCIDriverMB = OOP_GetMethodID(IID_Hidd_PCIDriver, 0);
    psd->hiddPCIDeviceMB = OOP_GetMethodID(IID_Hidd_PCIDevice, 0);
    psd->hwMB = OOP_GetMethodID(IID_HW, 0);

    if (vmd_disabled())
    {
        bug("[PCIVMD] Disabled by the user\n");
        return TRUE;
    }

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (!pci)
    {
        D(bug("[PCIVMD] %s: PCI unavailable!\n", __func__);)
        return FALSE;
    }

    /*
     * The device list is locked while it is being walked, and adding a
     * bus adds to it. So only collect the VMDs here.
     */
    ed.psd = psd;
    NEWLIST(&ed.found);
    hook.h_Entry = (HOOKFUNC)vmd_enumhook;
    hook.h_Data = &ed;
    HIDD_PCI_EnumDevices(pci, &hook, reqs);

    while ((c = (struct vmd_candidate *)REMHEAD(&ed.found)) != NULL)
    {
        if (!HIDD_PCIDevice_Obtain(c->device, vmd_owner))
        {
            struct VMDDomain *dom = VMD_CreateDomain(psd, c->device, c->features);

            if (dom)
            {
                struct TagItem drvtags[] =
                {
                    { aHidd_DriverData, (IPTR)dom   },
                    { TAG_DONE,         0           }
                };

                ADDTAIL(&psd->domains, &dom->vd_Node);
                if (!HW_AddDriver(pci, psd->vmdDriverClass, drvtags))
                    bug("[PCIVMD] Failed to register the bus of VMD %04x\n", dom->vd_Product);
            }
            else
            {
                HIDD_PCIDevice_Release(c->device);
            }
        }
        FreeMem(c, sizeof(struct vmd_candidate));
    }

    OOP_DisposeObject(pci);

    return TRUE;
}

ADD2INITLIB(PCIVMD_Init, 10)

ADD2LIBS("pci.hidd", 0, static struct Library *, __pcihidd)
