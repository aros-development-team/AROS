#ifndef AHCI_HEADER_H
#define AHCI_HEADER_H

/*
    Copyright � 2010, The AROS Development Team. All rights reserved
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

#include <dos/dos.h>
#include <dos/bptr.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>
#include <proto/oop.h>

#include <oop/oop.h>
#include <hidd/pci.h>
#include <hidd/irq.h>

#include <aros/system.h>
#include <aros/symbolsets.h>

#include <string.h>

#include "ahci_hba.h"

#define HBAHW_D(fmt, ...) D(bug("[HBAHW%d] " fmt, hba_chip->HBANumber, ##__VA_ARGS__))

#define HBA_TASK_STACKSIZE  16*1024
#define HBA_TASK_PRI        10

/* ahci.device base */
struct ahciBase {
    struct Device device;

    /*
        List of all found AHCI host devices (referred to as host bus adapters, or HBA's)
        Semaphore protects ONLY the integrity of the list, not individual HBA-chip struct
    */
    struct  SignalSemaphore chip_list_lock;
    struct  MinList chip_list;
};

/* HBA-chip struct */
struct ahci_hba_chip {
    struct  MinNode hba_chip_Node;

    IPTR    PCIProductID, PCIVendorID;

    APTR    abar;
    IPTR    IRQ;

    ULONG   Version;

    ULONG   HBANumber;

    ULONG   CommandSlotCount;

    ULONG   PortImplementedMask;
    ULONG   PortCountMax;
    ULONG   PortCount;

    ULONG   StartingPortNumber;

    /*
        List of all implemented ports on a given HBA
        Semaphore protects ONLY the integrity of the list, not individual HBA-port struct
    */
    struct  SignalSemaphore port_list_lock;
    struct  MinList port_list;

    HIDDT_IRQ_Handler *IntHandler;

};

struct ahci_hba_port_unit {
    /* exec's unit */
    struct  Unit port_exec_unit;

    /* Physical HBA-port number of controller, Used in e.g. PxCMD */
    ULONG   Port_HBA_Number;

    /* Port and it's device is unit number x in Aros system */
    ULONG   Port_Unit_Number;

    /* Port belongs to this HBA-chip */
    struct  ahci_hba_chip *parent_hba;
};

/* HBA-port struct */
struct ahci_hba_port {
    struct  MinNode ahci_port_Node;

    struct  ahci_hba_port_unit port_unit;

};

/* ahci_hbahw prototypes */
BOOL ahci_create_interrupt(struct ahci_hba_chip *hba_chip);
BOOL ahci_create_hbatask(struct ahci_hba_chip *hba_chip);
BOOL ahci_setup_hba(struct ahci_hba_chip *hba_chip);
BOOL ahci_init_hba(struct ahci_hba_chip *hba_chip);
BOOL ahci_reset_hba(struct ahci_hba_chip *hba_chip);
void ahci_enable_hba(struct ahci_hba_chip *hba_chip);
BOOL ahci_disable_hba(struct ahci_hba_chip *hba_chip);
BOOL ahci_add_port(struct ahci_hba_chip *hba_chip, ULONG port_unit_num, ULONG port_hba_num);

/* ahci_misc prototypes */
ULONG count_bits_set(ULONG x);

#endif // AHCI_HEADER_H

