/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Cadence GEM SANA-II driver, initialisation. The controller is
          found through pci.hidd - on the Pi 5 that is the synthetic
          function pcirp1.hidd presents for the GEM inside RP1 - and
          everything the PCI driver class provides (mapping, DMA memory and
          addresses, the interrupt) is reached through it from here.
*/

/* Bring-up diagnostics: probe results only. */
#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>
#include <aros/macros.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/openfirmware.h>

#include <devices/sana2.h>

#include "cdnsgem.h"

static OOP_AttrBase HiddPCIDeviceAttrBase;

/* Both sides NUL-terminated. Local rather than strcmp(): a driver reached
 * through a NULL StdCBase would fault on the libc one. */
static BOOL cdnsgem_streq(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b)
    {
        a++;
        b++;
    }

    return (*a == '\0' && *b == '\0');
}

static enum gem_phy_mode cdnsgem_phymode(APTR OpenFirmwareBase, APTR node)
{
    static const struct
    {
        const char         *name;
        enum gem_phy_mode   mode;
    } modes[] =
    {
        { "rgmii-rxid", GEM_PHY_MODE_RGMII_RXID },
        { "rgmii-txid", GEM_PHY_MODE_RGMII_TXID },
        { "rgmii-id",   GEM_PHY_MODE_RGMII_ID   },
        { "rgmii",      GEM_PHY_MODE_RGMII      },
    };
    APTR prop;
    const char *value;
    ULONG len, i;

    prop = OF_FindProperty(node, "phy-mode");
    if (prop && (len = OF_GetPropLen(prop)) > 0)
    {
        value = OF_GetPropValue(prop);

        if (value[len - 1] == '\0')
        {
            for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
            {
                if (cdnsgem_streq(value, modes[i].name))
                    return modes[i].mode;
            }
        }
    }

    return GEM_PHY_MODE_RGMII_ID;
}

/*
 * pci.hidd gives registers, DMA and the interrupt, but not how the board is
 * wired. That comes from the generic "cdns,macb" node when there is one;
 * the defaults are the Pi 5's (PHY 1, rgmii-id).
 */
static void cdnsgem_boardinfo(struct CDNSGEMBase *base, struct cdnsgem_hw *hw)
{
    APTR OpenFirmwareBase = base->cgm_OpenFirmwareBase;
    APTR node, phy, prop;

    hw->phyAddr = 1;
    hw->phyMode = GEM_PHY_MODE_RGMII_ID;
    hw->haveMacAddr = FALSE;

    if (!OpenFirmwareBase)
        return;

    node = OF_FindNodeByCompatible(NULL, "cdns,macb");
    if (!node)
        return;

    hw->phyMode = cdnsgem_phymode(OpenFirmwareBase, node);

    phy = OF_GetChild(node, NULL);
    if (phy)
    {
        prop = OF_FindProperty(phy, "reg");
        if (prop && OF_GetPropLen(prop) >= sizeof(ULONG))
            hw->phyAddr = AROS_BE2LONG(*(const ULONG *)OF_GetPropValue(prop)) & 0x1f;
    }

    /* Firmware fills this in at boot; the tree on disk holds zeros */
    prop = OF_FindProperty(node, "local-mac-address");
    if (prop && OF_GetPropLen(prop) == ETH_ADDRESSSIZE)
    {
        const UBYTE *mac = OF_GetPropValue(prop);

        CopyMem(mac, hw->macAddr, ETH_ADDRESSSIZE);
        hw->haveMacAddr = (mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) != 0;
    }
}

AROS_UFH3(static void, cdnsgem_enum,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, dev, A2),
    AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    OOP_Object **found = hook->h_Data;

    if (!*found)
        *found = dev;

    AROS_USERFUNC_EXIT
}

static OOP_Object *cdnsgem_findpci(struct CDNSGEMBase *base)
{
    OOP_Object *dev = NULL;
    struct Hook hook =
    {
        .h_Entry = (IPTR (*)())cdnsgem_enum,
        .h_Data  = &dev,
    };
    struct TagItem req[] =
    {
        { tHidd_PCI_VendorID,  CDNSGEM_PCI_VENDOR  },
        { tHidd_PCI_ProductID, CDNSGEM_PCI_PRODUCT },
        { TAG_DONE,            0                   }
    };

    HIDD_PCI_EnumDevices(base->cgm_PCI, &hook, req);

    return dev;
}

