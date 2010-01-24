#ifndef _AHCI_HEADER_H
#define _AHCI_HEADER_H

/*
    Copyright © 2010, The AROS Development Team. All rights reserved
    $Id$
*/

#include <exec/exec.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/devices.h>
#include <exec/resident.h>

#include <utility/utility.h>

#include <libraries/expansion.h>
#include <libraries/configvars.h>

#include <dos/bptr.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/oop.h>

#include <oop/oop.h>
#include <hidd/pci.h>

#include <aros/symbolsets.h>

#include <string.h>

#include "ahci_hba.h"

#define __OOP_NOATTRBASES__

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase           (XSD(cl)->PCIDeviceAB)

#define XSD(cl) (&((LIBBASETYPEPTR)cl->UserData)->asd)

struct ahci_staticdata {

    OOP_Object         *PCIObject;
    OOP_Object         *PCIDriver;
    OOP_AttrBase        PCIDeviceAB;

    /* Memory pool */
    APTR    ahci_MemPool;

    /* List of all found AHCI host devices (referred to as host bus adapters, or HBA) */
    struct  MinList ahci_hba_list;

};

/* One instance of HBA */
struct ahci_hba_chip {

    struct  MinNode hba_Node;

    IPTR    ProductID, VendorID;

    APTR    abar;
    IPTR    intline;

};

/* ahci.device base */
struct ahciBase {
    struct Device device;
    struct ahci_staticdata asd;
};

/* ahci_hbahw prototypes */
void ahci_init_hba(struct ahci_hba_chip *hba_chip);
void ahci_reset_hba(struct ahci_hba_chip *hba_chip);
BOOL ahci_enable_hba(struct ahci_hba_chip *hba_chip);
void ahci_disable_hba(struct ahci_hba_chip *hba_chip);

#endif // _AHCI_HEADER_H

