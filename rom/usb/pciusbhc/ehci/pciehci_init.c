/*
    Copyright © 2011, The AROS Development Team. All rights reserved
    $Id$
*/

#include LC_LIBDEFS_FILE

AROS_UFH3(void, ehc_enumhook,
	AROS_UFHA(struct Hook *,    hook,       A0),
	AROS_UFHA(OOP_Object *,     pcidevice,  A2),
	AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT
    LIBBASETYPE *ehd = (LIBBASETYPE *) hook->h_Data;

    BOOL Found = FALSE;

    KPRINTF2(DBL_DEVIO,("EHC Controller found...\n"));

    struct ehc_controller *ehc;
    if((ehc = AllocPooled(ehd->ehd_mempool, sizeof(struct ehc_controller)))) {

        OOP_GetAttr(pcidevice, aHidd_PCIDevice_Bus, &ehc->ehc_pcibus);
        OOP_GetAttr(pcidevice, aHidd_PCIDevice_Dev, &ehc->ehc_pcidev);
        OOP_GetAttr(pcidevice, aHidd_PCIDevice_Dev, &ehc->ehc_pcisub);

        /* Try to match the controller with previous unit nodes created based on the bus address (if any) */
        struct ehu_unit *ehu;
        if (!IsListEmpty(&ehd->ehd_unitlist)) {
            ForeachNode(&ehd->ehd_unitlist, ehu) {
                if(((ehu->ehu_pcibus == ehc->ehc_pcibus)&&(ehu->ehu_pcidev == ehc->ehc_pcidev))) {
                    Found = TRUE;
                    break;
                }
            }
        }

        /* Add the controller to the unit node or create a new one */
        if(Found) {
                KPRINTF2(DBL_DEVIO,("EHC Found matching unit #%ld (%04x:%04x)\n", ehu->ehu_unitnumber, ehu->ehu_pcibus, ehu->ehu_pcidev));
                ADDTAIL((struct List*)&ehu->ehu_cntrlist, (struct Node*)ehc);
        }else{
            if((ehu = AllocPooled(ehd->ehd_mempool, sizeof(struct ehu_unit)))) {
                ehu->ehu_pcibus = ehc->ehc_pcibus;
                ehu->ehu_pcidev = ehc->ehc_pcidev;
                if (!IsListEmpty(&ehd->ehd_unitlist)) {
                    struct ehu_unit *prev_ehu = (struct ehu_unit *)GetPred(ehu);
                    ehu->ehu_unitnumber = prev_ehu->ehu_unitnumber + 1;
                }else{
                    ehu->ehu_unitnumber = 0;
                }
                NEWLIST((struct MinList *)&ehu->ehu_cntrlist);
                ADDTAIL((struct List*)&ehu->ehu_cntrlist, (struct Node*)ehc);
                ADDTAIL((struct List*)&ehd->ehd_unitlist, (struct Node*)ehu);
                KPRINTF2(DBL_DEVIO,("EHC Created new unit #%ld (%04x:%04x)\n", ehu->ehu_unitnumber, ehu->ehu_pcibus, ehu->ehu_pcidev));
            }
        }
    }

    AROS_USERFUNC_EXIT
}

static int Init(LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    struct OOP_ABDescr attrbases[] = {
        { (STRPTR)IID_Hidd,             &HiddAttrBase },
        { (STRPTR)IID_Hidd_PCIDevice,   &HiddPCIDeviceAttrBase },
        { NULL, NULL }
    };

    OOP_ObtainAttrBases(attrbases);

    struct Hook pci_enumhooktag = {
        h_Entry:        (IPTR (*)()) ehc_enumhook,
        h_Data:         LIBBASE,
    };

    struct TagItem pci_enumtag [] = {
        { tHidd_PCI_Class,      PCI_BASE_CLASS_SERIAL },
        { tHidd_PCI_SubClass,   PCI_SUB_CLASS_USB },
        { tHidd_PCI_Interface,  PCI_INTERFACE_EHCI },
        { TAG_DONE, 0UL }
    };

    NEWLIST((struct MinList *)&ehd->ehd_unitlist);

    if((ehd->ehd_mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 16384, 4096))) {
        if((ehd->ehd_pcihidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_PCI, NULL))) {
            HIDD_PCI_EnumDevices(ehd->ehd_pcihidd, &pci_enumhooktag, (struct TagItem *) &pci_enumtag);
            if (!IsListEmpty(&ehd->ehd_unitlist)) {
                return TRUE;
            }
        }

        DeletePool(ehd->ehd_mempool);
        ehd->ehd_mempool = NULL;
    }
    return FALSE;
}

static int Expunge(LIBBASETYPEPTR LIBBASE) {
    LIBBASETYPE *ehd = (LIBBASETYPE *) LIBBASE;

    DeletePool(ehd->ehd_mempool);

    OOP_DisposeObject(ehd->ehd_pcihidd);
    return TRUE;
}

ADD2INITLIB(Init,0)
ADD2EXPUNGELIB(Expunge,0)