static BOOL cdnsgem_probe(struct CDNSGEMBase *base)
{
    struct cdnsgem_hw *hw = &base->cgm_HW;
    struct TagItem enable[] =
    {
        { aHidd_PCIDevice_isMEM,    TRUE },
        { aHidd_PCIDevice_isMaster, TRUE },
        { TAG_DONE,                 0    }
    };
    OOP_Object *dev;
    IPTR bar = 0, size = 0;
    ULONG mid, ver, cfg;
    LONG id1, id2;

    dev = cdnsgem_findpci(base);
    if (!dev)
    {
        D(bug("[cdnsgem] no GEM on any PCI bus\n");)
        return FALSE;
    }

    if (HIDD_PCIDevice_Obtain(dev, base->cgm_Device.dd_Library.lib_Node.ln_Name))
    {
        D(bug("[cdnsgem] GEM already owned\n");)
        return FALSE;
    }
    base->cgm_PCIDevice = dev;
    base->cgm_Owned = TRUE;

    OOP_GetAttr(dev, aHidd_PCIDevice_Driver, (IPTR *)&base->cgm_PCIDriver);
    OOP_GetAttr(dev, aHidd_PCIDevice_Base0, &bar);
    OOP_GetAttr(dev, aHidd_PCIDevice_Size0, &size);
    if (!base->cgm_PCIDriver || !bar || !size)
        return FALSE;

    hw->base = (IPTR)HIDD_PCIDriver_MapPCI(base->cgm_PCIDriver, (APTR)bar, size);
    if (!hw->base || hw->base == (IPTR)-1)
        return FALSE;

    OOP_SetAttrs(dev, enable);

    /* CFG6 and CFG8 exist from version 7; 64-bit DMA is what RP1 needs */
    mid = CDNSGEM_Read(hw, GEM_MID);
    ver = (mid & GEM_MID_VERSION_MASK) >> GEM_MID_VERSION_SHIFT;
    if (ver < 7)
    {
        D(bug("[cdnsgem] MID %08x: version %u not supported\n", mid, ver);)
        return FALSE;
    }

    cfg = CDNSGEM_Read(hw, GEM_CFG6);
    if (!(cfg & GEM_CFG6_DMA64))
    {
        D(bug("[cdnsgem] no 64-bit DMA (CFG6 %08x)\n", cfg);)
        return FALSE;
    }
    hw->qmask = 1 | GEM_CFG6_PRIQ_MASK(cfg);

    cfg = CDNSGEM_Read(hw, GEM_CFG8);
    hw->ntype1scr = GEM_CFG8_NUM_TYPE1_SCR(cfg);
    hw->ntype2scr = GEM_CFG8_NUM_TYPE2_SCR(cfg);

    cdnsgem_boardinfo(base, hw);

    /* MDIO needs the reset's clock divider and enable */
    CDNSGEM_HWReset(hw);

    id1 = CDNSGEM_MDIORead(hw, hw->phyAddr, MII_PHYSID1);
    id2 = CDNSGEM_MDIORead(hw, hw->phyAddr, MII_PHYSID2);

    D(bug("[cdnsgem] @ %p+%p, MID %08x, queues %04x, PHY %u: id %04x:%04x, mode %u\n",
          (APTR)hw->base, (APTR)size, mid, hw->qmask, hw->phyAddr, id1, id2,
          hw->phyMode);)

    if (id1 < 0 || id2 < 0 ||
        (id1 == 0 && id2 == 0) ||
        (id1 == 0xffff && id2 == 0xffff))
    {
        D(bug("[cdnsgem] PHY %u did not answer MDIO\n", hw->phyAddr);)
        return FALSE;
    }

    return TRUE;
}

APTR CDNSGEM_AllocDMA(struct CDNSGEMBase *base, ULONG size)
{
    return HIDD_PCIDriver_AllocPCIMem(base->cgm_PCIDriver, size);
}

