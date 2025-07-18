
#include <aros/debug.h>

#include <asm/io.h>

#include "library.h"
#include "DriverData.h"

OOP_AttrBase __IHidd_PCIDev;

static const struct {
    UWORD VendorID;
    UWORD ProductID;
    STRPTR Model;
} support[] = {
    { 0x8086, 0x2415, "Intel 82801AA"	},
    { 0x8086, 0x2425, "Intel 82801AB"	},
    { 0x8086, 0x2445, "Intel 82801BA"	},
    { 0x8086, 0x2485, "Intel ICH3"	},
    { 0x8086, 0x24c5, "Intel ICH4"	},
    { 0x8086, 0x24d5, "Intel ICH5"	},
        { 0x8086, 0x25a6, "ESB"         },
    { 0x8086, 0x266e, "Intel ICH6"	},
    { 0x8086, 0x27de, "Intel ICH7"	},
        { 0x8086, 0x2698, "ESB2"        },
    { 0x8086, 0x7195, "Intel 440MX"	},
    { 0x1039, 0x7012, "SIS 7012"	},
    { 0x10de, 0x01b1, "NVIDIA nForce"	},
        { 0x10de, 0x003a, "MCP04"       },
    { 0x10de, 0x006a, "NVIDIA nForce2"	},
        { 0x10de, 0x0059, "CK804"       },
    { 0x10de, 0x008a, "MCP2S AC'97 Audio Controller" },
    { 0x10de, 0x00da, "NVIDIA nForce3"	},
        { 0x10de, 0x00ea, "CK8S"        },
        { 0x10de, 0x026b, "MCP51"       },
    { 0x1022, 0x746d, "AMD 8111"	},
    { 0x1022, 0x7445, "AMD 768"		},
    { 0x10b9, 0x5455, "Ali 5455"	},
    {0,0,NULL},
};

static void i8x0_set_reg(struct ac97Base *ac97Base, ULONG reg, UWORD value)
{
    int count=1000000;
    
    while(count-- && (inb((IPTR)ac97Base->dmabase + ACC_SEMA) & 1));
    
    outw(value, (IPTR)ac97Base->mixerbase + reg);
}

static UWORD i8x0_get_reg(struct ac97Base *ac97Base, ULONG reg)
{
    int count=1000000;
    
    while(count-- && (inb((IPTR)ac97Base->dmabase + ACC_SEMA) & 1));

    return inw((IPTR)ac97Base->mixerbase + reg);
}

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/
#define ac97Base ((struct ac97Base *)hook->h_Data)
#define AHIsubBase ((struct DriverBase *)hook->h_Data)
static AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(OOP_Object *,     device, A2),
    AROS_UFHA(APTR,             msg,    A1))
{
    AROS_USERFUNC_INIT

    IPTR VendorID = 0, ProductID = 0, value = 0;
    int i;

    D(bug("[AHI:AC97] %s()\n", __func__));

    OOP_GetAttr(device, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(device, aHidd_PCIDevice_VendorID, &VendorID);

    D(bug("[AHI:AC97] %s: Querying PCI 'audio' device %04x:%04x\n", __func__, VendorID, ProductID));

    for (i=0; support[i].VendorID; i++)
    {
        if (VendorID == support[i].VendorID && ProductID == support[i].ProductID)
        {
            struct TagItem attrs[] = {
                { aHidd_PCIDevice_isIO,	    TRUE },
                { aHidd_PCIDevice_isMEM,    FALSE },
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE, 0UL },	
            };

            D(bug("[AHI:AC97] %s: Detected supported '%s' card\n", __func__, support[i].Model));

            ac97Base->mixer_set_reg = i8x0_set_reg;
            ac97Base->mixer_get_reg = i8x0_get_reg;

            OOP_SetAttrs(device, (struct TagItem *)&attrs);

            OOP_GetAttr(device, aHidd_PCIDevice_Base0, &value);
            ac97Base->mixerbase = (APTR)value;
            OOP_GetAttr(device, aHidd_PCIDevice_Base1, &value);
            ac97Base->dmabase = (APTR)value;
            OOP_GetAttr(device, aHidd_PCIDevice_INTLine, &value);
            ac97Base->irq_num = (ULONG)value;

            D(
                bug("[AHI:AC97] %s: Mixer IO base @ %p\n", __func__, ac97Base->mixerbase);
                bug("[AHI:AC97] %s: DMA IO base @ %p\n", __func__, ac97Base->dmabase);
            )

            if (VendorID == 0x1039 && ProductID == 0x7012)
            {
                /* SIS 7012 */
                ac97Base->off_po_sr     = DEFAULT_PO_PICB; /* swap registers */
                ac97Base->off_po_picb   = DEFAULT_PO_SR;
                ac97Base->size_shift    = 1; /* chip requires size in bytes, not samples */
            }
            else
            {
                /* All other cards */
                ac97Base->off_po_sr     = DEFAULT_PO_SR; /* swap registers */
                ac97Base->off_po_picb   = DEFAULT_PO_PICB;
                ac97Base->size_shift    = 0;
            }

#if defined(__AROS__) && (__WORDSIZE==64)
            ac97Base->buffer = CreatePool(MEMF_CLEAR | MEMF_31BIT, 131072, 65536);
#endif
            if ((value = (IPTR)AllocMem((8*32) + ALIGN_AC97OUT, MEMF_PUBLIC | MEMF_31BIT | MEMF_CLEAR)) != 0)
            {
                ac97Base->PCM_out = (APTR)ALIGN_AC97(value);
                D(bug("[AHI:AC97] %s: PCM_out base @ %p\n", __func__, ac97Base->PCM_out));

                ac97Base->cardfound = TRUE;

                outl(2, (IPTR)ac97Base->dmabase + GLOB_CNT);

                ac97Base->mixer_set_reg(ac97Base, AC97_RESET, 0);
                ac97Base->mixer_set_reg(ac97Base, AC97_POWERDOWN, 0);

                /* Set master volume to no attenuation, mute off */
                ac97Base->mixer_set_reg(ac97Base, AC97_MASTER_VOL,	    0x0000);
                ac97Base->mixer_set_reg(ac97Base, AC97_HEADPHONE_VOL,   0x0000);
                ac97Base->mixer_set_reg(ac97Base, AC97_TONE,	    0x0f0f);
                ac97Base->mixer_set_reg(ac97Base, AC97_PCM_VOL,	    0x0000);

                D(
                    bug("[AHI:AC97] %s:    Powerdown = %02x\n", __func__, ac97Base->mixer_get_reg(ac97Base, AC97_POWERDOWN));
                    bug("[AHI:AC97] %s:    GLOB_CNT = %08x\n", __func__, inl((IPTR)ac97Base->dmabase + GLOB_CNT));
                    bug("[AHI:AC97] %s:    GLOB_STA = %08x\n", __func__, inl((IPTR)ac97Base->dmabase + GLOB_STA));
                )

    /*
                int i;
                for (i=0; i < 64; i+=2)
                {
                    D(bug("[AHI:AC97] reg %02x = %04x\n", i, ac97Base->mixer_get_reg(ac97Base, i)));
                }
    */
                outl((ULONG)(IPTR)ac97Base->PCM_out, (IPTR)ac97Base->dmabase + PO_BDBAR);

                D(
                    bug("[AHI:AC97] %s:    PO_BDBAR=%p\n", __func__, (APTR)(IPTR)inl((IPTR)ac97Base->dmabase + PO_BDBAR));
                    bug("[AHI:AC97] %s:    PO_REGS=%08x\n", __func__, inl((IPTR)ac97Base->dmabase + PO_CIV));
                    bug("[AHI:AC97] %s:    PO_PICB=%04x\n", __func__, inw((IPTR)ac97Base->dmabase + ac97Base->off_po_picb));
                    bug("[AHI:AC97] %s:    PO_PIV=%02x\n", __func__, inb((IPTR)ac97Base->dmabase + PO_PIV));
                    bug("[AHI:AC97] %s:    PO_CR=%02x\n", __func__, inb((IPTR)ac97Base->dmabase + PO_CR));
                )
            }
        }
    }

    AROS_USERFUNC_EXIT
}
#undef ac97Base
#undef AHIsubBase

