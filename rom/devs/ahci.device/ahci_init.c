/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

/*

*/

static
AROS_UFH3(void, ahci_Enumerator,
    AROS_UFHA(struct Hook *,    hook,	    A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
	AROS_USERFUNC_INIT

    IPTR    VendorID, ProductID;

    APTR    Base5;
    IPTR    Base5size;

    IPTR    intline;

    static uint32_t HBACounter;

    LIBBASETYPE *LIBBASE = (LIBBASETYPE *)hook->h_Data;

    OOP_AttrBase HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base5, (APTR)&Base5);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size5, &Base5size);
    if( !(Base5 == 0) ) {
        struct ahci_hba_chip *hba_chip;
        if( (hba_chip = (struct ahci_hba_chip*) AllocVec(sizeof(struct ahci_hba_chip), MEMF_CLEAR|MEMF_PUBLIC)) ) {
            if( (hba_chip->IntHandler = (HIDDT_IRQ_Handler *)AllocVec(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC)) ){

                OOP_Object *PCIDriver;
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&PCIDriver);

                OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);

                hba_chip->PCIVendorID = VendorID;
                hba_chip->PCIProductID = ProductID;

                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base5, (APTR)&Base5);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size5, &Base5size);

                struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;
    	        mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
    	        mappci.PCIAddress = Base5;
    	        mappci.Length = Base5size;
    	        hba_chip->abar = (APTR)OOP_DoMethod(PCIDriver, (OOP_Msg)msg);

                OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &intline);
                hba_chip->IRQ = intline;

                hba_chip->HBANumber = ++HBACounter;

                struct TagItem attrs[] = {
                    { aHidd_PCIDevice_isIO,    FALSE },
                    { aHidd_PCIDevice_isMEM,    TRUE },
                    { aHidd_PCIDevice_isMaster, TRUE },
                    { TAG_DONE, 0UL },
                };
                OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);

                /* HBA-chip list is protected for us in Init */
                AddTail((struct List*)&LIBBASE->chip_list, (struct Node*)hba_chip);

                /* HBA-port linked list is semaphore protected */
                InitSemaphore(&hba_chip->port_list_lock);

                /* Initialize the HBA-port list */
                NEWLIST((struct MinList *)&hba_chip->port_list);

                D(bug("[AHCI] hba%d_chip @ %p\n",HBACounter, hba_chip));
                D(bug("[AHCI] hba%d IntHandler @ %p\n",HBACounter, hba_chip->IntHandler));

            }else{
                /* Failed to allocate HIDDT_IRQ_Handler */
                FreeVec(hba_chip->IntHandler);
            }
        }
    }
    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);

    AROS_USERFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE) {
    D(bug("[AHCI] Init\n"));

    OOP_Object *PCIObject;

    if ((PCIObject = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL))) {

        /* HBA-chip linked list is semaphore protected */
        InitSemaphore(&LIBBASE->chip_list_lock);

        /* Initialize the HBA-chip list */
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

        ObtainSemaphore(&LIBBASE->chip_list_lock);
        if ( !IsListEmpty(&LIBBASE->chip_list) ) {

            struct ahci_hba_chip *hba_chip;
            ForeachNode(&LIBBASE->chip_list, hba_chip) {
                if( ahci_setup_hba(hba_chip) ) {
                    D(bug("[AHCI] HBA-setup succeed!\n"));
                }else{
                    // de-allocate everything relating to this HBA-chip and remove it from the list
                    D(bug("[AHCI] HBA-setup failed!\n"));
                    REMOVE(hba_chip);
                }
            }
            ReleaseSemaphore(&LIBBASE->chip_list_lock);
            return TRUE;
        }else{
            /* Not a single AHCI HBA controller found */
            ReleaseSemaphore(&LIBBASE->chip_list_lock);
            OOP_DisposeObject(PCIObject); 
            return FALSE;
        }

        OOP_DisposeObject(PCIObject); 

    }else{
        D(bug("[AHCI] Failed to open PCI class\n"));
    }

    return FALSE;
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR LIBBASE, struct IORequest *iorq, uint32_t unitnum, uint32_t flags) {
    D(bug("[AHCI] Open\n"));
   
    /* Assume Open failed */
    iorq->io_Error = IOERR_OPENFAIL;

    /* Overly complex way of finding port unit... */
    ObtainSemaphore(&LIBBASE->chip_list_lock);
    struct ahci_hba_chip *hba_chip;
    ForeachNode(&LIBBASE->chip_list, hba_chip) {
        ObtainSemaphore(&hba_chip->port_list_lock);
        struct ahci_hba_port *hba_port;
        ForeachNode(&hba_chip->port_list, hba_port) {
            if( (hba_port->port_unit.Port_Unit_Number == unitnum) ) {
                /* set up IORequest */
                iorq->io_Device     = &LIBBASE->device;
                iorq->io_Unit       = &hba_port->port_unit.port_exec_unit;
                iorq->io_Error      = 0;

                hba_port->port_unit.port_exec_unit.unit_OpenCnt++;

                ReleaseSemaphore(&hba_chip->port_list_lock);
                ReleaseSemaphore(&LIBBASE->chip_list_lock);
                return TRUE;
            }
        }
        ReleaseSemaphore(&hba_chip->port_list_lock);
    }
    ReleaseSemaphore(&LIBBASE->chip_list_lock);

    return TRUE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR LIBBASE, struct IORequest *iorq) {
    D(bug("[AHCI] Close\n"));

    struct  Unit *port_exec_unit = (struct Unit *)iorq->io_Unit;

    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    
    /* Decrease use counters of unit */
    port_exec_unit->unit_OpenCnt--;

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

