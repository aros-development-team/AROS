/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI ATA bus driver
    Lang: English
*/

#define DSATA(x)

#include <aros/asmcall.h>
#include <aros/bootloader.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <hardware/ahci.h>
#include <hidd/ata.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include <string.h>

#include "library.h"
#include "pci.h"
#include "timer.h"

/*
 * Currently we support legacy ISA ports only on x86.
 * This can change only if someone ports AROS to PowerPC
 * retro-machine like PReP.
 */
#ifdef __i386__
#define SUPPORT_LEGACY
#endif
#ifdef __x86_64__
#define SUPPORT_LEGACY
#endif

struct ata_ProbedBus
{
    struct Node atapb_Node;
    UWORD       atapb_Vendor;
    UWORD       atapb_Product;
    IPTR        atapb_IOBase;
    IPTR        atapb_IOAlt;
    IPTR        atapb_INTLine;
    IPTR        atapb_DMABase;
    BOOL        atapb_80wire;
};

#define NAME_BUFFER 128

#define ATABUSNODEPRI_PROBED       50
#define ATABUSNODEPRI_PROBEDLEGACY 100
#define ATABUSNODEPRI_LEGACY       0

#define RANGESIZE0 8
#define RANGESIZE1 4
#define DMASIZE    16

/* static list of io/irqs that we can handle */
struct ata__legacybus 
{
    UWORD       lb_Port;
    UWORD       lb_Alt;
    UBYTE       lb_IRQ;
    UBYTE       lb_ControllerID;
    UBYTE       lb_Bus;
    const char *lb_Name;
};

static const struct ata__legacybus LegacyBuses[] = 
{
    {0x1f0, 0x3f4, 14, 0, 0, "ISA IDE0 primary channel"  },
    {0x170, 0x374, 15, 0, 1, "ISA IDE0 secondary channel"},
    {0x168, 0x36c, 10, 1, 0, "ISA IDE1 primary channel"  },
    {0x1e8, 0x3ec, 11, 1, 1, "ISA IDE1 secondary channel"},
    {    0,     0,  0, 0, 0, NULL                           }
};

/*
 * PCI BUS ENUMERATOR
 *   collect ALL ata/ide capable devices (including SATA and other) and
 *   spawn concurrent tasks.
 *
 * This function is growing too large. It will shorten drasticly once this whole mess gets converted into c++
 */