void CDNSGEM_FreeDMA(struct CDNSGEMBase *base, APTR mem)
{
    HIDD_PCIDriver_FreePCIMem(base->cgm_PCIDriver, mem);
}

UQUAD CDNSGEM_BusAddress(struct CDNSGEMBase *base, APTR cpu)
{
    return (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(base->cgm_PCIDriver, cpu);
}

BOOL CDNSGEM_AddInterrupt(struct CDNSGEMBase *base, struct Interrupt *irq)
{
    return HIDD_PCIDevice_AddInterrupt(base->cgm_PCIDevice, irq);
}

void CDNSGEM_RemInterrupt(struct CDNSGEMBase *base, struct Interrupt *irq)
{
    HIDD_PCIDevice_RemoveInterrupt(base->cgm_PCIDevice, irq);
}

static void cdnsgem_release(struct CDNSGEMBase *base)
{
    if (base->cgm_Owned)
    {
        HIDD_PCIDevice_Release(base->cgm_PCIDevice);
        base->cgm_Owned = FALSE;
    }

    if (base->cgm_PCI)
    {
        OOP_DisposeObject(base->cgm_PCI);
        base->cgm_PCI = NULL;
    }

    if (HiddPCIDeviceAttrBase)
    {
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
        HiddPCIDeviceAttrBase = 0;
    }
}

static int CDNSGEM_Init(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->cgm_Found = FALSE;
    LIBBASE->cgm_Unit = NULL;
    InitSemaphore(&LIBBASE->cgm_UnitSem);

    /* Board details only; the driver works without it */
    LIBBASE->cgm_OpenFirmwareBase = OpenResource("openfirmware.resource");

    LIBBASE->cgm_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->cgm_UtilityBase)
        return FALSE;

    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    LIBBASE->cgm_PCI = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

    if (!HiddPCIDeviceAttrBase || !LIBBASE->cgm_PCI || !cdnsgem_probe(LIBBASE))
    {
        cdnsgem_release(LIBBASE);
        CloseLibrary(LIBBASE->cgm_UtilityBase);
        return FALSE;
    }

    LIBBASE->cgm_Found = TRUE;

    return TRUE;
}

static int CDNSGEM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->cgm_Unit)
    {
        CDNSGEM_DeleteUnit(LIBBASE, LIBBASE->cgm_Unit);
        LIBBASE->cgm_Unit = NULL;
    }

    cdnsgem_release(LIBBASE);

    if (LIBBASE->cgm_UtilityBase)
    {
        CloseLibrary(LIBBASE->cgm_UtilityBase);
        LIBBASE->cgm_UtilityBase = NULL;
    }

    return TRUE;
}

