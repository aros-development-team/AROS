/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

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

    static ULONG HBACounter;

    LIBBASETYPE *LIBBASE = (LIBBASETYPE *)hook->h_Data;

    OOP_AttrBase HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base5, (APTR)&abar);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size5, &size);
    if( !(abar == 0) ) {

        struct ahci_hba_chip *hba_chip;
        if((hba_chip = (struct ahci_hba_chip*) AllocVecPooled(LIBBASE->ahci_MemPool, sizeof(struct ahci_hba_chip)))) {

            OOP_Object *PCIDriver;
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&PCIDriver);

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

            hba_chip->HBANumber = ++HBACounter;

            struct TagItem attrs[] = {
                { aHidd_PCIDevice_isIO,    FALSE },
                { aHidd_PCIDevice_isMEM,    TRUE },
                { aHidd_PCIDevice_isMaster, TRUE },
                { TAG_DONE, 0UL },
            };
            OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);

            /*
                HBA-chip list is protected for us in Init
            */
            AddTail((struct List*)&LIBBASE->chip_list, (struct Node*)hba_chip);

        }
    }

    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

	AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE) {
    D(bug("[AHCI] Init\n"));

    OOP_Object *PCIObject;

    if ((PCIObject = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL))) {
        if((LIBBASE->ahci_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, 8192, 4096))) {

            /* HBA linked list is semaphore protected */
            InitSemaphore(&LIBBASE->chip_list_lock);

            /* Initialize the list of found host bus adapters */
            ObtainSemaphore(&LIBBASE->chip_list_lock);
            NEWLIST((struct MinList *)&LIBBASE->chip_list);

            struct Hook FindHook = {
                h_Entry:    (IPTR (*)())ahci_Enumerator,
                h_Data:     LIBBASE,
            };

            struct TagItem Requirements[] = {
                {tHidd_PCI_Class,       0x01},
                {tHidd_PCI_SubClass,    0x06},
                {tHidd_PCI_Interface,   0x01},
                {TAG_DONE,              0x00}
            };

            HIDD_PCI_EnumDevices(PCIObject, &FindHook, Requirements);

            if ( !IsListEmpty(&LIBBASE->chip_list) ) {
                struct ahci_hba_chip *hba_chip;
                ForeachNode(&LIBBASE->chip_list, hba_chip) {
                    if( ahci_setup_hbatask(hba_chip) ) {
                        D(bug("[AHCI] Created HBA task\n"));
                    }else{
                        // de-allocate everything relating to this HBA-chip and remove it from the list
                        D(bug("[AHCI] Failed to create HBA task!\n"));
                        REMOVE(hba_chip);
                    }
                }
                ReleaseSemaphore(&LIBBASE->chip_list_lock);
            }else{
                /*
                    No AHCI controller found
                */
                ReleaseSemaphore(&LIBBASE->chip_list_lock);
                DeletePool(LIBBASE->ahci_MemPool);
                return FALSE;
            }

            return TRUE;
        }else{
            D(bug("[AHCI] Failed to create memory pool\n"));
        }
        OOP_DisposeObject(PCIObject); 
    }else{
        D(bug("[AHCI] Failed to open PCI class\n"));
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

    D(bug("[AHCI] BeginIO\n"));

	AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
	AROS_LHA(struct IORequest *, iorq, A1),
	LIBBASETYPEPTR, LIBBASE, 6, ahci)
{
	AROS_LIBFUNC_INIT

    D(bug("[AHCI] AbortIO\n"));

	return 0;

	AROS_LIBFUNC_EXIT
}

