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

        NEWLIST(&ehc->ehc_CtrlXFerQueue);
        NEWLIST(&ehc->ehc_IntXFerQueue);
        NEWLIST(&ehc->ehc_IsoXFerQueue);
        NEWLIST(&ehc->ehc_BulkXFerQueue);

        ehc->ehc_devicebase = ehd;
        ehc->ehc_pcideviceobject = pcidevice;

        OOP_GetAttr(pcidevice, aHidd_PCIDevice_Driver, (IPTR *)&ehc->ehc_pcidriverobject);

        OOP_GetAttr(pcidevice, aHidd_PCIDevice_Bus, &ehc->ehc_pcibus);
        OOP_GetAttr(pcidevice, aHidd_PCIDevice_Dev, &ehc->ehc_pcidev);
        OOP_GetAttr(pcidevice, aHidd_PCIDevice_Sub, &ehc->ehc_pcisub);
        OOP_GetAttr(pcidevice, aHidd_PCIDevice_INTLine, &ehc->ehc_intline);

        if(!(ehc->ehc_intline == 255)) {

            /* Initialize EHCI controller */

            /* Try to match the controller with previous unit nodes created based on the bus address (if any) */
            struct ehu_unit *ehu;

            struct Unitnode * ehu_unitnode;
            if (!IsListEmpty(&ehd->ehd_unitnodelist)) {

                ForeachNode(&ehd->ehd_unitnodelist, ehu_unitnode) {

                    ehu = (struct ehu_unit *) ehu_unitnode->ehu_unitptr;
                    if(((ehu->ehu_pcibus == ehc->ehc_pcibus)&&(ehu->ehu_pcidev == ehc->ehc_pcidev))) {
                        Found = TRUE;
                        break;
                    }
                }
            }

            /* Add the controller to the unit node or create a new one */
            if(Found) {
                    KPRINTF2(DBL_DEVIO,("EHC Found matching unit #%ld (%04x:%04x)\n", ehu->ehu_unitnumber, ehu->ehu_pcibus, ehu->ehu_pcidev));

                    ehc->ehc_unitptr = ehu_unitnode->ehu_unitptr;
                    ADDTAIL((struct MinList *) &ehu->ehu_cntrlist, (struct MinNode *) &ehc->ehc_contrnode);
            }else{
                if((ehu = AllocPooled(ehd->ehd_mempool, sizeof(struct ehu_unit)))) {
                    ehu->ehu_unitnode.ehu_unitptr = ehu;

                    ehc->ehc_unitptr = ehu;

                    ehu->ehu_pcibus = ehc->ehc_pcibus;
                    ehu->ehu_pcidev = ehc->ehc_pcidev;

                    if (!IsListEmpty(&ehd->ehd_unitnodelist)) {
                        struct Unitnode *prev_ehu_unitnode = (struct Unitnode *) GetTail(&ehd->ehd_unitnodelist);

                        struct ehu_unit *prev_ehu = (struct ehu_unit *) prev_ehu_unitnode->ehu_unitptr;
                        ehu->ehu_unitnumber = prev_ehu->ehu_unitnumber + 1;
                    }else{
                        ehu->ehu_unitnumber = 0;
                    }

                    NEWLIST((struct MinList *)&ehu->ehu_cntrlist);
                    ADDTAIL((struct MinList *)&ehu->ehu_cntrlist, (struct MinNode *) &ehc->ehc_contrnode);
                    ADDTAIL((struct MinList *)&ehd->ehd_unitnodelist, (struct MinNode *) &ehu->ehu_unitnode);

                    KPRINTF2(DBL_DEVIO,("EHC Created new unit #%ld (%04x:%04x)\n", ehu->ehu_unitnumber, ehu->ehu_pcibus, ehu->ehu_pcidev));
                }else{
                    FreePooled(ehd->ehd_mempool, ehc, sizeof(struct ehc_controller));
                }
            }
        }else{
            FreePooled(ehd->ehd_mempool, ehc, sizeof(struct ehc_controller));
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

    NEWLIST((struct MinList *)&ehd->ehd_unitnodelist);

    if((ehd->ehd_mempool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR | MEMF_SEM_PROTECTED, 16384, 4096))) {
        if((ehd->ehd_pcihidd = OOP_NewObject(NULL, (STRPTR) CLID_Hidd_PCI, NULL))) {

            /* Enumerate all EHCI controllers */
            HIDD_PCI_EnumDevices(ehd->ehd_pcihidd, &pci_enumhooktag, (struct TagItem *) &pci_enumtag);

            if (!IsListEmpty(&ehd->ehd_unitnodelist)) {

                struct Unitnode *ehu_unitnode;
                ForeachNode(&ehd->ehd_unitnodelist, ehu_unitnode) {

                    struct ehu_unit *ehu = (struct ehu_unit *)ehu_unitnode->ehu_unitptr;
                    if (!IsListEmpty(&ehu->ehu_cntrlist)) {

                        KPRINTF2(DBL_DEVIO,("EHC Unit #%ld(%p) has controller(s)\n", ehu->ehu_unitnumber, ehu->ehu_unitnode.ehu_unitptr));

                        struct ehc_controller *ehc;
                        ForeachNode(&ehu->ehu_cntrlist, ehc) {
                            KPRINTF2(DBL_DEVIO,("#%ld pointing to unit %p\n", ehc->ehc_pcisub, ehc->ehc_unitptr));
                        }
                        KPRINTF2(DBL_DEVIO,("\n"));
                    }
                }
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
