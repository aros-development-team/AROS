/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#define __OOP_NOATTRBASES__

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase           (asd->PCIDeviceAB)

/*

Game plan so far...

AHCI compromises of HBA's or host bus adapters which in turn can have upto 32 ports with independant DMA's and "c/d" que's
HBA's can implement less than 32 ports and by looking at the HBA's registers we know which ports it implements.

ahci.device collects all HBA's from PCI bus (class 1, subclass 6 and pi 1) via pci enumerator and sets up interrupt
and HBA_task code for each of them (or one).

Every implemented port gets a unit number even if no device sits on the port because of hotplugging.
In case of multiple HBA's first found HBA and its first implemented port gets unit number 0 and so on.

ahci_init.c
    - Enumerator
    - Device init code
ahci_device.c
    - Open, Close, BeginIO, AbortIO, etc... or append init code to device
ahci_hbahw
    - HBA specific code 

*/

static
AROS_UFH3(void, ahci_Enumerator,
    AROS_UFHA(struct Hook *,    hook,	    A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
	AROS_USERFUNC_INIT

    IPTR    VendorID, ProductID;

    APTR    abar;
    IPTR    size;

    IPTR    intline;

    struct ahci_staticdata *asd = hook->h_Data;

    struct ahci_hba_chip *hba_chip;
    if((hba_chip = (struct ahci_hba_chip*) AllocVecPooled(asd->ahci_MemPool, sizeof(struct ahci_hba_chip)))) {

        OOP_Object *PCIDriver;
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&PCIDriver);
	    asd->PCIDriver = PCIDriver;

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);
        hba_chip->VendorID = VendorID;
        hba_chip->ProductID = ProductID;

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base5, (APTR)&abar);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size5, &size);

        struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;
	    mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
	    mappci.PCIAddress = abar;
	    mappci.Length = size;
	    hba_chip->abar = (APTR)OOP_DoMethod(PCIDriver, (OOP_Msg)msg);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &intline);
        hba_chip->intline = intline;

        struct TagItem attrs[] = {
            { aHidd_PCIDevice_isIO,    FALSE },
            { aHidd_PCIDevice_isMEM,    TRUE },
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
        };
        OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);

        AddTail((struct List*)&asd->ahci_hba_list, (struct Node*)hba_chip);

    }
	AROS_USERFUNC_EXIT

}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE) {
    D(bug("[AHCI] Init\n"));

    struct ahci_staticdata *asd = &LIBBASE->asd;

    struct OOP_ABDescr attrbases[] = {
        { (STRPTR)IID_Hidd_PCIDevice,     &HiddPCIDeviceAttrBase },
        { NULL, NULL }
    }; 

    if (OOP_ObtainAttrBases(attrbases)) {
        if ((asd->PCIObject = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL))) {
            if((asd->ahci_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096))) {

                /* Initialize the list of found host bus adapters */
                asd->ahci_hba_list.mlh_Head     = (struct MinNode*) &asd->ahci_hba_list.mlh_Tail;
                asd->ahci_hba_list.mlh_Tail     = NULL;
                asd->ahci_hba_list.mlh_TailPred = (struct MinNode*) &asd->ahci_hba_list.mlh_Head;

                struct Hook FindHook = {
                    h_Entry:    (IPTR (*)())ahci_Enumerator,
                    h_Data:     asd,
                };

                struct TagItem Requirements[] = {
                    {tHidd_PCI_Class,       0x01},
                    {tHidd_PCI_SubClass,    0x06},
                    {tHidd_PCI_Interface,   0x01},
                    {TAG_DONE,              0x00}
                };

                HIDD_PCI_EnumDevices(asd->PCIObject, &FindHook, Requirements);

                struct ahci_hba_chip *hba_chip;
                struct ahci_hba *hba;
                int i;
                ForeachNode(&asd->ahci_hba_list, hba_chip) {
                    D(bug("[AHCI]  HBA abar = %08x\n", hba_chip->abar));
                    ahci_init_hba(hba_chip);

                    /* abar in hba_chip points to memory region with hba type structure */
                    hba = hba_chip->abar;

                    /* Parse some registers so it would look like this does something... or test if structures are in order */
                    /* Huge amount of information can be gathered just from hba's cap register */

                    D(bug("        cap       %08x\n", hba->cap));
                    D(bug("        ghc       %08x\n", hba->ghc));
                    D(bug("        is        %08x\n", hba->is));
                    D(bug("        pi        %08x\n", hba->pi));
                    D(bug("        vs        %08x\n", hba->vs));
                    D(bug("        ccc_ctl   %08x\n", hba->ccc_ctl));
                    D(bug("        ccc_ports %08x\n", hba->ccc_ports));
                    D(bug("        em_loc    %08x\n", hba->em_loc));
                    D(bug("        em_ctl    %08x\n", hba->em_ctl));
                    D(bug("        cap2      %08x\n", hba->cap2));
                    D(bug("        bohc      %08x\n", hba->bohc));

                    D(bug("        # of ports %d\n", (hba->cap & 0x1f)+1) );
//                  for (i = 0; i <= (hba->cap & 0x1f); i++) {
                    for (i = 0; i <= 31; i++) {
                        if( ((hba->pi) & (1<<i)) ) {
                            D(bug("        port %d implemented\n", i+1));
                            D(bug("        clb       %08x\n", hba->port[i].clb));
                        }
                    }
                    return TRUE;
                }
            }else{
                D(bug("[AHCI] Failed to create memory pool\n"));
            }
            OOP_DisposeObject(asd->PCIObject);   
        }else{
            D(bug("[AHCI] Failed to open PCI class\n"));
        }
        OOP_ReleaseAttrBases(attrbases);
    }else{
        D(bug("[AHCI] Failed to obtain AttrBases\n"));
    }

    return FALSE;
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR LIBBASE, struct IORequest *iorq, ULONG unitnum, ULONG flags) {
    D(bug("[AHCI] Open\n"));
    return TRUE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR LIBBASE, struct IORequest *iorq) {
    D(bug("[AHCI] Close\n"));
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)


AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, iorq, A1),
    LIBBASETYPEPTR, LIBBASE, 5, ahci)
{
	AROS_LIBFUNC_INIT

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
	AROS_LHA(struct IORequest *, iorq, A1),
	LIBBASETYPEPTR, LIBBASE, 6, ahci)
{
	AROS_LIBFUNC_INIT

	return 0;

	AROS_LIBFUNC_EXIT
}