AROS_LH1(void, CDNSGEM_BeginIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, CDNSGEM)
{
    AROS_LIBFUNC_INIT

    struct CDNSGEMUnit *unit = (APTR)req->ios2_Req.io_Unit;

    req->ios2_Req.io_Error = 0;

    if (!unit || !unit->cgu_InputPort)
    {
        req->ios2_Req.io_Error = IOERR_OPENFAIL;
        if (!(req->ios2_Req.io_Flags & IOF_QUICK))
            ReplyMsg(&req->ios2_Req.io_Message);
        return;
    }

    if (AttemptSemaphore(&unit->cgu_Lock))
        CDNSGEM_HandleRequest(LIBBASE, req);
    else
    {
        req->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(unit->cgu_InputPort, &req->ios2_Req.io_Message);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, CDNSGEM_AbortIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, CDNSGEM)
{
    AROS_LIBFUNC_INIT

    /*
     * Still NT_MESSAGE means still sitting on some queue; anything else
     * is either being processed right now or already replied, and in
     * neither case may it be unlinked here.
     */
    Disable();
    if (req->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE &&
        (req->ios2_Req.io_Flags & IOF_QUICK) == 0)
    {
        Remove((struct Node *)req);
        req->ios2_Req.io_Error = IOERR_ABORTED;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        ReplyMsg(&req->ios2_Req.io_Message);
    }
    Enable();

    return 0;

    AROS_LIBFUNC_EXIT
}

static const IPTR cdnsgem_rx_tags[] =
{
    S2_CopyToBuff,
    S2_CopyToBuff16
};

static const IPTR cdnsgem_tx_tags[] =
{
    S2_CopyFromBuff,
    S2_CopyFromBuff16,
    S2_CopyFromBuff32
};

static int CDNSGEM_Open(LIBBASETYPEPTR LIBBASE, struct IORequest *io,
                        ULONG unitnum, ULONG flags)
{
    struct Library *UtilityBase = LIBBASE->cgm_UtilityBase;
    struct IOSana2Req *req = (struct IOSana2Req *)io;
    struct CDNSGEMUnit *unit;
    struct Opener *opener;
    struct TagItem *tags;
    ULONG i;

    io->io_Error = IOERR_OPENFAIL;
    io->io_Unit = NULL;

    if (!LIBBASE->cgm_Found || unitnum != 0)
        return FALSE;

    if (io->io_Message.mn_Length < sizeof(struct IOSana2Req))
        return FALSE;

    /* The hardware side comes up on first open and then stays up */
    ObtainSemaphore(&LIBBASE->cgm_UnitSem);
    if (!LIBBASE->cgm_Unit)
        LIBBASE->cgm_Unit = CDNSGEM_CreateUnit(LIBBASE);
    unit = LIBBASE->cgm_Unit;
    ReleaseSemaphore(&LIBBASE->cgm_UnitSem);
    if (!unit)
        return FALSE;

    opener = AllocMem(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
    if (!opener)
        return FALSE;

    tags = req->ios2_BufferManagement;
    for (i = 0; i < sizeof(cdnsgem_rx_tags) / sizeof(IPTR); i++)
        opener->rx_function =
            (APTR)GetTagData(cdnsgem_rx_tags[i], (IPTR)opener->rx_function,
                             tags);
    for (i = 0; i < sizeof(cdnsgem_tx_tags) / sizeof(IPTR); i++)
        opener->tx_function =
            (APTR)GetTagData(cdnsgem_tx_tags[i], (IPTR)opener->tx_function,
                             tags);
    opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, 0, tags);

    if (!opener->rx_function || !opener->tx_function)
    {
        FreeMem(opener, sizeof(struct Opener));
        return FALSE;
    }

    NEWLIST(&opener->read_port.mp_MsgList);
    opener->read_port.mp_Flags = PA_IGNORE;
    NEWLIST(&opener->initial_stats);

    if (flags & SANA2OPF_PROM)
        unit->cgu_Flags |= IFF_PROMISC;

    req->ios2_BufferManagement = opener;
    io->io_Unit = (struct Unit *)unit;
    io->io_Error = 0;

    Disable();
    AddTail((struct List *)&unit->cgu_Openers, (struct Node *)opener);
    Enable();
    unit->cgu_OpenCount++;

    return TRUE;
}

static int CDNSGEM_Close(LIBBASETYPEPTR LIBBASE, struct IORequest *io)
{
    struct IOSana2Req *req = (struct IOSana2Req *)io;
    struct CDNSGEMUnit *unit = (APTR)io->io_Unit;
    struct Opener *opener = req->ios2_BufferManagement;
    struct Node *node;

    if (unit && opener)
    {
        Disable();
        Remove((struct Node *)opener);
        Enable();

        while ((node = RemHead((struct List *)&opener->initial_stats)))
            FreeMem(node, sizeof(struct TypeStats));

        FreeMem(opener, sizeof(struct Opener));
        req->ios2_BufferManagement = NULL;
        unit->cgu_OpenCount--;
    }

    io->io_Unit = (struct Unit *)-1;
    io->io_Device = (struct Device *)-1;

    return TRUE;
}

ADD2INITLIB(CDNSGEM_Init, 0)
ADD2EXPUNGELIB(CDNSGEM_Expunge, 0)
ADD2OPENDEV(CDNSGEM_Open, 0)
ADD2CLOSEDEV(CDNSGEM_Close, 0)
