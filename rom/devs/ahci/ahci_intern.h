/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef AHCI_INTERN_H
#define AHCI_INTERN_H

#include <exec/types.h>
#include <asm/io.h>

#include <devices/scsidisk.h>
#include <exec/devices.h>

#include <oop/oop.h>

#include <hardware/ahci.h>

#define Unit(io) ((struct ahci_Unit *)(io)->io_Unit)
#define IOStdReq(io) ((struct IOStdReq *)io)

/* Device types */
#define DEV_NONE        0x00
#define DEV_UNKNOWN     0x01
#define DEV_ATA         0x02
#define DEV_SATA        0x03
#define DEV_ATAPI       0x80
#define DEV_SATAPI      0x81
#define DEV_BROKEN      0xff


/* ahci.device base */
struct AHCIBase
{
   /*
    * Device structure - used to manage devices by Exec
    */
   struct Device           ahci_Device;

   /*
    * master task pointer
    */
   struct Task            *ahci_Daemon;

   /*
    * List of all units
    */
   struct MinList          ahci_Units;
   ULONG                   ahci_UnitCount;

   /*
    * memory pool
    */
   APTR                    ahci_MemPool;

   /* Frequently used object offsets */
   OOP_AttrBase            ahci_HiddPCIDeviceAttrBase;
   OOP_MethodID            ahci_HiddPCIDeviceMethodBase;

   OOP_MethodID            ahci_HiddPCIDriverMethodBase;
};

struct ahci_port;

#include <exec/semaphores.h>

struct cam_sim {
    struct MinNode    sim_Node;
    struct ahci_port *sim_Port;
    ULONG             sim_Unit;
    ULONG             sim_UseCount;
    struct SignalSemaphore sim_Lock;
    unsigned int      sim_Timeout;
    struct List       sim_IOs;
    ULONG             sim_Flags;
#define SIMB_MediaPresent       0       /* Media is present */
#define SIMB_OffLine            1       /* No new IOs are permitted */
#define SIMF_MediaPresent       (1 << SIMB_MediaPresent)
#define SIMF_OffLine            (1 << SIMB_OffLine)
    ULONG             sim_ChangeNum;
    struct Task      *sim_Monitor;
};

#endif /* AHCI_INTERN_H */