BOOL DriverInit( struct DriverBase* AHIsubBase )
{
    struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;

    ac97Base->dosbase = OpenLibrary( DOSNAME, 37 );
    ac97Base->sysbase = SysBase;

    D(bug("[AHI:AC97] %s()\n", __func__));

    if(DOSBase)
    {
        ac97Base->oopbase = (APTR)OpenLibrary(AROSOOP_NAME, 0);
        if (OOPBase)
        {
            __IHidd_PCIDev = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

            D(bug("[AHI:AC97] %s: Libraries opened\n", __func__));

            if (__IHidd_PCIDev)
            {
            OOP_Object *pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
            
            D(bug("[AHI:AC97] %s: PCIDevice AttrBase = %x\n", __func__, __IHidd_PCIDev));
            
            if (pci)
            {
                struct Hook FindHook = {
                    .h_Entry            = (IPTR(*)())Enumerator,
                    .h_Data             = ac97Base,
                };

                struct TagItem Reqs[] = {
                    { tHidd_PCI_Class,	        0x04 },
                    { tHidd_PCI_SubClass,       0x01 },
                    { TAG_DONE,                 0UL },
                };

                struct pHidd_PCI_EnumDevices enummsg = {
                    .mID                = OOP_GetMethodID(CLID_Hidd_PCI, moHidd_PCI_EnumDevices),
                    .callback           = &FindHook,
                    .requirements       = (struct TagItem *)&Reqs,
                }, *msg = &enummsg;

                D(bug("[AHI:AC97] %s: Got PCI object\n", __func__));

                ac97Base->cardfound = FALSE;

                OOP_DoMethod(pci, (OOP_Msg)msg);

                OOP_DisposeObject(pci);

                return ac97Base->cardfound;
            }
            }
        }
        else
        {
            Req("Unable to open 'oop.library'\n");
        }
    }
    else
    {
        Req( "Unable to open 'dos.library' version 37.\n" );
    }
  
    return FALSE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID DriverCleanup( struct DriverBase* AHIsubBase )
{
    struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;

    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    CloseLibrary( (struct Library*) DOSBase );
    CloseLibrary( (struct Library*) ac97Base->oopbase);
}

