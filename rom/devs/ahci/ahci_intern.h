/*
 * Copyright (C) 2011-2018, The AROS Development Team.  All rights reserved.
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
#include <utility/hooks.h>

#include <hardware/ahci.h>
#include <hidd/ahci.h>

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

struct ahci_port;

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

   /* Count of all hosts detected */
   ULONG                   ahci_HostCount;

   /*
    * List of all units
    */
   struct MinList          ahci_Units;

   /*
    * memory pool
    */
    APTR                    ahci_MemPool;

    struct Library              *ahci_OOPBase;
    struct Library              *ahci_UtilityBase;

    /* Frequently used object offsets */
    OOP_Class                   *ahciClass;
    OOP_Class                   *busClass;
    OOP_Class                   *unitClass;

    OOP_Object                  *storageRoot;

#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase                ahci_HWAttrBase;
    OOP_AttrBase                ahci_HiddAttrBase;
    OOP_AttrBase                ahci_HiddPCIDeviceAttrBase;
    OOP_AttrBase                ahci_HiddStorageUnitAttrBase;
    OOP_AttrBase                ahci_AHCIAttrBase;
    OOP_AttrBase                ahci_BusAttrBase;
    OOP_AttrBase                ahci_AHCIBusAttrBase;
    OOP_AttrBase                ahci_AHCIUnitAttrBase;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID                ahci_HiddPCIDeviceMethodBase;
    OOP_MethodID                ahci_HiddPCIDriverMethodBase;
    OOP_MethodID                ahci_HWMethodBase;
#endif
    struct List                 ahci_Controllers;
};

#if defined(__OOP_NOATTRBASES__)
#undef HWAttrBase
#define HWAttrBase     		(AHCIBase->ahci_HWAttrBase)
#undef HiddBusAB
#define HiddBusAB   	        (AHCIBase->ahci_BusAttrBase)
#undef HiddAHCIBusAB
#define HiddAHCIBusAB   	(AHCIBase->ahci_AHCIBusAttrBase)
#undef HiddAHCIUnitAB
#define HiddAHCIUnitAB  	(AHCIBase->ahci_AHCIUnitAttrBase)
#undef HiddAttrBase
#define HiddAttrBase            (AHCIBase->ahci_HiddAttrBase)
#undef HiddStorageUnitAB
#define HiddStorageUnitAB       (AHCIBase->ahci_HiddStorageUnitAttrBase)
#endif
#if defined(__OOP_NOMETHODBASES__)
#undef HWBase
#define HWBase                  (AHCIBase->ahci_HWMethodBase)
#endif
#define OOPBase                 (AHCIBase->ahci_OOPBase)
#define UtilityBase             (AHCIBase->ahci_UtilityBase)

typedef struct {
    struct MinNode     	dev_Node;
    struct AHCIBase   	*dev_AHCIBase;
    OOP_Object        	*dev_Controller;		/* AHCI HW Controller Object */
    OOP_Object        	*dev_Object;			/* PCI Device Object */
    struct ahci_softc 	*dev_softc;
	const char 			*dev_gen;
	const char 			*dev_revision;
    ULONG              	dev_HostID;
} *device_t;

struct ahci_Controller
{
    struct Node         ac_Node;
    OOP_Class           *ac_Class;
    OOP_Object          *ac_Object;
    device_t            ac_dev;
};

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

struct ahci_Unit;

struct ahci_Bus
{
    struct AHCIBase     *ab_Base;   /* device self */
    struct ahci_port    *ab_Port;
    OOP_Object          *ab_Unit;
};

struct ahci_Unit
{
   struct Unit         au_Unit;        /* exec's unit */
   struct ahci_Bus     *au_Bus;         /* Bus on which this unit is */
   ULONG               au_UnitNum;     /* Unit number as coded by device */
   UBYTE               au_Model[41];
   UBYTE               au_FirmwareRev[9];
   UBYTE               au_SerialNumber[21];
};

/* Function prototypes */

BOOL Hidd_AHCIBus_Start(OOP_Object *, struct AHCIBase *);
AROS_UFP3(BOOL, Hidd_AHCIBus_Open,
          AROS_UFPA(struct Hook *, h, A0),
          AROS_UFPA(OOP_Object *, obj, A2),
          AROS_UFPA(IPTR, reqUnit, A1));

#endif /* AHCI_INTERN_H */