static
AROS_UFH3(void, ata_PCIEnumerator_h,
    AROS_UFHA(struct Hook *, hook,   A0),
    AROS_UFHA(OOP_Object *,  Device, A2),
    AROS_UFHA(APTR,          message,A1))
{
    AROS_USERFUNC_INIT

    struct ataBase *base = hook->h_Data;
    OOP_Object *Driver;
    IPTR ProductID, VendorID, DMABase, DMASize, INTLine;
    IPTR IOBase, IOAlt, IOSize, AltSize, SubClass, Interface;
    int x;
    STRPTR owner;

    /*
     * obtain more or less useful data
     */
    OOP_GetAttr(Device, aHidd_PCIDevice_Driver   , (IPTR *)&Driver);
    OOP_GetAttr(Device, aHidd_PCIDevice_VendorID , &VendorID);
    OOP_GetAttr(Device, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(Device, aHidd_PCIDevice_SubClass , &SubClass);
    OOP_GetAttr(Device, aHidd_PCIDevice_Base4    , &DMABase);
    OOP_GetAttr(Device, aHidd_PCIDevice_Size4    , &DMASize);
    OOP_GetAttr(Device, aHidd_PCIDevice_Interface, &Interface);

    D(bug("[PCI-ATA] ata_PCIEnumerator_h: Found IDE device %04x:%04x\n", VendorID, ProductID));

    /* First check subclass */
    if ((SubClass == PCI_SUBCLASS_SCSI) || (SubClass == PCI_SUBCLASS_SAS))
    {
        D(bug("{PCI-ATA] Unsupported subclass %d\n", SubClass));
        return;
    }
    
    owner = HIDD_PCIDevice_Obtain(Device, base->lib.lib_Node.ln_Name);
    if (owner)
    {
        D(bug("{PCI-ATA] Already owned by %s\n", owner));
        return;
    }

    /*
     * SATA controllers may need a special treatment before becoming usable.
     * The machine's firmware (EFI on Mac) may operate them in native AHCI mode
     * and do not set up legacy mode by itself.
     * In this case we have to do it ourselves.
     * This code is based on incomplete ahci.device source code by DissyOfCRN.
     * CHECKME: In order to work on PPC it uses explicit little-endian I/O,
     * assuning AHCI register file is always little-endian. Is it correct ?
     */
    if (SubClass == PCI_SUBCLASS_SATA)
    {
        APTR hba_phys = NULL;
        IPTR hba_size = 0;
        volatile struct ahci_hwhba *hwhba;
        ULONG ghc, cap;

        OOP_GetAttr(Device, aHidd_PCIDevice_Base5, (IPTR *)&hba_phys);
        OOP_GetAttr(Device, aHidd_PCIDevice_Size5, &hba_size);

        DSATA(bug("[PCI-ATA] Device %04x:%04x is a SATA device, HBA 0x%p, size 0x%p\n", VendorID, ProductID, hba_phys, hba_size));

        /*
         * Obtain PCIDriver method base (lazy).
         * Methods are numbered subsequently, the same as attributes. This means
         * we can use the same mechanism for them (get base value and add offsets).
         */
        if (!base->PCIDriverMethodBase)
            base->PCIDriverMethodBase = OOP_GetMethodID(IID_Hidd_PCIDriver, 0);

        hwhba = HIDD_PCIDriver_MapPCI(Driver, hba_phys, hba_size);
        DSATA(bug("[PCI-ATA] Mapped at 0x%p\n", hwhba));

        if (!hwhba)
        {
            DSATA(bug("[PCI-ATA] Mapping failed, device will be ignored\n"));

            HIDD_PCIDevice_Release(Device);
            return;
        }

        cap = mmio_inl_le(&hwhba->cap);
        ghc = mmio_inl_le(&hwhba->ghc);
        DSATA(bug("[PCI-ATA] Capabilities: 0x%08X, host control: 0x%08X\n", cap, ghc));

        /*
         * Some hardware may report GHC_AE to be zero, together with CAP_SAM set (indicating
         * that the device doesn't support legacy IDE registers). Seems to be spec violation
         * (the AHCI specification says that in this cases GHC_AE is read-only bit which is
         * hardwired to 1).
         * Attempting to drive such a hardware causes ata.device to freeze.
         * This effect has been observed on Marvel 9172 controller (and some other HW,
         * according to user reports, but nobody has ever provided a debug log).
         */
        if (cap & CAP_SAM)
        {
            DSATA(bug("[PCI-ATA] Legacy mode is not supported, device will be ignored\n"));

            HIDD_PCIDriver_UnmapPCI(Driver, hba_phys, hba_size);
            HIDD_PCIDevice_Release(Device);
            return;
        }

        if (ghc & GHC_AE)
        {
            DSATA(bug("[PCI-ATA] AHCI enabled\n"));

            /*
             * This is ATA driver, not SATA driver, so i'd like to keep SATA-specific code
             * at a minimum. None of tests revealed a real need for BIOS handoff, no BIOS
             * was discovered to use controllers in SMI mode.
             * However, if on some machine we have problems, we can try
             * to #define this.
             */
#ifdef DO_SATA_HANDOFF
            ULONG version = mmio_inl_le(&hwhba->vs);
            ULONG cap2    = mmio_inl_le(&hwhba->cap2);

            DSATA(bug("[PCI-ATA] Version: 0x%08X, Cap2: 0x%08X\n", version, cap2));

            if ((version >= AHCI_VERSION_1_20) && (cap2 && CAP2_BOH))
            {
                ULONG bohc;

                DSATA(bug("[PCI-ATA] HBA supports BIOS/OS handoff\n"));

                bohc = mmio_inl_le(&hwhba->bohc);
                if (bohc && BOHC_BOS)
                {
                    struct IORequest *timereq;

                    DSATA(bug("[PCI-ATA] Device owned by BIOS, performing handoff\n"));

                    /*
                     * We need timer.device in order to perform delays.
                     * TODO: in ata_InitBus() it will be opened and closed again.
                     * This is not optimal, it could be opened and closed just once.
                     */
                   timereq = ata_OpenTimer(a->ATABase);
                   if (!timereq)
                   {
                        DSATA(bug("[PCI-ATA] Failed to open timer, can't perform handoff. Device will be ignored\n"));

                        HIDD_PCIDriver_UnmapPCI(Driver, hba_phys, hba_size);
                        HIDD_PCIDevice_Release(Device);
                        return;
                   }

                    mmio_outl_le(bohc | BOHC_OOS, &hwhba->bohc);
                    /* Spin on BOHC_BOS bit FIXME: Possible dead lock. No maximum time given on AHCI1.3 specs... */
                    while (mmio_inl_le(&hwhba->bohc) & BOHC_BOS);

                    ata_WaitTO(timereq, 0, 25000);
                    /* If after 25ms BOHC_BB bit is still set give bios a minimum of 2 seconds more time to run */

                    if (mmio_inl_le(&hwhba->bohc) & BOHC_BB)
                    {
                        DSATA(bug("[PCI-ATA] Delayed handoff, waiting...\n"));
                        ata_WaitTO(timereq, 2, 0);
                    }

                    DSATA(bug("[PCI-ATA] Handoff done\n"));
                    ata_CloseTimer(timereq);
                }
            }
#endif
            /* This resets GHC_AE bit, disabling AHCI */
            mmio_outl_le(0, &hwhba->ghc);
        }

        HIDD_PCIDriver_UnmapPCI(Driver, hwhba, hba_size);
    }
    
    /*
     * we can have up to two buses assigned to this device
     */
    for (x = 0; x < MAX_DEVICEBUSES; x++)
    {
        BYTE basePri = ATABUSNODEPRI_PROBED;

        /*
         * obtain I/O bases and interrupt line
         */
        if ((Interface & (1 << (x << 1))) || SubClass != PCI_SUBCLASS_IDE)
        {
            switch (x)
            {
            case 0:
                OOP_GetAttr(Device, aHidd_PCIDevice_Base0, &IOBase);
                OOP_GetAttr(Device, aHidd_PCIDevice_Size0, &IOSize);
                OOP_GetAttr(Device, aHidd_PCIDevice_Base1, &IOAlt);
                OOP_GetAttr(Device, aHidd_PCIDevice_Size1, &AltSize);
                break;

            case 1:
                OOP_GetAttr(Device, aHidd_PCIDevice_Base2, &IOBase);
                OOP_GetAttr(Device, aHidd_PCIDevice_Size2, &IOSize);
                OOP_GetAttr(Device, aHidd_PCIDevice_Base3, &IOAlt);
                OOP_GetAttr(Device, aHidd_PCIDevice_Size3, &AltSize);
                break;
            }
            OOP_GetAttr(Device, aHidd_PCIDevice_INTLine, &INTLine);
        }
        else if (LegacyBuses[base->legacycount].lb_ControllerID == 0)
        {
            IPTR isa_io_base;

            OOP_GetAttr(Driver, aHidd_PCIDriver_IOBase, &isa_io_base);
            D(bug("[PCI-ATA] Device using Legacy-Bus IOPorts @ 0x%p\n", isa_io_base));

            IOBase   = LegacyBuses[base->legacycount].lb_IOBase + isa_io_base;
            IOAlt    = LegacyBuses[base->legacycount].lb_IOAlt  + isa_io_base;
            INTLine  = LegacyBuses[base->legacycount].lb_INTLine;
            basePri  = ATABUSNODEPRI_PROBEDLEGACY;
            IOSize   = RANGESIZE0;
            AltSize  = RANGESIZE1;

            base->legacycount++;
        }
        else
        {
            D(bug("[PCI-ATA] Ran out of legacy buses\n"));
            IOBase = 0;
        }

        if (IOBase != 0 && IOSize == RANGESIZE0 && AltSize == RANGESIZE1 &&
            (DMASize >= DMASIZE || DMABase == 0 || SubClass == PCI_SUBCLASS_IDE))
        {
            struct ata_ProbedBus *probedbus;
            STRPTR str[2];
            int class_len, sub_len;

            D(bug("[PCI-ATA] ata_PCIEnumerator_h: Adding Bus %d - IRQ %d, IO: %x:%x, DMA: %x\n",
                  x, INTLine, IOBase, IOAlt, DMABase));

            OOP_GetAttr(Device, aHidd_PCIDevice_ClassDesc, (IPTR *)&str[0]);
            OOP_GetAttr(Device, aHidd_PCIDevice_SubClassDesc, (IPTR *)&str[1]);

            len = 6 + strlen(str[0]) + strlen(str[1]);

            probedbus = AllocVec(sizeof(struct ata_ProbedBus) + len, MEMF_ANY)
            if (probedbus)
            {
                STRPTR name = (char *)probedbus + sizeof(struct ata_ProbedBus);

                RawDoFmt("PCI %s %s", str, RAWFMTFUNC_STRING, name);

                probedbus->atapb_Node.ln_Name = name;
                probedbus->atapb_Node.ln_Pri  = basePri - (a->ata__buscount++);
                probedbus->atapb_Device       = Device;
                probedbus->atapb_Vendor       = VendorID;
                probedbus->atapb_Product      = ProductID;
                probedbus->atapb_IOBase       = IOBase;
                probedbus->atapb_IOAlt        = IOAlt;
                probedbus->atapb_INTLine      = INTLine;
                probedbus->atapb_DMABase      = DMABase ? DMABase + (x << 3) : 0;
                probedbus->atapb_80wire       = TRUE;

                Enqueue((struct List *)&a->probedbuses, &probedbus->atapb_Node);

                OOP_SetAttrsTags(Device, aHidd_PCIDevice_isIO, TRUE,
                                         aHidd_PCIDevice_isMaster, DMABase != 0,
                                         TAG_DONE);
            }
        }
        else
        {
            HIDD_PCIDevice_Release(Device);
        }
    }

    AROS_USERFUNC_EXIT
}

static const struct TagItem Requirements[] =
{
    {tHidd_PCI_Class, PCI_CLASS_MASSSTORAGE},
    {TAG_DONE,        0x00                 }
};

static int ata_pci_Scan(struct ataBase *base)
{
    OOP_Object *ata = OOP_NewObject(NULL, CLID_HW_Ata, NULL);
    APTR BootLoaderBase;
    struct ata_ProbedBus *probedbus;
    BOOL scanpci    = TRUE;
#ifdef SUPPORT_LEGACY
    BOOL scanlegacy = TRUE;
#endif
    int i;

    if (!ata)
        return FALSE;
    
    /* Prepare lists for probed/found ide buses */
    NEWLIST(&base.probedbuses);
    base->ata__buscount = 0;
    base->legacycount   = 0;

    /* Obtain command line parameters */
    BootLoaderBase = OpenResource("bootloader.resource");
    D(bug("[PCI-ATA] BootloaderBase = %p\n", BootLoaderBase));
    if (BootLoaderBase != NULL)
    {
        struct List *list;
        struct Node *node;

        list = (struct List *)GetBootInfo(BL_Args);
        if (list)
        {
            ForeachNode(list, node)
            {
                if (strncmp(node->ln_Name, "ATA=", 4) == 0)
                {
                    const char *cmdline = &node->ln_Name[4];

                    if (strstr(base->ata_CmdLine, "nopci"))
                    {
                        D(bug("[PCI-ATA] Disabling PCI device scan\n"));
                        scanpci = FALSE;
                    }
#ifdef SUPPORT_LEGACY
                    if (strstr(base->ata_CmdLine, "nolegacy"))
                    {
                        D(bug("[PCI-ATA] Disabling Legacy ports\n"));
                        scanlegacy = FALSE;
                    }
#endif
                }
            }
        }
    }

    D(bug("[PCI-ATA] ata_Scan: Enumerating devices\n"));

    if (scanpci && base->PCIDeviceAttrBase && base->PCIDriverAttrBase)
    {
        OOP_Object *pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        OOP_MethodID PCIBase = OOP_GetMethodID(IID_Hidd_PCI, 0);

        if (pci)
        {
            struct Hook FindHook =
            {
                .h_Entry: (IPTR (*)())ata_PCIEnumerator_h,
                .h_Data : base
            };

            D(bug("[PCI-ATA] ata_Scan: Checking for supported PCI devices ..\n"));

            HIDD_PCI_EnumDevices(pci, &FindHook, Requirements);
        }
    }

#ifdef SUPPORT_LEGACY
    if (scanlegacy)
    {
        UBYTE n = base->legacycount;

        D(bug("[PCI-ATA] ata_Scan: Adding Remaining Legacy-Buses\n"));

        while (LegacyBuses[n].lb_Port)
        {
            probedbus = AllocVec(sizeof(struct ata_ProbedBus), MEMF_ANY);
            if (probedbus)
            {
                probedbus->atapb_Node.ln_Name = legacyBuses[n].lb_Name;
                probedbus->atapb_Node.ln_Pri  = ATABUSNODEPRI_LEGACY - (Args.ata__buscount++);
                probedbus->atapb_Device       = Device;
                probedbus->atapb_Vendor       = 0;
                probedbus->atapb_Product      = 0;
                probedbus->atapb_IOBase       = LegacyBuses[n].lb_IOBase;
                probedbus->atapb_IOAlt        = LegacyBuses[n].lb_IOAlt;
                probedbus->atapb_INTLine      = legacybus->atalb_INTLine;
                probedbus->atapb_DMABase      = 0;
                probedbus->atapb_80wire       = FALSE;

                D(bug("[PCI-ATA] ata_Scan: Adding Legacy Bus - IO: %x:%x\n",
                      probedbus->atapb_IOBase, probedbus->atapb_IOAlt));

                Enqueue(&Args.probedbuses, &probedbus->atapb_Node);
            }
            n++;
        }
    }
#endif    

    D(bug("[PCI-ATA] ata_Scan: Registering Probed Buses..\n"));

    while ((probedbus = (struct ata_ProbedBus *)RemHead(&Args.probedbuses)) != NULL)
    {
        struct TagItem attrs[] =
        {
            {aHidd_HardwareName, probedbus->atapb_Node.ln_Name},
            {aHidd_Producer    , probedbus->atapb_Vendor      },
            {aHidd_Product     , probedbus->atapb_Product     },
            {aHidd_ATA_IOBase  , probedbus->atapb_IOBase      },
            {aHidd_ATA_IOAlt   , probedbus->atapb_IOAlt       },
            {aHidd_ATA_INTLine , probedbus->atapb_INTLine     },
            {aHidd_ATA_DMABase , probedbus->atapb_DMABase     },
            {aHidd_ATA_80Wire  , probedbus->atapb_80wire      },
            {TAG_DONE          , 0                            }
        };
        OOP_Object *bus;

        bus = HW_AddDriver(NULL, base->busClass, attrs);
        if (!bus)
        {
            D(bug("[PCI-ATA] Failed to create object for device %04X:%04X - IRQ %d, IO: %x:%x, DMA: %x\n",
                  probedbus->atapb_Vendor, probedbus->atapb_Product, probedbus->atapb_INTLine,
                  probedbus->atapb_IOBase, probedbus->atapb_IOAlt, probedbus->atapb_DMABase));

            if (probedbus->atapb_Device)
                HIDD_PCIDevice_Release(probedbus->atapb_Device);
        }

        FreeVec(probedbus);
    }

    return TRUE;
}

/*
 * ata.device main code has two init routines with 0 and 127 priorities.
 * All bus scanners must run between them.
 */
ADD2INITLIB(ata_pci_Scan, 30)
