#ifndef ATA_PCI_BUS_H
#define ATA_PCI_BUS_H

#include <oop/oop.h>
#include <exec/lists.h>

#include "ata_pci_controller.h"

/* Max Units per Bus reported by the bus class */
#define MAX_DEVICEBUSES                 2

/* PCI Configuration space registers */
#define IDE_IO_CFG                      0x54

#define IOCFG_SCR1                      (1 << 7) /* Secondary 1 cable report */
#define IOCFG_SCR0                      (1 << 6) /* Secondary 0 cable report */
#define IOCFG_PCR1                      (1 << 5) /* Primary 1 cable report   */
#define IOCFG_PCR0                      (1 << 4) /* Primary 0 cable report   */

struct ata_ProbedBus
{
    struct Node                 atapb_Node;
    OOP_Object                  *atapb_Parent;
    struct PCIDeviceRef         *atapb_Device;
    UWORD                       atapb_Vendor;
    UWORD                       atapb_Product;
    UBYTE                       atapb_BusNo;
    IPTR                        atapb_IOBase;
    IPTR                        atapb_IOAlt;
    IPTR                        atapb_INTLine;
    IPTR                        atapb_DMABase;
};

/* These values are used also for ln_Type */
#define ATABUSNODEPRI_PROBED            50
#define ATABUSNODEPRI_PROBEDLEGACY      100
#define ATABUSNODEPRI_LEGACY            0

struct PCIATABusData
{
    struct ata_ProbedBus        *bus;
    OOP_Object                  *pciDriver;
    APTR                        dmaBuf;
    void                        (*ata_HandleIRQ)(UBYTE, APTR);
    APTR                        irqData;
    APTR                        irqHandle;
};

#endif
